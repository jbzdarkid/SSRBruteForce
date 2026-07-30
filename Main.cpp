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

// A "goal" predicate for winOverride: the state holds a sausage FLOATING in mid-air -- a level or more up, with every
// end unsupported (no wall-top, no other sausage, and not resting on / impaled by Stephen). This is the reference's
// ladder-descent quirk: it lowers the speared/fork/head sausage but never re-checks gravity on a rider merely resting
// on it. Pointed at winOverride, the ordinary solver finds the shortest move sequence that produces such a state.
bool IsFloatingState(const LevelData* level) {
  const Stephen& man = level->GetStephen();
  const Vector<Sausage>& sausages = level->Sausages();
  for (int i = 0; i < (int)sausages.Size(); i++) {
    const Sausage& s = sausages[i];
    if (s.z < 1) continue;                                                        // on the ground floor
    if (man.HasFork() && s.IsAt(man.forkX, man.forkY, man.forkZ)) continue;       // impaled on the fork -> held
    auto endSupported = [&](s8 x, s8 y) -> bool {
      if (level->IsWall(x, y, s.z - 1)) return true;                              // wall-top
      if (man.x == x && man.y == y && man.z == s.z - 1) return true;             // Stephen's head
      if (man.HasFork() && man.forkX == x && man.forkY == y && man.forkZ == s.z - 1) return true; // his fork
      for (int j = 0; j < (int)sausages.Size(); j++) {
        if (j == i) continue;
        const Sausage& o = sausages[j];
        if (o.z == s.z - 1 && o.IsAt(x, y, o.z)) return true;                     // another sausage below
      }
      return false;
    };
    if (!endSupported(s.x1, s.y1) && !endSupported(s.x2, s.y2)) return true;
  }
  return false;
}

// Alt winOverride goal for the 3-3 Cold Escarpment rotation-drop divergence: Stephen faces West at (11,12) with his
// fork at (10,12), a horizontal "hat" sausage straddles (10,11)-(11,11) z=1 resting on a vertical base at
// (10,10)-(10,11) z=0. Pressing North here turns him North; the reference tips the hat one cell East off the fork and
// drops it, while Level2 leaves it on the fork. Point winOverride here and run |findpath| for the shortest demo to
// this pose -- then press North to reproduce the divergence.
bool IsEscarpmentRotationDrop(const LevelData* level) {
  const Stephen& man = level->GetStephen();
  if (!(man.x == 11 && man.y == 12 && man.z == 0 && man.dir == Left)) return false;
  if (!(man.HasFork() && man.forkX == 10 && man.forkY == 12 && man.forkZ == 0)) return false;
  const Vector<Sausage>& sausages = level->Sausages();
  bool hat = false, base = false;
  for (int i = 0; i < (int)sausages.Size(); i++) {
    const Sausage& s = sausages[i];
    if (s.IsAt(10, 11, 1) && s.IsAt(11, 11, 1)) hat = true;   // horizontal hat on the fork-swing cell
    if (s.IsAt(10, 10, 0) && s.IsAt(10, 11, 0)) base = true;  // vertical base under its west end
  }
  return hat && base;
}

