#include "Solver.h"
#include <unordered_set>
#include <unordered_map>

Solver::Solver(Level* level) {
  _level = level;
}

std::unordered_set<State> visitedNodes;
std::unordered_map<State, u8> winningNodes;

Vector<Direction> Solver::Solve(u32 moveLimit) {
  _moveLimit = 9999; // moveLimit
  visitedNodes.clear();
  winningNodes.clear();
  u8 distance = SolveInternal(0);
  Vector<Direction> solution(distance >> 2);
  volatile int k = 1;
  while (distance > 0) {
    Direction dir = (Direction)(distance & 0x4);
    solution.UnsafePush(dir);
    _level->Move(dir);
    State state = _level->GetState();
    distance = winningNodes[state];
  }
  return solution;
}

#define NO_PATH 0xF8

u8 Solver::SolveInternal(u8 depth) {
  if (depth > _moveLimit) return NO_PATH;

  State state = _level->GetState();
  // Do not recurse, as we already have visited this node.
  if (visitedNodes.find(state) != visitedNodes.end()) {
    auto search = winningNodes.find(state);
    if (search != winningNodes.end()) {
      // Not only have already visited this node, we know it results in victory -- and have already computed its shortest path.
      // Return that path so that our parent may compute its shortest path to victory.
      u32 total = (search->second >> 2) + depth;
      if (total < _moveLimit) _moveLimit = total;
      return search->second;
    }
    // Else, this node is not winning (or at least, we do not yet know if it's winning), and someone else is already considering it.
    // How do we know that the 'someone else' has the fastest path to this node? We're DFS not BFS.
    // But, if someone already got to this node, either they're an ancestor (ergo, shorter) *OR* their path has already completed (because DFS).
    // So if it's not winning it's either an ancestor or losing.

    return NO_PATH;
  }
  visitedNodes.insert(state);

  if (_level->Won()) {
    if (depth < _moveLimit) _moveLimit = depth;
    winningNodes[state] = 0;
    // We now need to write the "distance to victory" for each node in our ancestry.
    return 0;
  }

  u8 distanceToVictory = NO_PATH;

  // We encode the distance in the high 6 bits, and the most recent direction in the low 2 bits.
  // This lets us pass the path length alongside the most recent segment.
  if (_level->Move(Up)) {
    u8 distance = SolveInternal(depth + 1) + 0x4 + Up;
    if (distance < distanceToVictory) distanceToVictory = distance;
    _level->SetState(state);
  }

  if (_level->Move(Down)) {
    u8 distance = SolveInternal(depth + 1) + 0x4 + Down;
    if (distance < distanceToVictory) distanceToVictory = distance;
    _level->SetState(state);
  }

  if (_level->Move(Left)) {
    u8 distance = SolveInternal(depth + 1) + 0x4 + Left;
    if (distance < distanceToVictory) distanceToVictory = distance;
    _level->SetState(state);
  }

  if (_level->Move(Right)) {
    u8 distance = SolveInternal(depth + 1) + 0x4 + Right;
    if (distance < distanceToVictory) distanceToVictory = distance;
    _level->SetState(state);
  }

  if (distanceToVictory < NO_PATH) winningNodes[state] = distanceToVictory;

  return distanceToVictory;
}