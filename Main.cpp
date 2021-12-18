#include "Level.h"
#include "Solver.h"
#include <cstdio>

int main() {
  Level MaidensWalk(4, 4,
  "#0_#"
  "#0__"
  " _^_"
  "__  ");

  Level Seafinger(5, 5,
  "_##0 "
  "_##0_"
  "_##  "
  "   1^"
  "   1_");

  Level Test(3, 4,
  "##_"
  "##_"
  "_00"
  "_>_");

  Level* level = &Test;

  Vector<Direction> solution = Solver(level).Solve(10);
  printf("Best solver solution: %d\n", solution.Size());

  level->Print();
  printf("ULDR: ");
  int moves = 0;
  while (!level->Won()) {
    int ch = getchar();
    if (ch == '\n') {
      level->Print();
      printf("ULDR: ");
      continue;
    }
    else if (ch == 'u') level->Move(Up);
    else if (ch == 'd') level->Move(Down);
    else if (ch == 'l') level->Move(Left);
    else if (ch == 'r') level->Move(Right);
    moves++;
  }

  printf("Level completed in %d moves\n", moves);
}