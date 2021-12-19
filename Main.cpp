#include "Level.h"
#include "Solver.h"
#include <cstdio>

Level LachrymoseHead(5, 4,
  "_###_"
  "aab__"
  "__bcc"
  "^###_");

Level Southjaunt(4, 5,
  " __ "
  "____"
  "a##b"
  "a##b"
  "____");

Level InfantsBreak(7, 4,
  "  ___  "
  "  aab  "
  "##_<b# "
  "##   ##");

Level ComelyHearth(5, 5,
  "  ___"
  "__<_ "
  "aa#bB"
  "_##_ "
  "___  ");

Level LittleFire(4, 4,
  "____"
  "aa#b"
  "_#_b"
  "_>__");

Level Eastreach(5, 5,
  "a__# "
  "abb##"
  "_<_  "
  "_##  "
  " ##  ");

Level BaysNeck(4, 4,
  "__#_"
  " _ _"
  "a#_<"
  "a__ ");

Level BurningWharf(6, 3,
  ">_aa__"
  "_b## _"
  " B## _");

Level HappyPool(6, 6,
  " _aa__"
  "__   _"
  "_  # _"
  "_ #  _"
  "v   __"
  "_____ ");

Level MaidensWalk(4, 4,
  "#a_#"
  "#a__"
  " _^_"
  "__  ");

Level FieryJut(6, 4,
  "###  _"
  "###ab_"
  "###abv"
  "###  _");

Level MerchantsElegy(5, 4,
  "_____"
  "_#ab#"
  "^#ab#"
  "_____");

Level Seafinger(5, 5,
  "_##A "
  "_##a_"
  "_##  "
  "   b^"
  "   b_");

Level TheClover(7, 7,
  " aa____"
  " ##   _"
  " ## ##b"
  ">___##b"
  " ##____"
  " ##_   "
  " cc_   ");

Level InletShore(4, 4,
  "__a_"
  "^_a#"
  "#b__"
  "_b__");

Level TheAnchorage(10, 6,
  "   _______"
  "   _ _   _"
  " ####ABC _"
  " ####abc_^"
  "          "
  "__##      ");

int main() {
#define o(x) + 1
  u32 sausages = 0 SAUSAGES;
#undef o
  Vector<Level*> levels;
  if (sausages == 1) {
    levels.Push(&BaysNeck);
    levels.Push(&HappyPool);
    levels.Push(&MaidensWalk);
  } else if (sausages == 2) {
    levels.Push(&Southjaunt);
    levels.Push(&InfantsBreak);
    levels.Push(&ComelyHearth);
    levels.Push(&LittleFire);
    levels.Push(&Eastreach);
    levels.Push(&BurningWharf);
    levels.Push(&FieryJut);
    levels.Push(&MerchantsElegy);
    levels.Push(&Seafinger);
    levels.Push(&InletShore);
  } else if (sausages == 3) {
    // levels.Push(&LachrymoseHead);
    // levels.Push(&TheClover);
    levels.Push(&TheAnchorage);
  }

  /*
  for (Level* level : levels) {
    level->Print();
    Vector<Direction> solution = Solver(level).Solve(100);
    printf("Best solver solution: %d\n", solution.Size());
    const char* dirs = " UD L   R";
    for (Direction dir : solution) {
      putchar(dirs[dir]);
      level->Move(dir);
      level->Print();
      getchar();
    }
    putchar('\n');
  }
  return 0;
  */
  Level* level = &TheAnchorage;
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
    else if (ch == 'u' || ch == 'U') level->Move(Up);
    else if (ch == 'd' || ch == 'D') level->Move(Down);
    else if (ch == 'l' || ch == 'L') level->Move(Left);
    else if (ch == 'r' || ch == 'R') level->Move(Right);
    moves++;
  }

  printf("Level completed in %d moves\n", moves);
}