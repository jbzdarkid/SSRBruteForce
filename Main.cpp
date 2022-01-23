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

Level GreatTowerImanex(14, 11, "2-4 Great Tower (after imanex's start) (with no stacked sausages)",
  "       #  #   "
  "     1##1##   "
  "  1111 11 1   "
  "  1_______U___"
  "  1___________"
  "##1___________"
  " # _______cd__"
  " 11_____bbcd__"
  "##1___>__aa___"
  " # ___________"
  " 11L__________");

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

Level ColdJag(12, 5, "3-1 Cold Jag",
  "    222_3__1"
  "    1_bc____"
  "    1_bc____"
  "__##U_____11"
  "__##_>______",
  {},
  {},
  {Sausage{11, 0, 11, 1, 1}});

Level ColdFinger(17, 6, "3-2 Cold Finger",
  "____________     "
  "____________     "
  "3?_c___^____1    "
  "___c______1__    "
  "__________1__##__"
  "         __ _##__",
  {},
  {},
  {Sausage{3, 2, 3, 3, 1}, Sausage{3, 2, 3, 3, 2}},
  {Level::Tile::Over3});

Level ColdEscarpment(14, 16, "3-3 Cold Escarpment",
  "________      "
  "________      "
  "_______D      "
  "__aa__222     "
  "______232_____"
  "______222_____"
  "______ _______"
  "__b___ _______"
  "__b___222L_222"
  "______232  222"
  "______222__222"
  "___^     ___U_"
  "____    1L____"
  "___R1####_____"
  "     ####     "
  "      11      ");

Level ColdTrail(19, 10, "3-4 Cold Trail",
  "11111_______       "
  "11111_______       "
  "11111_______       "
  "11111________R1111 "
  "11111L_______ ## # "
  "11111___>___ _##_##"
  "____________       "
  "____________       "
  "_____________      "
  "______________     ");

Level ColdCliff(8, 8, "3-5 Cold Cliff",
  "2222    "
  "2222    "
  "2222    "
  "2222    "
  "2222__  "
  "_U__####"
  "____####"
  "     111");

Level ColdPit(10, 10, "3-6 Cold Pit",
  "  11111111"
  "  11111111"
  "  11111111"
  "  111__111"
  "1_111__111"
  "1    ##   "
  "1    ##   "
  "11$$1__   "
  "1 $$111   "
  "11111     ");

Level ColdPlateau(10, 10, "3-7 Cold Plateau",
  "1111111111"
  "1111111111"
  "1111111111"
  "1222111111"
  "1232111111"
  "1222111111"
  "1111111111"
  "1111111111"
  "   _$$    "
  "   _$$    ");

Level ColdHead(12, 12, "3-8 Cold Head",
  "        1$$1"
  "        1111"
  "        1211"
  "        1111"
  "        1111"
  "     11____ "
  "     11____ "
  "     __1111 "
  "11111__1111 "
  "1$$11__1111 "
  "1$$11_R1111 "
  "11111       ",
  Stephen{9, 9, 1, Down},
  {},
  {Sausage{7, 8, 7, 9, 1}, Sausage{8, 7, 9, 7, 1}});

Level ColdLadder(13, 8, "3-9 Cold Ladder",
  "__1111111    "
  "__1111111    "
  "__1111111    "
  "____aa___    "
  "____bc___##__"
  "____bc___##__"
  "__________   "
  "__________   ",
  Stephen{4, 2, 1, Right},
  {Ladder{4, 3, 0, Up}});

Level ColdSausage(13, 5, "3-10 Cold Sausage",
  "       111111"
  "###### 111111"
  "######1111111"
  "###### 111111"
  "       111111",
  Stephen{10, 4, 1, Left},
  {},
  {Sausage{8, 1, 9, 1, 1}, Sausage{10, 1, 11, 1, 1},
   Sausage{8, 2, 9, 2, 1},
   Sausage{8, 3, 9, 3, 1}, Sausage{10, 3, 11, 3, 1}});

