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

  printf("Traversal done in %zd nodes.\n", _visitedNodes.size());

  ComputeWinningStates();

  u32 winningStates = 0;
  for (State* state : _explored) {
    if (state->winDistance == UNWINNABLE) continue;
    
    winningStates++;
  }

  printf("Of the %zd nodes, %d are winning.\n", _visitedNodes.size(), winningStates);
  /*
  const char* dirs[] = {
    "",
    "North",
    "South",
    "",
    "West",
    "",
    "",
    "",
    "East",
  };

  for (State* state : _explored) {
    if (state->winDistance == UNWINNABLE) continue;
    Vector<Direction> losingMoves;
    if (state->u && state->u->winDistance == UNWINNABLE) losingMoves.Push(Up);
    if (state->d && state->d->winDistance == UNWINNABLE) losingMoves.Push(Down);
    if (state->l && state->l->winDistance == UNWINNABLE) losingMoves.Push(Left);
    if (state->r && state->r->winDistance == UNWINNABLE) losingMoves.Push(Right);
    if (losingMoves.Size() > 0) {
      printf("From this position, moving");
      for (Direction dir : losingMoves) printf(" %s", dirs[dir]);
      printf(" would be losing.\n");
      _level->SetState(state);
      _level->Print();
      printf("\n");
    }
  }
  */

  _level->SetState(initialState); // Be polite and make sure we restore the original level state
  if (initialState->winDistance == UNWINNABLE) return Vector<Direction>(); // Puzzle is unsolvable

  printf("Found the shortest # of moves: %d\n", initialState->winDistance);
  printf("Done computing victory states\n");

  DFSWinStates(initialState, 0, 0);

  s64 delta = _bestMillis - (_bestSolution.Size() * 160);
  printf("Delta duration: %lld.%lld seconds\n", delta / 1000, delta % 1000);
  printf("Solution duration: %lld.%02lld seconds\n", _bestMillis / 1000, _bestMillis % 1000);

  return _bestSolution.Copy();
}

void Solver::BFSStateGraph() {
  State* signal = new State();
  _unexplored.AddToTail(signal);
  u32 depth = 1;

  while (_unexplored.Head() != nullptr) {
    State* state = _unexplored.Head();

    if (state == signal) {
      printf("Finished processing depth %d", depth);
      if (_unexplored.Size() == 1) { // Only the dummy state is left in queue, queue is essentially empty
        printf(".\nBFS exploration finished, no more nodes to find.\n");
        break;
      }
      if (depth == 100) {
        printf(".\nCapping out at depth %d.\n", depth);
        break;
      }
      depth++;
      printf(", there are %d nodes to explore at depth %d\n", _unexplored.Size() - 1, depth);
      _unexplored.AddToTail(signal);
      _unexplored.AdvanceHead();
      continue;
    }

    _level->SetState(state);
    if (_level->Move(Up))    state->u = GetOrInsertState(state->depth + 1);
    _level->SetState(state);
    if (_level->Move(Down))  state->d = GetOrInsertState(state->depth + 1);
    _level->SetState(state);
    if (_level->Move(Left))  state->l = GetOrInsertState(state->depth + 1);
    _level->SetState(state);
    if (_level->Move(Right)) state->r = GetOrInsertState(state->depth + 1);

    _unexplored.AdvanceHead();
    _explored.AddToHead(state);
  }
}

State* Solver::GetOrInsertState(u16 depth) {
  std::pair<std::unordered_set<State>::iterator, bool> it;
  try {
    it = _visitedNodes.insert(_level->GetState());
  } catch (std::bad_alloc&) {
    putchar('k');
    return nullptr;
  }

  State* state = const_cast<State*>(&*it.first);
  if (!it.second) return state; // State was already analyzed

  state->depth = depth;

  if (_level->Won()) {
      state->winDistance = 0;
      if (!_foundWinningState) {
        printf("Found the first winning state!\n");
        _foundWinningState = true;
      }
      _explored.AddToHead(state); // No further exploration needed for winning states.
      return state;
  }

  // Once we find a winning state, we have reached the minimum depth for a solution.
  // Ergo, we should not explore the tree deeper than that solution. Since we're a BFS,
  // that means we should drain the unexplored queue but not add new nodes.
  if (true || !_foundWinningState) {
    _unexplored.AddToTail(state);
  }
  return state;
}

void Solver::ComputeWinningStates() {
  // If (somehow) we make it through the entire loop and find no win states, this is the correct exit condition.
  // It is very likely that this value will be updated during iteration.
  State* endOfLoop = _explored.Previous();

  while (true) {
    State* state = _explored.Current();
    if (state == nullptr) break; // All states are winning

    u16 winDistance = state->winDistance;

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

    if (winDistance + 1 < state->winDistance) {
      // TODO: I'm pretty sure I can avoid this (expensive) code entirely if I just run the DFS more carefully.
      // Then I could just have this be a boolean or something and save myself a bit of computation.
      // _explored.Pop(); // TODO: We should only need update the winDistance once (because of BFS traversal).

      state->winDistance = winDistance + 1;
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
  assert(nextState->depth > state->depth); // Move is sub-optimal, taking it is useless. (This also prevents us from infinite-looping).

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

  if (_level->WouldStephenStepOnGrill(state->stephen, dir)) totalMillis += 152; // TODO: Does this change while speared?
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
