#pragma once
#include "Types.h"
#include "Level.h"
#include <unordered_set>

struct Solver {
  Solver(Level* level);

  Vector<Direction> Solve(u16 maxDepth);

private:
  State* GetOrInsertState(u16 depth);

  Level* _level = nullptr;
  // u16 _maxDepth = 0;
  std::unordered_set<State> _visitedNodes;

  State* _unexploredH = nullptr;
  State* _unexploredT = nullptr;
  State* _exploredH = nullptr;
  State* _exploredT = nullptr;
};

