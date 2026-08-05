#include "CppUnitTest.h"
#include "LayerCache.h" // pulls in u128 (absl::uint128) via WitnessRNG/StdLib.h
#include "State.h"     // State (the record we sort + delta-encode)
#include "Levels.h"    // ColdTrail (to benchmark real Move expansion on cached states)

#include <absl/container/flat_hash_map.h> // mirrors Solver's _winningStates for the Stage-2 bench
#include <absl/container/flat_hash_set.h> // BFS closed set for the endgame-explosion proof

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <intrin.h>
#include <limits>
#include <string>
#include <utility>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

// Measures how well the sorted known-hash set (cache/_depth_global_bucket_*.bin) would shrink under
// delta encoding, and whether "double-delta" (delta-of-deltas) beats plain single-delta.
//
// Theory being checked: the hashes are ~uniform-random 128-bit values, so the sorted gaps are i.i.d.
// and roughly exponential with mean 2^128 / N. Because the difference of two i.i.d. exponentials is
// Laplace -- and |Laplace| is again exponential with the SAME mean -- the second differences have the
// same magnitude distribution as the first differences, plus a sign bit. So double-delta should cost
// ~1 bit/entry MORE than single-delta here, i.e. it should NOT help for random hashes.
namespace {
  // Significant bits of a 128-bit value (0 for zero).
  int SignificantBits(u128 v) {
    u64 hi = Uint128High64(v);
    if (hi != 0) return 128 - std::countl_zero(hi);
    u64 lo = Uint128Low64(v);
    if (lo != 0) return 64 - std::countl_zero(lo);
    return 0;
  }

  // Walk up from the current directory looking for a folder that contains cache/<relFile>.
  // If found, chdir into it (so LayerCache's relative "cache/..." path resolves) and return true.
  bool ChdirToCacheRoot(const std::string& relFile) {
    std::filesystem::path dir = std::filesystem::current_path();
    for (int i = 0; i < 10; i++) {
      std::error_code ec;
      if (std::filesystem::exists(dir / "cache" / relFile, ec)) {
        std::filesystem::current_path(dir);
        return true;
      }
      if (!dir.has_parent_path() || dir.parent_path() == dir) break;
      dir = dir.parent_path();
    }
    return false;
  }

  // Walk up from the current directory to find a folder that contains a "cache" subdirectory; chdir into it.
  bool ChdirToCacheDir() {
    std::filesystem::path dir = std::filesystem::current_path();
    for (int i = 0; i < 10; i++) {
      std::error_code ec;
      if (std::filesystem::is_directory(dir / "cache", ec)) {
        std::filesystem::current_path(dir);
        return true;
      }
      if (!dir.has_parent_path() || dir.parent_path() == dir) break;
      dir = dir.parent_path();
    }
    return false;
  }

  // Pick a per-depth layer file (bucket 0) whose on-disk size is closest to ~50 MB, and return its depth.
  // Excludes the "global" (known-hash) and "scratch" files. Assumes CWD is already the cache root. -1 if none.
  int FindIntermediateLayerDepth() {
    const std::string pre = "_depth_", suf = "_bucket_0.bin";
    const long long target = 50LL * 1024 * 1024;
    int bestDepth = -1;
    long long bestScore = (std::numeric_limits<long long>::max)();
    std::error_code ec;
    for (auto& entry : std::filesystem::directory_iterator(std::filesystem::path("cache"), ec)) {
      if (ec) break;
      if (!entry.is_regular_file()) continue;
      const std::string name = entry.path().filename().string();
      if (name.size() <= pre.size() + suf.size()) continue;
      if (name.compare(0, pre.size(), pre) != 0) continue;
      if (name.compare(name.size() - suf.size(), suf.size(), suf) != 0) continue;
      const std::string mid = name.substr(pre.size(), name.size() - pre.size() - suf.size());
      if (mid.empty() || !std::all_of(mid.begin(), mid.end(), [](char c) { return c >= '0' && c <= '9'; })) continue;
      long long diff = (long long)entry.file_size(ec) - target;
      if (diff < 0) diff = -diff;
      if (diff < bestScore) { bestScore = diff; bestDepth = std::stoi(mid); }
    }
    return bestDepth;
  }

  // Fixed-size record = one State. The scheme below only ever treats it as kRec raw bytes, so it works
  // for any record size / sausage count without per-field re-encoding.
  constexpr size_t kRec = sizeof(State);
  using Rec = std::array<u8, kRec>;

  u32 CommonPrefixLen(const Rec& a, const Rec& b) {
    u32 i = 0;
    while (i < kRec && a[i] == b[i]) i++;
    return i;
  }

  int VarintLen(u64 v) { int n = 1; while (v >= 0x80) { v >>= 7; n++; } return n; }
  void PutVarint(std::vector<u8>& o, u64 v) { while (v >= 0x80) { o.push_back((u8)(v | 0x80)); v >>= 7; } o.push_back((u8)v); }
  u64 GetVarint(const u8*& p) { u64 v = 0; int s = 0; u8 b; do { b = *p++; v |= (u64)(b & 0x7F) << s; s += 7; } while (b & 0x80); return v; }

  // Small stable hash used only to draw a uniform sub-sample (via trailing-zero count) of the state set.
  u64 Fnv1a(const void* p, size_t n) {
    const u8* b = (const u8*)p;
    u64 h = 1469598103934665603ULL;
    for (size_t i = 0; i < n; i++) { h ^= b[i]; h *= 1099511628211ULL; }
    return h;
  }

  struct FrontCodeStats { double bytesPerEntry; double avgCommon; };

  // Front-code (a.k.a. prefix-delta) an already-sorted sequence: per record store the common-prefix
  // length (varint) plus the differing suffix. Returns the amortized cost, no buffer materialized.
  FrontCodeStats MeasureFrontCode(const std::vector<Rec>& sorted) {
    u64 enc = 0, sumCommon = 0;
    for (size_t i = 0; i < sorted.size(); i++) {
      u32 c = (i == 0) ? 0 : CommonPrefixLen(sorted[i - 1], sorted[i]);
      enc += VarintLen(c) + (kRec - c);
      sumCommon += c;
    }
    const size_t n = sorted.size();
    return { (double)enc / n, n > 1 ? (double)sumCommon / (n - 1) : 0.0 };
  }

  // Byte length of the LEB128 varint of (cur - prev), treating each record as a big-endian integer.
  // memcmp order == big-endian unsigned order, so for sorted records cur >= prev and the delta is >= 0.
  u32 NumericDeltaVarintLen(const Rec& prev, const Rec& cur) {
    u8 d[kRec];
    int borrow = 0;
    for (int i = (int)kRec - 1; i >= 0; i--) {
      int diff = (int)cur[i] - (int)prev[i] - borrow;
      if (diff < 0) { diff += 256; borrow = 1; } else { borrow = 0; }
      d[i] = (u8)diff;
    }
    size_t firstNZ = 0;
    while (firstNZ < kRec && d[firstNZ] == 0) firstNZ++;
    if (firstNZ == kRec) return 1; // delta == 0 (a deduped set shouldn't produce this)
    int bitLen = (int)((kRec - 1 - firstNZ) * 8) + (8 - std::countl_zero(d[firstNZ]));
    return (u32)((bitLen + 6) / 7); // ceil(bitLen / 7) 7-bit groups
  }

  // --- 256-bit big integer (4 little-endian u64 limbs) over a 32-byte big-endian record, for the
  //     numeric value-delta codec (encode cur-prev, decode prev+delta). Used by the perf benchmark. ---
  // static_assert(sizeof(State) == 32, "numeric-delta codec assumes a 32-byte (4-limb) record");
  struct U256 { u64 limb[4]; };
  U256 ToU256(const Rec& r) { // r is big-endian (r[0] = MSB)
    U256 v{};
    for (int k = 0; k < 4; k++) {
      const u8* p = r.data() + (kRec - 8 * (k + 1));
      u64 x = 0; for (int b = 0; b < 8; b++) x = (x << 8) | p[b];
      v.limb[k] = x;
    }
    return v;
  }
  void FromU256(const U256& v, Rec& r) {
    for (int k = 0; k < 4; k++) {
      u8* p = r.data() + (kRec - 8 * (k + 1));
      u64 x = v.limb[k]; for (int b = 7; b >= 0; b--) { p[b] = (u8)(x & 0xFF); x >>= 8; }
    }
  }
  bool IsZero(const U256& v) { return (v.limb[0] | v.limb[1] | v.limb[2] | v.limb[3]) == 0; }
  U256 Sub(const U256& a, const U256& b) { // a >= b
    U256 r{}; u64 borrow = 0;
    for (int k = 0; k < 4; k++) {
      u64 sub = b.limb[k] + borrow;
      u64 nb = (sub < b.limb[k]) ? 1u : 0u; // bk + borrow overflowed
      if (a.limb[k] < sub) nb = 1;
      r.limb[k] = a.limb[k] - sub;
      borrow = nb;
    }
    return r;
  }
  U256 Add(const U256& a, const U256& b) {
    U256 r{}; u64 carry = 0;
    for (int k = 0; k < 4; k++) {
      u64 s = a.limb[k] + b.limb[k];
      u64 c1 = (s < a.limb[k]) ? 1u : 0u;
      u64 s2 = s + carry;
      u64 c2 = (s2 < s) ? 1u : 0u;
      r.limb[k] = s2; carry = c1 | c2;
    }
    return r;
  }
  void ShiftRight7(U256& d) {
    d.limb[0] = (d.limb[0] >> 7) | (d.limb[1] << 57);
    d.limb[1] = (d.limb[1] >> 7) | (d.limb[2] << 57);
    d.limb[2] = (d.limb[2] >> 7) | (d.limb[3] << 57);
    d.limb[3] = (d.limb[3] >> 7);
  }
  void OrBitsAt(U256& d, u64 v, int shift) {
    int limb = shift >> 6, off = shift & 63;
    if (limb < 4) d.limb[limb] |= (v << off);
    if (off > 57 && limb + 1 < 4) d.limb[limb + 1] |= (v >> (64 - off));
  }
  void EncodeNumericDelta(std::vector<u8>& out, const U256& prev, const U256& cur) {
    U256 d = Sub(cur, prev);
    for (;;) {
      u8 byte = (u8)(d.limb[0] & 0x7F);
      ShiftRight7(d);
      if (IsZero(d)) { out.push_back(byte); break; }
      out.push_back((u8)(byte | 0x80));
    }
  }
  U256 DecodeNumericDelta(const u8*& p) {
    U256 d{}; int shift = 0;
    for (;;) { u8 byte = *p++; OrBitsAt(d, (u64)(byte & 0x7F), shift); shift += 7; if (!(byte & 0x80)) break; }
    return d;
  }