// winOverride goal for the "gap 2" log-roll head-hat divergence. Detects the pre-roll pose: Stephen stands ON a sausage
// (the log), carries a head-hat, and that hat's FAR half (the end not over his head) rests on a THIRD sausage which is
// itself riding the log. When he then presses ACROSS the log (a log-roll), the log rolls and carries that third sausage
// out from under the hat's far end -- and the reference flings the head-hat off Stephen onto the moving sausage while
// Level2 keeps it on his head. Point winOverride here, |findpath| to this pose, then press across the log to reproduce.
bool IsLogRollHatDivergence(const LevelData* level) {
  const Stephen& man = level->GetStephen();
  const Vector<Sausage>& sausages = level->Sausages();
  s8 logNo = level->GetSausage(man.x, man.y, man.z - 1);   // Stephen must be standing on a sausage
  if (logNo == -1) return false;
  s8 hatNo = level->GetSausage(man.x, man.y, man.z + 1);   // ...with a head-hat above him
  if (hatNo == -1) return false;
  const Sausage& log = sausages[logNo];
  const Sausage& hat = sausages[hatNo];
  // A log-roll only fires when Stephen faces ALONG the log's long axis and presses across it.
  bool canRoll = (log.IsHorizontal() && (man.dir == Up || man.dir == Down))
              || (log.IsVertical()   && (man.dir == Left || man.dir == Right));
  if (!canRoll) return false;
  // The hat's far half is the end that isn't over Stephen's head; it must actually bridge off that cell.
  bool firstOnHead = (hat.x1 == man.x && hat.y1 == man.y);
  s8 farX = firstOnHead ? hat.x2 : hat.x1;
  s8 farY = firstOnHead ? hat.y2 : hat.y1;
  if (farX == man.x && farY == man.y) return false;        // hat sits squarely on the head (no cantilever) -> no divergence
  s8 midNo = level->GetSausage(farX, farY, man.z);         // the sausage under the hat's far end (hat.z-1 == man.z)
  if (midNo == -1 || midNo == logNo || midNo == hatNo) return false;
  // The "mid" must ride the LOG (rest on one of the log's ends) so that the roll carries it away this turn.
  const Sausage& mid = sausages[midNo];
  bool midOnLog = level->GetSausage(mid.x1, mid.y1, mid.z - 1) == logNo
               || level->GetSausage(mid.x2, mid.y2, mid.z - 1) == logNo;
  return midOnLog;
}

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
        if (level->Move(dirs[order[oi]])) { segment.push_back(dirs[order[oi]]); moved = true; break; }
      }
      if (!moved) { level->SetState(before); break; } // dead end -- every move refused

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
  for (int i = 1; i < (int)tree.size(); i++) {
    if (childCount[i] != 0) continue; // interior landmark -- already on some leaf's root path
    std::vector<Direction> path = rootPath(i);
    if ((int)path.size() > longestDemo) longestDemo = (int)path.size();
    char name[32]; snprintf(name, sizeof(name), "%05d.dem", n++);
    std::ofstream out(outDir + "/" + name);
    for (Direction d : path) out << DIR_NAMES[d] << '\n';
    out << "Stop\n" << tree[i].state << '\n';
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

// Replay every .dem in |dir| through THIS build's engine and compare the engine's end-of-simulation geometry to the
// state recorded on the demo's trailing line (written by whichever engine generated it). Prints how many demos the
// current engine ends differently on. Built into both engines, so running it from the reference build over Level2's
// demos reports exactly where the reference disagrees with Level2 (and, since Level2 tracks the game almost perfectly,
// with the game). Non-destructive: the demos are only read.
static void Reverify(Level* level, const std::string& dir) {
  State start = level->GetState();
  int total = 0, changed = 0;
  std::vector<std::string> mismatches;
  for (const auto& e : std::filesystem::directory_iterator(dir)) {
    if (e.path().extension() != ".dem") continue;
    std::vector<Direction> moves;
    std::string recorded;
    {
      std::ifstream in(e.path());
      std::string line;
      bool afterStop = false;
      while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line == "Stop") { afterStop = true; continue; }
        if (afterStop) { if (recorded.empty()) recorded = line; continue; }
        if      (line == "North") moves.push_back(Up);
        else if (line == "South") moves.push_back(Down);
        else if (line == "East")  moves.push_back(Right);
        else if (line == "West")  moves.push_back(Left);
      }
    }
    level->SetState(start);
    for (Direction d : moves) {
      if (!level->Move(d)) break;
      if (level->Won()) break;
    }
    std::ostringstream oss;
    oss << level->GetState();
    total++;
    if (oss.str() != recorded) {
      changed++;
      if (mismatches.size() < 20) mismatches.push_back(e.path().filename().string());
    }
  }
  printf("Reverify %s: %d demos, %d end differently from the recorded state.\n", dir.c_str(), total, changed);
  for (const std::string& m : mismatches) printf("  %s\n", m.c_str());
}

bool TestLevel(Level* level, std::vector<Direction> moves) {
  printf("=== initial state ===\n");
  level->Print();

  State previousState = level->GetState();
  u32 totalUnits = 0;
  for (int i = 0; i < (int)moves.size(); i++) {
    Direction dir = moves[i];
    bool success = level->Move(dir);

    State state = level->GetState();
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
  Solver solver(level);
  std::vector<Direction> solution = solver.Solve();

  if (solution.empty()) return false;

  std::ofstream out("solved.dem");
  for (Direction dir : solution) out << DIR_NAMES[dir] << '\n';
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
    if (test->NumSausages() != NUM_SAUSAGES) continue;
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
      if (demoPath == "reverify") {
        std::string safe = test->name;
        for (char& c : safe) if (!std::isalnum((unsigned char)c)) c = '_';
        std::string dir = (argc >= 4) ? std::string{ argv[3] } : ("oracle-demos/" + safe);
        Reverify(test, dir);
        return 0;
      }
      if (demoPath == "findpath") {
        // Let the ordinary solver find the shortest path to an alternate win state, written to solved.dem. Swap the
        // goal predicate for the scenario being reproduced.
        test->winOverride = &IsLogRollHatDivergence;
        bool ok = SolveLevel(test);
        printf(ok ? "Wrote solved.dem: shortest path to the alt win state.\n" : "No such state reachable.\n");
        return ok ? 0 : 5;
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
