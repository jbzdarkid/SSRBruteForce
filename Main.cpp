#include "Levels.h"
#include "Solver2.h"

#include <cctype>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <locale>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

const char* DIR_NAMES[] = {"None", "North", "West", "Jump", "Crouch", "East", "South"};

// -------- RRT-style novelty explorer: grow a SPARSE tree of "landmark" states over the reachable graph. Memory is
// O(distinct regions), NOT O(all states), so it never becomes a BFS closed set. Sparsification is an OCCUPANCY GRID:
// each state's signature (positions quantized by |bin|, plus exact cook/facing) hashes to a u64 cell id, and a
// landmark is kept only for the FIRST state to fall in each cell -- an O(1) hash check that replaces the old O(N) L1
// nearest-neighbor scan, so a single tree can grow arbitrarily deep without slowing down. Growth is frontier-biased:
// each iteration expands the least-fanned-out of a few random candidates (fewest children = the growing frontier),
// then random-rollouts from it, planting a chain of new landmarks wherever the walk enters a fresh cell. Each
// landmark's root-anchored path (segments up the parent chain) is a long demo -- ideal for the oracle, which validates
// every intermediate move of a path in one replay. Not complete; it's a sampler. |bin| is the resolution dial (1 =
// every distinct state; larger = coarser regions).
static u64 BucketKey(const State& s, int bin) {
  u64 h = 1469598103934665603ull; // FNV-1a offset basis
  auto mix = [&](s64 v) { h = (h ^ (u64)v) * 1099511628211ull; };
  mix(s.stephen.x / bin); mix(s.stephen.y / bin); mix(s.stephen.z / bin); mix((s64)s.stephen.dir);
  mix(s.stephen.forkX / bin); mix(s.stephen.forkY / bin); mix(s.stephen.forkZ / bin); mix((s64)s.stephen.forkDir);
  for (int i = 0; i < NUM_SAUSAGES; i++) {
    const Sausage& sa = s.sausages[i];
    mix(sa.x1 / bin); mix(sa.y1 / bin); mix(sa.x2 / bin); mix(sa.y2 / bin); mix(sa.z / bin); mix((s64)sa.flags);
  }
  return h;
}

struct RRTNode {
  State state;
  int parent;
  std::vector<Direction> segment; // moves from |parent|'s state to this state
  int depth;                       // total moves from the root
};

// Print an unimplemented-move gap once per kind, with the scenario it fired on: the level layout, the pre-move state
// and the direction pressed -- everything needed to paste a MAKE_SYMMETRICAL_TEST straight out of the log. Leaves
// |level| restored to |before| so the caller can carry on treating the move as refused.
static void ReportGap(const UnimplementedMove& gap, Level* level, const State& before, Direction dir) {
  static std::vector<const char*> seen;
  for (const char* s : seen) if (strcmp(s, gap.what) == 0) return;
  seen.push_back(gap.what);

  level->SetState(before);
  printf("\n*** UNIMPLEMENTED: %s\n*** pressing %s from:\n", gap.what, DIR_NAMES[dir]);
  level->Print();
  std::cout << before << std::endl;
}

