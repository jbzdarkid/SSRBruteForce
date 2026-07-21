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

#ifdef USE_LEVEL2
static int PopCount(u32 v) { int c = 0; while (v) { v &= v - 1; c++; } return c; }

// Novelty + complexity guided Monte-Carlo explorer (runs on Level2, the engine under test). From the level start it
// does many random rollouts, at each step sampling the next input with weight proportional to how "distinctive" it is
// (how many mechanics its feature mask fired, whether the state changed) plus a novelty bonus for feature masks and
// consecutive-mask 2-grams not seen before. The path is archived whenever a move reaches a NOVEL Level2 state (so
// every first-entry transition -- including pure-geometry ones with no special feature -- gets probed) or produces a
// novel feature signature. Each archived path is a pure demo; the oracle replays it independently and an external
// diff compares trajectories. Broad state coverage is what lets the geometry-only divergences surface.
static void Explore(Level* level, int rollouts, int maxDepth, u32 seed, const std::string& outDir) {
  std::mt19937 rng(seed);
  State start = level->GetState();
  std::unordered_set<u32> seenMask;
  std::unordered_set<u64> seen2gram;
  std::set<State> seenState;
  seenState.insert(start);
  struct Archived { std::vector<Direction> path; State end; u32 mask; }; // path + Level2 end-state + the move's feature mask
  std::vector<Archived> archive;
  std::unordered_map<u32, std::vector<int>> maskBuckets; // feature mask -> its archive slots, for mechanic-diverse eviction
  Direction dirs[4] = { Up, Down, Left, Right };
  const int kMaxArchive = 40000;

  for (int r = 0; r < rollouts; r++) {
    level->SetState(start);
    std::vector<Direction> path;
    u32 prevMask = 0;
    for (int step = 0; step < maxDepth; step++) {
      State cur = level->GetState();
      double score[4]; u32 feat4[4]; bool acc4[4];
      double total = 0;
      for (int i = 0; i < 4; i++) {
        level->SetState(cur);
        level->_feat = 0;
        bool ok = level->Move(dirs[i]);
        feat4[i] = level->_feat; acc4[i] = ok;
        State res = level->GetState();
        bool changed = !(res == cur);
        u32 m = feat4[i];
        u64 gram = ((u64)prevMask << 32) | m;
        double nov = 0;
        if (m && seenMask.find(m) == seenMask.end()) nov += 6.0;
        if (m && seen2gram.find(gram) == seen2gram.end()) nov += 3.0;
        double s = 0.15 + PopCount(m) * 1.0 + nov + (changed ? 0.5 : 0.0);
        if (!ok) s = 0.0; // reject: never sample a refused move -- reroll among the accepted ones instead
        score[i] = s; total += s;
      }
      if (total <= 0.0) break; // no accepted move from here -- end the rollout rather than emit a rejected no-op
      double pick = (rng() / (double)0xFFFFFFFFu) * total;
      int d = 0; for (; d < 3; d++) { if (pick < score[d]) break; pick -= score[d]; }
      level->SetState(cur);
      level->_feat = 0;
      level->Move(dirs[d]);
      path.push_back(dirs[d]);
      u32 mask = feat4[d];
      u64 gram = ((u64)prevMask << 32) | mask;
      bool novel = false;
      if (seenState.insert(level->GetState()).second) novel = true;   // first time we reach this Level2 state
      if (mask && seenMask.insert(mask).second) novel = true;
      if (mask && seen2gram.insert(gram).second) novel = true;
      if (novel) {
        // Archive the demo. Below the (fixed) size cap we just append; once full we keep exploring and reservoir-evict,
        // but bias the eviction to preserve MECHANIC diversity: drop a demo from whichever feature mask is currently the
        // most over-represented (with a uniformly random pick *within* that dominant bucket). Rare/unique mechanics are
        // thus never crowded out by common ones (e.g. plain-geometry mask==0 moves), and the retained sample stays an
        // unbiased reservoir across the whole exploration instead of just the first-40000 prefix.
        State end = level->GetState();
        if ((int)archive.size() < kMaxArchive) {
          maskBuckets[mask].push_back((int)archive.size());
          archive.push_back({ path, end, mask });
        } else {
          u32 evMask = 0; size_t best = 0;
          for (const auto& kv : maskBuckets) if (kv.second.size() > best) { best = kv.second.size(); evMask = kv.first; }
          auto& bucket = maskBuckets[evMask];
          int pos = (int)(rng() % bucket.size());
          int slot = bucket[pos];
          bucket[pos] = bucket.back(); bucket.pop_back(); // swap-pop the evicted slot out of its bucket
          archive[slot] = { path, end, mask };
          maskBuckets[mask].push_back(slot);
        }
      }
      prevMask = mask;
      if (level->Won()) break;
    }
  }

  // Write each archived path as a standard .dem: the moves, then a "Stop" line, then Level2's end-of-simulation
  // geometry as raw ints (Stephen's body+fork pose, then each sausage's two cells + z). GetState already sorted the
  // sausages, so it's canonical. Move-replayers ignore the trailing non-move lines; the oracle replays the moves and
  // compares its own end geometry to that final line for a position divergence (and reports a loss on death).
  std::filesystem::create_directories(outDir);
  for (const auto& e : std::filesystem::directory_iterator(outDir))
    if (e.path().extension() == ".dem") std::filesystem::remove(e.path());
  int n = 0;
  for (const auto& a : archive) {
    char name[32]; snprintf(name, sizeof(name), "%05d.dem", n++);
    std::ofstream out(outDir + "/" + name);
    for (Direction d : a.path) out << DIR_NAMES[d] << '\n';
    out << "Stop\n" << a.end << '\n'; // State operator<< = Stephen + oracle-sorted sausages (matches the oracle's line)
  }
  printf("Explored %s: wrote %zu demos -> %s/ (%zu masks, %zu 2-grams)\n",
         level->name, archive.size(), outDir.c_str(), seenMask.size(), seen2gram.size());
}
#endif

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

  for (int i = 0; i < (int)moves.size(); i++) {
    Direction dir = moves[i];
    bool success = level->Move(dir);

    State state = level->GetState();
    printf("\n=== move %d: %s %s ===\n", (i+1), DIR_NAMES[dir], (success ? "SUCCEEDED" : "FAILED"));
    std::cout << state << std::endl;
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
#ifdef USE_LEVEL2
      if (demoPath == "explore") {
        int rollouts = (argc >= 4) ? atoi(argv[3]) : 3000;
        u32 seed = (argc >= 5) ? (u32)strtoul(argv[4], nullptr, 0) : 0xC0FFEEu;
        std::string safe = test->name;
        for (char& c : safe) if (!std::isalnum((unsigned char)c)) c = '_';
        Explore(test, rollouts, 80, seed, "oracle-demos/" + safe);
        return 0;
      }
#endif
      if (demoPath == "reverify") {
        std::string safe = test->name;
        for (char& c : safe) if (!std::isalnum((unsigned char)c)) c = '_';
        std::string dir = (argc >= 4) ? std::string{ argv[3] } : ("oracle-demos/" + safe);
        Reverify(test, dir);
        return 0;
      }
      if (demoPath == "findpath") {
        // Let the ordinary solver find the shortest path to an alternate win state, written to solved.dem. Build the
        // reference engine (no /DUSE_LEVEL2) so the path is reference-legal. Swap the goal predicate for the scenario
        // being reproduced.
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
