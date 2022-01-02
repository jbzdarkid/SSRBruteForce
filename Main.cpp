#include "Level.h"
#include "Solver.h"
#include <cstdio>
#include <string>
#include <fstream>

Level LachrymoseHead(5, 4, "1-1 Lachrymose Head",
  "_###_"
  "aab__"
  "__bcc"
  "^###_");

Level Southjaunt(4, 5, "1-2 Southjaunt",
  " >_ "
  "____"
  "a##b"
  "a##b"
  "____");

Level InfantsBreak(7, 4, "1-3 Infant's Break",
  "  ___  "
  "  aab  "
  "##_<b# "
  "##   ##");

Level ComelyHearth(5, 5, "1-4 Comely Hearth",
  "  ___"
  "__<_ "
  "aa#bB"
  "_##_ "
  "___  ");

Level LittleFire(4, 4, "1-5 Little Fire",
  "____"
  "aa#b"
  "_#_b"
  "_>__");

Level Eastreach(5, 5, "1-6 Eastreach",
  "a__# "
  "abb##"
  "_<_  "
  "_##  "
  " ##  ");

Level BaysNeck(4, 4, "1-7 Bay's Neck",
  "__#_"
  " _ _"
  "a#_<"
  "a__ ");

Level BurningWharf(6, 3, "1-8 Burning Wharf",
  ">_aa__"
  "_b## _"
  " B## _");

Level HappyPool(6, 6, "1-9 Happy Pool",
  " _aa__"
  "__   _"
  "_  # _"
  "_ #  _"
  "v   __"
  "_____ ");

Level MaidensWalk(4, 4, "1-10 Maiden's Walk",
  "#a_#"
  "#a__"
  " _^_"
  "__  ");

Level FieryJut(6, 4, "1-11 Fiery Jut",
  "###  _"
  "###ab_"
  "###abv"
  "###  _");

Level MerchantsElegy(5, 4, "1-12 Merchant's Elegy",
  "_____"
  "_#ab#"
  "^#ab#"
  "_____");

Level Seafinger(5, 5, "1-13 Seafinger",
  "_##A "
  "_##a_"
  "_##  "
  "   b^"
  "   b_");

Level TheClover(7, 7, "1-14 The Clover",
  " aa____"
  " ##   _"
  " ## ##b"
  ">___##b"
  " ##____"
  " ##_   "
  " cc_   ");

Level InletShore(4, 4, "1-15 Inlet Shore",
  "__a_"
  "^_a#"
  "#b__"
  "_b__");

Level TheAnchorage(10, 6, "1-16 The Anchorage",
  "   _______"
  "   _ _   _"
  " ####ABC _"
  " ####abc_^"
  "          "
  "__##      ");

Level EmersonJetty(19, 17, "2-1 Emerson Jetty",
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

Level SadFarm(11, 9, "2-2 Sad Farm",
  "   11111   "
  "___1___1   "
  "___1___1   "
  "___1_1_1   "
  "_a___1_1   "
  "_a___1_1111"
  "___1_____##"
  "___1_____##"
  "^__1111    ");

Level Cove(8, 6, "2-3 Cove",
  "__1__#v_"
  "_____#__"
  "________"
  "   __ab_"
  "    _ab_"
  "    ____");

Level ThePaddock(10, 6, "2-5 The Paddock",
  " 11____111"
  "1_#_aa_# 1"
  "1_#_bb_# 1"
  "111__v_111"
  " ________ "
  " ________ ");

Level BeautifulHorizon(8, 5, "2-6 Beautiful Horizon",
  "   ____#"
  "  1__1_#"
  " ab____#"
  "1ab__>_#"
  "  1     ");

Level BarrowSet(18, 6, "2-7 Barrow Set",
  "   ___            "
  "   ___1___________"
  "_________111_aa___"
  "_##__>__1111_bb_1_"
  "_##_____ __ ______"
  "________1111______");

Level RoughField(9, 5, "2-8 Rough Field",
  "________ "
  "__>_____ "
  "____1_111"
  "_ab____#1"
  "_ab____#1");

Level FallowEarth(7, 4, "2-9 Fallow Earth",
  "_a#11__"
  "_A#_1__"
  "___>___"
  "  ___  ");

Level TwistyFarm(10, 14, "2-10 Twisty Farm",
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

Level ColdLadder(13, 8, "3-10 Cold Ladder",
  "__1111111    "
  "__1111111    "
  "__1111111    "
  "____aa___    "
  "____bc___##__"
  "____bc___##__"
  "__________   "
  "__________   ",
  Stephen{4, 2, 1, Right});

Level ColdLadder2(13, 8, "3-10 Cold Ladder part 2",
  "__1111111    "
  "__1111111    "
  "aa1111111    "
  "__b______    "
  "__b___c__##__"
  "______c__##__"
  "__________   "
  "__________   ",
  Stephen{2, 2, 1, Right});

Level ColdFinger(17, 6, "3-? Cold Finger",
  "___a________     "
  "___ac_______     "
  "1__bc_______1    "
  "__^b______1__    "
  "__________1__##__"
  "         __ _##__");

Level NotReallySludgeCoast(9, 8, "NotReallySludgeCoast",
  "11111^___"
  "11111aa__"
  "11___bb__"
  "_________"
  "_________"
  "    ##   "
  "    ##   "
  "    __   ");

Level LandsEnd(13, 7, "Land's End",
  "      _1     "
  "_1___ __  3  "
  "____ ___223__"
  "_____ __223__"
  "____ ________"
  "   ##________"
  "   ##________",
  Stephen{10, 2, 3, Left},
  {Sausage{9, 2, 9, 3, 2}},
  {Ladder{1, 1, 0, Left}, Ladder{7, 0, 0, Left}, Ladder{8, 2, 0, Left}, Ladder{8, 2, 1, Left}, Ladder{10, 3, 2, Left}});

int main() {
#define o(x) + 1
  u32 sausages = SAUSAGES;
#undef o
  Vector<Level*> levels;
  /*
  if (sausages == 1) {
    levels.Push(&BaysNeck);
    levels.Push(&HappyPool);
    levels.Push(&MaidensWalk);
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
    // levels.Push(&TheAnchorage);
    levels.Push(&ColdLadder2);
  }
  */
  /*
  for (Level* level : levels) {
    std::string levelName(level->name);
    levelName = levelName.substr(0, levelName.find_first_of(' '));
    std::ofstream file(levelName + ".dem");

    Vector<Direction> solution = Solver(level).Solve();
    const char* dirs[] = {
      "",
      "North",
      "South",
      "",
      "West",
      "",
      "",
      "",
      "East",
    };
    for (Direction dir : solution) file << dirs[dir] << '\n';
    file.close();
  }
  return 0;
  //*/

  Level Unsolvable(2, 3, "",
    "##"
    "aa"
    ">_");

  Level OnlySolvable(4, 5, "",
    "1##1"
    "1##1"
    "1aa1"
    "1__1"
    "1>_1");

  Level CanLose(2, 5, "",
    "##"
    "##"
    "aa"
    "__"
    ">_");

  LandsEnd.InteractiveSolver();

  //*
  for (Level* level : {&Cove}) {
    Vector<Direction> solution = Solver(level).Solve();
    const char* dirs = " UD L   R";
    for (Direction dir : solution) {
      level->Print();
      putchar(dirs[dir]);
      getchar();
      level->Move(dir);
    }
    level->Print();
  }
  return 0;
  //*/
}