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

Level OverworldSausage2(24, 12, "2-final Overworld sausage",
  "    ______   11_________"
  "11111_11________________"
  "   1___1111__a1_________"
  "     __U111__a1_________"
  "     ___111__11_________"
  "     ___111___1_________"
  "11   ___________________"
  "___   ___1___________   "
  "____________111_________"
  "__ _____>__1111____1____"
  " _ ________ __ _________"
  " __________1111_________");

Level ColdJag(12, 5, "3-1 Cold Jag",
  "    222_3__1"
  "    1_ab____"
  "    1_ab____"
  "__##U_____11"
  "__##_>______",
  {},
  {},
  {Sausage{11, 0, 11, 1, 1}});

Level ColdFinger(17, 6, "3-2 Cold Finger",
  "____________     "
  "____________     "
  "3?_a___^____1    "
  "___a______1__    "
  "__________1__##__"
  "         __ _##__",
  {},
  {},
  {Sausage{3, 2, 3, 3, 1}, Sausage{3, 2, 3, 3, 2}},
  {Tile::Over3});

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
  "______________     ",
  {},
  {},
  {Sausage{1, 2, 2, 2, 1}, Sausage{1, 3, 1, 4, 1}, Sausage{2, 3, 2, 4, 1}});

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

Level OverworldSausage3(13, 14, "3-final Overworld sausage",
  "__1_1_1_1    "
  "_________    "
  "__1_1_1_1    "
  "  __a____    "
  "  __a_1_1    "
  "  __1____    "
  "  ____1_1_11 "
  "  ________11 "
  "   R1________"
  "   __________"
  "     ___  ___"
  "    1_     __"
  "    1      ^_"
  "    1      __");

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
  {Tile::Over2});

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


