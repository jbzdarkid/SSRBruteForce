#pragma once
#include "LevelData.h"
#include "WitnessRNG/StdLib.h"

#include <random>

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

  template <typename H>
  friend H AbslHashValue(H h, const State2& s) {
    h = H::combine(std::move(h), _hashSalt);
    return H::combine_contiguous(std::move(h), reinterpret_cast<const u8*>(&s), sizeof(State2));
  }

private:
  // There is about a 9% chance of a hash collision if we use all 2^31 states that fit in my RAM.
  // Randomize the hash to ensure this won't mess up on subsequent runs.
  static inline u32 _hashSalt = [] { return std::random_device{}(); }();
};