static void RRTExplore(Level* level, int iterations, int rolloutLen, int bin, int frontierK, u32 seed, const std::string& outDir) {
  const int INF = 1 << 30;
  if (bin < 1) bin = 1;
  std::mt19937 rng(seed);
  State start = level->GetState();
  std::vector<RRTNode> tree;
  std::vector<int> childCount;            // per node -- frontier proxy + leaf detection
  std::unordered_set<u64> occupied;       // O(1) sparsifier: one landmark per grid cell
  tree.push_back({ start, -1, {}, 0 });
  childCount.push_back(0);
  occupied.insert(BucketKey(start, bin));
  Direction dirs[4] = { Up, Down, Left, Right };

  int maxDepth = 0, productiveRollouts = 0;
  for (int it = 0; it < iterations; it++) {
    // Frontier proxy: of |frontierK| random candidates, expand the one with the FEWEST children -- interior nodes have
    // already fanned out, so the least-branched ones are the growing frontier. O(1) each (vs the old O(N) isolation scan).
    int from = 0;
    if (tree.size() > 1) {
      int bestChildren = INF;
      for (int k = 0; k < frontierK; k++) {
        int cand = (int)(rng() % tree.size());
        if (childCount[cand] < bestChildren) { bestChildren = childCount[cand]; from = cand; }
      }
    }

    level->SetState(tree[from].state);
    std::vector<Direction> segment;
    bool grew = false;
    for (int step = 0; step < rolloutLen; step++) {
      State before = level->GetState();
      int order[4] = { 0, 1, 2, 3 };
      for (int i = 3; i > 0; i--) { int j = (int)(rng() % (i + 1)); std::swap(order[i], order[j]); }
      bool moved = false;
      for (int oi = 0; oi < 4; oi++) {
        level->SetState(before);
        // Reject bonks: the game accepts wall/grill/turn-bonks that change nothing, but they never advance an optimal
        // solution -- treat a no-op move as refused so no demo contains one.
        // A move that hits a gap in the engine is reported (once per kind) and then treated as refused, so one hunt
        // surfaces every distinct gap instead of dying on the first.
        try {
          if (level->Move(dirs[order[oi]]) && !(level->GetState() == before)) { segment.push_back(dirs[order[oi]]); moved = true; break; }
        } catch (const UnimplementedMove& gap) {
          ReportGap(gap, level, before, dirs[order[oi]]);
        }
      }
      if (!moved) { level->SetState(before); break; } // dead end -- every move refused or a no-op

      State cur = level->GetState();
      if (occupied.insert(BucketKey(cur, bin)).second) { // first state in this grid cell -> a genuinely new region
        int depth = tree[from].depth + (int)segment.size();
        childCount[from]++;
        tree.push_back({ cur, from, segment, depth });
        childCount.push_back(0);
        if (depth > maxDepth) maxDepth = depth;
        grew = true;
        from = (int)tree.size() - 1; // chain: further landmarks this rollout branch off the one just planted
        segment.clear();
      }
      if (level->Won()) break;
    }
    if (grew) productiveRollouts++;
  }

  // Root-anchored path for a landmark: walk up the parent chain, concatenating segments front-to-back.
  auto rootPath = [&](int idx) {
    std::vector<int> chain;
    for (int i = idx; i != -1; i = tree[i].parent) chain.push_back(i);
    std::vector<Direction> path;
    for (int ci = (int)chain.size() - 1; ci >= 0; ci--) {
      const auto& seg = tree[chain[ci]].segment;
      path.insert(path.end(), seg.begin(), seg.end());
    }
    return path;
  };

  // Only LEAF landmarks (childCount 0) need a demo: a leaf's root-anchored path already traverses every one of its
  // ancestors, so replaying leaves implicitly validates all interior landmark states (one long path checks every
  // intermediate for free). Write them, tracking the longest leaf demo as we go.
  std::filesystem::create_directories(outDir);
  for (const auto& e : std::filesystem::directory_iterator(outDir))
    if (e.path().extension() == ".dem") std::filesystem::remove(e.path());
  int n = 0, longestDemo = 0;
  Solver scorer(level);
  for (int i = 1; i < (int)tree.size(); i++) {
    if (childCount[i] != 0) continue; // interior landmark -- already on some leaf's root path
    std::vector<Direction> path = rootPath(i);
    if ((int)path.size() > longestDemo) longestDemo = (int)path.size();
    char name[32]; snprintf(name, sizeof(name), "%05d.dem", n++);
    std::ofstream out(outDir + "/" + name);
    for (Direction d : path) out << DIR_NAMES[d] << '\n';
    out << "Stop\n" << tree[i].state << '\n';
    // Record this engine's per-move ComputeScore units so the oracle can check its tick-simulated timing against ours,
    // move by move -- the timing analogue of the final-state check. ComputeScore matches sausages by array slot, so it
    // needs the UNSORTED state (GetState(false)); the sorted GetState() reorders slots and would fabricate phantom drops.
    level->SetState(start);
    State prev = level->GetState(false);
    out << "Units:";
    for (Direction d : path) {
      level->Move(d);
      State cur = level->GetState(false);
      out << ' ' << scorer.ComputeScore(prev, d, cur);
      prev = cur;
    }
    out << '\n';
  }

  // --- Diagnostics (all O(N) -- no pairwise distance) ---
  int landmarks = (int)tree.size() - 1;
  int depthBuckets[8] = { 0 };
  for (int i = 1; i < (int)tree.size(); i++) { int db = tree[i].depth / 10; if (db > 7) db = 7; depthBuckets[db]++; }

  printf("RRT %s [iters=%d rollout=%d bin=%d frontierK=%d seed=0x%X]\n",
         level->name, iterations, rolloutLen, bin, frontierK, seed);
  printf("  landmarks=%d  leaves=%d  maxDepth=%d moves  longestLeafDemo=%d moves  productiveIters=%d/%d\n",
         landmarks, n, maxDepth, longestDemo, productiveRollouts, iterations);
  printf("  depth histogram (0-9,10-19,...,70+): ");
  for (int b = 0; b < 8; b++) printf("%d%s ", depthBuckets[b], b == 7 ? "+" : "");
  printf("\n");
  printf("  wrote %d leaf demos (of %d landmarks) -> %s/\n", n, landmarks, outDir.c_str());
}

