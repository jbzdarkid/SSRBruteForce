#pragma once
#include "Types.h"
#include "Level.h"
#include <unordered_set>

struct Solver {
  Solver(Level* level);

  Vector<Direction> Solve();

private:
  void BFSStateGraph();
  State* GetOrInsertState(u16 depth);

  void ComputeWinningStates();

  void DFSWinStates(State* state, u64 totalMillis, u16 backwardsMovements);
  void ComputePenaltyAndRecurse(State* state, State* nextState, Direction dir, u64 totalMillis, u16 backwardsMovements);

  Level* _level = nullptr;
  std::unordered_set<State> _visitedNodes;
  u16 _maxDepth = 0;

  bool _foundWinningState = false;
  LinkedList<State> _unexplored;
  LinkedLoop<State> _explored;

  Vector<Direction> _solution;
  Vector<Direction> _bestSolution;
  u64 _bestMillis = (u64)-1;
  u16 _bestBackwardsMovements = 0;
};
