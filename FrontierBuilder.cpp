#include "FrontierBuilder.h"

#include <algorithm>
#include <vector>

FrontierBuilder::FrontierBuilder(u32 depth, u32 numBuckets) {
  _numBuckets = numBuckets;

  _knownHashes.resize(_numBuckets);
  _currentLayer.resize(_numBuckets);
  _uncheckedStates.resize(_numBuckets);
  _uncheckedStateHashes.resize(_numBuckets);

  for (u32 bucket = 0; bucket < _numBuckets; bucket++) {
    _knownHashes[bucket] = LayerCache<u128>("depth", "global", "bucket", bucket);

    _currentLayer[bucket] = LayerCache<State>("depth", depth, "bucket", bucket);

    _uncheckedStates[bucket] = LayerCache<State>("depth", "scratch", "bucket", bucket);
    _uncheckedStateHashes[bucket] = LayerCache<u128>("depth", "scratch", "bucket", bucket, "hash");
  }
}

void FrontierBuilder::AddStateUnchecked(const State& state) {
  u128 hash = state.Hash128();
  u32 bucket = Uint128High64(hash) % _numBuckets;
  _uncheckedStates[bucket].Add(state);
  _uncheckedStateHashes[bucket].Add(hash);
}

u64 FrontierBuilder::ProcessStates() {
  u64 totalNewStates = 0;
  for (u32 bucket = 0; bucket < _numBuckets; bucket++) {
    _uncheckedStates[bucket].FinishWriteAndResetRead();
    _uncheckedStateHashes[bucket].FinishWriteAndResetRead();

    // Sort the *hashes* of the new states, retaining their original index (to look up the original state).
    std::vector<IndexedHash> hashesToInsert;
    u32 i = 0;
    for (u128 hash : _uncheckedStateHashes[bucket]) hashesToInsert.emplace_back(hash, i++);

    std::vector<bool> insertedHashes = MergeToDisk(_knownHashes[bucket], std::move(hashesToInsert));
    _knownHashes[bucket].FinishWriteAndResetRead();

    i = 0;
    for (const State& state : _uncheckedStates[bucket]) {
      // Add the state if its hash was newly added to the knownHashes
      if (insertedHashes[i++]) {
        _currentLayer[bucket].Add(state);
        totalNewStates++;
      }
    }

    _currentLayer[bucket].FinishWriteAndResetRead();
  }

  return totalNewStates;
}

std::vector<bool> FrontierBuilder::MergeToDisk(LayerCache<u128>& knownHashes, std::vector<IndexedHash>&& hashesToInsert) {
  std::vector<bool> insertedHashes(hashesToInsert.size(), false);

  std::sort(hashesToInsert.begin(), hashesToInsert.end());
  hashesToInsert.erase(std::unique(hashesToInsert.begin(), hashesToInsert.end()), hashesToInsert.end());

  size_t j = 0;
  for (u128 existing : knownHashes) {
    // Add any hashes that are below the current element
    while (j < hashesToInsert.size() && hashesToInsert[j].hash < existing) {
      knownHashes.Add(hashesToInsert[j].hash);
      insertedHashes[hashesToInsert[j].index] = true;
      j++;
    }

    if (j < hashesToInsert.size() && hashesToInsert[j].hash == existing) j++; // Skip already-inserted hashes
    knownHashes.Add(existing);
  }

  // Add any hashes above the highest existing element
  while (j < hashesToInsert.size()) {
    knownHashes.Add(hashesToInsert[j].hash);
    insertedHashes[hashesToInsert[j].index] = true;
    j++;
  }

  return insertedHashes;
}
