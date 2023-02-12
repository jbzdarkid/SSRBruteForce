#include "Solver.h"
#include "Level.h"
#include <unordered_set>
#include <unordered_map>

Solver::Solver(Level* level) {
  _level = level;
}

Solver::~Solver() {
  printf("Destroying _visitedNodes\n");
}

u16 Score(State* state) {
  u16 score = 0;
  u16 sidesCooked;
  u8 flags;
#define o(x) { \
    flags = state->sausages[x].flags; \
    sidesCooked = (flags & Sausage::Flags::FullyCooked); \
    if (sidesCooked == Sausage::Flags::FullyCooked) score += 100; \
    else score += __popcnt16(sidesCooked); \
  }

  SAUSAGES;
#undef o
  return score;
}

Vector<Direction> Solver::Solve() {
  printf("Solving %s\n", _level->name);

  State* initialState;
  _visitedNodes2.CopyAdd(_level->GetState(), &initialState);
  initialState->shallow = _shallowAlloc.make<ShallowState>();
  _unexplored.AddToTail(initialState);

  BFSStateGraph();

  printf("Traversal done in %zd nodes.\n", _visitedNodes2.Size());

  ComputeWinningStates();

  u32 winningStates = 0;
  for (ShallowState* shallow : _explored2) {
    if (shallow->winDistance != UNWINNABLE) winningStates++;
  }

  printf("Of the %zd nodes, %d are winning.\n", _visitedNodes2.Size(), winningStates);

  _level->SetState(initialState); // Be polite and make sure we restore the original level state
  if (initialState->shallow == nullptr || initialState->shallow->winDistance == UNWINNABLE) {
    printf("Automatic solver could not find a solution.\n");
    u16 bestScore = 0;
    for (State* state : _explored) {
      u16 score = Score(state);
      if (score > bestScore) bestScore = score;
    }
    printf("Best score: %d\n", bestScore);
    for (State* state : _explored) {
      u16 score = Score(state);
      if (score == bestScore) {
        state->shallow->winDistance = 0;
      }
    }

    ComputeWinningStates();
  }

  printf("Found the shortest # of moves: %d\n", initialState->shallow->winDistance);
  printf("Done computing victory states\n");

  DFSWinStates(initialState, 0, 0);

  s64 delta = _bestMillis - (_bestSolution.Size() * 160);
  printf("Delta duration: %.03f seconds\n", delta / 1000.0);
  printf("Solution duration: %lld.%02lld seconds\n", _bestMillis / 1000, _bestMillis % 1000);

  return _bestSolution.Copy();
}

void Solver::BFSStateGraph() {
  State depthMarker;
  u16 depth = 0;
  _unexplored.AddToTail(&depthMarker);

  while (_unexplored.Head() != nullptr) {
    State* state = _unexplored.Head();
    if (state->shallow->winDistance == 0) { // Delayed addition of winning nodes to keep _explored in order
      _unexplored.AdvanceHead();
      _explored.AddToTail(state);
      continue;
    }

    if (state == &depthMarker) {
      printf("Finished processing depth %d, ", depth);
      if (_unexplored.Size() == 1) { // Only the dummy state is left in queue, queue is essentially empty
        printf("BFS exploration complete (no nodes remaining).\n");
        break;
      } else if (_visitedNodes2.Size() > 100'000'000) {
        printf("giving up (too many nodes).\n");
        break;
      } else if (depth == _winningDepth + 2) {
        // I add a small fudge-factor here (2 iterations) to search for solutions
        // which potentially take more moves, but are faster in realtime.
        printf("not exploring any further, since the winning state was at depth %d.\n", _winningDepth);
        break;
      }

      depth++;
      _unexplored.AdvanceHead();
      printf("there are %d nodes to explore at depth %d\n", _unexplored.Size(), depth);
      _unexplored.AddToTail(&depthMarker);
      continue;
    }

    _level->SetState(state);
    if (_level->Move(Up))    state->u = GetOrInsertState(depth);
    _level->SetState(state);
    if (_level->Move(Down))  state->d = GetOrInsertState(depth);
    _level->SetState(state);
    if (_level->Move(Left))  state->l = GetOrInsertState(depth);
    _level->SetState(state);
    if (_level->Move(Right)) state->r = GetOrInsertState(depth);

    _unexplored.AdvanceHead();
    _explored.AddToTail(state);
  }
}

State* Solver::GetOrInsertState(u16 depth) {
  State* state;
  bool inserted = _visitedNodes2.CopyAdd(_level->GetState(), &state);
  if (!inserted) return state; // State was already analyzed, or allocation failed

  if (_visitedNodes2.Size() % 100'000 == 0) {
    //_level->Print();
  }

  state->shallow = _shallowAlloc.make<ShallowState>();

  if (_level->Won()) {
      state->shallow->winDistance = 0;
      if (_winningDepth == UNWINNABLE) {
        // Once we find a winning state, we have reached the minimum depth for a solution.
        // Ergo, we should not explore the tree deeper than that solution. Since we're a BFS,
        // that means we should finish the current depth, but not explore any further.
        _winningDepth = depth + 1; // +1 because the winning move is at the *next* depth, not the current one.
        printf("Found the first winning state at depth %d!\n", _winningDepth);
      }
      // Even though this state is winning, we add it to the unexplored list as usual,
      // in order to keep the _explored list in depth-sorted order.
  }

  _unexplored.AddToTail(state);
  return state;
}

