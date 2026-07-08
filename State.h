#pragma once
#include "LevelData.h"
#include "WitnessRNG/StdLib.h"

#include <cstring>
#include <random>

#include <absl/hash/hash.h>

// This is a shallow copy of the State struct -- it does not include positions of stephen nor the sausages
// and is thus much smaller. Fortunately, we can determine the shortest path without knowledge of actual state.
struct ShallowState {
  ShallowState* next = nullptr;
  ShallowState* u = nullptr;
  ShallowState* d = nullptr;
  ShallowState* l = nullptr;
  ShallowState* r = nullptr;

#define UNWINNABLE 0xFFFE
  u16 winDistance = UNWINNABLE;
};

struct State {
  Stephen stephen;

  Sausage sausages[NUM_SAUSAGES];

  // Used to build the tree, ergo not part of the hashing or comparison algos
  State* next = nullptr;
  State* u = nullptr;
  State* d = nullptr;
  State* l = nullptr;
  State* r = nullptr;
  ShallowState* shallow; // TODO: Consider the benefits of using shallow->l = (ShallowState*)state as a placeholder?

  bool operator==(const State& other) const;
  size_t Hash() const;
};

namespace std {
template<> struct hash<State> {
  size_t operator()(const State& state) const {
  return state.Hash();
  }
};
}

// New version for Solver2
struct State2 {
  Stephen stephen;
  Sausage sausages[NUM_SAUSAGES];

  bool operator==(const State2& other) const {
    if (stephen != other.stephen) return false;
#define o(x) if (sausages[x] != other.sausages[x]) return false;
    SAUSAGES
#undef o
    return true;
  }

  bool operator<(const State2& other) const {
    return std::memcmp(this, &other, sizeof(State2)) < 0;
  }

  template <typename H>
  friend H AbslHashValue(H hash, const State2& state) {
    return H::combine_contiguous(std::move(hash), reinterpret_cast<const u8*>(&state), sizeof(State2));
  }

  u128 Hash128() const {
    // Two separate seeds because absl picks a random seed per process, and this way we get a fully random 128 bits.
    static const u32 seedA = std::random_device{}();
    static const u32 seedB = std::random_device{}();
    return absl::MakeUint128(absl::HashOf(seedA, this), absl::HashOf(seedB, this));
  }
};
