#include "Solver.h"
#include "Level.h"
#include <unordered_set>
#include <unordered_map>

Solver::Solver(Level* level) {
  _level = level;
}

Vector<Direction> Solver::Solve() {
  State* initialState = GetOrInsertState();
  _maxDepth = 0xFFFF; // maxDepth;
  _unexploredH = initialState;
  _unexploredT = _unexploredH;

  bool foundWinningState = false;
  while (_unexploredH != nullptr) {
    State* state = _unexploredH;

    _level->SetState(state);
    if (_level->Move(Up))    state->u = GetOrInsertState();
    _level->SetState(state);
    if (_level->Move(Down))  state->d = GetOrInsertState();
    _level->SetState(state);
    if (_level->Move(Left))  state->l = GetOrInsertState();
    _level->SetState(state);
    if (_level->Move(Right)) state->r = GetOrInsertState();

    _unexploredH = state->next;

    state->next = _exploredH;
    _exploredH = state;
    if (!_exploredT) _exploredT = _exploredH;
  }

  printf("Traversal done in %ld nodes\n", _visitedNodes.size());

  // We are done traversing and building the graph.
  // Now, determine the victory routes:

  State* startOfLoop = _exploredH;
  while (true) {
    State* state = _exploredH;
    if (state->next == startOfLoop) break; // TODO: Compare # of (shortest) victory paths with and without this early exit.

    u16 winDistance = 0xFFFF;

    State* nextState = state->u;
    if (nextState != nullptr && nextState->winDistance < winDistance) {
      winDistance = nextState->winDistance;
    }
    nextState = state->d;
    if (nextState != nullptr && nextState->winDistance < winDistance) {
      winDistance = nextState->winDistance;
    }
    nextState = state->l;
    if (nextState != nullptr && nextState->winDistance < winDistance) {
      winDistance = nextState->winDistance;
    }
    nextState = state->r;
    if (nextState != nullptr && nextState->winDistance < winDistance) {
      winDistance = nextState->winDistance;
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

  printf("Found the shortest path %d\n", state->winDistance);
  printf("Done computing victory states\n");

  if (initialState->winDistance == 0xFFFF) return Vector<Direction>(); // Puzzle is unsolvable

  DFSWinStates(initialState, 0);
  u64 expectedMillis = _bestSolution.Size() * 160;
  printf("Delta duration: %lld\n", _bestMillis - expectedMillis);
  printf("Solution duration: %lld.%02lld seconds", _bestMillis / 1000, _bestMillis % 1000);

  return _bestSolution.Copy();
}

State* Solver::GetOrInsertState() {
  auto it = _visitedNodes.insert(_level->GetState());
  State* state = const_cast<State*>(&*it.first);
  if (it.second) { // State was not yet analyzed
    if (_level->Won()) {
       state->winDistance = 0;
       _foundWinningState = true;
    }

    // Once we find a winning state, we have reached the minimum depth for a solution.
    // Ergo, we should not explore the tree deeper than that solution. Since we're a BFS,
    // that means we should drain the unexplored queue but not add new nodes.
    if (!_foundWinningState) {
      if (_unexploredT != nullptr) _unexploredT->next = state;
      _unexploredT = state;
    }
  }
  return state;
}

void Solver::DFSWinStates(State* state, u64 totalMillis) {
  if (state->winDistance == 0 && totalMillis < _bestMillis) {
    _bestSolution = _solution.Copy();
    _bestMillis = totalMillis;
    return;
  }

  ComputePenaltyAndRecurse(state, state->u, Up, totalMillis);
  ComputePenaltyAndRecurse(state, state->d, Down, totalMillis);
  ComputePenaltyAndRecurse(state, state->l, Left, totalMillis);
  ComputePenaltyAndRecurse(state, state->r, Right, totalMillis);
}

void Solver::ComputePenaltyAndRecurse(State* state, State* nextState, Direction dir, u64 totalMillis) {
  if (!nextState) return; // Move would be illegal
  if (nextState->winDistance >= state->winDistance) return; // Move leads away from victory *or* is not winning.

  // Compute the duration of this motion
  if (state->stephen.sausageSpeared == -1) {
    totalMillis += 160;

#define o(x) if (state->s##x != nextState->s##x) totalMillis += 38;
    SAUSAGES;
#undef o
  } else { // No sausage speared
    totalMillis += 158;

#define o(x) if (state->s##x != nextState->s##x) totalMillis += 4;
    SAUSAGES;
#undef o
  }

  if (_level->WouldStepOnGrill(state->stephen.x, state->stephen.y, dir)) totalMillis += 152; // TODO: Does this change while speared?

  if (totalMillis > _bestMillis) return; // This solution is not faster than the known best path.

  _solution.Push(dir);
  DFSWinStates(nextState, totalMillis);
  _solution.Pop();
}