Level LandsEnd(13, 7, "5-5 Land's End",
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

Level FolkloreSetup(8, 9, "6-2.5 Folklore Setup",
  "________"
  "41_1____"
  "41_1_1__"
  "4111112 "
  "41_1_11 "
  "41_1_11 "
  "4111111 "
  "5444451 "
  "        ",
  Stephen{1,1,1,Down},
  {},
  {Sausage{3,1,3,2,1}, Sausage{1,6,2,6,1}, Sausage{4,6,5,6,1}});

Level TheSplittingBough1(16, 6, "6-5 The Splitting Bough (Part 1/2)",
  "_______1________"
  "__aa_________##_"
  "_______111_1_##_"
  "____<__1________"
  "___________     "
  "________        ");

Level SuspensionBridgeSetup(30, 14, "6-10.5 Suspension Bridge Setup",
  "___ _ _     _ _1_____________4"
  "________ _ _ _ 1___11____1___4"
  "1______ _ _ _ _____11________4"
  "________ ___ _ ___aa__55____54"
  "_______ _ ___ __1_bb_________4"
  "___1___________<___________114"
  "_________11_________11_____114"
  "______________     _______    "
  "       ______       ______    "
  "       _________ _________    "
  "       _______1_ _1_______    "
  "       _________ _________    "
  "       __11__ 1   1 ______    "
  "       ______                 ");

Level CuriousDragonsSetup(13, 14, "6-11.5 Curious Dragons Setup",
  "__1___444___1"
  "______444___1"
  "5____5444__44"
  "______444__44"
  "<___aa444__44"
  "____bb444__44"
  "    ________ "
  "    ________ "
  "    ____     "
  "    ____     "
  "    _1__     "
  "    _1__     "
  "    ____     "
  "    ____     ");
  
Level CuriousDragons2(11, 9, "6-11 Curious Dragons (Part 2/2)",
  "    _______"
  "    11___1_"
  "_aBb_____U_"
  "_acC_______"
  "_1  _______"
  "<U  _______"
  "    __##___"
  "    __##___"
  "    _______");
  

// ABC: Lachrymose Head
// DE: Southjaunt
// FG: Infant's Break
// HI: Comely Hearth
// JK: Little Fire
// LM: Eastreach
// N: Bay's Neck
// OP: Burning Wharf
// Q: Happy Pool
// R: Maiden's Walk
// ST: Fiery Jut
// TU: Merchant's Elegy
// VW: Seafinger
// YZa: The Clover
// bc: Inlet Shore
// cef: The Anchorage
// g: Overworld Sausage 1
Level Overworld1(29, 27, "Overworld1",
  "   _______     _____         "
  "   _ _   ___________         "
  " ____fde __1_gg______        "
  " ____fde_^____________       "
  "          __________         "
  "____ZZ_______________        "
  "    __   ________ _ _>_PP__  "
  "    __ __a_______N__<_O__ _  "
  "   >_____a__c_   N__  O__ _  "
  "    ______^_c_ _QQ______L___ "
  "    ___   _b____   _JJ_KLMM__"
  "___XYY_   _b___  _ ____K_<_  "
  "___X_______R___ _  __>_____  "
  "___  __UV__R__v   __ ___ __  "
  "   W^^_UV_ _^________<_      "
  "   W_______________HH_II     "
  "      ___  ____________      "
  "      ___ST__1________       "
  "      ___STv_____1____       "
  "      ___  ________FFG       "
  "            ________<G_      "
  "            _______ >___     "
  "             __^____________ "
  "              _____D__EAAB__ "
  "                ___D__E__BCC "
  "                _______^____ "
  "                ____         ",
  Stephen{15, 22, 0, Up});

// AB: Beautiful Horizon
// CD: The Paddock
// EF: The Great Tower
// GH: Barrow Set
// I: Sad Farm
// JK: Cove
// L: Emerson Jetty
// M: Fallow Earth
// NO: Rough Field
// PQ: Twisty Farm
// R: Overworld Sausage 2
Level Overworld2(48, 44, "Overworld2",
  "                               _____            "
  "                              1__1__            "
  "                             AB_____            "
  "                            _AB__>__            "
  "                              111____111        "
  "                              1___CC___1        "
  "                              1___DD___1        "
  "                              111__v_111        "
  "                               ________         "
  "                     1111 11 1 ________         "
  "                     1______________            "
  "                     1______________            "
  "                     1______________            "
  "                      ______________            "
  "                     1______________            "
  "                     1______________            "
  "                      ______EF______            "
  "                     1______EF______            "
  "           ______   11_______>______            "
  "          11_11_____________________            "
  "          1___1111__R1______________            "
  "            ___111>_R1______________            "
  "            ___111__11______________            "
  "            ___111___1______________            "
  "       11   ________________________            "
  "      1___   ___1___________   11111            "
  "     1_____________111_GG______1___1            "
  "    1____ _____>__1111_HH_1____1___1            "
  "1_____  _ ________ __ _________1_1_1            "
  "1____   __________1111_______I___1_1            "
  "1__      ________   __1___v__I___1_1111         "
  " 1__     __>_____   ___________1_______         "
  "  1__    ____1_111  ___________1_______         "
  "   1______NO_____1     __JK_^__1111  ___  _     "
  "  >__P  __NO_____1      _JK_____1__________1 1  "
  "  ___P 1__M 11__        ___________1__     __   "
  "  _QQ_  __M__1__            ______11_1     1_   "
  "     _______>___            _LL_______      _   "
  "           ___              __________      _   "
  "                                _____       _   "
  "                            11111_____>_________"
  "                               _1___     _   _1_"
  "                               _>___     _1  ___"
  "                                ___      _      ",
  Stephen{32,42,0,Right});

// A: Cold Trail
// B: Cold Cliff
// CE: Cold Plateau
// GF: Cold Jag
// J: Cold Finger
// KM: Cold Pit
// N: Cold Head
Level Overworld3(49, 30, "Overworld3",
  "                 1111111111                      "
  "            __   11111111112222                  "
  "            __   11111111112222                  "
  "          N111   12221EE1C12BB2                  "
  "          N111   12321111C12222                  "
  "         R11v1   122211111122>2__                "
  "          1111   1111111111_U______              "
  "          1111111111>111111__D_____              "
  "          1111111   _1111111111 111              "
  "          1111111   _111111111111111_______      "
  "              ___      111111MM11111_______      "
  "              _______  1K1__11112211_______      "
  "              _______1_1Kv__1111AA11_________    "
  "              ____             12211L_______     "
  "   __   __________             11111___>___ _    "
  "   __5_5_5_5                   ____________      "
  "   _________                   ____________      "
  "   __5_5_5_5                   _____________     "
  "     __z____                   ______________    "
  "     __z_5_5                    222_1__1  ___    "
  "     __1____  2  _____          1_GF____ _____   "
  "     ____5_5_1111L_D__          1_GF__________   "
  "     ________11    222      __________11____1111<"
  "      R1______U_1___U___    _____>__________11111"
  "      _________I_ ____1_____________     ___U_   "
  "        ___  __I1___________________      ___    "
  "       1_     _H__22____1__J___^____1            "
  "       1      ^H_R22_______J______1__            "
  "       1      ____22______________1______        "
  "       1               ________  __ _____        "
  );

// A: Gator Paddock
// BC: Crunchy Leaves
// FG: Sludge Coast
// HI: Wretch's Retreat
// J: Toad's Folly
Level Overworld4(38, 32, "Overworld4",
  "                   _______111_        "
  "                   ____I__121_        "
  "                   ____I__111_        "
  "                   __1________        "
  "                   ____HH_>___        "
  "                   ___________        "
  "                 111__________        "
  "                 121__________        "
  "____11111211 222 111________3_        "
  "____1______1 232______________        "
  "_AA________1 222__  222________       "
  "____1______1____  _1252__^_JJ___      "
  "____11111111_BB_  _1222__________     "
  "_______________v_C________________    "
  "______12_____1_ _C__________1____R1111"
  "_____v12__________   _____________    "
  "_______1____12221111111111_______     "
  "____________12321111111111______      "
  "4455__1_____12221111111111_____       "
  "3334__2_____11111111111111_____       "
  "3333________11111111111111_____       "
  "3333_____________1_____F>______       "
  "___U__2________________F__GG___       "
  "2222__1__________11________           "
  "___2__1______________    __           "
  "       _____             __           "
  "       ____              __           "
  "       ____                           "
  "       ____                           "
  "       ____                           "
  "       R1__1111_____                  "
  "       ____                           ",
  Stephen{37,14,1,Left},
  {},
  {Sausage{2,19,2,20,4}});

int main() {
  Level Test(6, 6, "Test",
    "______"
    "__a___"
    "__a___"
    "___^__"
    "______"
    "______", {}, {}, {Sausage{2, 2, 3, 2, 1}});

  Level* level = &CuriousDragons2;
#if _DEBUG
  level->InteractiveSolver();
#endif

  const char* DIRS[] = {
    nullptr,
    "North",
    "West",
    nullptr,
    nullptr,
    "East",
    "South",
  };
  for (Direction dir : {
    Right, Up, Up, Up, Right,
    Left, Left, Left, Down, Up,
    Up, Left, Right, Up, Down,
    Up, Right, Left, Left, Left,
    Up, Down, Down, Left, Right,
    Up, Down, Down, Left, Right,
    Right, Down, // Down the ladder

    // More dubious stuff here
    Left, Up, Left, Left, Right
    })
  {
    level->Print();
    printf("%s\n", DIRS[dir]);
    level->Move(dir);
  }
  Vector<Direction> solution = Solver(level).Solve();
  std::string levelName(level->name);
  levelName = levelName.substr(0, levelName.find_first_of(' '));
  std::ofstream file(levelName + ".dem");

  for (Direction dir : solution) file << DIRS[dir] << '\n';
  file.close();

  for (Direction dir : solution) {
    level->Print();
    printf("%s\n", DIRS[dir]);
    getchar();
    level->Move(dir);
  }
  level->Print();
  //*/
}