bool TestLevel(Level* level, std::vector<Direction> moves) {
  printf("=== initial state ===\n");
  level->Print();

  State previousState = level->GetState();
  u32 totalUnits = 0;
  for (int i = 0; i < (int)moves.size(); i++) {
    Direction dir = moves[i];
    bool success;
    try {
      success = level->Move(dir);
    } catch (const UnimplementedMove& gap) {
      ReportGap(gap, level, previousState, dir);
      return false; // the replay can't continue past a move we don't model
    }

    State state = level->GetState();
#if OVERWORLD_HACK
    level->RefreshOverworldWalls();
#endif
    printf("\n=== move %d: %s %s ===\n", (i+1), DIR_NAMES[dir], (success ? "SUCCEEDED" : "FAILED"));

    u32 moveUnits = Solver(level).ComputeScore(previousState, dir, state);
    moveUnits = (moveUnits + 999) / 1000; // Compensating for any tie-breaks.
    totalUnits += moveUnits;
    previousState = state;

    std::cout << state << ' ' << moveUnits << ' ' << totalUnits << std::endl;
    level->Print();

    if (level->Won()) break; // Demos have trailing moves
    if (!success) break;
  }

  return level->Won();
}

bool SolveLevel(Level* level) {
#if OVERWORLD_HACK
  State start = level->GetState();
#endif
  Solver solver(level);
  std::vector<Direction> solution = solver.Solve();

  if (solution.empty()) return false;

  std::ofstream out("solved.dem");
#if OVERWORLD_HACK
  // Replay the solution so each move that steps onto a level entrance can be tagged for later stitching.
  level->SetState(start);
  for (Direction dir : solution) {
    out << DIR_NAMES[dir] << '\n';
    level->Move(dir);
    level->RefreshOverworldWalls();
    if (const char* entered = level->EnteredLevel()) out << "Level " << entered << '\n';
  }
#else
  for (Direction dir : solution) out << DIR_NAMES[dir] << '\n';
#endif
  return true;
}

int main(int argc, char* argv[]) {
  setvbuf(stdout, nullptr, _IONBF, 0); // Disable stdout buffering so we see partial output on crash.
  std::cout.imbue(std::locale("en-US")); // Used for cout decimal formatting in some places.

  if (argc == 1) {
    std::cout << "Invalid args\n";
    return 0;
  }

  std::string filter = std::string{ argv[1] };
  std::string demoPath;
  if (argc >= 3) demoPath = std::string{ argv[2] };

  bool surveyAll = (filter == "ALL");
  for (Level* test : tests) {
    if (test->GetSausages().Size() != NUM_SAUSAGES) continue;
    if (!surveyAll && !std::strstr(test->name, filter.c_str())) continue; // Failed to match filter

    if (!demoPath.empty()) {
      if (demoPath == "rrt") {
        int iterations = (argc >= 4) ? atoi(argv[3]) : 2000;
        int rolloutLen = (argc >= 5) ? atoi(argv[4]) : 40;
        int bin        = (argc >= 6) ? atoi(argv[5]) : 3;
        u32 seed       = (argc >= 7) ? (u32)strtoul(argv[6], nullptr, 0) : 0xC0FFEEu;
        std::string safe = test->name;
        for (char& c : safe) if (!std::isalnum((unsigned char)c)) c = '_';
        RRTExplore(test, iterations, rolloutLen, bin, 8, seed, "oracle-demos/" + safe);
        return 0;
      }
      std::ifstream file(demoPath);
      if (!file.is_open()) {
        printf("Failed to open demo file\n");
        return 1;
      }

      std::vector<Direction> buffer;

      std::string line;
      while (std::getline(file, line)) {
        if (line == "North") buffer.push_back(Up);
        if (line == "South") buffer.push_back(Down);
        if (line == "East")  buffer.push_back(Right);
        if (line == "West")  buffer.push_back(Left);
        if (line == "Undo") {
          buffer.pop_back();
          continue;
        }
      }

      // "units" mode: emit only the per-move ComputeScore units line, computed from UNSORTED slot-matched states
      // exactly as the RRT writer does, so the timing checker can re-score a saved demo against the CURRENT
      // ComputeScore without re-running the 100k RRT.
      if (argc >= 4 && std::string(argv[3]) == "units") {
        Solver scorer(test);
        State start = test->GetState();
        test->SetState(start);
        State prev = test->GetState(false);
        printf("Units:"); // plain integers (no locale grouping), matching the RRT writer's ofstream output
        for (Direction dir : buffer) {
          test->Move(dir);
          State cur = test->GetState(false);
          printf(" %d", scorer.ComputeScore(prev, dir, cur));
          prev = cur;
        }
        printf("\n");
        return 0;
      }

      printf("Testing level %s\n", test->name);
      bool success = TestLevel(test, std::move(buffer));
      if (!success) printf("Demo replay did not solve the level.\n");
      return success ? 0 : 2;
    } else {
      printf("Solving level %s\n", test->name);

      bool success = SolveLevel(test);
      if (!success) printf("Solver could not solve the level.\n");
      return success ? 0 : 3;
    }
  }

  if (surveyAll) return 0;
  // No level matched the filter for this build's sausage count.
  printf("No level matched filter '%s' for a %d-sausage build.\n", filter.c_str(), NUM_SAUSAGES);
  return 4;
}