  // Same subtraction/addition, but via the sbb/adc carry intrinsics (to see if manual carry logic was the cost).
  U256 SubI(const U256& a, const U256& b) {
    U256 r{}; unsigned char bor = 0;
    bor = _subborrow_u64(bor, a.limb[0], b.limb[0], &r.limb[0]);
    bor = _subborrow_u64(bor, a.limb[1], b.limb[1], &r.limb[1]);
    bor = _subborrow_u64(bor, a.limb[2], b.limb[2], &r.limb[2]);
    bor = _subborrow_u64(bor, a.limb[3], b.limb[3], &r.limb[3]);
    return r;
  }
  U256 AddI(const U256& a, const U256& b) {
    U256 r{}; unsigned char car = 0;
    car = _addcarry_u64(car, a.limb[0], b.limb[0], &r.limb[0]);
    car = _addcarry_u64(car, a.limb[1], b.limb[1], &r.limb[1]);
    car = _addcarry_u64(car, a.limb[2], b.limb[2], &r.limb[2]);
    car = _addcarry_u64(car, a.limb[3], b.limb[3], &r.limb[3]);
    return r;
  }
  void EncodeNumericDeltaI(std::vector<u8>& out, const U256& prev, const U256& cur) {
    U256 d = SubI(cur, prev);
    for (;;) {
      u8 byte = (u8)(d.limb[0] & 0x7F);
      ShiftRight7(d);
      if (IsZero(d)) { out.push_back(byte); break; }
      out.push_back((u8)(byte | 0x80));
    }
  }

  // A byte permutation that puts low-entropy positions (constants, padding) first so the sorted stream
  // shares long prefixes -- i.e. we CHOOSE the total ordering to make the deltas as small as possible.
  std::array<u8, kRec> EntropyPermutation(const std::vector<Rec>& recs) {
    std::vector<std::array<u64, 256>> hist(kRec);
    for (auto& h : hist) h.fill(0);
    for (auto& r : recs) for (size_t p = 0; p < kRec; p++) hist[p][r[p]]++;
    std::array<double, kRec> H{};
    for (size_t p = 0; p < kRec; p++) {
      double h = 0;
      for (int v = 0; v < 256; v++) if (hist[p][v]) { double pr = (double)hist[p][v] / recs.size(); h -= pr * std::log2(pr); }
      H[p] = h;
    }
    std::array<u8, kRec> perm; for (u8 i = 0; i < kRec; i++) perm[i] = i;
    std::sort(perm.begin(), perm.end(), [&](u8 a, u8 b) { return H[a] < H[b]; });
    return perm;
  }

  Rec Permute(const Rec& r, const std::array<u8, kRec>& perm) {
    Rec o; for (size_t i = 0; i < kRec; i++) o[i] = r[perm[i]]; return o;
  }
}

