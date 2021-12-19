#include "Solver.h"
#include "Level.h"
#include <unordered_set>
#include <unordered_map>

Solver::Solver(Level* level) {
  _level = level;
}

Vector<Direction> Solver::Solve(u16 maxDepth) {
  State* initialState = GetOrInsertState(0);
  _maxDepth = 0xFFFF; // maxDepth;
  _unexploredH = initialState;
  _unexploredT = _unexploredH;

  while (_unexploredH != nullptr) {
    State* state = _unexploredH;
    if (state->depth * state->depth > _maxDepth) {
      _unexploredH = state->next;
      continue; // This should be safe.
    }

    _level->SetState(state);
    if (_level->Move(Up))    state->u = GetOrInsertState(state->depth);
    _level->SetState(state);
    if (_level->Move(Down))  state->d = GetOrInsertState(state->depth);
    _level->SetState(state);
    if (_level->Move(Left))  state->l = GetOrInsertState(state->depth);
    _level->SetState(state);
    if (_level->Move(Right)) state->r = GetOrInsertState(state->depth);

    _unexploredH = state->next;

    state->next = _exploredH;
    _exploredH = state;
    if (!_exploredT) _exploredT = _exploredH;
  }

  printf("Traversal done\n");

  // We are done traversing and building the graph.
  // Now, determine the victory routes:

  State* startOfLoop = _exploredH;
  while (true) {
    State* state = _exploredH;
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
      _exploredT->next = state;
      _exploredT = state;
    }

    _exploredH = _exploredH->next;
  }

  _level->SetState(initialState);
  State* state = initialState;
  if (state->winDistance == 0xFFFF) return Vector<Direction>(); // Unsolvable

  Vector<Direction> solution(state->winDistance);
  printf("Found the shortest path %d\n", state->winDistance);

  // TODO: Compute all winning, fastest paths (do a DFS using the logic below)
  // TODO: Evaluate additional costs for winning paths (# burned steps, # sausage pushes, # rotations?)

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
    } else {
      return Vector<Direction>(); // Somehow our tree lead to a dead end.
    }
  }

  return solution;
}

State* Solver::GetOrInsertState(u16 depth) {
  State state = _level->GetState();
  auto search = _visitedNodes.find(state);
  if (search != _visitedNodes.end()) return const_cast<State*>(&*search);

  //if (_visitedNodes.size() % 1'000'000 == 0) {
  //  printf("%lld million nodes\n", _visitedNodes.size() / 1'000'000);
  //  _level->Print();
  //}

  if (_level->Won()) { // No need to do further exploration if it's a winning state
    state.winDistance = 0;
    _maxDepth = depth;
    printf("Improved maxDepth to %d\n", depth);
    return const_cast<State*>(&*_visitedNodes.insert(state).first);
  } else {
    state.depth = depth + 1;
    State* newState = const_cast<State*>(&*_visitedNodes.insert(state).first);
    if (_unexploredT != nullptr) _unexploredT->next = newState;
    _unexploredT = newState;
    return newState;
  }
}
