#pragma once
#include "LevelData.h"
#include "WitnessRNG/StdLib.h"

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

#define o(x) +1
  Sausage sausages[SAUSAGES];
#undef o

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

// Per-run salt folded into State2's hash (set once at solver startup). The stage-1 memo stores 64-bit hashes only, so
// two distinct states that share a hash get falsely deduped (one subtree skipped). Re-salting per run makes those rare
// collisions land on DIFFERENT state pairs each time, so running a level 2-3 times and keeping the shortest solution
// drives the miss probability down geometrically. It must be folded INTO the hash (not XOR'd onto the output, which
// would leave colliding pairs colliding); seeding it first perturbs the whole mixing trajectory.
inline u64 g_stateHashSalt = 0;

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
    h = H::combine(std::move(h), g_stateHashSalt);
    return H::combine_contiguous(std::move(h), reinterpret_cast<const u8*>(&s), sizeof(State2));
  }
};