#pragma once
#include "LayerCache.h"
#include "Level.h"
#include "State.h"

#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>
#include <vector>

struct Solver2 {
  Solver2(Level* level);

  std::vector<Direction> Solve();

private:
  Level* _level = nullptr;

  // Stage 1
  u64 _maxStateHashes = 0;
  absl::flat_hash_set<size_t> _exploredStateHashes;
  // ReadableLayerCache<State2> _previousLayer;
  // WritableLayerCache<State2> _currentLayer;
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
