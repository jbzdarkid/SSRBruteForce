#pragma once
#include "Types.h"
#include "Level.h"
#include <unordered_set>

struct Solver {
  Solver(Level* level);

  Vector<Direction> Solve();

private:
  State* GetOrInsertState();
  void DFSWinStates(State* state);

  Level* _level = nullptr;
  std::unordered_set<State> _visitedNodes;
  u16 _maxDepth = 0;

  bool _foundWinningState = false;
  State* _unexploredH = nullptr;
  State* _unexploredT = nullptr;
  State* _exploredH = nullptr;
  State* _exploredT = nullptr;

  Vector<Direction> _solution;
  Vector<Vector<Direction>> _allSolutions;
};
