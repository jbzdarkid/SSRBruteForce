#pragma once
#include "Types.h"
#include "Level.h"
#include "WitnessRNG/StdLib.h"

struct Solver {
  Solver(Level* level);
  ~Solver();

  Vector<Direction> Solve();

private:
  void BFSStateGraph();
  State* GetOrInsertState(u16 depth);

  void CreateShallowStates();
  void ComputeWinningStates();

  void DFSWinStates(State* state, u64 totalMillis, u16 backwardsMovements);
  void ComputePenaltyAndRecurse(State* state, State* nextState, Direction dir, u64 totalMillis, u16 backwardsMovements);

  Level* _level = nullptr;
  NodeHashSet<State> _visitedNodes2 = NodeHashSet<State>(0x7FFFFF); // Choose a relatively large initial size because we'll need it.
  u16 _winningDepth = UNWINNABLE;
  LinkedList<State> _unexplored;
  LinkedList<State> _explored;

  LinearAllocator _shallowAlloc;
  LinkedLoop<ShallowState> _explored2;

  Vector<Direction> _solution;
  Vector<Direction> _bestSolution;
  u64 _bestMillis = (u64)-1;
  u16 _bestBackwardsMovements = 0;
};
