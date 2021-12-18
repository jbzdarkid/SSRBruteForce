#include "Solver.h"
#include "Level.h"
#include <unordered_set>
#include <unordered_map>

Solver::Solver(Level* level) {
  _level = level;
}

std::unordered_set<State> visitedNodes;
std::unordered_map<State, u16> winningNodes;
std::unordered_set<State> losingNodes;

State* unexploredH;
State* unexploredT;
State* exploredH;
State* exploredT;

State* Solver::GetOrInsertState() {
  State state = _level->GetState();
  auto search = visitedNodes.find(state);
  if (search != visitedNodes.end()) return const_cast<State*>(&*search);

  State* newState = const_cast<State*>(&*visitedNodes.insert(state).first);
  if (_level->Won()) {
    newState->winDistance = 0;
  } else { // No need to explore if it's a winning state
    if (unexploredT) unexploredT->next = newState;
    unexploredT = newState;
  }
  return newState;
}

Vector<Direction> Solver::Solve(u32 moveLimit) {
  State* initialState = GetOrInsertState();
  unexploredH = initialState;
  unexploredT = unexploredH;

  while (unexploredH != nullptr) {
    State* state = unexploredH;
    // TODO: Check depth here and continue

    _level->SetState(state);

    if (_level->Move(Up)) {
      state->u = GetOrInsertState();
      _level->SetState(state);
    }

    if (_level->Move(Down)) {
      state->d = GetOrInsertState();
      _level->SetState(state);
    }

    if (_level->Move(Left)) {
      state->l = GetOrInsertState();
      _level->SetState(state);
    }

    if (_level->Move(Right)) {
      state->r = GetOrInsertState();
      // _level->SetState(state);
    }

    unexploredH = state->next;

    state->next = exploredH;
    exploredH = state;
    if (!exploredT) exploredT = exploredH;
  }

  // We are done traversing and building the graph.
  // Now, determine the victory routes:

  State* startOfLoop = exploredH;
  while (true) {
    State* state = exploredH;
    if (state->next == startOfLoop) break;

    u16 winDistance = 0xFFFF;

    for (State* nextState : {state->u, state->d, state->l, state->r}) {
      if (nextState == nullptr) continue;
      u16 distance = nextState->winDistance;
      if (distance < winDistance) winDistance = distance;
    }

    if (winDistance != 0xFFFF) {
      state->winDistance = winDistance + 1;
      startOfLoop = state->next; // If we come back around to the element after this one, and make no further progress, stop.
    } else { // Return it to the queue, maybe it's winning later.
      exploredT->next = state;
      exploredT = state;
    }

    exploredH = exploredH->next;
  }

  State* state = initialState;
  Vector<Direction> solution(state->winDistance);
  while (state->winDistance > 0) {
    if (state->u && state->u->winDistance == state->winDistance - 1) {
      solution.UnsafePush(Up);
      state = state->u;
    } else if (state->d && state->d->winDistance == state->winDistance - 1) {
      solution.UnsafePush(Down);
      state = state->d;
    } else if (state->l && state->l->winDistance == state->winDistance - 1) {
      solution.UnsafePush(Left);
      state = state->l;
    } else if (state->r && state->r->winDistance == state->winDistance - 1) {
      solution.UnsafePush(Right);
      state = state->r;
    }
  }

  _level->SetState(initialState);
  return solution;
}