Level ColdTerrace(10, 10, "3-11 Cold Terrace",
  "22211__222"
  "____U_____"
  "__1_______"
  "__Ucc_1___"
  "__________"
  "___<______"
  "     ##   "
  "     ##   "
  "     __   "
  "     __   ",
  {},
  {Ladder{2, 1, 2, Up}, Ladder{7, 1, 2, Up}},
  {Sausage{0, 0, 1, 0, 2}, Sausage{8, 0, 9, 0, 2}});

Level ColdHorizon(15, 5, "3-12 Cold Horizon",
  "222222______111"
  "2  222______111"
  "2  222______111"
  "2##___ _ _ __#_"
  "2##____ _ _ _##",
  Stephen{3, 2, 2, Up},
  {Ladder{1, 4, 1, Left}, Ladder{1, 4, 0, Left}},
  {Sausage{4, 0, 5, 0, 2}, Sausage{4, 1, 4, 2, 2}});

// Cold Gate doesn't exist. Okay? Okay.

Level ColdFrustration(10, 9, "3-14 Cold Frustration",
  "2  _____  "
  "111L_D__  "
  "1    222  "
  "U_1_#_U___"
  "_a_ #___1_"
  "_a1_#_____"
  "_b__22____"
  "^b_L22____"
  "____22____");

Level WretchsRetreat(11, 8, "4-1 Wretch's Retreat",
  "     1     "
  "     #     "
  "_______111_"
  "____a__121_"
  "____a__111_"
  "__1_____U__"
  "____bb_>___"
  "___________");

Level ToadsFolly(10, 10, "4-2 Toad's Folly",
  "__________"
  "_##_______"
  "_##_____2_"
  "__________"
  "222L______"
  "252L_^_cc_"
  "222_______"
  "__________"
  "________1_"
  " _________",
  {},
  {},
  {Sausage{7, 2, 8, 2, 2}, Sausage{7, 8, 8, 8, 1}});


Level SludgeCoast(10, 11, "4-3 Sludge Coast",
  "11111_____"
  "11111_____"
  "11222?____"
  "11111_____"
  "11111_____"
  "11aU______"
  "__a_______"
  "_____bb___"
  "    ##    "
  "    ##    "
  "    __    ",
  Stephen{3, 0, 1, Right},
  {},
  {},
  {Level::Tile::Over2});

Level SlopeView(18, 8, "5-1 Slope View",
  "        $$     1  "
  "        $$___bb_1 "
  "2222222211__R11_  "
  "2222322211______  "
  "2222222211___cc_##"
  "2222222211L_____##"
  "           ##     "
  "           ##     ",
  Stephen{4, 5, 2, Left},
  {Ladder{4, 3, 2, Down}, Ladder{7, 5, 2, Right}},
  {Sausage{6, 3, 6, 4}});


Level LandsEnd(13, 7, "Land's End",
  "      _1     "
  "_1___ __  3  "
  "____ ___223__"
  "_____ __223__"
  "____ ________"
  "   ##________"
  "   ##________",
  Stephen{10, 2, 3, Left},
  {Ladder{1, 1, 0, Left}, Ladder{7, 0, 0, Left}, Ladder{8, 2, 0, Left}, Ladder{8, 2, 1, Left}, Ladder{10, 3, 2, Left}},
  {Sausage{9, 2, 9, 3, 2}});


int main() {
  // GreatTowerImanex.InteractiveSolver();

  //*
  Level* level = &ColdLadder;
  Vector<Direction> solution = Solver(level).Solve();
  std::string levelName(level->name);
  levelName = levelName.substr(0, levelName.find_first_of(' '));
  std::ofstream file(levelName + ".dem");

  const char* dirs[] = {
    nullptr,
    "North",
    "West",
    nullptr,
    nullptr,
    "East",
    "South",
  };
  for (Direction dir : solution) file << dirs[dir] << '\n';
  file.close();

  for (Direction dir : solution) {
    level->Print();
    printf("%s\n", dirs[dir]);
    getchar();
    level->Move(dir);
  }
  level->Print();
  //*/
}
