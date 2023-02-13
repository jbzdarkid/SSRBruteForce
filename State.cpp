#include "State.h"

bool State::operator==(const State& other) const {
  if (stephen != other.stephen) return false;
#define o(x) if (sausages[x] != other.sausages[x]) return false;
    SAUSAGES;
#undef o
  return true;
}

// From MSVC's type_traits
#if defined _WIN64
constexpr size_t _FNV_offset_basis = 14695981039346656037ULL;
constexpr size_t _FNV_prime        = 1099511628211ULL;
constexpr size_t GoldenRatio       = 0x9e3779b97f4a7c15;
#else
constexpr size_t _FNV_offset_basis = 2166136261U;
constexpr size_t _FNV_prime        = 16777619U;
constexpr size_t GoldenRatio       = 0x9e3779b9;
#endif

size_t msvc_hash_internal(u8* bytes, size_t length) {
  size_t h = _FNV_offset_basis;
  for (size_t i=0; i<length; i++) {
    h ^= bytes[i];
    h *= _FNV_prime;
  }
  return h;
}

size_t msvc_hash(u32 value) {
  return msvc_hash_internal((u8*)&value, sizeof(value));
}

size_t msvc_hash(u64 value) {
  return msvc_hash_internal((u8*)&value, sizeof(value));
}

void combine_hash(size_t& a, u64 b) {
  a ^= GoldenRatio + (a << 6) + (a >> 2) + msvc_hash(b);
}

/*
u32 triple32_hash(u32 x) {
  x ^= x >> 16;
  x *= 0x45d9f3bU;
  x ^= x >> 11;
  x *= 0xac4c1b51U;
  x ^= x >> 15;
  x *= 0x31848babU;
  x ^= x >> 14;
  return x;
}

void combine_hash(u32& a, u32 b) {
  a ^= 0x9e3779b9 + (a << 6) + (a >> 2) + triple32_hash(b);
}

size_t triple32_hash(u64 x) {
  u32 a = (u32)x;
  combine_hash(a, (u32)(x >> 32));
  return a;
}

void combine_hash(u32& a, u64 b) {
  combine_hash(a, (u32)b);
  combine_hash(a, (u32)(b >> 32));
}
*/

size_t State::Hash() const {
  static_assert(sizeof(Stephen) == 8);
  static_assert(sizeof(Sausage) == 8);
//   u32 hash = triple32_hash(*(u64*)&stephen);
// #define o(x) combine_hash(hash, *(u64*)&sausages[x]);
//   SAUSAGES
// #undef o

  size_t hash = msvc_hash(*(u64*)&stephen);
#define o(x) combine_hash(hash, *(u64*)&sausages[x]);
  SAUSAGES
#undef o

  return hash;
}
