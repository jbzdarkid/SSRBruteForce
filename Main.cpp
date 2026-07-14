#include "Solver2.h"
#include "Levels.h" // Ordered second because it redefines Level

#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <locale>
#include <string>
#include <unordered_set>
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

// Print Stephen's pose and every sausage's footprint/flags for a captured state -- the shared per-state dump used by
// both the demo replay (TestLevel) and the exhaustive engine diff (DiffEngines).
void PrintStateDetail(const State& state) {
  printf("  stephen: body=(%d,%d,%d) dir=%s  fork=(%d,%d,%d) forkDir=%s\n",
    state.stephen.x, state.stephen.y, state.stephen.z, DIR_NAMES[state.stephen.dir],
    state.stephen.forkX, state.stephen.forkY, state.stephen.forkZ,
    DIR_NAMES[state.stephen.forkDir]);
  for (int j = 0; j < NUM_SAUSAGES; ++j) {
    const Sausage& s = state.sausages[j];
    std::string flags;
    if (s.flags & Sausage::Flags::Cook1A) flags += "Cook1A ";
    if (s.flags & Sausage::Flags::Cook1B) flags += "Cook1B ";
    if (s.flags & Sausage::Flags::Cook2A) flags += "Cook2A ";
    if (s.flags & Sausage::Flags::Cook2B) flags += "Cook2B ";
    if (s.flags & Sausage::Flags::Rolled) flags += "Rolled ";
    if (flags.empty()) flags = "none";
    else flags.pop_back(); // drop trailing space

    printf("  sausage[%d]: (%d,%d)-(%d,%d) z=%d flags=0x%02x [%s]\n",
      j, s.x1, s.y1, s.x2, s.y2, s.z, (unsigned)s.flags, flags.c_str());
  }
}

bool TestLevel(Level* level, std::vector<Direction> moves) {
  printf("=== initial state ===\n");
  level->Print();

  for (int i = 0; i < (int)moves.size(); i++) {
    Direction dir = moves[i];
    bool success = level->Move(dir);

    State state = level->GetState();
    printf("\n=== move %d: %s %s ===\n", (i+1), DIR_NAMES[dir], (success ? "SUCCEEDED" : "FAILED"));
    PrintStateDetail(state);
    level->Print();

    if (level->Won()) break; // Demos have trailing moves
    if (!success) break;
  }

  return level->Won();
}

bool SolveLevel(Level* level) {
  Solver2 solver(level);
  std::vector<Direction> solution = solver.Solve();

  if (solution.empty()) return false;

  std::ofstream out("solved.dem");
  for (Direction dir : solution) out << DIR_NAMES[dir] << '\n';
  return true;
}

