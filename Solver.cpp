#include "Solver.h"
#include "Level.h"
#include <unordered_set>
#include <unordered_map>

Solver::Solver(Level* level) {
  _level = level;
}

std::unordered_set<State> visitedNodes;
std::unordered_map<State, u16> winningNodes;

Vector<Direction> Solver::Solve(u32 moveLimit) {
  _moveLimit = 9999; // moveLimit
  visitedNodes.clear();
  winningNodes.clear();
  State initialState = _level->GetState();

  // IDK, but somehow we aren't finding shortest paths properly.
  // And this is not even sufficient. We somehow are missing winning nodes because we skipped over them.
  SolveInternal(0);
  visitedNodes.clear();
  u16 distance = SolveInternal(0);

  // Once we've done the brute force, unpack the winningNodes to find a solution.
  Vector<Direction> solution(distance >> 4);
  while (distance > 0) {
    // I'm being lazy and only showing one path. But theoretically there are multiple paths, and the data is all here.
    Direction dir = (Direction)(distance & Direction::Any);
    if (dir & Up) dir = Up;
    if (dir & Down) dir = Down;
    if (dir & Left) dir = Left;
    if (dir & Right) dir = Right;

    solution.UnsafePush(dir);
    _level->Move(dir);
    State state = _level->GetState();
    distance = winningNodes[state];
  }
  assert(_level->Won());

  _level->SetState(initialState);
  return solution;
}

#define NO_PATH 0xFFE0

u16 Solver::SolveInternal(u16 depth) {
  if (depth > _moveLimit) return NO_PATH;
  State state = _level->GetState();

  // Do not recurse, as we already have visited this node.
  if (visitedNodes.find(state) != visitedNodes.end()) {
    auto search = winningNodes.find(state);
    if (search != winningNodes.end()) {
      // Not only have already visited this node, we know it results in victory -- and have already computed its shortest path.
      // Return that path so that our parent may compute its shortest path to victory.
      // u32 total = (search->second >> 2) + depth;
      // if (total < _moveLimit) _moveLimit = total;
      return search->second;
    }
    // Else, this node is not winning (or at least, we do not yet know if it's winning), and someone else is already considering it.
    // How do we know that the 'someone else' has the fastest path to this node? We're DFS not BFS.
    // Ergo, if someone already got to this node, either they're an ancestor (ergo, shorter) *OR* their path has already completed (because DFS).
    // So if it's not winning it's either an ancestor or losing.
    // ^ This is apparently wrong in some cases.

    return NO_PATH;
  }
  visitedNodes.insert(state);

  if (_level->Won()) {
    if (depth < _moveLimit) _moveLimit = depth;
    winningNodes[state] = 0;
    // We now need to write the "distance to victory" for each node in our ancestry.
    return 0;
  }

  u16 distanceToVictory = NO_PATH;
  u8 victoryDirections = 0;

  // We encode the distance in the high 12 bits, and all optimal directions in the low 4 bits.
  // This lets us pass the path length alongside the solution tree

#define TRY_MOVE(dir) \
  if (_level->Move(dir)) { \
    u16 distance = SolveInternal(depth + 1) & ~Direction::Any; \
    if (distance < distanceToVictory) { \
      distanceToVictory = distance; \
      victoryDirections = dir; \
    } else if (distance == distanceToVictory) { \
      victoryDirections |= dir; \
    } \
    _level->SetState(state); \
  }

  TRY_MOVE(Up);
  TRY_MOVE(Down);
  TRY_MOVE(Left);
  TRY_MOVE(Right);

  // Add one (because we are now one step further from victory) as well as all valid directions which lead to victory.
  distanceToVictory = distanceToVictory + 0x10 + victoryDirections;
  if (distanceToVictory < NO_PATH) winningNodes[state] = distanceToVictory;
  return distanceToVictory;
}