#pragma once
#include "LargePageAllocator.h"
#include "LayerCache.h"
#include "Level.h"
#include "State.h"

#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>
#include <absl/hash/hash.h>
#include <functional>
#include <vector>

struct Solver2 {
  Solver2(Level* level, u32 hashtableSize = 27);

  std::vector<Direction> Solve();

private:
  Level* _level = nullptr;

  // Stage 1
  u64 _maxStateHashes = 0;
  // Backed by large pages when built with USE_LARGE_PAGES; otherwise identical to the default allocator.
  absl::flat_hash_set<size_t, absl::Hash<size_t>, std::equal_to<size_t>, LargePageAllocator<size_t>> _exploredStateHashes;
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
