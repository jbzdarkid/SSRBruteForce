#pragma once
// Common.h -- the project's core header.
//
// Owns the fixed-width type aliases, the debug-break assert macro, and the Vector / NArray container
// wrappers. These formerly came from the WitnessRNG submodule's StdLib.h; providing our own std::vector-backed
// wrappers here decouples the build from the submodule while preserving the exact call-site API
// (operator(), Size(), Push(), Copy(), SortedCopyIntoArray(), ...), so no call sites had to change.
//
// The submodule stays checked out for other reasons, but this build no longer includes its header.
#include <algorithm>       // std::copy, std::copy_n, std::partial_sort_copy
#include <cstring>          // strlen/memset/memcpy, used transitively by callers of this header
#include <initializer_list>
#include <utility>          // std::move
#include <vector>

#include "absl/numeric/int128.h" // u128

using u8 = unsigned char;
using s8 = signed char;
using u16 = unsigned short;
using u32 = unsigned int;
using s32 = int;
using u64 = unsigned long long;
using s64 = long long;
using u128 = absl::uint128;

#ifndef assert
#ifdef _DEBUG
#define assert(cond) \
do { \
  if (!(cond)) { \
    __debugbreak(); \
  } \
} while (0)
#else
#define assert(cond) do {} while (0)
#endif
#endif // #ifndef assert

// A growable array backed by std::vector, exposing the subset of the old WitnessRNG Vector API the project uses.
// Copying is deliberately disabled (use Copy()) to keep the ownership semantics of the original; moving is fine.
template <typename T>
class Vector {
public:
  Vector() = default;
  Vector(const Vector&) = delete;            // copy with Copy()
  Vector& operator=(const Vector&) = delete;
  Vector(Vector&&) noexcept = default;
  Vector& operator=(Vector&&) noexcept = default;

  // Range-based iteration. begin()/end() return raw pointers (not iterators) so a Vector's storage can be passed
  // directly where a `const T*` is expected (e.g. the solver's Settle/MarkDoubleMoves take a `const Sausage*`).
  T* begin() { return _data.data(); }
  const T* begin() const { return _data.data(); }
  T* end() { return _data.data() + _data.size(); }
  const T* end() const { return _data.data() + _data.size(); }

  // Copy |obj| onto the end of the vector, growing if needed.
  void Push(const T& obj) { _data.push_back(obj); }

  // Number of initialized elements.
  int Size() const { return (int)_data.size(); }

  T& operator[](int index) {
    assert(index >= 0 && index < (int)_data.size());
    return _data[index];
  }
  const T& operator[](int index) const {
    assert(index >= 0 && index < (int)_data.size());
    return _data[index];
  }

  // Copy the contents into a new Vector (the sanctioned way to duplicate one).
  Vector<T> Copy() const {
    Vector<T> result;
    result._data = _data;
    return result;
  }

  // Copy the contents into |dest|, a raw array of |destSize| bytes.
  void CopyIntoArray(T* dest, size_t destSize) const {
    assert(destSize == _data.size() * sizeof(T));
    std::copy(_data.begin(), _data.end(), dest);
  }

  // Sort and copy the contents into |dest|, a raw array of |destSize| bytes. |cmp| returns <0, 0, >0 like strcmp.
  using CompareFunc = s8 (*)(const T&, const T&);
  void SortedCopyIntoArray(T* dest, size_t destSize, CompareFunc cmp) const {
    assert(destSize == _data.size() * sizeof(T));
    std::partial_sort_copy(_data.begin(), _data.end(), dest, dest + _data.size(),
                           [cmp](const T& a, const T& b) { return cmp(a, b) < 0; });
  }

  // Set the contents from |src|, a raw array of |srcSize| bytes.
  void CopyFromArray(const T* src, size_t srcSize) {
    assert(srcSize == _data.size() * sizeof(T));
    std::copy_n(src, _data.size(), _data.begin());
  }

private:
  std::vector<T> _data;
};

// An N-dimensional (up to 4D) array backed by a flat std::vector, exposing the subset of the old WitnessRNG NArray
// API the project uses (2D grids). Indexing matches the original: index = ((a*maxB + b)*maxC + c)*maxD + d.
template <typename T>
class NArray {
public:
  NArray() = default;
  NArray(u8 maxA, u8 maxB = 1, u8 maxC = 1, u8 maxD = 1)
    : _maxA(maxA), _maxB(maxB), _maxC(maxC), _maxD(maxD),
      _data((size_t)maxA * maxB * maxC * maxD) {
    assert(maxA > 0 && maxB > 0 && maxC > 0 && maxD > 0);
  }
  NArray(const NArray&) = delete;
  NArray& operator=(const NArray&) = delete;
  NArray(NArray&&) noexcept = default;
  NArray& operator=(NArray&&) noexcept = default;

  T& operator()(u8 a, u8 b = 0, u8 c = 0, u8 d = 0) { return _data[Index(a, b, c, d)]; }
  const T& operator()(u8 a, u8 b = 0, u8 c = 0, u8 d = 0) const { return _data[Index(a, b, c, d)]; }

  void Fill(const T& value) {
    for (T& x : _data) x = value;
  }

private:
  size_t Index(u8 a, u8 b, u8 c, u8 d) const {
    assert(a < _maxA && b < _maxB && c < _maxC && d < _maxD);
    size_t index = a;
    if (_maxB > 1) index = index * _maxB + b;
    if (_maxC > 1) index = index * _maxC + c;
    if (_maxD > 1) index = index * _maxD + d;
    return index;
  }

  int _maxA = 0, _maxB = 0, _maxC = 0, _maxD = 0;
  std::vector<T> _data;
};
