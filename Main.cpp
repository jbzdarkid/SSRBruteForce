#include "Solver2.h"
#include "Levels.h" // Ordered second because it redefines Level

#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <locale>
#include <string>
#include <vector>

const char* DIR_NAMES[] = {"None", "North", "West", "Jump", "Crouch", "East", "South"};

bool TestLevel(Level* level, std::vector<Direction> moves) {
  printf("=== initial state ===\n");
  level->Print();

  for (int i = 0; i < (int)moves.size(); i++) {
    Direction dir = moves[i];
    bool success = level->Move(dir);

    State state = level->GetState();
    printf("\n=== move %d: %s %s ===\n", (i+1), DIR_NAMES[dir], (success ? "SUCCEEDED" : "FAILED"));
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
    level->Print();

    if (level->Won()) break; // Demos have trailing moves
    if (!success) break;
  }

  return level->Won();
}

bool SolveLevel(Level* level) {
  Solver2 solver(level, level->hashtableSize);
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
static void DiffEngines(Level* level) {
  State start = level->GetState();
  std::unordered_set<State> visited;
  std::vector<State> frontier;
  visited.insert(start);
  frontier.push_back(start);
  const size_t kCap = 60'000'000; // hard cap so a huge level can't OOM
  size_t explored = 0, diverged = 0;
  Direction dirs[] = { Up, Down, Left, Right };
  while (!frontier.empty()) {
    State cur = frontier.back();
    frontier.pop_back();
    explored++;
    for (Direction d : dirs) {
      level->SetState(&cur);
      bool retRef = level->BaseLevel::Move(d);
      State stateRef = level->GetState();
      level->SetState(&cur);
      bool ret2 = level->Move(d);
      State state2 = level->GetState();
      bool div = (retRef != ret2) || (retRef && !(stateRef == state2));
      if (div && diverged++ < 5)
        printf("DIVERGENCE #%zu dir=%s retRef=%d ret2=%d\n", diverged, DIR_NAMES[d], retRef, ret2);
      if (retRef && visited.size() < kCap && visited.insert(stateRef).second)
        frontier.push_back(stateRef);
    }
  }
  printf("DiffEngines: explored=%zu diverged=%zu visited=%zu\n", explored, diverged, visited.size());
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

  for (Level* test : tests) {
    if (test->NumSausages() != NUM_SAUSAGES) continue;
    if (!std::strstr(test->name, filter.c_str())) continue; // Failed to match filter

    if (!demoPath.empty()) {
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
      DiffEngines(test);
      return 0;
#endif
      printf("Solving level %s\n", test->name);

      bool success = SolveLevel(test);
      if (!success) printf("Solver could not solve the level.\n");
      return success ? 0 : 3;
    }
  }

  // No level matched the filter for this build's sausage count.
  printf("No level matched filter '%s' for a %d-sausage build.\n", filter.c_str(), NUM_SAUSAGES);
  return 4;
}
