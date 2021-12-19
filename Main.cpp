#include "Level.h"
#include "Solver.h"
#include <cstdio>

Level LachrymoseHead(5, 4, "Lachrymose Head",
  "_###_"
  "aab__"
  "__bcc"
  "^###_");

Level Southjaunt(4, 5, "Southjaunt",
  " >_ "
  "____"
  "a##b"
  "a##b"
  "____");

Level InfantsBreak(7, 4, "Infant's Break",
  "  ___  "
  "  aab  "
  "##_<b# "
  "##   ##");

Level ComelyHearth(5, 5, "Comely Hearth",
  "  ___"
  "__<_ "
  "aa#bB"
  "_##_ "
  "___  ");

Level LittleFire(4, 4, "Little Fire",
  "____"
  "aa#b"
  "_#_b"
  "_>__");

Level Eastreach(5, 5, "Eastreach",
  "a__# "
  "abb##"
  "_<_  "
  "_##  "
  " ##  ");

Level BaysNeck(4, 4, "Bay's Neck",
  "__#_"
  " _ _"
  "a#_<"
  "a__ ");

Level BurningWharf(6, 3, "Burning Wharf",
  ">_aa__"
  "_b## _"
  " B## _");

Level HappyPool(6, 6, "Happy Pool",
  " _aa__"
  "__   _"
  "_  # _"
  "_ #  _"
  "v   __"
  "_____ ");

Level MaidensWalk(4, 4, "Maiden's Walk",
  "#a_#"
  "#a__"
  " _^_"
  "__  ");

Level FieryJut(6, 4, "Fiery Jut",
  "###  _"
  "###ab_"
  "###abv"
  "###  _");

Level MerchantsElegy(5, 4, "Merchant's Elegy",
  "_____"
  "_#ab#"
  "^#ab#"
  "_____");

Level Seafinger(5, 5, "Seafinger",
  "_##A "
  "_##a_"
  "_##  "
  "   b^"
  "   b_");

Level TheClover(7, 7, "The Clover",
  " aa____"
  " ##   _"
  " ## ##b"
  ">___##b"
  " ##____"
  " ##_   "
  " cc_   ");

Level InletShore(4, 4, "Inlet Shore",
  "__a_"
  "^_a#"
  "#b__"
  "_b__");

Level TheAnchorage(10, 6, "The Anchorage",
  "   _______"
  "   _ _   _"
  " ####ABC _"
  " ####abc_^"
  "          "
  "__##      ");

Level EmersonJetty(19, 17, "Emerson Jetty",
  "         ___  _    "
  "____1__________1 1 "
  "_______1__     __  "
  "______11_1     1_  "
  "_aa__  ___      _  "
  "_____  ___      _  "
  "        _       _  "
  "        __>________"
  "            _   _1_"
  "            _1  ___"
  "            _      "
  "            _      "
  "           1__     "
  "             _     "
  "           ____    "
  "          ##_1_    "
  "          ##___    ");

Level SadFarm(11, 9, "Sad Farm",
  "   11111   "
  "___1___1   "
  "___1___1   "
  "___1_1_1   "
  "_a___1_1   "
  "_a___1_1111"
  "___1_____##"
  "___1_____##"
  "^__1111    ");

Level Cove(8, 6, "Cove",
  "__1__#v_"
  "_____#__"
  "________"
  "   __ab_"
  "    _ab_"
  "    ____");

Level ThePaddock(10, 6, "The Paddock",
  " 11____111"
  "1_#_aa_# 1"
  "1_#_bb_# 1"
  "111__v_111"
  " ________ "
  " ________ ");

Level BeautifulHorizon(8, 5, "Beautiful Horizon",
  "   ____#"
  "  1__1_#"
  " ab____#"
  "1ab__>_#"
  "  1     ");

Level BarrowSet(18, 6, "Barrow Set",
  "   ___            "
  "   ___1___________"
  "_________111_aa___"
  "_##__>__1111_bb_1_"
  "_##_____ __ ______"
  "________1111______");

Level RoughField(9, 5, "Rough Field",
  "________ "
  "__>_____ "
  "____1_111"
  "_ab____#1"
  "_ab____#1");

Level FallowEarth(7, 4, "Fallow Earth",
  "_a#11__"
  "_A#_1__"
  "___>___"
  "  ___  ");

Level TwistyFarm(10, 14, "Twisty Farm",
  "       11 "
  "      1_##"
  "     1__##"
  "    1____ "
  "1_____  # "
  "1____   ##"
  "1__       "
  " 1__      "
  "  1__     "
  "   1_____ "
  "  >__a  _ "
  "  ___a 1_ "
  "  _bb_  _ "
  "     ____ ");

int main() {
#define o(x) + 1
  u32 sausages = SAUSAGES;
#undef o
  Vector<Level*> levels;
  if (sausages == 1) {
    //levels.Push(&BaysNeck);
    //levels.Push(&HappyPool);
    //levels.Push(&MaidensWalk);
    levels.Push(&EmersonJetty);
    levels.Push(&SadFarm);
    levels.Push(&FallowEarth);
  } else if (sausages == 2) {
    //levels.Push(&Southjaunt);
    //levels.Push(&InfantsBreak);
    //levels.Push(&ComelyHearth);
    //levels.Push(&LittleFire);
    //levels.Push(&Eastreach);
    //levels.Push(&BurningWharf);
    //levels.Push(&FieryJut);
    //levels.Push(&MerchantsElegy);
    //levels.Push(&Seafinger);
    //levels.Push(&InletShore);
    levels.Push(&Cove);
    levels.Push(&ThePaddock);
    levels.Push(&BeautifulHorizon);
    levels.Push(&BarrowSet);
    levels.Push(&RoughField);
    levels.Push(&TwistyFarm);
  } else if (sausages == 3) {
    // levels.Push(&LachrymoseHead);
    // levels.Push(&TheClover);
    levels.Push(&TheAnchorage);
  }

  bool stepThrough = true;
  /*
  for (Level* level : levels) {
    printf("Solving %s\n", level->name);
    Vector<Direction> solution = Solver(level).Solve(100);
    printf("Best solver solution: %d\n", solution.Size());
    const char* dirs = " UD L   R";
    for (Direction dir : solution) {
      if (stepThrough) level->Print();
      putchar(dirs[dir]);
      if (stepThrough) getchar();
      if (stepThrough) level->Move(dir);
    }
    if (stepThrough) level->Print();
    putchar('\n');
  }
  return 0;
  //*/

  Level test(5, 4, "Test",
    "1a##_"
    "_a##_"
    "^____"
    "_____");
  test.InteractiveSolver();
}