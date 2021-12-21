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
    if (state->next == startOfLoop) break;

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

  _allSolutions.Resize(0);
  _solution = Vector<Direction>(state->winDistance);
  DFSWinStates(state);

  printf("Found %d solution%s of length %d\n", _allSolutions.Size(), _allSolutions.Size() == 1 ? "" : "s",  state->winDistance);

  // TODO: I should just compute these while we DFS. This is a waste of RAM (and some CPU).
  // TODO: Are there other things we care about? Yes, let's optimize backward motion > rotations.
  u8 bestBurned = 0xFF;
  u8 bestPushes = 0xFF;
  u8 bestRotations = 0xFF;
  Vector<Direction> bestSolution;
  for (const Vector<Direction>& solution : _allSolutions) {
    u8 burned = 0;
    u8 pushes = 0;
    u8 rotations = 0;
    Direction lastDir = None;
    State* state = initialState;
    for (Direction dir : solution) {
      if (dir != lastDir) {
        rotations++;
        lastDir = dir;
      }

      // TODO: Time pushing two sausages, one at a time, vs pushing two at once. And update this cost to match.
      State* nextState;
      if (dir == Up) nextState = state->u;
      else if (dir == Down) nextState = state->d;
      else if (dir == Left) nextState = state->l;
      else if (dir == Right) nextState = state->r;
      assert(nextState);
#define o(x) if (state->s##x != nextState->s##x) pushes++;
      SAUSAGES;
#undef o
      state = nextState;
      
      // TODO: Burned steps.
    }

    if ((burned < bestBurned)
     || (burned == bestBurned && pushes < bestPushes)
     || (burned == bestBurned && pushes == bestPushes && rotations < bestRotations)) {
      bestBurned = burned;
      bestPushes = pushes;
      bestRotations = rotations;
      bestSolution = solution.Copy();
    }
  }

  return bestSolution;
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

void Solver::DFSWinStates(State* state) {
  if (state->winDistance == 0) {
    _allSolutions.Emplace(_solution.Copy());
    return;
  }

  if (state->u && state->u->winDistance == state->winDistance - 1) {
    _solution.UnsafePush(Up);
    DFSWinStates(state->u);
    _solution.Pop();
  }
  if (state->d && state->d->winDistance == state->winDistance - 1) {
    _solution.UnsafePush(Down);
    DFSWinStates(state->d);
    _solution.Pop();
  }
  if (state->l && state->l->winDistance == state->winDistance - 1) {
    _solution.UnsafePush(Left);
    DFSWinStates(state->l);
    _solution.Pop();
  }
  if (state->r && state->r->winDistance == state->winDistance - 1) {
    _solution.UnsafePush(Right);
    DFSWinStates(state->r);
    _solution.Pop();
  }
}