TEST_CLASS(DeltaEncodingTests) {
public:
  // Investigation, not a pass/fail behavior test: it reports bytes/entry for raw vs single-delta vs
  // double-delta, and asserts the expected ordering (single-delta helps; double-delta does not).
  // Skips gracefully (passes) when no compressed cache file is present.
  TEST_METHOD(HashSingleVsDoubleDelta) {
    const std::string fileRel = "_depth_global_bucket_0.bin";
    const std::filesystem::path savedCwd = std::filesystem::current_path();

    if (!ChdirToCacheRoot(fileRel)) {
      Logger::WriteMessage("SKIP: cache/_depth_global_bucket_0.bin not found; run a -Zstd solve first.\n");
      return;
    }

    const u64 maxSamples = 5'000'000; // gap statistics are stationary, so a few frames is plenty

    u64 n = 0, outOfOrder = 0, deltaCount = 0, doubleCount = 0;
    u128 prevHash = 0, prevDelta = 0;
    bool haveHash = false, haveDelta = false;
    double sumSingleBits = 0, sumDoubleBits = 0;
    u64 sumSingleBytes = 0, sumDoubleBytes = 0;

    try {
      LayerCache<u128> known("depth", "global", "bucket", 0);
      for (u128 h : known) {
        if (haveHash) {
          if (h < prevHash) outOfOrder++;
          u128 d = h - prevHash; // sorted => non-negative gap
          int db = SignificantBits(d);
          sumSingleBits += db;
          sumSingleBytes += (db + 7) / 8;
          deltaCount++;

          if (haveDelta) {
            // Second difference is signed; store magnitude bits + 1 sign bit.
            u128 mag = (d >= prevDelta) ? (d - prevDelta) : (prevDelta - d);
            int mb = SignificantBits(mag);
            sumDoubleBits += (double)mb + 1.0;   // +1 sign bit
            sumDoubleBytes += (mb + 1 + 7) / 8;  // magnitude + sign, byte-granular
            doubleCount++;
          }
          prevDelta = d;
          haveDelta = true;
        }
        prevHash = h;
        haveHash = true;
        if (++n >= maxSamples) break;
      }
    } catch (const std::exception& e) {
      std::filesystem::current_path(savedCwd);
      Logger::WriteMessage((std::string("SKIP: could not read cache (") + e.what()
        + "). Was it built with -Zstd?\n").c_str());
      return;
    }

    std::filesystem::current_path(savedCwd);

    if (deltaCount == 0) {
      Logger::WriteMessage("SKIP: cache file had no samples.\n");
      return;
    }

    const double singleBits  = sumSingleBits / deltaCount;
    const double singleBytes = (double)sumSingleBytes / deltaCount;
    const double doubleBits  = doubleCount ? sumDoubleBits / doubleCount : 0.0;
    const double doubleBytes = doubleCount ? (double)sumDoubleBytes / doubleCount : 0.0;

    Logger::WriteMessage(std::format(
      "\n=== Known-hash delta-encoding investigation ===\n"
      "samples (hashes)         : {}\n"
      "out-of-order pairs       : {}\n"
      "raw                      : 128.00 bits  16.00 bytes/entry  (1.00x)\n"
      "single-delta             : {:6.2f} bits  {:5.2f} bytes/entry  ({:.2f}x)\n"
      "double-delta (of deltas) : {:6.2f} bits  {:5.2f} bytes/entry  ({:.2f}x)\n"
      "double vs single         : {:+.2f} bits/entry\n"
      "conclusion               : {}\n",
      n, outOfOrder,
      singleBits, singleBytes, 16.0 / singleBytes,
      doubleBits, doubleBytes, 16.0 / doubleBytes,
      doubleBits - singleBits,
      (doubleBits < singleBits ? "double-delta HELPS (unexpected)"
                               : "double-delta does NOT help (expected for random hashes)")).c_str());

    // Delta-coding only makes sense on a sorted stream.
    Assert::IsTrue(outOfOrder == 0, L"known-hash file is not sorted ascending");
    // Single-delta must recover real savings (~98 bits) vs the 128 raw bits.
    Assert::IsTrue(singleBits < 120.0, L"single-delta did not compress as expected");
    // Double-delta should not beat single-delta for uniform-random hashes.
    Assert::IsTrue(doubleBits >= singleBits, L"double-delta unexpectedly beat single-delta");
  }

  // Establishes a TOTAL ORDERING over raw states and delta-encodes the sorted stream via front-coding
  // (shared-prefix elision). This scales to any record size and needs no per-field bit-packing. Also tries
  // an entropy-optimized byte ordering, since the ordering is the knob that controls how small the deltas get.
  TEST_METHOD(StateTotalOrderingFrontCoding) {
    const std::filesystem::path savedCwd = std::filesystem::current_path();
    if (!ChdirToCacheDir()) {
      Logger::WriteMessage("SKIP: no cache/ directory found.\n");
      return;
    }
    const int depth = FindIntermediateLayerDepth();
    if (depth < 0) {
      std::filesystem::current_path(savedCwd);
      Logger::WriteMessage("SKIP: no intermediate _depth_<N>_bucket_0.bin layer found.\n");
      return;
    }

    std::vector<Rec> recs;
    const size_t cap = 12'000'000; // bounds memory; a ~50 MB (on-disk) layer holds well under this
    try {
      LayerCache<State> layer("depth", depth, "bucket", 0);
      for (const State& s : layer) {
        Rec r; std::memcpy(r.data(), &s, kRec);
        recs.push_back(r);
        if (recs.size() >= cap) break;
      }
    } catch (const std::exception& e) {
      std::filesystem::current_path(savedCwd);
      Logger::WriteMessage((std::string("SKIP: could not read layer (") + e.what() + "). Built with -Zstd?\n").c_str());
      return;
    }
    std::filesystem::current_path(savedCwd);
    if (recs.size() < 2) { Logger::WriteMessage("SKIP: too few states.\n"); return; }
    const size_t n = recs.size();

    // --- Natural total ordering (State's memcmp order) ---
    std::sort(recs.begin(), recs.end());
    const FrontCodeStats natural = MeasureFrontCode(recs);

    // "Just reuse LayerCache<State> + zstd" option: zstd-3 on the raw sorted state bytes (no custom codec).
    std::vector<u8> zbuf(ZSTD_compressBound(n * kRec));
    const double zstdNatural = (double)ZSTD_compress(zbuf.data(), zbuf.size(), recs.data(), n * kRec, 3) / n;

    // Round-trip the first R records to prove the delta codec is lossless.
    const size_t R = (std::min)(n, (size_t)1'000'000);
    std::vector<u8> enc;
    for (size_t i = 0; i < R; i++) {
      u32 c = (i == 0) ? 0 : CommonPrefixLen(recs[i - 1], recs[i]);
      PutVarint(enc, c);
      enc.insert(enc.end(), recs[i].begin() + c, recs[i].end());
    }
    const u8* p = enc.data();
    Rec prev{}; size_t mismatches = 0;
    for (size_t i = 0; i < R; i++) {
      u32 c = (u32)GetVarint(p);
      Rec cur;
      std::memcpy(cur.data(), prev.data(), c);
      std::memcpy(cur.data() + c, p, kRec - c); p += kRec - c;
      if (cur != recs[i]) mismatches++;
      prev = cur;
    }

    // Numeric value-delta: treat each record as a big-endian integer and store varint(cur - prev).
    u64 deltaBytes = 0;
    Rec prevRec{};
    for (const Rec& r : recs) { deltaBytes += NumericDeltaVarintLen(prevRec, r); prevRec = r; }
    const double numericDelta = (double)deltaBytes / n;

    // --- Entropy-optimized total ordering (low-entropy bytes first) ---
    const std::array<u8, kRec> perm = EntropyPermutation(recs);
    for (auto& r : recs) r = Permute(r, perm);
    std::sort(recs.begin(), recs.end());
    const FrontCodeStats entropyOrder = MeasureFrontCode(recs);
    const double zstdEntropy = (double)ZSTD_compress(zbuf.data(), zbuf.size(), recs.data(), n * kRec, 3) / n;

    // Numeric value-delta on the SAME entropy-permuted order (constant bytes are now high-order and cancel).
    u64 deltaBytesPerm = 0;
    Rec prevPerm{};
    for (const Rec& r : recs) { deltaBytesPerm += NumericDeltaVarintLen(prevPerm, r); prevPerm = r; }
    const double numericDeltaPerm = (double)deltaBytesPerm / n;

    Logger::WriteMessage(std::format(
      "\n=== State total-ordering: front-coding vs numeric delta vs zstd ===\n"
      "layer file               : cache/_depth_{}_bucket_0.bin\n"
      "states sampled           : {}\n"
      "record size              : {} bytes\n"
      "raw                      : {} bytes/entry (1.00x)\n"
      "hash baseline (current)  : 16 bytes/entry\n"
      "front-code nat (memcmp)  : {:.2f} bytes/entry ({:.2f}x)  avg shared prefix {:.1f} B\n"
      "front-code entropy-perm  : {:.2f} bytes/entry ({:.2f}x)  avg shared prefix {:.1f} B\n"
      "numeric delta  (memcmp)  : {:.2f} bytes/entry ({:.2f}x)  varint(cur-prev)\n"
      "numeric delta  entropy   : {:.2f} bytes/entry ({:.2f}x)  varint on permuted order\n"
      "zstd sorted    (natural) : {:.2f} bytes/entry ({:.2f}x)  LayerCache<State>+zstd, no custom codec\n"
      "zstd sorted    (entropy) : {:.2f} bytes/entry ({:.2f}x)\n"
      "round-trip mismatches    : {} (of {} checked)\n",
      depth, n, kRec, kRec,
      natural.bytesPerEntry, kRec / natural.bytesPerEntry, natural.avgCommon,
      entropyOrder.bytesPerEntry, kRec / entropyOrder.bytesPerEntry, entropyOrder.avgCommon,
      numericDelta, kRec / numericDelta,
      numericDeltaPerm, kRec / numericDeltaPerm,
      zstdNatural, kRec / zstdNatural,
      zstdEntropy, kRec / zstdEntropy,
      mismatches, R).c_str());

    Assert::IsTrue(mismatches == 0, L"front-coding round-trip is not lossless");
    Assert::IsTrue(natural.bytesPerEntry < (double)kRec, L"front-coding did not reduce size");
  }

  // Confirms the scheme scales to the FULL known-set (not just one depth). The full unique-state set for a
  // bucket is the UNION of every _depth_<N>_bucket_0.bin (each unique state lives in exactly one depth layer,
  // so the union is duplicate-free). We can't hold ~800M states in RAM, so we take a uniform sub-sample of the
  // whole union at three nested densities and show bytes/entry falls monotonically as density rises -- meaning
  // the full (denser) set compresses at least as well as the densest sample we measured.
  TEST_METHOD(FullSetScalingByDensity) {
    const std::filesystem::path savedCwd = std::filesystem::current_path();
    if (!ChdirToCacheDir()) { Logger::WriteMessage("SKIP: no cache/ directory found.\n"); return; }

    std::vector<int> depths;
    {
      const std::string pre = "_depth_", suf = "_bucket_0.bin";
      std::error_code ec;
      for (auto& e : std::filesystem::directory_iterator(std::filesystem::path("cache"), ec)) {
        if (ec) break;
        const std::string name = e.path().filename().string();
        if (name.size() <= pre.size() + suf.size()) continue;
        if (name.compare(0, pre.size(), pre) != 0) continue;
        if (name.compare(name.size() - suf.size(), suf.size(), suf) != 0) continue;
        const std::string mid = name.substr(pre.size(), name.size() - pre.size() - suf.size());
        if (mid.empty() || !std::all_of(mid.begin(), mid.end(), [](char c) { return c >= '0' && c <= '9'; })) continue;
        depths.push_back(std::stoi(mid));
      }
    }
    if (depths.empty()) { std::filesystem::current_path(savedCwd); Logger::WriteMessage("SKIP: no depth layers.\n"); return; }

    // Keep a uniform 1/16 sub-sample of the union of all depths (state kept iff its hash has >=4 trailing zeros).
    const int tKeep = 4;
    const size_t cap = 90'000'000; // RAM safety (~2.9 GB)
    std::vector<Rec> V; V.reserve(55'000'000);
    u64 totalStates = 0; bool capped = false;
    try {
      for (int d : depths) {
        LayerCache<State> layer("depth", d, "bucket", 0);
        for (const State& s : layer) {
          totalStates++;
          if (std::countr_zero(Fnv1a(&s, kRec)) >= tKeep) {
            Rec r; std::memcpy(r.data(), &s, kRec); V.push_back(r);
            if (V.size() >= cap) { capped = true; break; }
          }
        }
        if (capped) break;
      }
    } catch (const std::exception& e) {
      std::filesystem::current_path(savedCwd);
      Logger::WriteMessage((std::string("SKIP: read failed (") + e.what() + ")\n").c_str());
      return;
    }
    std::filesystem::current_path(savedCwd);
    if (V.size() < 1000) { Logger::WriteMessage("SKIP: too few sampled states.\n"); return; }

    std::sort(V.begin(), V.end());

    // Three nested densities measured in a single pass over the sorted sample (subsets stay sorted).
    struct Acc { Rec prev{}; bool has = false; u64 enc = 0, n = 0; };
    Acc a16, a64, a256;
    auto feed = [](Acc& a, const Rec& r) {
      u32 c = a.has ? CommonPrefixLen(a.prev, r) : 0;
      a.enc += VarintLen(c) + (kRec - c); a.prev = r; a.has = true; a.n++;
    };
    for (const Rec& r : V) {
      int t = std::countr_zero(Fnv1a(r.data(), kRec));
      feed(a16, r);
      if (t >= 6) feed(a64, r);
      if (t >= 8) feed(a256, r);
    }
    const double bp16 = (double)a16.enc / a16.n, bp64 = (double)a64.enc / a64.n, bp256 = (double)a256.enc / a256.n;

    // Entropy-ordered front-coding at the densest sampled rate (all of V == 1/16).
    const std::array<u8, kRec> perm = EntropyPermutation(V);
    for (auto& r : V) r = Permute(r, perm);
    std::sort(V.begin(), V.end());
    const FrontCodeStats ent = MeasureFrontCode(V);

    // bytes/entry is ~linear in log2(density); the full set is 16x denser than the 1/16 sample (two more x4 steps).
    const double projFull = bp16 - (bp256 - bp16);

    Logger::WriteMessage(std::format(
      "\n=== Full-set scaling (bucket 0, union of {} depths) ===\n"
      "states scanned           : {}{}\n"
      "record size / hash cost  : {} B / 16 B\n"
      "density 1/256  n={:>10} : {:.2f} bytes/entry (natural)\n"
      "density 1/64   n={:>10} : {:.2f} bytes/entry (natural)\n"
      "density 1/16   n={:>10} : {:.2f} bytes/entry (natural)\n"
      "density 1/16   n={:>10} : {:.2f} bytes/entry (entropy-ordered)\n"
      "extrapolated full set    : ~{:.2f} bytes/entry (natural, before entropy ordering)\n",
      depths.size(), totalStates, (capped ? " (read capped)" : ""), kRec,
      a256.n, bp256, a64.n, bp64, a16.n, bp16, V.size(), ent.bytesPerEntry, projFull).c_str());

    Assert::IsTrue(a256.n > 0 && a64.n > 0 && a16.n > 0, L"insufficient nested samples");
    Assert::IsTrue(bp16 <= bp64 && bp64 <= bp256, L"front-coding did not improve with density (scaling failed)");
  }

  // Measures the CPU cost (encode + decode) of front-coding vs the numeric value-delta codec across a few
  // intermediate layers, on the entropy-permuted order both would deploy on. Verifies both round-trip.
  TEST_METHOD(CodecPerf) {
    const std::filesystem::path savedCwd = std::filesystem::current_path();
    if (!ChdirToCacheDir()) { Logger::WriteMessage("SKIP: no cache/ directory found.\n"); return; }

    // Enumerate depth layers (bucket 0) and pick a few spread across sizes (~15 / 35 / 60 MB on disk).
    std::vector<std::pair<long long, int>> layers;
    {
      const std::string pre = "_depth_", suf = "_bucket_0.bin";
      std::error_code ec;
      for (auto& e : std::filesystem::directory_iterator(std::filesystem::path("cache"), ec)) {
        if (ec) break;
        const std::string name = e.path().filename().string();
        if (name.size() <= pre.size() + suf.size()) continue;
        if (name.compare(0, pre.size(), pre) != 0) continue;
        if (name.compare(name.size() - suf.size(), suf.size(), suf) != 0) continue;
        const std::string mid = name.substr(pre.size(), name.size() - pre.size() - suf.size());
        if (mid.empty() || !std::all_of(mid.begin(), mid.end(), [](char c) { return c >= '0' && c <= '9'; })) continue;
        layers.emplace_back((long long)e.file_size(ec), std::stoi(mid));
      }
    }
    if (layers.empty()) { std::filesystem::current_path(savedCwd); Logger::WriteMessage("SKIP: no layers.\n"); return; }

    std::vector<int> chosen;
    for (long long target : { 15LL * 1024 * 1024, 35LL * 1024 * 1024, 60LL * 1024 * 1024 }) {
      int best = -1; long long bestDiff = (std::numeric_limits<long long>::max)();
      for (auto& [sz, d] : layers) { long long diff = sz > target ? sz - target : target - sz; if (diff < bestDiff) { bestDiff = diff; best = d; } }
      if (best >= 0 && std::find(chosen.begin(), chosen.end(), best) == chosen.end()) chosen.push_back(best);
    }

    Logger::WriteMessage("\n=== Codec perf: front-coding vs numeric delta (entropy order) ===\n");
    Logger::WriteMessage("enc/dec ns/state; delta = manual limbs, delta-I = _subborrow_u64/_addcarry_u64\n");

    using clk = std::chrono::steady_clock;
    auto ns = [](clk::time_point a, clk::time_point b) { return (double)std::chrono::duration_cast<std::chrono::nanoseconds>(b - a).count(); };

    for (int depth : chosen) {
      std::vector<Rec> recs;
      const size_t cap = 10'000'000;
      try {
        LayerCache<State> layer("depth", depth, "bucket", 0);
        for (const State& s : layer) { Rec r; std::memcpy(r.data(), &s, kRec); recs.push_back(r); if (recs.size() >= cap) break; }
      } catch (const std::exception&) { continue; }
      if (recs.size() < 2) continue;
      const size_t n = recs.size();

      std::sort(recs.begin(), recs.end());
      const std::array<u8, kRec> perm = EntropyPermutation(recs);
      for (auto& r : recs) r = Permute(r, perm);
      std::sort(recs.begin(), recs.end());

      // Front-coding: encode, then decode + verify.
      std::vector<u8> fbuf; fbuf.reserve(n * (kRec / 2));
      auto f0 = clk::now();
      { Rec prev{}; bool has = false;
        for (const Rec& r : recs) { u32 c = has ? CommonPrefixLen(prev, r) : 0; PutVarint(fbuf, c); fbuf.insert(fbuf.end(), r.begin() + c, r.end()); prev = r; has = true; } }
      auto f1 = clk::now();
      size_t fMis = 0;
      { const u8* p = fbuf.data(); Rec prev{};
        for (size_t i = 0; i < n; i++) { u32 c = (u32)GetVarint(p); Rec cur; std::memcpy(cur.data(), prev.data(), c); std::memcpy(cur.data() + c, p, kRec - c); p += kRec - c; if (cur != recs[i]) fMis++; prev = cur; } }
      auto f2 = clk::now();

      // Numeric delta: encode, then decode + verify.
      std::vector<u8> dbuf; dbuf.reserve(n * (kRec / 2));
      auto d0 = clk::now();
      { U256 prev{}; for (const Rec& r : recs) { U256 cur = ToU256(r); EncodeNumericDelta(dbuf, prev, cur); prev = cur; } }
      auto d1 = clk::now();
      size_t dMis = 0;
      { const u8* p = dbuf.data(); U256 prev{};
        for (size_t i = 0; i < n; i++) { U256 d = DecodeNumericDelta(p); U256 cur = Add(prev, d); Rec rc; FromU256(cur, rc); if (rc != recs[i]) dMis++; prev = cur; } }
      auto d2 = clk::now();

      // Numeric delta via _subborrow_u64 / _addcarry_u64 intrinsics.
      std::vector<u8> ibuf; ibuf.reserve(n * (kRec / 2));
      auto i0 = clk::now();
      { U256 prev{}; for (const Rec& r : recs) { U256 cur = ToU256(r); EncodeNumericDeltaI(ibuf, prev, cur); prev = cur; } }
      auto i1 = clk::now();
      size_t iMis = 0;
      { const u8* p = ibuf.data(); U256 prev{};
        for (size_t k = 0; k < n; k++) { U256 d = DecodeNumericDelta(p); U256 cur = AddI(prev, d); Rec rc; FromU256(cur, rc); if (rc != recs[k]) iMis++; prev = cur; } }
      auto i2 = clk::now();

      Logger::WriteMessage(std::format(
        "d{:<3} n={:>9}  front {:5.1f}/{:5.1f} | delta {:5.1f}/{:5.1f} | delta-I {:5.1f}/{:5.1f}   (mis {}/{}/{})\n",
        depth, n,
        ns(f0, f1) / n, ns(f1, f2) / n, ns(d0, d1) / n, ns(d1, d2) / n, ns(i0, i1) / n, ns(i1, i2) / n, fMis, dMis, iMis).c_str());

      Assert::IsTrue(fMis == 0 && dMis == 0 && iMis == 0, L"codec round-trip mismatch");
    }

    std::filesystem::current_path(savedCwd);
  }

  // Bounded benchmark on a LATER layer using EXISTING cache data (no re-solve): times the actual Move
  // expansion (SetState + Move x4 + heuristic) the solver runs per state, so we can compare it against the
  // codec's ~15 ns/state and see whether the decode is anywhere near the hot path. Assumes the cache is a
  // 3-sausage "3-4 Cold Trail" solve (matches NUM_SAUSAGES == 3 in this test build).
  TEST_METHOD(ExpansionCostLaterLayer) {
    const std::filesystem::path savedCwd = std::filesystem::current_path();
    if (!ChdirToCacheDir()) { Logger::WriteMessage("SKIP: no cache/ directory found.\n"); return; }

    int maxDepth = -1;
    {
      const std::string pre = "_depth_", suf = "_bucket_0.bin";
      std::error_code ec;
      for (auto& e : std::filesystem::directory_iterator(std::filesystem::path("cache"), ec)) {
        if (ec) break;
        const std::string name = e.path().filename().string();
        if (name.size() <= pre.size() + suf.size()) continue;
        if (name.compare(0, pre.size(), pre) != 0) continue;
        if (name.compare(name.size() - suf.size(), suf.size(), suf) != 0) continue;
        const std::string mid = name.substr(pre.size(), name.size() - pre.size() - suf.size());
        if (mid.empty() || !std::all_of(mid.begin(), mid.end(), [](char c) { return c >= '0' && c <= '9'; })) continue;
        maxDepth = (std::max)(maxDepth, std::stoi(mid));
      }
    }
    if (maxDepth < 0) { std::filesystem::current_path(savedCwd); Logger::WriteMessage("SKIP: no depth layers.\n"); return; }

    std::vector<State> states;
    const size_t cap = 2'000'000;
    try {
      LayerCache<State> layer("depth", maxDepth, "bucket", 0);
      for (const State& s : layer) { states.push_back(s); if (states.size() >= cap) break; }
    } catch (const std::exception& e) {
      std::filesystem::current_path(savedCwd);
      Logger::WriteMessage((std::string("SKIP: read failed (") + e.what() + ")\n").c_str());
      return;
    }
    std::filesystem::current_path(savedCwd);
    if (states.empty()) { Logger::WriteMessage("SKIP: no states.\n"); return; }

    Level* level = &ColdTrail; // the cached states are from a ColdTrail solve

    volatile u64 sink = 0;
    u64 validMoves = 0, wonBreaks = 0;
    const auto t0 = std::chrono::steady_clock::now();
    for (const State& state : states) {
      for (Direction dir : { Up, Down, Left, Right }) {
        level->SetState(state);            // solver resets before each move (not fully transactional)
        if (level->Won()) { wonBreaks++; break; }
        if (!level->Move(dir)) continue;   // discard illegal moves
        if (level->heuristic && !level->heuristic(level, 0)) continue;
        State ns = level->GetState();
        sink += (u64)ns.stephen.x;         // touch the result so nothing is optimized away
        validMoves++;
      }
    }
    const auto t1 = std::chrono::steady_clock::now();
    (void)sink;

    const size_t n = states.size();
    const double totalNs = (double)std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
    const double nsPerState = totalNs / n;
    const double nsPerMove = totalNs / (double)(n * 4);

    Logger::WriteMessage(std::format(
      "\n=== Expansion (Move) cost on later layer d{} (existing cache) ===\n"
      "states processed         : {}\n"
      "valid moves generated    : {} ({:.2f} per state)\n"
      "expansion cost           : {:.1f} ns/state   ({:.1f} ns/move-attempt)\n"
      "front-coded decode       : ~15 ns/state (from CodecPerf)\n"
      "=> decode is ~{:.1f}% of the per-state expansion cost\n",
      maxDepth, n, validMoves, (double)validMoves / n, nsPerState, nsPerMove, 15.0 / nsPerState * 100.0).c_str());

    Assert::IsTrue(n > 0, L"no states processed");
  }

  // Benchmarks the two proposed single-threaded Stage-2 tweaks on real states + the real Move engine:
  //   A: baseline (Won() re-checked each of 4 dirs; lookup in a large set ~ full _winningStates)
  //   B: Won() hoisted out of the direction loop (lookup still in the large set)
  //   C: Won() hoisted + lookup in a small set (~ the depth-d+1 winner frontier)
  // Lookups mostly miss (successors are depth-d+1 states), so A/B isolates the Won() hoist and B/C the set size.
  TEST_METHOD(Stage2InnerLoopVariants) {
    const std::filesystem::path savedCwd = std::filesystem::current_path();
    if (!ChdirToCacheDir()) { Logger::WriteMessage("SKIP: no cache/ directory found.\n"); return; }

    int maxDepth = -1;
    {
      const std::string pre = "_depth_", suf = "_bucket_0.bin";
      std::error_code ec;
      for (auto& e : std::filesystem::directory_iterator(std::filesystem::path("cache"), ec)) {
        if (ec) break;
        const std::string name = e.path().filename().string();
        if (name.size() <= pre.size() + suf.size()) continue;
        if (name.compare(0, pre.size(), pre) != 0) continue;
        if (name.compare(name.size() - suf.size(), suf.size(), suf) != 0) continue;
        const std::string mid = name.substr(pre.size(), name.size() - pre.size() - suf.size());
        if (mid.empty() || !std::all_of(mid.begin(), mid.end(), [](char c) { return c >= '0' && c <= '9'; })) continue;
        maxDepth = (std::max)(maxDepth, std::stoi(mid));
      }
    }
    if (maxDepth < 0) { std::filesystem::current_path(savedCwd); Logger::WriteMessage("SKIP: no depth layers.\n"); return; }

    std::vector<State> states;
    const size_t cap = 1'000'000;
    try {
      LayerCache<State> layer("depth", maxDepth, "bucket", 0);
      for (const State& s : layer) { states.push_back(s); if (states.size() >= cap) break; }
    } catch (const std::exception& e) {
      std::filesystem::current_path(savedCwd);
      Logger::WriteMessage((std::string("SKIP: read failed (") + e.what() + ")\n").c_str());
      return;
    }
    std::filesystem::current_path(savedCwd);
    if (states.size() < 2) { Logger::WriteMessage("SKIP: too few states.\n"); return; }
    const size_t n = states.size();

    Level* level = &ColdTrail;

    // Representative lookup targets: bigSet ~ the full cumulative _winningStates; smallSet ~ one depth's frontier.
    absl::flat_hash_map<State, u32> bigSet, smallSet;
    bigSet.reserve(n);
    for (size_t i = 0; i < n; i++) { bigSet.emplace(states[i], 0u); if (i % 500 == 0) smallSet.emplace(states[i], 0u); }

    using clk = std::chrono::steady_clock;
    auto nsPer = [n](clk::time_point a, clk::time_point b) { return (double)std::chrono::duration_cast<std::chrono::nanoseconds>(b - a).count() / n; };
    volatile u64 sink = 0;

    // A: baseline
    u64 winsA = 0;
    auto a0 = clk::now();
    for (const State& state : states) {
      for (Direction dir : { Up, Down, Left, Right }) {
        level->SetState(state);
        if (level->Won()) { winsA++; break; }
        if (!level->Move(dir)) continue;
        if (level->heuristic && !level->heuristic(level, 0)) continue;
        State ns = level->GetState();
        if (bigSet.find(ns) != bigSet.end()) { winsA++; break; }
      }
    }
    auto a1 = clk::now();

    // B: Won() hoisted (first move reuses the SetState from the Won check)
    u64 winsB = 0;
    auto b0 = clk::now();
    for (const State& state : states) {
      level->SetState(state);
      if (level->Won()) { winsB++; continue; }
      bool firstDir = true;
      for (Direction dir : { Up, Down, Left, Right }) {
        if (!firstDir) level->SetState(state);
        firstDir = false;
        if (!level->Move(dir)) continue;
        if (level->heuristic && !level->heuristic(level, 0)) continue;
        State ns = level->GetState();
        if (bigSet.find(ns) != bigSet.end()) { winsB++; break; }
      }
    }
    auto b1 = clk::now();

    // C: Won() hoisted + lookup in the smaller frontier set
    u64 winsC = 0;
    auto c0 = clk::now();
    for (const State& state : states) {
      level->SetState(state);
      if (level->Won()) { winsC++; continue; }
      bool firstDir = true;
      for (Direction dir : { Up, Down, Left, Right }) {
        if (!firstDir) level->SetState(state);
        firstDir = false;
        if (!level->Move(dir)) continue;
        if (level->heuristic && !level->heuristic(level, 0)) continue;
        State ns = level->GetState();
        if (smallSet.find(ns) != smallSet.end()) { winsC++; break; }
      }
    }
    auto c1 = clk::now();

    sink += winsA + winsB + winsC; (void)sink;

    const double A = nsPer(a0, a1), B = nsPer(b0, b1), C = nsPer(c0, c1);
    Logger::WriteMessage(std::format(
      "\n=== Stage 2 inner-loop variants (later layer d{}, {} states) ===\n"
      "big set / small set      : {} / {} entries\n"
      "A baseline (Won in loop) : {:.1f} ns/state\n"
      "B Won() hoisted          : {:.1f} ns/state  ({:+.1f}% vs A)\n"
      "C Won() hoist + frontier : {:.1f} ns/state  ({:+.1f}% vs A)\n",
      maxDepth, n, bigSet.size(), smallSet.size(),
      A, B, (B / A - 1) * 100.0, C, (C / A - 1) * 100.0).c_str());

    // The Won() hoist must not change which states are winning (same set, same logic).
    Assert::AreEqual(winsA, winsB, L"Won() hoist changed the winning-state count");
  }

  // Confirms (safely, no unbounded exploration) whether an EARLIER pillar gate would have helped. The tightest gate
  // that keeps a solution admits only the earliest knockdowns (pillar cleared by depth 84 -- the earliest any reachable
  // state clears it is depth 83). We build the set of states reachable from JUST those early knockdowns while keeping
  // the pillar clear (memory-capped, so it can never blow up), then measure what fraction of the ACTUAL gated cache
  // layers past depth 94 those early lineages already reproduce. If coverage is ~100%, the late (buffer) knockdowns add
  // nothing -- an earlier gate would produce the same deep layers, so it would NOT have relieved the explosion.
  TEST_METHOD(ToadsFollyEarlyKnockdownCoverage) {
    const std::filesystem::path savedCwd = std::filesystem::current_path();
    if (!ChdirToCacheDir()) { Logger::WriteMessage("SKIP: no cache/ directory found.\n"); return; }
    Level* level = &ToadsFolly;
    auto occupied = [](const State& s) {
      for (const Sausage& sg : s.sausages) if (sg.IsAt(8, 2, 2)) return true;
      return false;
    };

    // 1) Earliest-knockdown seeds: every pillar-cleared state at depth 83-84 (survive the tightest meaningful gate).
    std::vector<State> seeds;
    for (int d : { 83, 84 }) {
      for (int b = 0; b < 32; b++) {
        try {
          LayerCache<State> layer("depth", d, "bucket", b);
          for (const State& s : layer) if (!occupied(s)) seeds.push_back(s);
        } catch (const std::exception&) {}
      }
    }
    Logger::WriteMessage(std::format("earliest-knockdown seeds (pillar cleared by depth 84): {}\n", seeds.size()).c_str());
    if (seeds.empty()) { std::filesystem::current_path(savedCwd); Logger::WriteMessage("SKIP: no early seeds found.\n"); return; }

    // 2) Build the pillar-clear reachable set from those early seeds, HARD-capped so RAM stays bounded (~1.5 GB).
    const size_t cap = 30'000'000;
    absl::flat_hash_set<State> seen(seeds.begin(), seeds.end());
    std::vector<State> cur(seeds), next;
    bool hitCap = false; int reachedDepth = 84;
    const auto t0 = std::chrono::steady_clock::now();
    for (int step = 1; step <= 60 && !hitCap; step++) {
      next.clear();
      for (const State& st : cur) {
        for (Direction dir : { Up, Down, Left, Right }) {
          level->SetState(st);
          if (!level->Move(dir)) continue;
          State ns = level->GetState();
          if (occupied(ns)) continue; // keep the pillar clear (an earlier gate would too)
          if (seen.insert(ns).second) { next.push_back(ns); if (seen.size() >= cap) { hitCap = true; break; } }
        }
        if (hitCap) break;
      }
      cur.swap(next); reachedDepth = 84 + step;
      if (cur.empty()) break;
    }
    auto el = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - t0).count();
    Logger::WriteMessage(std::format("early-reachable set: {} states, BFS reached ~depth {}{}  ({}s)\n",
      seen.size(), reachedDepth, hitCap ? " (memory-capped)" : "", (long long)el).c_str());

    // 3) Coverage: for gated cache layers past the cutoff, what fraction is already reproduced by early knockdowns?
    //    Only trust depths comfortably below reachedDepth (early lineages need a few extra moves to arrive).
    std::string out = "\n=== gated cache layer vs early-knockdown coverage (bucket 0) ===\n depth | pillar-clear states | reproduced by early | coverage%\n";
    for (int d : { 94, 96, 98, 100, 103, 106, 110 }) {
      if (d > reachedDepth - 3) { out += std::format("  {:>4} | (beyond early-BFS reach -- skipped)\n", d); continue; }
      u64 nClear = 0, covered = 0;
      try {
        LayerCache<State> layer("depth", d, "bucket", 0);
        for (const State& s : layer) { if (occupied(s)) continue; nClear++; if (seen.contains(s)) covered++; if (nClear >= 3000000) break; }
      } catch (const std::exception&) {}
      out += std::format("  {:>4} | {:>19} | {:>19} | {:6.2f}%\n", d, nClear, covered, nClear ? 100.0 * covered / nClear : 0.0);
    }
    Logger::WriteMessage(out.c_str());

    std::filesystem::current_path(savedCwd);
    Assert::IsTrue(!seeds.empty());
  }

  // Designs a second (endgame) filter that keeps the loose pillar buffer. Replays the confirmed 142-move solution to
  // find the tight region its sausages occupy during the cooking endgame, then measures how many deep cache states
  // (depths 120-141) have a sausage OUTSIDE that region -- i.e. the prune yield of an "all sausages within the cooking
  // box" filter that bites before the state space blows up.
  TEST_METHOD(ToadsFollySecondFilterDesign) {
    const std::filesystem::path savedCwd = std::filesystem::current_path();
    if (!ChdirToCacheDir()) { Logger::WriteMessage("SKIP: no cache/ directory found.\n"); return; }
    const std::filesystem::path root = std::filesystem::current_path();
    Level* level = &ToadsFolly;

    std::vector<Direction> moves;
    { std::ifstream in(root / "toads-folly-142.dem"); std::string line;
      while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line == "North") moves.push_back(Up); else if (line == "South") moves.push_back(Down);
        else if (line == "East") moves.push_back(Right); else if (line == "West") moves.push_back(Left);
      }
    }
    if (moves.empty()) { std::filesystem::current_path(savedCwd); Logger::WriteMessage("SKIP: toads-folly-142.dem not found.\n"); return; }

    auto maxXY = [](const State& s, int& mx, int& my) {
      mx = -99; my = -99;
      for (const Sausage& sg : s.sausages) {
        mx = (std::max)(mx, (std::max)((int)sg.x1, (int)sg.x2));
        my = (std::max)(my, (std::max)((int)sg.y1, (int)sg.y2));
      }
    };

    // 1) Replay, recording the sausages' (maxX, maxY) at each depth.
    std::vector<std::pair<int, int>> env(moves.size() + 1, { -99, -99 });
    const State init = level->GetState();
    level->SetState(init);
    { int mx, my; maxXY(level->GetState(), mx, my); env[0] = { mx, my }; }
    u32 depth = 0;
    for (Direction d : moves) { if (!level->Move(d)) break; depth++; int mx, my; maxXY(level->GetState(), mx, my); env[depth] = { mx, my }; if (level->Won()) break; }

    std::string out = "\n=== 142-solution sausage envelope (max x, max y over all halves) by depth ===\n";
    for (u32 d = 90; d <= depth; d += 2) out += std::format("  depth {:>3}: maxX={:>2} maxY={:>2}\n", d, env[d].first, env[d].second);
    for (int D : { 100, 110, 120, 125 }) {
      int mx = -99, my = -99;
      for (u32 d = (u32)D; d <= depth; d++) { mx = (std::max)(mx, env[d].first); my = (std::max)(my, env[d].second); }
      out += std::format("tightest sound box for gate depth >= {}: maxX<={}, maxY<={}\n", D, mx, my);
    }
    Logger::WriteMessage(out.c_str());

    // 2) Prune yield: fraction of deep cache states with a sausage OUTSIDE a candidate box.
    struct Box { int bx, by; };
    Box boxes[] = { { 4, 3 }, { 5, 3 }, { 5, 4 }, { 6, 4 } };
    std::string out3 = "\n=== prune yield of 'every sausage half x<=BX and y<=BY' (bucket 0, first <=3M) ===\n";
    for (const Box& box : boxes) {
      out3 += std::format(" box x<={} y<={}:\n", box.bx, box.by);
      for (int d : { 120, 125, 130, 135, 141 }) {
        u64 n = 0, pruned = 0;
        try {
          LayerCache<State> layer("depth", d, "bucket", 0);
          for (const State& s : layer) {
            n++;
            bool outOfBox = false;
            for (const Sausage& sg : s.sausages)
              if (sg.x1 > box.bx || sg.x2 > box.bx || sg.y1 > box.by || sg.y2 > box.by) { outOfBox = true; break; }
            if (outOfBox) pruned++;
            if (n >= 3000000) break;
          }
        } catch (const std::exception&) {}
        out3 += std::format("   depth {:>3}: n={:>9} pruned={:>9} ({:5.1f}%)\n", d, n, pruned, n ? 100.0 * pruned / n : 0.0);
      }
    }
    Logger::WriteMessage(out3.c_str());

    std::filesystem::current_path(savedCwd);
    Assert::IsTrue(!moves.empty());
  }

  // Regression guard: the two-filter heuristic (pillar + cooking box) must not prune any state on the confirmed
  // 142-move solution. Fails loudly if either gate is ever tightened enough to kill the solution.
  TEST_METHOD(ToadsFollyHeuristicKeepsSolution) {
    const std::filesystem::path savedCwd = std::filesystem::current_path();
    if (!ChdirToCacheDir()) { Logger::WriteMessage("SKIP: no cache/ directory found.\n"); return; }
    const std::filesystem::path root = std::filesystem::current_path();
    Level* level = &ToadsFolly;
    Assert::IsTrue(level->heuristic != nullptr, L"ToadsFolly has no heuristic");

    std::vector<Direction> moves;
    { std::ifstream in(root / "toads-folly-142.dem"); std::string line;
      while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line == "North") moves.push_back(Up); else if (line == "South") moves.push_back(Down);
        else if (line == "East") moves.push_back(Right); else if (line == "West") moves.push_back(Left);
      }
    }
    if (moves.empty()) { std::filesystem::current_path(savedCwd); Logger::WriteMessage("SKIP: toads-folly-142.dem not found.\n"); return; }

    const State initialState = level->GetState();
    level->SetState(initialState);
    int prunedAt = -1; u32 depth = 0;
    if (!level->heuristic(level, depth)) prunedAt = 0;
    for (Direction d : moves) {
      if (!level->Move(d)) break;
      depth++;
      if (prunedAt < 0 && !level->heuristic(level, depth)) prunedAt = (int)depth;
      if (level->Won()) break;
    }
    const bool won = level->Won();
    std::filesystem::current_path(savedCwd);
    Logger::WriteMessage(std::format("142-solution vs admissible heuristic: replayed {} moves, won={}, first pruned depth={} (-1 = never pruned)\n",
      depth, won ? "yes" : "no", prunedAt).c_str());
    Assert::IsTrue(won, L"demo did not reach Won()");
    Assert::AreEqual(-1, prunedAt, L"heuristic pruned a state on the confirmed optimal path");
  }

  // Toward a RIGID (provably sound) endgame bound rather than the survey box. (1) Measures the per-move speed limit --
  // how far a sausage half can move in one move, including being pushed/chain-rolled (GetState(false) keeps sausage
  // indices stable so we can track each one across the move). (2) From that, a sausage that is not fully cooked needs
  // at least (Manhattan-distance-to-grill / speed) moves just to reach the grill, so a state at depth d with budget
  // 142-d is provably dead if any uncooked sausage is farther than that. Reports where such a rigid bound actually bites.
  TEST_METHOD(ToadsFollyRigidBound) {
    const std::filesystem::path savedCwd = std::filesystem::current_path();
    if (!ChdirToCacheDir()) { Logger::WriteMessage("SKIP: no cache/ directory found.\n"); return; }
    Level* level = &ToadsFolly;
    const int W = 10, H = 10, kWin = 142;
    const std::pair<int, int> grill[] = { {1,1},{2,1},{1,2},{2,2} };
    auto manhToGrill = [&](int x, int y) { int m = 999; for (auto& g : grill) m = (std::min)(m, std::abs(x - g.first) + std::abs(y - g.second)); return m; };

    // 1) Max per-move sausage-half displacement (Manhattan and Chebyshev), tracked by index via GetState(false).
    int maxManh = 0, maxCheb = 0; State exB{}, exA{}; int exDir = -1;
    for (int d : { 100, 110, 119 }) {
      for (int b = 0; b < 2; b++) {
        try {
          LayerCache<State> layer("depth", d, "bucket", b);
          u64 cnt = 0;
          for (const State& s : layer) {
            for (Direction dir : { Up, Down, Left, Right }) {
              level->SetState(s);
              if (!level->Move(dir)) continue;
              State ns = level->GetState(false); // unsorted: sausage[i] still corresponds to s.sausage[i]
              for (int i = 0; i < NUM_SAUSAGES; i++) {
                const Sausage& a = s.sausages[i]; const Sausage& c = ns.sausages[i];
                int m1 = std::abs(c.x1 - a.x1) + std::abs(c.y1 - a.y1);
                int m2 = std::abs(c.x2 - a.x2) + std::abs(c.y2 - a.y2);
                int ch = (std::max)({ std::abs(c.x1 - a.x1), std::abs(c.y1 - a.y1), std::abs(c.x2 - a.x2), std::abs(c.y2 - a.y2) });
                int mm = (std::max)(m1, m2);
                if (mm > maxManh) { maxManh = mm; exB = s; exA = ns; exDir = dir; }
                maxCheb = (std::max)(maxCheb, ch);
              }
            }
            if (++cnt >= 300000) break;
          }
        } catch (const std::exception&) {}
      }
    }
    std::string out = std::format("\n=== per-move sausage-half displacement (sampled depths 100/110/119) ===\n"
      "max Manhattan step = {}   max Chebyshev step = {}\n", maxManh, maxCheb);
    if (exDir >= 0) {
      const char* dn = exDir == Up ? "Up" : exDir == Down ? "Down" : exDir == Left ? "Left" : exDir == Right ? "Right" : "?";
      out += std::format("worst example (move {}):\n", dn);
      for (int i = 0; i < NUM_SAUSAGES; i++)
        out += std::format("  s{}: ({},{})-({},{}) z{}  ->  ({},{})-({},{}) z{}\n", i,
          (int)exB.sausages[i].x1, (int)exB.sausages[i].y1, (int)exB.sausages[i].x2, (int)exB.sausages[i].y2, (int)exB.sausages[i].z,
          (int)exA.sausages[i].x1, (int)exA.sausages[i].y1, (int)exA.sausages[i].x2, (int)exA.sausages[i].y2, (int)exA.sausages[i].z);
    }
    Logger::WriteMessage(out.c_str());

    // 2) Rigid bite: an uncooked sausage needs >= ceil(ManhToGrill / maxManh) moves to reach the grill.
    const int speed = (std::max)(1, maxManh);
    int worstCell = 0; for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) worstCell = (std::max)(worstCell, manhToGrill(x, y));
    std::string out2 = std::format("\n=== rigid 'uncooked sausage too far to reach the grill' bound (speed {}/move) ===\n"
      "worst on-grid Manhattan-to-grill = {}  (so the bound cannot bite until budget < {})\n"
      " depth | budget | dead if manhToGrill > | # of 100 grid cells that fails\n", speed, worstCell, worstCell);
    for (int depth : { 120, 125, 128, 130, 132, 135, 138, 141 }) {
      int budget = kWin - depth;
      int thresh = budget * speed;
      int dead = 0; for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) if (manhToGrill(x, y) > thresh) dead++;
      out2 += std::format("  {:>4} | {:>6} | {:>21} | {}\n", depth, budget, thresh, dead);
    }
    Logger::WriteMessage(out2.c_str());

    std::filesystem::current_path(savedCwd);
    Assert::IsTrue(true);
  }

  // RIGID cooking-time bound, measured on a synthetic OPEN arena (no cache touched). A ground sausage rolls 1 tile/move
  // and only moves when Stephen pushes it, so cooking a sausage that starts D tiles from the grill costs real moves:
  // travel + flips + Stephen shuffling. An open arena is a relaxation of the real level (obstacles only slow things
  // down), so the BFS minimum here is a valid LOWER bound on the real cook time -> a sound "uncooked sausage too far
  // to cook in time" prune. This test never opens LayerCache or the solver, so the 649 GB cache is untouched.
  TEST_METHOD(ToadsFollyCookTimeBound) {
    const int W = 18, H = 8;
    // Play area rows 0-5 (2x2 grill top-left); row 6 is a void moat; row 7 holds two pre-cooked, unreachable decoy
    // sausages so the level has NUM_SAUSAGES(=3) and "all cooked" <=> the target sausage is cooked (index-independent).
    const char* grid =
      "__________________"
      "_##_______________"
      "_##_______________"
      "__________________"
      "__________________"
      "__________________"
      "                  "
      "__________________";
    auto grillManh = [](int x, int y) { int m = 999; for (int gx = 1; gx <= 2; gx++) for (int gy = 1; gy <= 2; gy++) m = (std::min)(m, std::abs(x - gx) + std::abs(y - gy)); return m; };

    std::string out = "\n=== min moves to FULLY COOK one sausage vs its distance from the 2x2 grill (open arena) ===\n"
      " dist | sausage start | min cook moves\n";
    const size_t cap = 12'000'000;

    // --- diagnostic: for the closest case, check whether a lone sausage can be cooked at all ---
    {
      Level dbg(W, H, "cook-dbg", grid, Stephen{ 5, 2, 0, Right }, {},
        { Sausage{ 3, 2, 4, 2, 0, Sausage::None },
          Sausage{ 0, 7, 1, 7, 0, Sausage::FullyCooked },
          Sausage{ 3, 7, 4, 7, 0, Sausage::FullyCooked } });
      auto target = [](const State& s) -> const Sausage& { for (const Sausage& sg : s.sausages) if (!sg.IsFullyCooked()) return sg; return s.sausages[0]; };
      State s0 = dbg.GetState();
      absl::flat_hash_set<State> seen; seen.insert(s0);
      std::vector<State> cur{ s0 }, next; int maxCooked = 0; bool moved = false;
      const Sausage& t0 = target(s0);
      for (int d = 0; d < 40; d++) {
        next.clear();
        for (const State& s : cur) {
          const Sausage& t = target(s);
          maxCooked = (std::max)(maxCooked, std::popcount((unsigned)(t.flags & Sausage::FullyCooked)));
          if (t.x1 != t0.x1 || t.y1 != t0.y1 || t.x2 != t0.x2 || t.y2 != t0.y2) moved = true;
          for (Direction dir : { Up, Down, Left, Right }) { dbg.SetState(s); if (!dbg.Move(dir)) continue; State ns = dbg.GetState(); if (seen.insert(ns).second) next.push_back(ns); }
        }
        cur.swap(next); if (cur.empty()) break;
      }
      Logger::WriteMessage(std::format("\n[diag] lone sausage at (3,2)-(4,2), Stephen clear at (8,4): reachable={}, target-moved={}, max cooked faces={}/4\n",
        seen.size(), moved ? "yes" : "no", maxCooked).c_str());
    }

    for (int col = 3; col <= 14; col++) {
      const int sy = 2;
      int dist = (std::min)(grillManh(col, sy), grillManh(col + 1, sy));
      Level cook(W, H, "cook-test", grid, Stephen{ (s8)(col + 2), (s8)2, 0, Right }, {},
        { Sausage{ (s8)col, (s8)sy, (s8)(col + 1), (s8)sy, 0, Sausage::None },
          Sausage{ 0, 7, 1, 7, 0, Sausage::FullyCooked },
          Sausage{ 3, 7, 4, 7, 0, Sausage::FullyCooked } });
      auto allCooked = [&](const State& s) { for (const Sausage& sg : s.sausages) if (!sg.IsFullyCooked()) return false; return true; };
      absl::flat_hash_set<State> seen; State init = cook.GetState(); seen.insert(init);
      std::vector<State> cur{ init }, next; int found = -1; bool capped = false;
      for (int d = 0; d <= 80 && found < 0 && !capped; d++) {
        for (const State& s : cur) if (allCooked(s)) { found = d; break; }
        if (found >= 0) break;
        next.clear();
        for (const State& s : cur) {
          for (Direction dir : { Up, Down, Left, Right }) {
            cook.SetState(s); if (!cook.Move(dir)) continue;
            State ns = cook.GetState();
            if (seen.insert(ns).second) { next.push_back(ns); if (seen.size() >= cap) { capped = true; break; } }
          }
          if (capped) break;
        }
        cur.swap(next); if (cur.empty()) break;
      }
      out += std::format("  {:>3} | ({},{})-({},{}) | {}\n", dist, col, sy, col + 1, sy, capped ? std::string("capped") : std::to_string(found));
    }
    Logger::WriteMessage(out.c_str());
    Assert::IsTrue(true);
  }

  // Calibrates a cook-distance heuristic "uncooked sausage: distToGrill + C > budget => prune" against the confirmed
  // 142-move solution: what is the largest C that never prunes a state on the solution? Then shows how deep that bites.
  // Reads only the demo file; no LayerCache / solver, so the cache is untouched.
  TEST_METHOD(ToadsFollyCookDistanceCalibration) {
    const std::filesystem::path savedCwd = std::filesystem::current_path();
    if (!ChdirToCacheDir()) { Logger::WriteMessage("SKIP: no cache/ directory found.\n"); return; }
    const std::filesystem::path root = std::filesystem::current_path();
    Level* level = &ToadsFolly;
    const int kWin = 142;
    const std::pair<int, int> grill[] = { {1,1},{2,1},{1,2},{2,2} };
    auto distToGrill = [&](const Sausage& sg) {
      int best = 999;
      for (auto& g : grill) {
        best = (std::min)(best, std::abs((int)sg.x1 - g.first) + std::abs((int)sg.y1 - g.second));
        best = (std::min)(best, std::abs((int)sg.x2 - g.first) + std::abs((int)sg.y2 - g.second));
      }
      return best;
    };

    std::vector<Direction> moves;
    { std::ifstream in(root / "toads-folly-142.dem"); std::string line;
      while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line == "North") moves.push_back(Up); else if (line == "South") moves.push_back(Down);
        else if (line == "East") moves.push_back(Right); else if (line == "West") moves.push_back(Left);
      }
    }
    if (moves.empty()) { std::filesystem::current_path(savedCwd); Logger::WriteMessage("SKIP: toads-folly-142.dem not found.\n"); return; }

    int minSlackNF = 999, bindNFd = -1, bindNFdist = -1; // "not fully cooked" definition
    int minSlack0 = 999, bind0d = -1, bind0dist = -1;    // "0 cooked faces" definition
    auto eval = [&](u32 d) {
      State s = level->GetState();
      for (const Sausage& sg : s.sausages) {
        int cooked = std::popcount((unsigned)(sg.flags & Sausage::FullyCooked));
        int dist = distToGrill(sg), slack = (kWin - (int)d) - dist;
        if (cooked < 4 && slack < minSlackNF) { minSlackNF = slack; bindNFd = (int)d; bindNFdist = dist; }
        if (cooked == 0 && slack < minSlack0) { minSlack0 = slack; bind0d = (int)d; bind0dist = dist; }
      }
    };
    const State init = level->GetState();
    level->SetState(init); eval(0);
    u32 depth = 0;
    for (Direction dir : moves) { if (!level->Move(dir)) break; depth++; eval(depth); if (level->Won()) break; }
    std::filesystem::current_path(savedCwd);

    std::string out = std::format("\n=== cook-distance calibration on the 142 solution (largest safe C) ===\n"
      " 'not fully cooked' def: max safe C = {}  (binds at depth {}, dist {})\n"
      " 'fully uncooked'   def: max safe C = {}  (binds at depth {}, dist {})\n",
      minSlackNF, bindNFd, bindNFdist, minSlack0, bind0d, bind0dist);
    // Bite: for a few candidate C, the distance threshold above which an uncooked sausage is pruned, by depth.
    for (int C : { 5, minSlack0, minSlackNF }) {
      if (C < 0) continue;
      out += std::format("\n C={}: prune uncooked sausage whose distToGrill exceeds:\n", C);
      for (int d : { 118, 120, 122, 125, 128, 130, 133, 135 }) out += std::format("   depth {:>3}: dist > {}\n", d, (kWin - d) - C);
    }
    Logger::WriteMessage(out.c_str());
    Assert::IsTrue(!moves.empty());
  }

  // PROOF (not survey): builds the REAL Toad's Folly cooking geometry as a one-sausage relaxation and computes, by
  // exhaustive enumeration + reverse BFS, the exact minimum number of moves to FULLY COOK a sausage that is currently
  // fully uncooked at Manhattan distance D from the 2x2 grill. Rows 0-6 are byte-for-byte the real level (every wall and
  // board edge that could brace a roll is preserved); row 7 is a void moat that confines Stephen+target to the cooking
  // area and isolates the two forced decoys (NUM_SAUSAGES==3), which are pre-cooked and never move. Deleting only the
  // far-away rows 7-9 floor cannot shorten a cook near the grill and adds no bracing, so F_cook(D) here is the true
  // real-geometry floor. C_sound = min_D (F_cook(D) - D) is then the LARGEST provably-sound constant for the Filter-3
  // rule "fully-uncooked sausage: distToGrill + C > budget => prune". Purely in-memory; the 649 GB cache is untouched.
  TEST_METHOD(ToadsFollyCookFloorProof) {
    const char* grid =
      "__________"
      "_##_______"
      "_##_____2_"
      "__________"
      "222L______"
      "252L______"
      "222_______"
      "          "  // void moat: confines play to rows 0-6 and seals off the decoy shelf below
      "__________"
      "__________";
    Level lvl(10, 10, "cook-floor", grid, Stephen{ 5, 5, 0, Up }, {},
      { Sausage{ 7, 5, 8, 5, 0, Sausage::None },           // target: fully uncooked, free to roam rows 0-6
        Sausage{ 0, 8, 1, 8, 0, Sausage::FullyCooked },    // decoy 1: parked below the moat, unreachable + inert
        Sausage{ 3, 8, 4, 8, 0, Sausage::FullyCooked } }); // decoy 2

    auto distBox = [](int px, int py) { int dx = px < 1 ? 1 - px : (px > 2 ? px - 2 : 0); int dy = py < 1 ? 1 - py : (py > 2 ? py - 2 : 0); return dx + dy; };
    auto allCooked = [](const State& s) { for (const Sausage& sg : s.sausages) if (!sg.IsFullyCooked()) return false; return true; };

    // ---- exhaustive forward enumeration of the reachable single-sausage state space (real Move mechanics) ----
    const u32 CAP = 6'000'000;
    absl::flat_hash_map<State, u32> id; id.reserve(1'000'000);
    std::vector<State> states; states.reserve(1'000'000);
    std::vector<std::pair<u32, u32>> edges; edges.reserve(4'000'000);
    auto intern = [&](const State& s) -> u32 {
      auto it = id.find(s); if (it != id.end()) return it->second;
      u32 n = (u32)states.size(); id.emplace(s, n); states.push_back(s); return n;
    };
    u32 seedId = intern(lvl.GetState());
    std::vector<u32> q{ seedId };
    bool capped = false;
    for (size_t head = 0; head < q.size() && !capped; head++) {
      u32 u = q[head]; State su = states[u]; // copy: `states` may reallocate inside intern()
      for (Direction dir : { Up, Down, Left, Right }) {
        lvl.SetState(su); if (!lvl.Move(dir)) continue;
        u32 before = (u32)states.size();
        u32 cid = intern(lvl.GetState());
        edges.emplace_back(u, cid);
        if (cid == before) { if (states.size() >= CAP) { capped = true; break; } q.push_back(cid); }
      }
    }
    const u32 N = (u32)states.size();

    // ---- reverse adjacency in CSR form ----
    std::vector<u32> revStart(N + 1, 0);
    for (auto& e : edges) revStart[e.second + 1]++;
    for (u32 i = 0; i < N; i++) revStart[i + 1] += revStart[i];
    std::vector<u32> revNodes(edges.size());
    { std::vector<u32> cur(revStart.begin(), revStart.begin() + N);
      for (auto& e : edges) revNodes[cur[e.second]++] = e.first; }

    // ---- reverse BFS from every fully-cooked (goal) state => dist[i] = min forward moves to fully cook the target ----
    std::vector<u16> dist(N, 0xFFFF);
    std::vector<u32> bfs; bfs.reserve(N);
    for (u32 i = 0; i < N; i++) if (allCooked(states[i])) { dist[i] = 0; bfs.push_back(i); }
    const u32 goalCount = (u32)bfs.size();
    for (size_t head = 0; head < bfs.size(); head++) {
      u32 u = bfs[head]; u16 d = dist[u];
      for (u32 k = revStart[u]; k < revStart[u + 1]; k++) { u32 p = revNodes[k]; if (dist[p] == 0xFFFF) { dist[p] = (u16)(d + 1); bfs.push_back(p); } }
    }

    // ---- F_cook(D) = min moves-to-fully-cook over reachable states whose target is FULLY uncooked at distToGrill D ----
    std::array<int, 40> best; best.fill(999);
    for (u32 i = 0; i < N; i++) {
      if (dist[i] == 0xFFFF) continue; // fully-uncooked configs that can never be cooked are unwinnable anyway
      int uncooked = 0; const Sausage* tgt = nullptr;
      for (const Sausage& sg : states[i].sausages) if ((sg.flags & Sausage::FullyCooked) == 0) { uncooked++; tgt = &sg; }
      if (uncooked != 1) continue; // premise of Filter 3: exactly one fully-uncooked sausage (rest cooked)
      int D = (std::min)(distBox(tgt->x1, tgt->y1), distBox(tgt->x2, tgt->y2));
      if (D >= 0 && D < (int)best.size() && dist[i] < best[D]) best[D] = dist[i];
    }

    std::string out = std::format("\n=== PROVEN min moves to fully cook a fully-uncooked sausage vs distToGrill (real Toad's Folly geometry) ===\n"
      " states enumerated = {}{}, goal (all-cooked) states = {}\n"
      "  D | F_cook(D) | F_cook(D)-D\n", N, capped ? "  *** CAPPED -- NOT A COMPLETE PROOF ***" : "", goalCount);
    int cFloor = 999, cFloorD = -1;
    for (int D = 0; D < (int)best.size(); D++) {
      if (best[D] == 999) continue;
      out += std::format("  {:>1} | {:>9} | {:>11}\n", D, best[D], best[D] - D);
      if (best[D] - D < cFloor) { cFloor = best[D] - D; cFloorD = D; }
    }
    out += std::format("\n  Largest provably-sound C = min_D (F_cook(D)-D) = {}  (binds at D={})\n"
      "  Filter 3 currently uses C=8  =>  {}.\n",
      cFloor, cFloorD, cFloor >= 8 ? "PROVEN SOUND" : "NOT sound as a hard bound (survey only) -- this is the sound floor");
    Logger::WriteMessage(out.c_str());
    Assert::IsFalse(capped); // a capped enumeration would not be a valid proof
  }

  // Evaluates an ADMISSIBLE endgame heuristic (a true lower bound on moves-remaining-to-win) for Toad's Folly, for the
  // goal "prove 142 is optimal AND keep any 143/144 solutions". Part A replays the confirmed 142-move solution and
  // asserts H(state) <= 142-depth at every step (necessary condition for admissibility -- an admissible H can never
  // exceed the true remaining on an optimal path). Part B reads the REAL cached layers (read-only) at several depths and
  // reports what fraction of states an admissible prune (depth + H > T) would remove, for T=141 (the proof) and T=144
  // (keep-slower). H terms are each provably <= remaining: (all-cooked) Stephen must walk home; (uncooked) Stephen must
  // reach the grill to finish cooking AND the farthest uncooked sausage must reach the grill, then Stephen returns home
  // (+6 = min grill->home Manhattan). The 649 GB cache is only read.
  TEST_METHOD(ToadsFollyAdmissibleHeuristicBite) {
    const std::filesystem::path savedCwd = std::filesystem::current_path();
    if (!ChdirToCacheDir()) { Logger::WriteMessage("SKIP: no cache/ directory found.\n"); return; }
    const std::filesystem::path root = std::filesystem::current_path();
    const int kWin = 142;
    const std::pair<int, int> grill[] = { {1,1},{2,1},{1,2},{2,2} };
    auto md = [](int ax, int ay, int bx, int by) { return std::abs(ax - bx) + std::abs(ay - by); };
    auto dGrillPt = [&](int x, int y) { int m = 99; for (auto& g : grill) m = (std::min)(m, md(x, y, g.first, g.second)); return m; };
    const int grillToHome = 6; // proven min Manhattan from a grill cell (2,2) to home (5,5)
    auto H = [&](const State& s) -> int {
      int homeD = md(s.stephen.x, s.stephen.y, 5, 5);
      int maxSaus = -1; bool anyUncooked = false;
      for (const Sausage& sg : s.sausages)
        if (!sg.IsFullyCooked()) { anyUncooked = true; maxSaus = (std::max)(maxSaus, (std::min)(dGrillPt(sg.x1, sg.y1), dGrillPt(sg.x2, sg.y2))); }
      if (!anyUncooked) return homeD;
      int base = (std::max)(dGrillPt(s.stephen.x, s.stephen.y), maxSaus) + grillToHome;
      return (std::max)(base, homeD);
    };

    // ---- Part A: admissibility on the confirmed optimal path (H must never exceed 142-depth) ----
    std::vector<Direction> moves;
    { std::ifstream in(root / "toads-folly-142.dem"); std::string line;
      while (std::getline(in, line)) { if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line == "North") moves.push_back(Up); else if (line == "South") moves.push_back(Down);
        else if (line == "East") moves.push_back(Right); else if (line == "West") moves.push_back(Left); } }
    std::string outA = "\n=== admissible H vs true remaining on the 142 solution (H must be <= remaining) ===\n depth | remaining | H | slack\n";
    int worstViolation = -9999; bool haveDem = !moves.empty();
    if (haveDem) {
      Level* level = &ToadsFolly;
      const State init = level->GetState(); level->SetState(init);
      auto check = [&](u32 d) { int rem = kWin - (int)d, h = H(level->GetState()); worstViolation = (std::max)(worstViolation, h - rem);
        if (d >= 118 || d % 20 == 0) outA += std::format("  {:>4} | {:>9} | {:>2} | {:>4}\n", d, rem, h, rem - h); };
      check(0); u32 depth = 0;
      for (Direction dir : moves) { if (!level->Move(dir)) break; depth++; check(depth); if (level->Won()) break; }
    }
    outA += std::format(" worst (H - remaining) over the whole solution = {}  ({})\n",
      worstViolation, worstViolation <= 0 ? "ADMISSIBLE on the optimal path" : "INADMISSIBLE -- would prune the optimum!");
    Logger::WriteMessage(outA.c_str());

    // ---- Part B: how hard does an admissible prune bite the REAL cached layers? ----
    std::string outB = "\n=== admissible-prune bite on real cached states (bucket 0 sample, <=5M/depth) ===\n"
      " depth | sampled | prune%% @T=141 (prove) | prune%% @T=144 (keep 143/144)\n";
    for (int d : { 120, 125, 130, 133, 135, 137, 139 }) {
      u64 n = 0, cut141 = 0, cut144 = 0;
      try {
        LayerCache<State> layer("depth", d, "bucket", 0);
        for (const State& s : layer) { int h = H(s); int reach = d + h; if (reach > 141) cut141++; if (reach > 144) cut144++; if (++n >= 5'000'000) break; }
      } catch (const std::exception&) {}
      if (n == 0) { outB += std::format("  {:>4} | (no cache layer)\n", d); continue; }
      outB += std::format("  {:>4} | {:>7} | {:>20.2f} | {:>27.2f}\n", d, n, 100.0 * cut141 / n, 100.0 * cut144 / n);
    }
    Logger::WriteMessage(outB.c_str());

    std::filesystem::current_path(savedCwd);
    Assert::IsTrue(worstViolation <= 0 || !haveDem); // fail loudly if H is inadmissible on the optimal path
  }

  // Projects how many states an admissible-heuristic search would actually explore, using the REAL per-depth layer sizes
  // (from the production solve log) times the REAL keep-fraction (states with depth + H <= T) measured on the cache. An
  // admissible bound never prunes an ancestor of a <=T solution, so pruning the tail to zero cascades: those states never
  // spawn the multi-billion deep layers. This is the feasibility check for "prove 142 optimal AND keep 143/144" (T=144).
  // Read-only; cache untouched.
  TEST_METHOD(ToadsFollyAdmissibleProofFeasibility) {
    const std::filesystem::path savedCwd = std::filesystem::current_path();
    if (!ChdirToCacheDir()) { Logger::WriteMessage("SKIP: no cache/ directory found.\n"); return; }
    const std::pair<int, int> grill[] = { {1,1},{2,1},{1,2},{2,2} };
    auto md = [](int ax, int ay, int bx, int by) { return std::abs(ax - bx) + std::abs(ay - by); };
    auto dGrillPt = [&](int x, int y) { int m = 99; for (auto& g : grill) m = (std::min)(m, md(x, y, g.first, g.second)); return m; };
    auto H = [&](const State& s) -> int {
      int homeD = md(s.stephen.x, s.stephen.y, 5, 5);
      int maxSaus = -1; bool anyUncooked = false;
      for (const Sausage& sg : s.sausages)
        if (!sg.IsFullyCooked()) { anyUncooked = true; maxSaus = (std::max)(maxSaus, (std::min)(dGrillPt(sg.x1, sg.y1), dGrillPt(sg.x2, sg.y2))); }
      if (!anyUncooked) return homeD;
      return (std::max)((std::max)(dGrillPt(s.stephen.x, s.stephen.y), maxSaus) + 6, homeD);
    };

    // Real layer sizes (new states per depth) from the production solve log.
    const std::pair<int, u64> layer[] = {
      {119,34092956},{120,39977141},{121,47494623},{122,57147657},{123,69873790},{124,86480553},
      {125,108392131},{126,136717048},{127,173471795},{128,219993674},{129,279051027},{130,352140272},
      {131,442967347},{132,553249845},{133,688151452},{134,849821654},{135,1045643067},{136,1278772917},
      {137,1559830728},{138,1893346548},{139,2293693983},{140,2766202737} };

    std::string out = "\n=== admissible-heuristic proof feasibility (real layer sizes x measured keep-fraction) ===\n"
      " depth |    layer size | keep%% T=141 | keep%% T=144 |  kept T=141 |  kept T=144\n";
    u64 totalAll = 0, keptProve = 0, keptKeep = 0;
    for (auto& [d, size] : layer) {
      totalAll += size;
      u64 n = 0, k141 = 0, k144 = 0;
      try {
        LayerCache<State> lc("depth", d, "bucket", 0);
        for (const State& s : lc) { int f = d + H(s); if (f <= 141) k141++; if (f <= 144) k144++; if (++n >= 3'000'000) break; }
      } catch (const std::exception&) {}
      double f141 = n ? (double)k141 / n : 0, f144 = n ? (double)k144 / n : 0;
      u64 kp = (u64)(size * f141), kk = (u64)(size * f144);
      keptProve += kp; keptKeep += kk;
      out += std::format("  {:>4} | {:>13} | {:>10.2f} | {:>10.2f} | {:>11} | {:>11}\n", d, size, 100 * f141, 100 * f144, kp, kk);
    }
    out += std::format("\n  unpruned states, depths 119-140      : {:>15}\n"
      "  explored with H, T=141 (prove 142)   : {:>15}   ({:.1f}x smaller)\n"
      "  explored with H, T=144 (keep 143/144): {:>15}   ({:.1f}x smaller)\n",
      totalAll, keptProve, keptProve ? (double)totalAll / keptProve : 0,
      keptKeep, keptKeep ? (double)totalAll / keptKeep : 0);
    Logger::WriteMessage(out.c_str());

    std::filesystem::current_path(savedCwd);
    Assert::IsTrue(true);
  }

  // Answers "is the top-left (grill) box enough, or do we also need the home box?" by measuring, on the real cached
  // layers: what fraction of states are already all-cooked (where the grill box prunes nothing), and how much extra the
  // home box removes on top of the grill box. Budget 145 (the wired value). Read-only; cache untouched.
  TEST_METHOD(ToadsFollyGrillBoxSufficiency) {
    const std::filesystem::path savedCwd = std::filesystem::current_path();
    if (!ChdirToCacheDir()) { Logger::WriteMessage("SKIP: no cache/ directory found.\n"); return; }
    auto gd = [](int x, int y) { int dx = x < 1 ? 1 - x : (x > 2 ? x - 2 : 0); int dy = y < 1 ? 1 - y : (y > 2 ? y - 2 : 0); return dx + dy; };
    std::string out = "\n=== grill box vs grill+home box on real cached states (budget 145, bucket 0, <=3M/depth) ===\n"
      " depth | sampled | all-cooked%% | keep%% grill-only | keep%% grill+home | home box removes\n";
    for (int d : { 125, 130, 133, 135, 137, 139 }) {
      const int slack = 145 - d, reach = slack - 6;
      u64 n = 0, cooked = 0, keepGrill = 0, keepFull = 0;
      try {
        LayerCache<State> lc("depth", d, "bucket", 0);
        for (const State& s : lc) {
          bool allCooked = true, grillPruned = false;
          for (const Sausage& sg : s.sausages) {
            if (sg.IsFullyCooked()) continue;
            allCooked = false;
            if ((std::min)(gd(sg.x1, sg.y1), gd(sg.x2, sg.y2)) > reach) grillPruned = true;
          }
          if (!allCooked && gd(s.stephen.x, s.stephen.y) > reach) grillPruned = true;
          bool homePruned = (std::abs(s.stephen.x - 5) + std::abs(s.stephen.y - 5)) > slack;
          if (allCooked) cooked++;
          if (!grillPruned) keepGrill++;
          if (!grillPruned && !homePruned) keepFull++;
          if (++n >= 3'000'000) break;
        }
      } catch (const std::exception&) {}
      if (n == 0) { out += std::format("  {:>4} | (no cache layer)\n", d); continue; }
      out += std::format("  {:>4} | {:>7} | {:>10.2f} | {:>16.2f} | {:>16.2f} | {:>15.2f}%%\n",
        d, n, 100.0 * cooked / n, 100.0 * keepGrill / n, 100.0 * keepFull / n, 100.0 * (keepGrill - keepFull) / n);
    }
    Logger::WriteMessage(out.c_str());
    std::filesystem::current_path(savedCwd);
    Assert::IsTrue(true);
  }

  // Cheap re-test of the wired heuristic on KNOWN winning states (no BFS): replay the confirmed 142-move solution and
  // apply the heuristic to every state on it. The heuristic must keep all of them (first-pruned depth = -1). Also reports
  // the tightest x+y margin the winning path leaves under the limit -- i.e. how many moves the 145 budget could be
  // tightened before it would ever prune the known solution. Reads only the demo file; the cache is untouched.
  TEST_METHOD(ToadsFollyHeuristicKeepsKnownWinners) {
    const std::filesystem::path savedCwd = std::filesystem::current_path();
    if (!ChdirToCacheDir()) { Logger::WriteMessage("SKIP: no cache/ directory found.\n"); return; }
    Level* lvl = &ToadsFolly;
    if (lvl->heuristic == nullptr) { std::filesystem::current_path(savedCwd); Assert::Fail(L"ToadsFolly has no heuristic"); }
    const State startState = lvl->GetState();

    std::vector<Direction> moves;
    { std::ifstream in(std::filesystem::current_path() / "toads-folly-142.dem"); std::string line;
      while (std::getline(in, line)) { if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line == "North") moves.push_back(Up); else if (line == "South") moves.push_back(Down);
        else if (line == "East") moves.push_back(Right); else if (line == "West") moves.push_back(Left); } }
    if (moves.empty()) { std::filesystem::current_path(savedCwd); Logger::WriteMessage("SKIP: toads-folly-142.dem not found.\n"); return; }

    // Replay the known-winning solution into a per-depth list of states.
    std::vector<State> path; lvl->SetState(startState); path.push_back(startState);
    for (Direction dir : moves) { if (!lvl->Move(dir)) break; path.push_back(lvl->GetState()); if (lvl->Won()) break; }

    // Apply the heuristic to every winning state; track the first (if any) it prunes and the tightest x+y margin.
    int firstPruned = -1, minSlack = 999, minSlackDepth = -1;
    for (int d = 0; d < (int)path.size(); d++) {
      lvl->SetState(path[d]);
      if (!lvl->heuristic(lvl, (u32)d) && firstPruned < 0) firstPruned = d;
      if (d >= 94) { // grill bound is only active in the danger zone; margin = limit - worst uncooked/Stephen x+y
        const int limit = 143 - d;
        int worst = -1;
        for (const Sausage& s : path[d].sausages) if (!s.IsFullyCooked()) worst = (std::max)(worst, (int)(s.x1 + s.y1));
        if (worst >= 0) {
          worst = (std::max)(worst, (int)(path[d].stephen.x + path[d].stephen.y));
          int slack = limit - worst;
          if (slack < minSlack) { minSlack = slack; minSlackDepth = d; }
        }
      }
    }

    std::string out = std::format("\n=== heuristic re-tested on the {} known winning states of the 142 solution ===\n"
      "  first pruned depth = {}  ({})\n"
      "  tightest x+y margin on the winning path = {} (at depth {}): the 145 budget could be tightened by up to {} before\n"
      "  the known 142 solution would be pruned.\n",
      path.size(), firstPruned, firstPruned < 0 ? "never pruned -- all winners kept" : "PRUNED A WINNER!",
      minSlack, minSlackDepth, minSlack);
    Logger::WriteMessage(out.c_str());
    std::filesystem::current_path(savedCwd);
    Assert::AreEqual(-1, firstPruned);
  }
};
