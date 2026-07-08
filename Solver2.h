#pragma once
#include "Level.h"
#include "State.h"

#include <absl/container/flat_hash_map.h>
#include <vector>

class Solver2 {
public:
  Solver2(Level* level, u32 numBuckets = 32);

  std::vector<Direction> Solve();

private:
  Level* _level = nullptr;
  u32 _numBuckets = 0;

  // Stage 1
  bool _winningStateFound = false;

  void ProcessOneLayer(u32 depth);

  // Stage 2
  absl::flat_hash_map<State2, u32> _winningStates;

  void FindWinningStates(u32 depth);

  void FindFastestSolution(const State2& state, std::vector<Direction>& solution, u32 score);
  u32 ComputeScore(const State2& state, Direction dir, const State2& newState2);

  // Stage 3
  u32 _bestScore = 0xFFFF'FFFF;
  std::vector<Direction> _bestSolution;
};