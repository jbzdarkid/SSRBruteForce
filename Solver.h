#pragma once
#include "Level.h"
#include "WitnessRNG/StdLib.h"

#include <unordered_set>
#include <unordered_map>
#include <vector>

struct Solver {
  Solver(Level* level);

  std::vector<Direction> Solve();

private:
  bool SolveRecursive(u16 depth);
  void FindBestSolutionRecursive(u16 depth, u64 cost);
  void ComputePenaltyAndRecurse(const State& state, Direction dir, u16 depth, u64 cost);
  bool WouldStephenStepOnGrill(Stephen stephen, Direction dir) const;

  Level* _level = nullptr;
  u16 _bestDepth = 0;

  std::unordered_set<State, State> _visited;
  std::unordered_set<State, State> _winning;
  std::unordered_map<State, u64, State> _winningCosts;

  std::vector<Direction> _solution;
  std::vector<Direction> _bestSolution;
  u64 _bestCost = (u64)-1;
  u16 _bestBackwardsMovements = 0;
};
