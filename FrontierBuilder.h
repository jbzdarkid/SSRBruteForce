#pragma once
#include "LayerCache.h"
#include "State.h"

#include <vector>

struct IndexedHash {
  u128 hash;
  u32 index;
  bool operator<(const IndexedHash& other) const { return hash < other.hash; }
  bool operator==(const IndexedHash& other) const { return hash == other.hash; }
};

class FrontierBuilder {
public:
  FrontierBuilder(u32 depth, u32 numBuckets);

  void AddStateUnchecked(const State& state);
  u64 ProcessStates();

private:
  std::vector<bool> MergeToDisk(LayerCache<u128>& knownHashes, std::vector<IndexedHash>&& hashesToInsert);

  u32 _numBuckets = 0;
  std::vector<LayerCache<u128>> _knownHashes;
  std::vector<LayerCache<State>> _currentLayer;
  std::vector<LayerCache<State>> _uncheckedStates;
  std::vector<LayerCache<u128>> _uncheckedStateHashes;
};
