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

#if HASH_CACHING
  size_t hash = 0;
#endif

  bool operator==(const State& other) const;
  size_t Hash() const;
};

namespace std {
template<> struct hash<State> {
  size_t operator()(const State& state) const {
#if HASH_CACHING
  return state.hash;
#else
  return state.Hash();
#endif
  }
};
}
