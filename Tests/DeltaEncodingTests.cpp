#include "CppUnitTest.h"
#include "LayerCache.h" // pulls in u128 (absl::uint128) via Common.h
#include "State.h"     // State (the record we sort + delta-encode)
#include "Levels.h"    // ColdTrail (to benchmark real Move expansion on cached states)

#include <absl/container/flat_hash_map.h> // mirrors Solver's _winningStates for the Stage-2 bench

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <format>
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
  static_assert(sizeof(State) == 32, "numeric-delta codec assumes a 32-byte (4-limb) record");
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
};