#ifdef USE_DIFF_ENGINES
// Exhaustively BFS the reference-reachable state space from the level's start, replaying every direction through both
// the reference (BaseLevel::Move) and the Level2 shadow (Move) at each state, and report the first few divergences. A
// state is compared/expanded only when the reference ACCEPTS the move: a rejected move leaves garbage in _stephen
// (Move mutates before its support check), so its post-state is meaningless. Build /DUSE_LEVEL2 /DUSE_DIFF_ENGINES and
// run with a level name but NO demo path.
static void DiffEngines(Level* level, bool summaryOnly = false) {
  if (summaryOnly) printf("== %s ==\n", level->name);
  State start = level->GetState();
  std::unordered_set<State> visited;
  std::vector<State> frontier;
  visited.insert(start);
  frontier.push_back(start);
  const size_t kCap = 60'000'000; // hard cap so a huge level can't OOM
  size_t explored = 0, diverged = 0;
  // Classify divergences so we can tell harmless no-ops (Level2 accepts a move the reference refuses but nothing moves)
  // from real ones (Level2 reaches a state the reference cannot, or the two accepted results disagree).
  size_t noopAccept = 0, newStateAccept = 0, refAcceptsL2Refuses = 0, bothAcceptDiffer = 0;
  Direction dirs[] = { Up, Down, Left, Right };
  while (!frontier.empty()) {
    State cur = frontier.back();
    frontier.pop_back();
    explored++;
    for (Direction d : dirs) {
      level->SetState(&cur);
      bool retRef = level->BaseLevel::Move(d);
      State stateRef = level->GetState();
      // A level may flag genuinely-buggy reference scenarios via its heuristic (returns false for them). When the
      // reference lands in such a state, ignore the whole transition -- don't diff it, don't explore past it -- so the
      // survey can focus on fixable divergences elsewhere (per the "reject genuine bugs to drill in" workflow).
      if (retRef && level->heuristic) {
        level->SetState(&stateRef);
        if (!level->heuristic(level)) continue;
      }
      level->SetState(&cur);
      bool ret2 = level->Move(d);
      State state2 = level->GetState();
      bool div = (retRef != ret2) || (retRef && !(stateRef == state2));
      if (div) {
        if      (!retRef && ret2 && (state2 == cur)) noopAccept++;
        else if (!retRef && ret2)                    newStateAccept++;
        else if (retRef && !ret2)                    refAcceptsL2Refuses++;
        else                                         bothAcceptDiffer++;
        // Dump the first few divergences in full: the pre-move board/pose and each engine's post-move result. A
        // rejected move's post-state is meaningless (the reference mutates before failing; Level2 leaves it untouched),
        // so only print an engine's "after" when it actually accepted.
        ++diverged;
        if (!summaryOnly && diverged <= 3) {
          printf("\n===== DIVERGENCE #%zu  dir=%s  retRef=%d ret2=%d =====\n", diverged, DIR_NAMES[d], retRef, ret2);
          printf("--- before ---\n");
          level->SetState(&cur); level->Print(); PrintStateDetail(cur);
          if (retRef) { printf("--- reference after ---\n"); level->SetState(&stateRef); level->Print(); PrintStateDetail(stateRef); }
          if (ret2)   { printf("--- level2 after ---\n");    level->SetState(&state2);  level->Print(); PrintStateDetail(state2); }
        }
      }
      if (retRef && visited.size() < kCap && visited.insert(stateRef).second)
        frontier.push_back(stateRef);
    }
    // Detail drill-down only prints the first 3 divergences, so stop once we have them instead of grinding to the 60M
    // cap -- makes per-level diagnosis fast. The ALL/count survey (summaryOnly) still explores fully for exact totals.
    if (!summaryOnly && diverged >= 3) break;
  }
  printf("DiffEngines: explored=%zu diverged=%zu visited=%zu\n", explored, diverged, visited.size());
  printf("  breakdown: noopAccept=%zu newStateAccept=%zu refAcceptsL2Refuses=%zu bothAcceptDiffer=%zu\n",
         noopAccept, newStateAccept, refAcceptsL2Refuses, bothAcceptDiffer);
}
#endif

int main(int argc, char* argv[]) {
  setvbuf(stdout, nullptr, _IONBF, 0); // Disable stdout buffering so we see partial output on crash.
  std::cout.imbue(std::locale("en-US")); // Used for cout decimal formatting in some places.

  if (argc == 1) {
    std::cout << "Invalid args\n";
    return 0;
  }

  std::string filter = std::string{ argv[1] };
  std::string demoPath;
  if (argc == 3) demoPath = std::string{ argv[2] };

  bool surveyAll = (filter == "ALL");
  for (Level* test : tests) {
    if (test->NumSausages() != NUM_SAUSAGES) continue;
    if (!surveyAll && !std::strstr(test->name, filter.c_str())) continue; // Failed to match filter

    if (!demoPath.empty()) {
#ifdef USE_DIFF_ENGINES
      if (demoPath == "count") { // full exact divergence count for a single level (no per-divergence dumps)
        DiffEngines(test, true);
        return 0;
      }
#endif
      if (demoPath == "findpath") {
        // Let the ordinary solver find the shortest path to an alternate win state, written to solved.dem. Build the
        // reference engine (no /DUSE_LEVEL2) so the path is reference-legal. Swap the goal predicate for the scenario
        // being reproduced.
        test->winOverride = &IsEscarpmentRotationDrop;
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
#ifdef USE_DIFF_ENGINES
      DiffEngines(test, surveyAll);
      if (!surveyAll) return 0;
      continue;
#endif
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
