#include "Level.h"
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

  Level& level = MaidensWalk;

  level.Print();
  printf("ULDR: ");
  int moves = 0;
  while (!level.Won()) {
    int ch = getchar();
    if (ch == '\n') {
      level.Print();
      printf("ULDR: ");
      continue;
    }
    else if (ch == 'u') level.Move(Up);
    else if (ch == 'd') level.Move(Down);
    else if (ch == 'l') level.Move(Left);
    else if (ch == 'r') level.Move(Right);
    moves++;
  }

  printf("Level completed in %d moves\n", moves);
}