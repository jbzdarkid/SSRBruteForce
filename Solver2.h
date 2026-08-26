#pragma once
#include "Level2.h"
#include "State.h"

#include <absl/container/flat_hash_map.h>
#include <vector>

class Solver {
public:
  Solver(Level* level, u32 numBuckets = 32);

  std::vector<Direction> Solve();

  s32 ComputeScore(const State& state, Direction dir, const State& newState);

private:
  Level* _level = nullptr;
  u32 _numBuckets = 0;

  // Stage 1
  bool _winningStateFound = false;

  u64 ProcessOneLayer(u32 depth);

  // Stage 2
  absl::flat_hash_map<State, u32> _winningStates;

  void FindWinningStates(s32 depth);

  // Stage 3
  std::vector<Direction> FindFastestSolution(const State& initialState);
};