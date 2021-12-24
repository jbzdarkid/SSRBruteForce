#include "Solver.h"
#include "Level.h"
#include <unordered_set>
#include <unordered_map>

Solver::Solver(Level* level) {
  _level = level;
}

Vector<Direction> Solver::Solve() {
  printf("Solving %s\n", _level->name);

  auto it = _visitedNodes.insert(_level->GetState());
  State* initialState = const_cast<State*>(&*it.first);
  _unexplored = LinkedList<State>(initialState);

  _maxDepth = 0xFFFF;

  BFSStateGraph();

  printf("Traversal done in %zd nodes\n", _visitedNodes.size());

  ComputeWinningStates();

  _level->SetState(initialState); // Be polite and make sure we restore the original level state
  if (initialState->winDistance == 0xFFFF) return Vector<Direction>(); // Puzzle is unsolvable

  printf("Found the shortest # of moves: %d\n", initialState->winDistance);
  printf("Done computing victory states\n");

  DFSWinStates(initialState, 0, 0);

  u64 delta = _bestMillis - (_bestSolution.Size() * 160);
  printf("Delta duration: %lld.%lld seconds\n", delta / 1000, delta % 1000);
  printf("Solution duration: %lld.%02lld seconds\n", _bestMillis / 1000, _bestMillis % 1000);

  return _bestSolution.Copy();
}

void Solver::BFSStateGraph() {
  State* dummy = new State();
  _unexplored.AddToTail(dummy);
  u32 depth = 0;

  while (_unexplored.Head() != nullptr) {
    State* state = _unexplored.Head();

    if (state == dummy && !_foundWinningState) {
      printf("Finished processing depth %d, there are %d nodes to explore at depth %d\n", depth, _unexplored.Size() - 1, depth + 1);
      depth++;
      _unexplored.AddToTail(dummy);
      _unexplored.AdvanceHead();
      continue;
    }

    _level->SetState(state);
    if (_level->Move(Up))    state->u = GetOrInsertState();
    _level->SetState(state);
    if (_level->Move(Down))  state->d = GetOrInsertState();
    _level->SetState(state);
    if (_level->Move(Left))  state->l = GetOrInsertState();
    _level->SetState(state);
    if (_level->Move(Right)) state->r = GetOrInsertState();

    _unexplored.AdvanceHead();
    _explored.AddToHead(state);
  }
}

State* Solver::GetOrInsertState() {
  std::pair<std::unordered_set<State>::iterator, bool> it;
  try {
    it = _visitedNodes.insert(_level->GetState());
  } catch (std::bad_alloc&) {
    putchar('k');
    return nullptr;
  }

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
      _unexplored.AddToTail(state);
    }
  }
  return state;
}

void Solver::ComputeWinningStates() {
  // If (somehow) we make it through the entire loop and find no win states, this is the correct exit condition.
  // It is very likely that this value will be updated during iteration.
  State* endOfLoop = _explored.Previous();

  while (true) {
    State* state = _explored.Current();
    if (state == nullptr) {
      // Level is unsolvable, I think
      break;
    }

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
      _explored.Pop(); // Remove ourselves from the loop
      endOfLoop = _explored.Previous(); // If we come back around without making any progress, stop.
    }

    if (state == endOfLoop) break; // We just processed the last node, and it wasn't winning.

    _explored.Advance();
  }
}

void Solver::DFSWinStates(State* state, u64 totalMillis, u16 backwardsMovements) {
  if (state->winDistance == 0) {
    if (totalMillis < _bestMillis
     || (totalMillis == _bestMillis && backwardsMovements > _bestBackwardsMovements)) {
      _bestSolution = _solution.Copy();
      _bestMillis = totalMillis;
      _bestBackwardsMovements = backwardsMovements;
    }
    return;
  }

  ComputePenaltyAndRecurse(state, state->u, Up, totalMillis, backwardsMovements);
  ComputePenaltyAndRecurse(state, state->d, Down, totalMillis, backwardsMovements);
  ComputePenaltyAndRecurse(state, state->l, Left, totalMillis, backwardsMovements);
  ComputePenaltyAndRecurse(state, state->r, Right, totalMillis, backwardsMovements);
}

void Solver::ComputePenaltyAndRecurse(State* state, State* nextState, Direction dir, u64 totalMillis, u16 backwardsMovements) {
  if (!nextState) return; // Move would be illegal
  if (nextState->winDistance >= state->winDistance) return; // Move leads away from victory *or* is not winning.

  // Compute the duration of this motion
  if (state->stephen.sausageSpeared == -1) {
    totalMillis += 160;

#define o(x) if (state->sausages[x] != nextState->sausages[x]) totalMillis += 38;
    SAUSAGES;
#undef o
  } else { // No sausage speared
    totalMillis += 158;

#define o(x) if (state->sausages[x] != nextState->sausages[x]) totalMillis += 4;
    SAUSAGES;
#undef o
  }

  if (_level->WouldStepOnGrill(state->stephen.x, state->stephen.y, dir)) totalMillis += 152; // TODO: Does this change while speared?
  // TODO: Time sausage pushes as fork pushes (same latency as rotations?)
  // TODO: Time motion w/ sausage hat
  // TODO: Time motion w/ fork carry
  // TODO: Time motion as forkless -> rotations *and* lateral motion
  // TODO: Time motion when pushing a block
  // TODO: Ladder climbs while speared / non-speared?

  if (totalMillis > _bestMillis) return; // This solution is not faster than the known best path.

  if (state->stephen.dir == Up && dir == Down)         backwardsMovements++;
  else if (state->stephen.dir == Down && dir == Up)    backwardsMovements++;
  else if (state->stephen.dir == Left && dir == Right) backwardsMovements++;
  else if (state->stephen.dir == Right && dir == Left) backwardsMovements++;

  _solution.Push(dir);
  DFSWinStates(nextState, totalMillis, backwardsMovements);
  _solution.Pop();
}
