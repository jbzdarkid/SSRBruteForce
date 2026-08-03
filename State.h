#pragma once
#include "LevelData.h"
#include "WitnessRNG/StdLib.h"

#include <cstring>
#include <random>

#include <absl/hash/hash.h>

struct State {
  Stephen stephen;
  Sausage sausages[NUM_SAUSAGES];
#if OVERWORLD_HACK
  u64 overworldSausages = 0;
#endif

  bool operator==(const State& other) const {
    if (stephen != other.stephen) return false;
#if OVERWORLD_HACK
    if (overworldSausages != other.overworldSausages) return false;
#endif
#define o(x) if (sausages[x] != other.sausages[x]) return false;
    SAUSAGES
#undef o
    return true;
  }

  bool operator<(const State& other) const {
    return std::memcmp(this, &other, sizeof(State)) < 0;
  }

  friend std::ostream& operator<<(std::ostream& o, const State& s) {
    o << s.stephen;
    for (const Sausage& sausage : s.sausages) o << " | " << sausage;
    return o;
  }

  template <typename H>
  friend H AbslHashValue(H hash, const State& state) {
    return H::combine_contiguous(std::move(hash), reinterpret_cast<const u8*>(&state), sizeof(State));
  }

  u128 Hash128() const {
    // Two separate seeds because absl picks a random seed per process, and this way we get a fully random 128 bits.
    static const u32 seedA = std::random_device{}();
    static const u32 seedB = std::random_device{}();
    return absl::MakeUint128(absl::HashOf(seedA, *this), absl::HashOf(seedB, *this));
  }
};