void Solver::CreateShallowStates() {
  _explored2 = LinkedLoop<ShallowState>(); // Clear the shallow state list in case we're re-evaluating after failing to win.

  // Second iteration because we need shallow copies of the udlr states. (maybe not?)
  printf("Populating shallow states:                                                                          |\n");
  for (State* state : _explored) {
    ShallowState* shallow = state->shallow;
    if (state->u) shallow->u = state->u->shallow;
    if (state->d) shallow->d = state->d->shallow;
    if (state->l) shallow->l = state->l->shallow;
    if (state->r) shallow->r = state->r->shallow;
    _explored2.AddCurrent(shallow);
    if (_explored2.Size() % (_explored.Size() / 100) == 0) printf("#");
  }

  printf("|\n");
}

void Solver::ComputeWinningStates() {
  /* Even though we generate the list in order, we should not remove states from the loop, and we may need to do multiple loops.
     For example, consider this graph: A is at depth 0, B and D are at 1, C is at 2.
     A -> (D)
       \v   ^\
         B -> C

    Although C->D is a sub-optimal move (depth is decreasing), C is still a winning state. Since we process in depth order,
    we will process C, D, B, A -- ergo when we check D the first time it won't be marked winning.
    But, we do still want to mark C as winning -- since we haven't *truly* computed the costs yet, we don't know if it's faster.
  */

  // Reducing the memory required to represent the tree by removing sausages positions from State.
  // This helps to avoid paging with very large trees.
  CreateShallowStates();

  printf("Computing winning states to achieve the best score\n");

  // If (somehow) we make it through the entire loop and find no win states, this is the correct exit condition.
  // It is very likely that this value will be updated during iteration.
  ShallowState* endOfLoop = _explored2.Previous();

  while (true) {
    ShallowState* shallow = _explored2.Current();
    if (shallow == nullptr) break; // All states are winning

    u16 winDistance = shallow->winDistance;

    ShallowState* nextState = shallow->u;
    if (nextState != nullptr && nextState->winDistance < winDistance) {
      winDistance = nextState->winDistance;
    }
    nextState = shallow->d;
    if (nextState != nullptr && nextState->winDistance < winDistance) {
      winDistance = nextState->winDistance;
    }
    nextState = shallow->l;
    if (nextState != nullptr && nextState->winDistance < winDistance) {
      winDistance = nextState->winDistance;
    }
    nextState = shallow->r;
    if (nextState != nullptr && nextState->winDistance < winDistance) {
      winDistance = nextState->winDistance;
    }

    if (winDistance + 1 < shallow->winDistance) {
      shallow->winDistance = winDistance + 1;
      endOfLoop = _explored2.Previous(); // If we come back around without making any progress, stop.
    }

    if (shallow == endOfLoop) break; // We just processed the last node, and it wasn't winning.

    _explored2.Advance();
  }
}

void Solver::DFSWinStates(State* state, u64 totalMillis, u16 backwardsMovements) {
  if (state->shallow->winDistance == 0) {
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
  if (nextState->shallow->winDistance == UNWINNABLE) return; // Move is not ever winning
  if (state->shallow->winDistance != nextState->shallow->winDistance + 1) return; // Move leads away from victory

  // Compute the duration of this motion

  // Speared state is not saved, because it's recoverable. Memory > speed tradeoff.
  // This is gross. It gets a little cleaner if I can use for-each, but not much.
  bool sausageSpeared = false;
  if (state->stephen.HasFork()) {
#define o(x) +1
    for (u8 i=0; i<SAUSAGES; i++) {
#undef o
      const Sausage& sausage = state->sausages[i];
      if (state->stephen.z != sausage.z) continue;
      if ((state->stephen.x == sausage.x1 && state->stephen.y == sausage.y1)
        || (state->stephen.x == sausage.x2 && state->stephen.y == sausage.y2)) {
        sausageSpeared = true;
        break;
      }
    }
  }
  if (!sausageSpeared) {
    totalMillis += 160;

#define o(x) if (state->sausages[x] != nextState->sausages[x]) totalMillis += 38;
    SAUSAGES;
#undef o
  } else { // Movements are faster while spearing a sausage
    totalMillis += 158;

#define o(x) if (state->sausages[x] != nextState->sausages[x]) totalMillis += 4;
    SAUSAGES;
#undef o
  }

  if (_level->WouldStephenStepOnGrill(state->stephen, dir)) totalMillis += 152; // TODO: Does this change while speared?
  // TODO: Does the sausage movement cost depend on your *current state* or the *next state*? I.e. if you unspear and roll a sausage behind you, do you pay for it?
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
