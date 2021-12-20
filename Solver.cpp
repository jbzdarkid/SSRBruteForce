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
    if (state->depth > _maxDepth + 10) { // I hope this is right. I've been wrong here before. Being cautious.
      break; // Because we are doing BFS, we are done once we reach the first node that's too deep.
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

  printf("Found the shortest path %d\n", state->winDistance);

  _allSolutions.Resize(0);
  _solution = Vector<Direction>(state->winDistance);
  DFSWinStates(state);

  printf("Found %d solution%s of length %d\n", _allSolutions.Size(), _allSolutions.Size() == 1 ? "" : "s",  state->winDistance);

  // TODO: I should just compute these while we DFS. This is a waste.
  // TODO: Evaluate additional costs for winning paths (# burned steps, # sausage pushes, # rotations?)
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
    if (depth < _maxDepth) {
      _maxDepth = depth;
      printf("Improved maxDepth to %d\n", depth);
    }
    return const_cast<State*>(&*_visitedNodes.insert(state).first);
  } else {
    state.depth = depth + 1; // TODO: I bet there's a simpler approach here -- like we could put a signal state into the list, instead of repeatedly computing depth.
    // Then, also take the unexploredT logic out -- just do that in the main block. And keep a boolean for 'this is the last loop, guys'
    State* newState = const_cast<State*>(&*_visitedNodes.insert(state).first);
    if (_unexploredT != nullptr) _unexploredT->next = newState;
    _unexploredT = newState;
    return newState;
  }
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
