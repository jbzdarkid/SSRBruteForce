#include "CppUnitTest.h"
#include "Level.h"
#include "Level2.h"
#include <string>
#include <utility>

#include "TestSymmetryHelper.h"

static_assert(NUM_SAUSAGES == 3); // Assumed for this and all other tests

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
namespace Microsoft::VisualStudio::CppUnitTestFramework {
  template<> inline std::wstring ToString<Direction>(const Direction& dir) {
    switch (dir) {
      case None:   return L"None";
      case Up:     return L"Up";
      case Left:   return L"Left";
      case Jump:   return L"Jump";
      case Crouch: return L"Crouch";
      case Right:  return L"Right";
      case Down:   return L"Down";
    }
    assert(false);
    return L"unknown";
  }
}

Direction Rotate90(Direction dir) {
  switch (dir) {
  case Left:  return Up;
  case Up:    return Right;
  case Right: return Down;
  case Down:  return Left;
  default: return dir;
  }
}

Direction Flip(Direction dir) {
  switch (dir) {
  case Left:  return Right;
  case Up:    return Up;
  case Right: return Left;
  case Down:  return Down;
  default: return dir;
  }
}

#define MAKE_SYMMETRICAL_TEST(TestName) \
  TEST_METHOD(TestName##_Rotate0)   { TestName([&](Direction dir) { return dir; }); } \
  TEST_METHOD(TestName##_Rotate90)  { TestName([&](Direction dir) { return Rotate90(dir); }); } \
  TEST_METHOD(TestName##_Rotate180) { TestName([&](Direction dir) { return Rotate90(Rotate90(dir)); }); } \
  TEST_METHOD(TestName##_Rotate270) { TestName([&](Direction dir) { return Rotate90(Rotate90(Rotate90(dir))); }); } \
  TEST_METHOD(TestName##_Flip_Rotate0)   { TestName([&](Direction dir) { return Flip(dir); }); } \
  TEST_METHOD(TestName##_Flip_Rotate90)  { TestName([&](Direction dir) { return Rotate90(Flip(dir)); }); } \
  TEST_METHOD(TestName##_Flip_Rotate180) { TestName([&](Direction dir) { return Rotate90(Rotate90(Flip(dir))); }); } \
  TEST_METHOD(TestName##_Flip_Rotate270) { TestName([&](Direction dir) { return Rotate90(Rotate90(Rotate90(Flip(dir)))); }); } \
  void TestName(std::function<Direction(Direction)> symmetry)

TEST_CLASS(OneOffTests) {
  MAKE_SYMMETRICAL_TEST(SpearSausageDuringBurnedStep) {
    TestSymmetryHelper level(symmetry, LevelType(10, 7, "arena",
      "          "
      " _____   A"
      " _____   A"
      " >#cc1   B"
      " _____   B"
      " _____    "
      "          "));
    level.AssertPosition(1, 3, Right);
    level.AssertSausage({3, 3, 4, 3, 0, Sausage::None});
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(1, 3, Right);
    level.AssertSausage({2, 3, 3, 3, 0, Sausage::Cook1A});
  }

  MAKE_SYMMETRICAL_TEST(SpearedSausageBouncesOffGrill) {
    TestSymmetryHelper level(symmetry, LevelType(10, 7, "arena",
      "          "
      " _____   A"
      " _____   A"
      " >_cc1   B"
      " _###_   B"
      " _____    "
      "          "));
    level.AssertPosition(1, 3, Right);
    level.AssertSausage({3, 3, 4, 3, 0, Sausage::None});
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(2, 3, Right);
    level.AssertSausage({3, 3, 4, 3, 0, Sausage::None});
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(2, 3, Right);
    level.AssertSausage({3, 3, 4, 3, 0, Sausage::Cook1A | Sausage::Cook2A});
  }

  // 2c) Staircase roll where the top sausage is carried onto a Wall1 top, then a further push rolls the bottom on
  // while the top STAYS floating on the wall (it now has non-moving support, so it isn't carried).
  MAKE_SYMMETRICAL_TEST(StackedStaircaseRollOntoWall) {
    TestSymmetryHelper level(symmetry, LevelType(10, 7, "arena",
      "          "
      " _______  "
      " ___1___  "
      " >______  "
      " _______  "
      " _______  "
      "          ",
      {}, {}, { Sausage{3, 3, 3, 4, 0}, Sausage{3, 2, 3, 3, 1}, Sausage{9, 1, 9, 2, 0} }));
    level.AssertPosition(1, 3, Right);
    level.AssertSausage({3, 3, 3, 4, 0, Sausage::None});
    level.AssertSausage({3, 2, 3, 3, 1, Sausage::None});
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(2, 3, Right);
    level.AssertSausage({4, 3, 4, 4, 0, Sausage::Rolled});
    level.AssertSausage({4, 2, 4, 3, 1, Sausage::Rolled});
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(3, 3, Right);
    level.AssertSausage({5, 3, 5, 4, 0, Sausage::None});
    level.AssertSausage({4, 2, 4, 3, 1, Sausage::Rolled});
  }

  // 2d) Staircase roll where the top sausage's carry is blocked by a Wall2: it's left behind, then drops to the ground
  // once the bottom rolls out from under it (the fork is clear of it, so nothing catches it).
  MAKE_SYMMETRICAL_TEST(StackedStaircaseRollDrop) {
    TestSymmetryHelper level(symmetry, LevelType(10, 7, "arena",
      "          "
      " _______  "
      " _______  "
      " >______  "
      " _______  "
      " ___2___  "
      "          ",
      {}, {}, { Sausage{3, 3, 3, 4, 0}, Sausage{3, 4, 3, 5, 1}, Sausage{9, 1, 9, 2, 0} }));
    level.AssertPosition(1, 3, Right);
    level.AssertSausage({3, 3, 3, 4, 0, Sausage::None});
    level.AssertSausage({3, 4, 3, 5, 1, Sausage::None});
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(2, 3, Right);
    level.AssertSausage({4, 3, 4, 4, 0, Sausage::Rolled});
    level.AssertSausage({3, 4, 3, 5, 0, Sausage::None});
  }

  // 3a) Perpendicular stack (horizontal bottom, vertical top overhanging south). Pushing the bottom east slides it;
  // the top is carried east one cell and rolls (it moves across its own axis). No double-move.
  MAKE_SYMMETRICAL_TEST(StackedPerpendicularPush) {
    TestSymmetryHelper level(symmetry, LevelType(10, 7, "arena",
      "          "
      " _______  "
      " _______  "
      " >______  "
      " _______  "
      " _______  "
      "          ",
      {}, {}, { Sausage{3, 3, 4, 3, 0}, Sausage{4, 3, 4, 4, 1}, Sausage{9, 1, 9, 2, 0} }));
    level.AssertPosition(1, 3, Right);
    level.AssertSausage({3, 3, 4, 3, 0, Sausage::None});
    level.AssertSausage({4, 3, 4, 4, 1, Sausage::None});
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(2, 3, Right);
    level.AssertSausage({4, 3, 5, 3, 0, Sausage::None});
    level.AssertSausage({5, 3, 5, 4, 1, Sausage::Rolled});
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(3, 3, Right);
    level.AssertSausage({5, 3, 6, 3, 0, Sausage::None});
    level.AssertSausage({6, 3, 6, 4, 1, Sausage::None});
  }

  // 3d) Perpendicular stack, Stephen behind the overhang. Pushing the bottom north rolls it; the vertical top, aligned
  // with the motion and supported by a perpendicular base, DOUBLE-MOVES north by two cells (sliding).
  MAKE_SYMMETRICAL_TEST(StackedPerpendicularDoubleMove) {
    TestSymmetryHelper level(symmetry, LevelType(10, 7, "arena",
      "          "
      " _______  "
      " _______  "
      " _______  "
      " _______  "
      " __^____  "
      "          ",
      {}, {}, { Sausage{3, 3, 4, 3, 0}, Sausage{4, 3, 4, 4, 1}, Sausage{9, 1, 9, 2, 0} }));
    level.AssertPosition(3, 5, Up);
    level.AssertSausage({3, 3, 4, 3, 0, Sausage::None});
    level.AssertSausage({4, 3, 4, 4, 1, Sausage::None});
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(3, 4, Up);
    level.AssertSausage({3, 2, 4, 2, 0, Sausage::Rolled});
    level.AssertSausage({4, 1, 4, 2, 1, Sausage::None});
  }

  // 3b) Perpendicular stack, Stephen steps UNDER the south overhang as he pushes. His body holding one end cancels the
  // double-move, so the top is carried just one cell (not two).
  MAKE_SYMMETRICAL_TEST(StackedPerpendicularHeld) {
    TestSymmetryHelper level(symmetry, LevelType(10, 7, "arena",
      "          "
      " _______  "
      " _______  "
      " _______  "
      " _______  "
      " ___^___  "
      "          ",
      {}, {}, { Sausage{3, 3, 4, 3, 0}, Sausage{4, 3, 4, 4, 1}, Sausage{9, 1, 9, 2, 0} }));
    level.AssertPosition(4, 5, Up);
    level.AssertSausage({3, 3, 4, 3, 0, Sausage::None});
    level.AssertSausage({4, 3, 4, 4, 1, Sausage::None});
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(4, 4, Up);
    level.AssertSausage({3, 2, 4, 2, 0, Sausage::Rolled});
    level.AssertSausage({4, 2, 4, 3, 1, Sausage::None});
  }

  // Staircase roll where the top's carry is wall-blocked, but Stephen's fork ends up directly under it: instead of
  // dropping, the top is caught and held at z=1 on the fork.
  MAKE_SYMMETRICAL_TEST(StackedStaircaseRollForkCatch) {
    TestSymmetryHelper level(symmetry, LevelType(10, 7, "arena",
      "          "
      " _______  "
      " ___2___  "
      " >______  "
      " _______  "
      " _______  "
      "          ",
      {}, {}, { Sausage{3, 3, 3, 4, 0}, Sausage{3, 2, 3, 3, 1}, Sausage{9, 1, 9, 2, 0} }));
    level.AssertPosition(1, 3, Right);
    level.AssertSausage({3, 3, 3, 4, 0, Sausage::None});
    level.AssertSausage({3, 2, 3, 3, 1, Sausage::None});
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(2, 3, Right);
    level.AssertSausage({4, 3, 4, 4, 0, Sausage::Rolled});
    level.AssertSausage({3, 2, 3, 3, 1, Sausage::None});
  }

  // A 3-high vertical tower. Pushing the bottom east rolls the whole stack east as a unit -- each level is carried by the
  // one beneath it (the carry recurses up the tower), and every sausage rolls across its own long axis.
  MAKE_SYMMETRICAL_TEST(StackedTowerThreeHighRoll) {
    TestSymmetryHelper level(symmetry, LevelType(10, 7, "arena",
      "          "
      " _______  "
      " _______  "
      " >______  "
      " _______  "
      " _______  "
      "          ",
      {}, {}, { Sausage{3, 3, 3, 4, 0}, Sausage{3, 3, 3, 4, 1}, Sausage{3, 3, 3, 4, 2} }));
    level.AssertPosition(1, 3, Right);
    level.AssertSausage({3, 3, 3, 4, 0, Sausage::None});
    level.AssertSausage({3, 3, 3, 4, 1, Sausage::None});
    level.AssertSausage({3, 3, 3, 4, 2, Sausage::None});
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(2, 3, Right);
    level.AssertSausage({4, 3, 4, 4, 0, Sausage::Rolled});
    level.AssertSausage({4, 3, 4, 4, 1, Sausage::Rolled});
    level.AssertSausage({4, 3, 4, 4, 2, Sausage::Rolled});
  }

  // Perpendicular stack whose vertical top would double-move north by two, but the second cell of that slide is a Wall2.
  // The double-move is capped to a single cell rather than refused: the top slides north just one cell.
  MAKE_SYMMETRICAL_TEST(StackedDoubleMoveBlockedByWall) {
    TestSymmetryHelper level(symmetry, LevelType(10, 7, "arena",
      "          "
      " ___2___  "
      " _______  "
      " _______  "
      " _______  "
      " __^____  "
      "          ",
      {}, {}, { Sausage{3, 3, 4, 3, 0}, Sausage{4, 3, 4, 4, 1}, Sausage{9, 1, 9, 2, 0} }));
    level.AssertPosition(3, 5, Up);
    level.AssertSausage({3, 3, 4, 3, 0, Sausage::None});
    level.AssertSausage({4, 3, 4, 4, 1, Sausage::None});
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(3, 4, Up);
    level.AssertSausage({3, 2, 4, 2, 0, Sausage::Rolled});
    level.AssertSausage({4, 2, 4, 3, 1, Sausage::None});
  }

  // Staircase roll where the carried top's destination is a Wall2 (so it is left behind and drops to z=0), and the cell
  // it lands on is a grill: the dropped top cooks on landing.
  MAKE_SYMMETRICAL_TEST(StackedRollDropOntoGrill) {
    TestSymmetryHelper level(symmetry, LevelType(10, 7, "arena",
      "          "
      " _______  "
      " _______  "
      " >______  "
      " _______  "
      " __#2___  "
      "          ",
      {}, {}, { Sausage{3, 3, 3, 4, 0}, Sausage{3, 4, 3, 5, 1}, Sausage{9, 1, 9, 2, 0} }));
    level.AssertPosition(1, 3, Right);
    level.AssertSausage({3, 3, 3, 4, 0, Sausage::None});
    level.AssertSausage({3, 4, 3, 5, 1, Sausage::None});
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(2, 3, Right);
    level.AssertSausage({4, 3, 4, 4, 0, Sausage::Rolled});
    level.AssertSausage({3, 4, 3, 5, 0, Sausage::Cook2A});
  }

  // Stephen spears the bottom of a stack (jammed against a wall), then drags it sideways. The speared bottom translates
  // rigidly (no roll), but the sausage riding on top is carried along and DOES roll across its own axis.
  MAKE_SYMMETRICAL_TEST(StackedCarryWhileSpeared) {
    TestSymmetryHelper level(symmetry, LevelType(10, 7, "arena",
      "          "
      " _______  "
      " _______  "
      " >___1__  "
      " _______  "
      " _______  "
      "          ",
      {}, {}, { Sausage{3, 3, 4, 3, 0}, Sausage{3, 3, 4, 3, 1}, Sausage{9, 1, 9, 2, 0} }));
    level.AssertPosition(1, 3, Right);
    level.AssertSausage({3, 3, 4, 3, 0, Sausage::None});
    level.AssertSausage({3, 3, 4, 3, 1, Sausage::None});
    level.AssertMoveSucceeds(Right); // spear the bottom; nothing moves yet
    level.AssertPosition(2, 3, Right);
    level.AssertSausage({3, 3, 4, 3, 0, Sausage::None});
    level.AssertSausage({3, 3, 4, 3, 1, Sausage::None});
    level.AssertMoveSucceeds(Up); // drag north: bottom slides rigidly, top is carried and rolls
    level.AssertPosition(2, 2, Right);
    level.AssertSausage({3, 2, 4, 2, 0, Sausage::None});
    level.AssertSausage({3, 2, 4, 2, 1, Sausage::Rolled});
  }

  // A vertical top sausage spans two supports: one end on a horizontal base, the other on Stephen's head. When Stephen
  // TURNS, his body counts as a wall, so the top is held in place even as the turn shoves the base out from under it.
  // (During a STEP his body would only cancel a double-move; only a turn pins the sausage. 3-3 Cold Escarpment move 69.)
  MAKE_SYMMETRICAL_TEST(StackedTopHeldByBodyDuringTurn) {
    TestSymmetryHelper level(symmetry, LevelType(10, 7, "arena",
      "          "
      " _______  "
      " _______  "
      " _______  "
      " ____>__  "
      " _______  "
      "          ",
      {}, {}, { Sausage{4, 3, 5, 3, 0}, Sausage{5, 3, 5, 4, 1}, Sausage{9, 1, 9, 2, 0} }));
    level.AssertPosition(5, 4, Right);
    level.AssertSausage({4, 3, 5, 3, 0, Sausage::None});
    level.AssertSausage({5, 3, 5, 4, 1, Sausage::None});
    level.AssertMoveSucceeds(Up); // turn east->north: the base slides west, the top is pinned by Stephen's head
    level.AssertPosition(5, 4, Up);
    level.AssertSausage({3, 3, 4, 3, 0, Sausage::None});
    level.AssertSausage({5, 3, 5, 4, 1, Sausage::None});
  }

  // Spot test for 3-2 Cold Finger move 20 (a turn drops a fork-borne rider). A vertical rider spans a base sausage
  // (north end) and Stephen's fork (south end). Stephen turns east->north: the corner-sweep shoves the base out north
  // and the fork swings away, so the rider loses BOTH supports and drops straight down -- it is NOT carried north with
  // the base (the reference's rotation corner-sweep never carries riders).
  MAKE_SYMMETRICAL_TEST(TurnDropsForkBorneRider) {
    TestSymmetryHelper level(symmetry, LevelType(10, 7, "arena",
      "__________"
      "___a______"
      "___a______"
      "__>_______"
      "__________"
      "__________"
      "__________",
      {}, {}, { Sausage{3, 2, 3, 3, 1}, Sausage{9, 1, 9, 2, 0} }));
    level.AssertPosition(2, 3, Right);
    level.AssertSausage({3, 1, 3, 2, 0, Sausage::None});          // base
    level.AssertSausage({3, 2, 3, 3, 1, Sausage::None});          // vertical rider on base (north) + fork (south)
    level.AssertMoveSucceeds(Up);                                 // turn east->north
    level.AssertPosition(2, 3, Up);
    level.AssertSausage({3, 0, 3, 1, 0, Sausage::None});          // base shoved north by the corner-sweep
    level.AssertSausage({3, 2, 3, 3, 0, Sausage::None});          // rider DROPS straight down (not carried north)
  }

  // Spot test for a 3-12 Cold Horizon divergence (DiffEngines DIFF #8): a fork-hat whose far (non-fork) end rests on a
  // WALL top is anchored -- when Stephen backs away and the fork slides out from under it, the sausage STAYS on the wall
  // rather than riding off with the fork. The reference anchors on wall terrain (IsSausageCarried's IsWall(other, z)
  // check); Level2's PlanHatCarry only checks for a non-moving SAUSAGE below, so it wrongly carries it. Passes on the
  // reference; currently FAILS on Level2, pinning the gap.
  MAKE_SYMMETRICAL_TEST(ForkHatAnchoredOnWall) {
    TestSymmetryHelper level(symmetry, LevelType(10, 7, "arena",
      "          "
      " _______  "
      " ___1___  "
      " __>____  "
      " _______  "
      " _______  "
      "          ",
      {}, {}, { Sausage{4, 2, 4, 3, 1}, Sausage{9, 1, 9, 2, 0}, Sausage{9, 4, 9, 5, 0} }));
    level.AssertPosition(3, 3, Right);
    level.AssertSausage({4, 2, 4, 3, 1, Sausage::None});
    level.AssertMoveSucceeds(Left); // back away west; the fork leaves (4,3) from under the fork-hat
    level.AssertPosition(2, 3, Right);
    level.AssertSausage({4, 2, 4, 3, 1, Sausage::None}); // fork-hat STAYS, anchored on the Wall1 at (4,2)
  }

  // 3-14 Cold Frustration move-86 reproduction. Faithful local snapshot of the demo's m85 end-state, using the ACTUAL
  // 3-14 terrain from Levels.h (the ASCII Print hides terrain under sausages: (4,3) is a GRILL and (3,4) is VOID, both
  // masked by the base/rider in the trace). Sausage indices match the demo: [0]=vertical base, [1]=cooked decoy,
  // [2]=horizontal rider bridging the base (west end) and Stephen's fork (east end, over the (4,3) grill).
  MAKE_SYMMETRICAL_TEST(RiderCarriedByMovingBaseOffFork) {
    TestSymmetryHelper level(symmetry, LevelType(10, 9, "arena",
      "2  _____  "
      "111L_D__  "
      "1    222  "
      "U_1_#<U___"
      "___ #___1_"
      "__1_#_____"
      "____22____"
      "___R22____"
      "____22____",
      {}, {}, { Sausage{3, 3, 3, 4, 0}, Sausage{7, 5, 7, 6, 0, Sausage::Flags(0x0f)}, Sausage{3, 3, 4, 3, 1, Sausage::Rolled} }));
    level.AssertPosition(5, 3, Left);
    level.AssertSausage({3, 3, 3, 4, 0, Sausage::None});
    level.AssertSausage({3, 3, 4, 3, 1, Sausage::Rolled});
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(5, 3, Left);                                          // Stephen STAYS at (5,3)
    level.AssertSausage({4, 3, 4, 4, 0, Sausage::Flags(Sausage::Cook1A | Sausage::Cook2A)}); // base slid east onto the grill, cooked
    level.AssertSausage({4, 3, 5, 3, 1, Sausage::Rolled});                     // rider carried east onto Stephen's head
  }
};

TEST_CLASS(LogicTests) {
  MAKE_SYMMETRICAL_TEST(BasicLocomotion) {
    TestSymmetryHelper level(symmetry, LevelType(10, 7, "arena",
      "          "
      " _____   A"
      " _____   A"
      " >____   B"
      " _____   B"
      " _____   C"
      "         C"));

    level.AssertPosition(1, 3, Right);

    // Walk east across the platform to the far edge; the fork ends hanging over the void.
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(2, 3, Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(3, 3, Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(4, 3, Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(5, 3, Right);

    // Walking forward off the east edge is rejected (the body would step into the void).
    level.AssertMoveFails(Right);

    // Turn south and walk down to the SE corner; that far edge is impassable on foot too.
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(5, 3, Down);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(5, 4, Down);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(5, 5, Down);
    level.AssertMoveFails(Down);

    // Spin in place in the corner: each perpendicular press just rotates the body, sweeping the fork
    // freely around -- including out over the void on the south and east sides.
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(5, 5, Right);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(5, 5, Up);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(5, 5, Left);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(5, 5, Down);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(5, 5, Right);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(5, 5, Down);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(5, 5, Left);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(5, 5, Up);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(5, 5, Right);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(5, 5, Down);

    // Back north out of the corner (walking backward keeps the facing direction).
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(5, 4, Down);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(5, 3, Down);

    // Face east, then back west along row 3 (still facing east the whole way).
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(5, 3, Right);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(4, 3, Right);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(3, 3, Right);

    // Rotate a full turn in place at the center; the fork sweeps through all four directions.
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(3, 3, Up);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(3, 3, Left);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(3, 3, Down);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(3, 3, Right);
  }

  MAKE_SYMMETRICAL_TEST(SingleGrill) {
    TestSymmetryHelper level(symmetry, LevelType(10, 7, "arena",
      "          "
      " _____   A"
      " _____   A"
      " >_#__   B"
      " _____   B"
      " _____   C"
      "         C"));

    level.AssertPosition(1, 3, Right);

    // The fork may hang over a grill -- walking forward parks the body beside it with the fork on top.
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(2, 3, Right);

    // Spin in place next to the grill; each perpendicular press just swings the fork around the body.
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(2, 3, Down);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(2, 3, Left);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(2, 3, Up);

    // Work around to sit just south of the grill, facing away from it.
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(2, 4, Up);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(2, 4, Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(3, 4, Right);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(3, 4, Down);

    // Backing north onto the grill is bounced: the body steps on, the grill kicks it straight back to
    // where it started -- so nothing moves, yet the move still "succeeds".
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(3, 4, Down);

    // Shuffle one column east and up to row 3, clear of the grill, to set up a head-on approach.
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(3, 4, Left);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(4, 4, Left);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(4, 4, Down);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(4, 3, Down);

    // Turn to face the grill (the fork swings over it again, which is allowed)...
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(4, 3, Left);

    // ...but walking the body forward onto it is bounced back exactly the same way.
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(4, 3, Left);
  }

  MAKE_SYMMETRICAL_TEST(SingleSausage) {
    TestSymmetryHelper level(symmetry, LevelType(10, 7, "arena",
      "          "
      " _____   A"
      " _____   A"
      " >_cc_   B"
      " _____   B"
      " _____    "
      "          "));

    level.AssertPosition(1, 3, Right);
    level.AssertSausage({ 3, 3, 4, 3, 0, Sausage::None });

    // Push the sausage east with the fork (end-on -> slides one cell, no roll).
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(2, 3, Right);
    level.AssertSausageMoved(Right);

    // Get south of it, then rotate the fork up into its underside to shove it west.
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(2, 3, Down);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(2, 4, Down);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(2, 4, Left);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(3, 4, Left);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(4, 4, Left);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(5, 4, Left);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(5, 4, Down);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(5, 4, Right);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(5, 4, Up);
    level.AssertSausageMoved(Left);

    // ...and once more, another cell west.
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(5, 4, Left);
    level.AssertSausageMoved(Left);

    // Reposition north of it and shove it west a third time.
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(5, 4, Down);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(5, 3, Down);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(5, 3, Right);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(4, 3, Right);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(3, 3, Right);
    level.AssertSausageMoved(Left);

    // Come around the west side and push it back east twice.
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(3, 3, Down);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(3, 2, Down);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(3, 2, Right);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(2, 2, Right);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(1, 2, Right);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(1, 2, Up);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(1, 2, Left);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(1, 2, Down);
    level.AssertSausageMoved(Right);

    level.AssertMoveSucceeds(Right);
    level.AssertPosition(1, 2, Right);
    level.AssertSausageMoved(Right);

    // Get above it and back up into it so it rolls south (Rolled toggles each roll).
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(2, 2, Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(3, 2, Right);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(3, 2, Up);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(3, 3, Up);
    level.AssertSausageMoved(Down, Sausage::Rolled);

    level.AssertMoveSucceeds(Down);
    level.AssertPosition(3, 4, Up);
    level.AssertSausageMoved(Down, Sausage::Rolled);

    // Line up west of it and push it east toward the edge.
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(3, 4, Right);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(2, 4, Right);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(2, 4, Up);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(2, 5, Up);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(2, 5, Left);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(3, 5, Left);
    level.AssertSausageMoved(Right);

    // Push it laterally off the edge: it slides one unit out over the void.
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(4, 5, Left);
    level.AssertSausageMoved(Right);

    // Roll it north up the edge column (Rolled toggles each roll).
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(4, 5, Down);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(4, 5, Right);
    level.AssertSausageMoved(Up, Sausage::Rolled);

    level.AssertMoveSucceeds(Up);
    level.AssertPosition(4, 5, Up);
    level.AssertSausageMoved(Up, Sausage::Rolled);

    level.AssertMoveSucceeds(Left);
    level.AssertPosition(4, 5, Left);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(5, 5, Left);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(5, 5, Up);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(5, 4, Up);
    level.AssertSausageMoved(Up, Sausage::Rolled);

    level.AssertMoveSucceeds(Up);
    level.AssertPosition(5, 3, Up);
    level.AssertSausageMoved(Up, Sausage::Rolled);

    // Pushing the sausage off the top edge (vertically) is rejected.
    level.AssertMoveFails(Up);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(5, 3, Left);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(5, 3, Down);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(5, 2, Down);
    level.AssertMoveFails(Up);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(5, 2, Left);
    level.AssertMoveFails(Up);
  }

  MAKE_SYMMETRICAL_TEST(SingleWall) {
    TestSymmetryHelper level(symmetry, LevelType(10, 7, "arena",
      "          "
      " _____   A"
      " _____   A"
      " >_1__   B"
      " _____   B"
      " _____   C"
      "         C"));
    level.AssertPosition(1, 3, Right);

    // Walking forward into the wall is rejected -- the fork can't enter the wall cell ahead.
    level.AssertMoveFails(Right);

    // Spin to face away, then back (east) into the wall: the body can't enter it either.
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(1, 3, Down);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(1, 3, Left);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(2, 3, Left);
    level.AssertMoveFails(Right);

    // Sit just west of the wall facing down, then rotate the fork east toward it: it bonks -- the
    // move reports success but Stephen doesn't turn (the wall blocks the body's swing).
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(2, 3, Down);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(2, 3, Down);

    // Step north so the wall sits in the rotation's corner sweep; now turning toward it is rejected
    // outright rather than bonking.
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(2, 2, Down);
    level.AssertMoveFails(Right);

    // Rotations that sweep clear of the wall all succeed -- spin around to face it again from the north.
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(2, 2, Left);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(2, 2, Up);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(2, 2, Right);

    // Turning back down, with the wall again in the corner sweep, is rejected.
    level.AssertMoveFails(Down);
  }

  MAKE_SYMMETRICAL_TEST(SingleSausageAndWall) {
    TestSymmetryHelper level(symmetry, LevelType(10, 7, "arena",
      "          "
      " _____   A"
      " ___1_   A"
      " >_cc_   B"
      " _____   B"
      " _____    "
      "          "));
    // Walks a single sausage all the way around the arena -- testing slides, rolls, every edge, and
    // the wall sitting just NE of the sausage's start cell.
    level.AssertPosition(1, 3, Right);

    // Maneuver to the sausage's south side; rotating the fork up into the sausage or the nearby wall
    // is blocked -- some attempts bonk (report success without turning), others are rejected.
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(1, 3, Up);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(1, 3, Left);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(2, 3, Left);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(2, 3, Down);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(2, 3, Down);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(2, 4, Down);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(2, 4, Right);
    level.AssertMoveFails(Up);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(3, 4, Right);
    level.AssertMoveFails(Up);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(3, 4, Down);
    level.AssertMoveFails(Up);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(3, 4, Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(4, 4, Right);

    // Now at the sausage's SE corner (still on its start cell). Shove it west, roll it north and back
    // south beside the wall, then push it east again.
    level.AssertSausage({3, 3, 4, 3, 0, Sausage::None});
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(4, 4, Up);
    level.AssertSausageMoved(Left);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(4, 4, Right);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(3, 4, Right);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(2, 4, Right);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(2, 4, Down);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(2, 3, Down);
    level.AssertSausageMoved(Up, Sausage::Rolled);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(2, 3, Left);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(2, 3, Left);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(1, 3, Left);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(1, 3, Up);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(1, 2, Up);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(1, 2, Right);
    level.AssertSausageMoved(Down, Sausage::Rolled);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(1, 2, Up);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(1, 3, Up);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(1, 3, Left);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(2, 3, Left);
    level.AssertSausageMoved(Right);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(2, 3, Down);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(2, 4, Down);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(2, 5, Down);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(2, 5, Left);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(3, 5, Left);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(3, 5, Up);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(3, 4, Up);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(2, 4, Up);
    level.AssertSausageMoved(Left);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(1, 4, Up);
    level.AssertSausageMoved(Left);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(1, 5, Up);
    level.AssertSausageMoved(Down);

    // Backed into the south-west corner: pushing the sausage off the south or west edge is rejected.
    // Then push it east along the bottom edge to the far corner.
    level.AssertMoveFails(Down);
    level.AssertMoveFails(Left);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(2, 5, Up);
    level.AssertSausageMoved(Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(3, 5, Up);
    level.AssertSausageMoved(Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(4, 5, Up);
    level.AssertSausageMoved(Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(5, 5, Up);
    level.AssertSausageMoved(Right);
    level.AssertMoveFails(Right);

    // Push it north up the east column to the top row; the top edge stops it.
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(5, 4, Up);
    level.AssertSausageMoved(Up);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(5, 3, Up);
    level.AssertSausageMoved(Up);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(5, 2, Up);
    level.AssertSausageMoved(Up);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(5, 1, Up);
    level.AssertSausageMoved(Up);

    // Top edge stops it. Now walk the sausage back around the perimeter -- west along the top, down
    // the far side and around -- finishing back down the east column.
    level.AssertMoveFails(Up);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(4, 1, Up);
    level.AssertSausageMoved(Left);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(3, 1, Up);
    level.AssertSausageMoved(Left);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(3, 2, Up);
    level.AssertSausageMoved(Down);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(3, 3, Up);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(3, 3, Left);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(4, 3, Left);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(5, 3, Left);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(5, 3, Down);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(5, 2, Down);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(5, 1, Down);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(5, 1, Right);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(4, 1, Right);
    level.AssertSausageMoved(Left);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(4, 1, Up);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(4, 1, Left);
    level.AssertSausageMoved(Down, Sausage::Rolled);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(3, 1, Left);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(2, 1, Left);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(2, 1, Up);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(2, 2, Up);
    level.AssertSausageMoved(Down, Sausage::Rolled);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(2, 2, Left);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(2, 2, Down);
    level.AssertSausageMoved(Right);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(2, 3, Down);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(2, 4, Down);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(2, 5, Down);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(2, 5, Left);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(3, 5, Left);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(4, 5, Left);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(4, 5, Up);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(4, 4, Up);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(3, 4, Up);
    level.AssertSausageMoved(Left);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(2, 4, Up);
    level.AssertSausageMoved(Left);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(1, 4, Up);
    level.AssertSausageMoved(Left);
    level.AssertMoveFails(Left);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(1, 3, Up);
    level.AssertSausageMoved(Up);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(1, 2, Up);
    level.AssertSausageMoved(Up);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(1, 1, Up);
    level.AssertSausageMoved(Up);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(2, 1, Up);
    level.AssertSausageMoved(Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(3, 1, Up);
    level.AssertSausageMoved(Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(4, 1, Up);
    level.AssertSausageMoved(Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(5, 1, Up);
    level.AssertSausageMoved(Right);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(5, 2, Up);
    level.AssertSausageMoved(Down);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(5, 3, Up);
    level.AssertMoveFails(Up);
  }

  MAKE_SYMMETRICAL_TEST(SingleSausageAndGrill) {
    TestSymmetryHelper level(symmetry, LevelType(10, 7, "arena",
      "          "
      " _____   A"
      " __cc_   A"
      " >_#__   B"
      " _____   B"
      " _____    "
      "          "));
    level.AssertPosition(1, 3, Right);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(1, 3, Up);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(1, 2, Up);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(1, 2, Left);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(2, 2, Left);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(2, 2, Up);
    level.AssertSausage({3, 2, 4, 2, 0, Sausage::None});
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(2, 2, Right);
    level.AssertSausage({3, 3, 4, 3, 0, Sausage::Cook1B | Sausage::Rolled});
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(2, 2, Down);
    level.AssertSausageMoved(Down, Sausage::Rolled);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(2, 3, Down);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(2, 4, Down);
    level.AssertMoveFails(Right);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(2, 5, Down);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(2, 5, Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(3, 5, Right);
    level.AssertMoveFails(Up);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(4, 5, Right);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(4, 5, Up);
    level.AssertSausageMoved(Left);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(4, 5, Left);
    level.AssertSausageMoved(Left);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(3, 5, Left);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(2, 5, Left);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(1, 5, Left);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(1, 5, Down);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(1, 4, Down);
    level.AssertSausageMoved(Up, Sausage::Rolled);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(1, 4, Left);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(1, 4, Up);
    level.AssertSausage({2, 3, 3, 3, 0, Sausage::Cook1B | Sausage::Cook2B | Sausage::Rolled});
    level.AssertMoveFails(Right);
  }

  MAKE_SYMMETRICAL_TEST(SingleSausageAndGrillAndWall) {
    TestSymmetryHelper level(symmetry, LevelType(10, 7, "arena",
      "          "
      " ______  A"
      " ______  A"
      " >_#___  B"
      " __c11_  B"
      " __c___   "
      "          "));
    level.AssertPosition(1, 3, Right);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(1, 3, Down);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(1, 4, Down);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(1, 5, Down);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(1, 5, Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(2, 5, Right);
    level.AssertSausage({3, 4, 3, 5, 0, Sausage::None});
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(2, 4, Right);
    level.AssertSausage({3, 3, 3, 4, 0, Sausage::Cook1A});
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(2, 3, Right);
    level.AssertSausage({3, 2, 3, 3, 0, Sausage::Cook1A | Sausage::Cook2A});
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(2, 2, Right);
    level.AssertSausageMoved(Up);
    level.AssertMoveFails(Down);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(1, 2, Right);
    level.AssertSausageMoved(Left);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(1, 3, Right);
    level.AssertSausageMoved(Down);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(1, 4, Right);
    level.AssertSausageMoved(Down);
    level.AssertMoveFails(Right);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(1, 5, Right);
    level.AssertSausageMoved(Down);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(2, 5, Right);
    level.AssertSausageMoved(Right);
    level.AssertMoveFails(Up);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(1, 5, Right);
    level.AssertSausageMoved(Left);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(1, 4, Right);
    level.AssertSausageMoved(Up);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(1, 3, Right);
    level.AssertSausageMoved(Up);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(1, 2, Right);
    level.AssertSausageMoved(Up);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(2, 2, Right);
    level.AssertSausageMoved(Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(3, 2, Right);
    level.AssertSausageMoved(Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(4, 2, Right);
    level.AssertSausageMoved(Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(5, 2, Right);
    level.AssertSausageMoved(Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(6, 2, Right);
    level.AssertSausageMoved(Right);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(6, 3, Right);
    level.AssertSausageMoved(Down);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(6, 4, Right);
    level.AssertSausageMoved(Down);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(6, 5, Right);
    level.AssertSausageMoved(Down);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(5, 5, Right);
    level.AssertSausageMoved(Left);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(4, 5, Right);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(4, 5, Down);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(4, 5, Left);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(5, 5, Left);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(5, 5, Down);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(5, 5, Right);
    level.AssertSausageMoved(Up);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(6, 5, Right);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(6, 5, Down);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(6, 4, Down);
    level.AssertSausageMoved(Up);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(6, 3, Down);
    level.AssertSausageMoved(Up);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(6, 3, Right);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(6, 3, Up);
    level.AssertSausageMoved(Left, Sausage::Rolled);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(6, 2, Up);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(6, 1, Up);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(6, 1, Right);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(5, 1, Right);
    level.AssertSausageMoved(Left, Sausage::Rolled);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(4, 1, Right);
    level.AssertSausageMoved(Left, Sausage::Rolled);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(3, 1, Right);
    level.AssertSausageMoved(Left, Sausage::Rolled);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(3, 1, Up);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(3, 1, Left);
    level.AssertSausageMoved(Down);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(2, 1, Left);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(2, 1, Up);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(2, 2, Up);
    level.AssertSausageMoved(Down);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(2, 3, Up);
    level.AssertSausageMoved(Down);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(2, 3, Left);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(1, 3, Left);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(1, 3, Down);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(1, 4, Down);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(1, 4, Right);
    level.AssertSausageMoved(Right, Sausage::Rolled);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(2, 4, Right);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(1, 4, Right);
    level.AssertSausageMoved(Left);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(1, 3, Right);
    level.AssertSausageMoved(Up);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(1, 2, Right);
    level.AssertSausageMoved(Up);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(2, 2, Right);
    level.AssertSausage({3, 2, 3, 3, 0, Sausage::Cook1A | Sausage::Cook2A | Sausage::Cook2B | Sausage::Rolled});
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(1, 2, Right);
    level.AssertSausageMoved(Left);
    level.AssertMoveFails(Right);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(1, 3, Right);
    level.AssertSausageMoved(Down);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(2, 3, Right);
    level.AssertSausage({3, 3, 3, 4, 0, Sausage::Cook1A | Sausage::Cook1B | Sausage::Cook2A | Sausage::Cook2B | Sausage::Rolled});
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(1, 3, Right);
    level.AssertSausageMoved(Left);
    level.AssertMoveFails(Right);
  }

  MAKE_SYMMETRICAL_TEST(TwoSausages) {
    TestSymmetryHelper level(symmetry, LevelType(10, 7, "arena",
      "          "
      " _______ A"
      " _______ A"
      " >__bb__  "
      " ___cc__  "
      " _______  "
      "          "));

    level.AssertPosition(1, 3, Right);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(1, 3, Down);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(1, 4, Down);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(1, 4, Left);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(2, 4, Left);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(3, 4, Left);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(3, 4, Down);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(3, 4, Right);
    level.AssertSausage({4, 2, 5, 2, 0, Sausage::Rolled});
    level.AssertSausage({4, 3, 5, 3, 0, Sausage::Rolled});
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(4, 4, Right);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(4, 4, Up);
    level.AssertSausage({4, 1, 5, 1, 0, Sausage::None});
    level.AssertSausage({4, 2, 5, 2, 0, Sausage::None});
    level.AssertMoveFails(Up);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(4, 4, Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(5, 4, Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(6, 4, Right);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(6, 4, Down);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(6, 3, Down);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(6, 2, Down);
    level.AssertMoveFails(Left);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(6, 2, Right);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(6, 2, Up);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(6, 2, Left);
    level.AssertSausage({3, 1, 4, 1, 0, Sausage::None});
    level.AssertSausage({4, 3, 5, 3, 0, Sausage::Rolled});
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(5, 2, Left);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(4, 2, Left);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(4, 2, Down);
    level.AssertSausageMoved(Right);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(4, 3, Down);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(4, 3, Right);
    level.AssertSausageMoved(Up, Sausage::Rolled);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(5, 3, Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(6, 3, Right);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(6, 3, Down);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(6, 2, Down);
    level.AssertSausageMoved(Up, Sausage::Rolled);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(6, 2, Right);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(6, 2, Up);
    level.AssertSausage({2, 1, 3, 1, 0, Sausage::None});
    level.AssertSausage({4, 1, 5, 1, 0, Sausage::Rolled});
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(6, 2, Left);
    level.AssertSausage({1, 1, 2, 1, 0, Sausage::None});
    level.AssertSausage({3, 1, 4, 1, 0, Sausage::Rolled});
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(6, 2, Down);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(6, 1, Down);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(6, 1, Left);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(5, 1, Left);
    level.AssertSausage({0, 1, 1, 1, 0, Sausage::None});
    level.AssertSausage({2, 1, 3, 1, 0, Sausage::Rolled});
    level.AssertMoveFails(Left);
  }

  MAKE_SYMMETRICAL_TEST(TwoSausagesAndWall) {
    TestSymmetryHelper level(symmetry, LevelType(10, 7, "arena",
      "          "
      " _______ A"
      " _______ A"
      " >_cb_1_  "
      " __cb___  "
      " _______  "
      "          "));

    level.AssertPosition(1, 3, Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(2, 3, Right);
    level.AssertSausage({5, 3, 5, 4, 0, Sausage::Rolled});
    level.AssertSausage({4, 3, 4, 4, 0, Sausage::Rolled});
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(3, 3, Right);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(3, 4, Right);
    level.AssertSausageMoved(Down);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(3, 5, Right);
    level.AssertSausageMoved(Down);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(4, 5, Right);
    level.AssertSausageMoved(Right);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(4, 4, Right);
    level.AssertSausage({5, 2, 5, 3, 0, Sausage::Rolled});
    level.AssertSausage({5, 4, 5, 5, 0, Sausage::Rolled});
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(5, 4, Right);
    level.AssertSausageMoved(Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(6, 4, Right);
    level.AssertSausageMoved(Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(7, 4, Right);
    level.AssertSausageMoved(Right);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(7, 3, Right);
    level.AssertSausageMoved(Up);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(7, 2, Right);
    level.AssertSausageMoved(Up);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(6, 2, Right);
    level.AssertSausageMoved(Left);
    level.AssertSausage({5, 2, 5, 3, 0, Sausage::Rolled});
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(5, 2, Right);
    level.AssertSausageMoved(Left, Sausage::Rolled);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(5, 2, Up);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(5, 2, Left);
    level.AssertSausageMoved(Down);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(6, 2, Left);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(6, 2, Up);
    level.AssertSausage({7, 2, 7, 3, 0, Sausage::Rolled});
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(6, 2, Right);
    level.AssertSausageMoved(Down);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(7, 2, Right);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(7, 2, Up);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(7, 3, Up);
    level.AssertSausageMoved(Down);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(7, 3, Right);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(7, 3, Down);
    level.AssertSausageMoved(Left, Sausage::Rolled);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(7, 2, Down);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(7, 2, Right);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(7, 2, Up);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(7, 2, Left);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(6, 2, Left);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(5, 2, Left);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(4, 2, Left);
    level.AssertSausage({4, 3, 4, 4, 0, Sausage::None});
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(4, 2, Down);
    level.AssertSausageMoved(Right, Sausage::Rolled);
    level.AssertMoveFails(Right);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(4, 2, Left);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(5, 2, Left);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(6, 2, Left);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(7, 2, Left);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(7, 2, Up);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(7, 3, Up);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(7, 3, Right);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(7, 3, Down);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(7, 3, Down);
    level.AssertSausage({4, 3, 4, 4, 0, Sausage::None});
    level.AssertSausage({5, 4, 5, 5, 0, Sausage::Rolled});
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(7, 4, Down);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(7, 5, Down);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(7, 5, Right);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(6, 5, Right);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(6, 5, Down);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(6, 5, Left);
    level.AssertSausageMoved(Up);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(6, 5, Up);
    level.AssertSausageMoved(Up);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(6, 5, Left);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(5, 5, Left);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(4, 5, Left);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(3, 5, Left);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(2, 5, Left);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(2, 5, Down);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(2, 4, Down);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(2, 4, Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(3, 4, Right);
    level.AssertMoveFails(Right);
  }

  MAKE_SYMMETRICAL_TEST(TwoSausagesAndWallSpear) {
    TestSymmetryHelper level(symmetry, LevelType(10, 7, "arena",
      "          "
      " _______ A"
      " _______ A"
      " >_cb1__  "
      " __cb___  "
      " _______  "
      "          "));

    level.AssertPosition(1, 3, Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(2, 3, Right);
    level.AssertMoveFails(Right);
    level.AssertSausage({3, 3, 3, 4, 0, Sausage::None});
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(2, 2, Right);
    level.AssertSausageMoved(Up);
    level.AssertMoveFails(Right);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(2, 1, Right);
    level.AssertSausageMoved(Up);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(3, 1, Right);
    level.AssertSausageMoved(Right);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(3, 2, Right);
    level.AssertSausage({4, 4, 4, 5, 0, Sausage::None});
    level.AssertSausage({4, 2, 4, 3, 0, Sausage::None});
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(2, 2, Right);
    level.AssertSausageMoved(Left);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(2, 3, Right);
    level.AssertSausageMoved(Down);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(3, 3, Right);
    level.AssertSausage({5, 4, 5, 5, 0, Sausage::Rolled});
    level.AssertSausage({4, 3, 4, 4, 0, Sausage::None});
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(3, 2, Right);
    level.AssertSausageMoved(Up);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(3, 1, Right);
    level.AssertSausageMoved(Up);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(4, 1, Right);
    level.AssertSausageMoved(Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(5, 1, Right);
    level.AssertSausageMoved(Right);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(5, 2, Right);
    level.AssertSausageMoved(Down);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(4, 2, Right);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(4, 2, Up);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(4, 1, Up);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(4, 1, Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(5, 1, Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(6, 1, Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(7, 1, Right);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(7, 1, Down);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(7, 2, Down);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(7, 3, Down);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(7, 4, Down);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(7, 5, Down);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(7, 5, Right);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(6, 5, Right);
    level.AssertSausage({5, 4, 5, 5, 0, Sausage::Rolled});
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(5, 5, Right);
    level.AssertSausageMoved(Left, Sausage::Rolled);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(5, 5, Down);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(5, 5, Left);
    level.AssertSausageMoved(Up);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(4, 5, Left);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(3, 5, Left);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(2, 5, Left);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(2, 5, Up);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(2, 4, Up);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(2, 3, Up);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(2, 3, Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(3, 3, Right);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(3, 4, Right);
    level.AssertSausageMoved(Down);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(4, 4, Right);
    level.AssertSausageMoved(Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(5, 4, Right);
    level.AssertSausageMoved(Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(6, 4, Right);
    level.AssertSausageMoved(Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(7, 4, Right);
    level.AssertSausageMoved(Right);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(7, 3, Right);
    level.AssertSausageMoved(Up);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(7, 2, Right);
    level.AssertSausageMoved(Up);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(7, 1, Right);
    level.AssertSausageMoved(Up);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(6, 1, Right);
    level.AssertSausageMoved(Left);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(5, 1, Right);
  }

  MAKE_SYMMETRICAL_TEST(LogRoll) {
    TestSymmetryHelper level(symmetry, LevelType(10, 7, "arena",
      "          "
      " _D___   A"
      " 11cc_   A"
      " >____   B"
      " ____1   B"
      " ___R1    "
      "          "));
    level.AssertPosition(1, 3, Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(2, 3, Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(3, 3, Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(4, 3, Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(5, 3, Right);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(5, 3, Up);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(5, 2, Up);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(5, 1, Up);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(5, 1, Left);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(4, 1, Left);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(3, 1, Left);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(2, 1, Left);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(2, 2, Left);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(1, 2, Left);
    level.AssertMoveFails(Left);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(1, 2, Down);
    level.AssertMoveFails(Down);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(1, 2, Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(2, 2, Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(3, 2, Right);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(3, 2, Up);
    level.AssertSausage({3, 2, 4, 2, 0, Sausage::None});
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(3, 1, Up);
    level.AssertSausageMoved(Up, Sausage::Rolled);
    level.AssertMoveFails(Down);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(3, 1, Left);
    level.AssertMoveFails(Left);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(4, 1, Left);
    level.AssertMoveFails(Right);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(4, 1, Down);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(4, 2, Down);
    level.AssertSausageMoved(Down, Sausage::Rolled);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(4, 3, Down);
    level.AssertSausageMoved(Down, Sausage::Rolled);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(4, 4, Down);
    level.AssertSausageMoved(Down, Sausage::Rolled);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(4, 4, Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(5, 4, Right);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(5, 4, Up);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(5, 5, Up);
    level.AssertMoveFails(Left);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(5, 5, Right);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(5, 5, Down);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(4, 5, Down);
  }

  MAKE_SYMMETRICAL_TEST(CarrySausageUpLadder) {
    TestSymmetryHelper level(symmetry, LevelType(10, 7, "arena",
    "          "
    " __c____ A"
    " __c111_ A"
    " >__121_ B"
    " ___111_ B"
    " ____U__  "
    "          "));
    level.AssertPosition(1, 3, Right);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(1, 3, Up);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(1, 2, Up);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(1, 1, Up);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(1, 1, Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(2, 1, Right);
    level.AssertSausage({3, 1, 3, 2, 0, Sausage::None});
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(2, 2, Right);
    level.AssertSausageMoved(Down);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(2, 3, Right);
    level.AssertSausageMoved(Down);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(2, 4, Right);
    level.AssertSausageMoved(Down);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(2, 5, Right);
    level.AssertSausageMoved(Down);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(3, 5, Right);
    level.AssertSausageMoved(Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(4, 5, Right);
    level.AssertSausageMoved(Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(5, 5, Right);
    level.AssertSausageMoved(Right);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(5, 4, Right);
    level.AssertSausage({6, 4, 6, 5, 1, Sausage::None});
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(6, 4, Right);
    level.AssertSausageMoved(Right);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(6, 3, Right);
    level.AssertSausageMoved(Up);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(6, 2, Right);
    level.AssertSausageMoved(Up);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(5, 2, Right);
    level.AssertSausageMoved(Left);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(4, 2, Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(5, 2, Right);
    level.AssertSausage({7, 2, 7, 3, 0, Sausage::Rolled});
  }

  MAKE_SYMMETRICAL_TEST(ParallelSausages) {
    TestSymmetryHelper level(symmetry, LevelType(10, 7, "arena",
      "          "
      " _______ A"
      " _______ A"
      " >_bb___  "
      " __1____  "
      " __U____  "
      "          ",
      {}, {}, { Sausage{3, 3, 4, 3, 1} }));
    // Two horizontal sausages stacked directly (z0 + z1), a parallel pair. Push east along their long axis:
    // the pair SLIDES one cell, top carried with bottom, no double-move. (== commented 1a StackedParallelSlide.)
    level.AssertPosition(1, 3, Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(2, 3, Right);
    level.AssertSausage({4, 3, 5, 3, 0, Sausage::None});
    level.AssertSausage({4, 3, 5, 3, 1, Sausage::None});
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(2, 3, Up);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(2, 3, Left);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(3, 3, Left);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(3, 3, Up);
    // Push the parallel pair across its short axis: both ROLL one cell together (Rolled toggles on each).
    // (== commented 1b StackedParallelRoll.)
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(3, 3, Right);
    level.AssertSausage({4, 4, 5, 4, 0, Sausage::Rolled});
    level.AssertSausage({4, 4, 5, 4, 1, Sausage::Rolled});
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(2, 3, Right);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(2, 3, Up);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(2, 4, Up);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(2, 5, Up);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(2, 5, Left);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(3, 5, Left);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(3, 4, Left);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(4, 4, Left);
    level.AssertSausageMoved(Right);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(3, 4, Left);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(3, 5, Left);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(4, 5, Left);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(4, 5, Down);
    // Top now sits one cell east of bottom -> a staircase stack. Sliding it horizontally keeps the offset.
    // (== commented 2a StackedStaircaseSlide.)
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(4, 4, Down);
    level.AssertSausage({4, 3, 5, 3, 0, Sausage::None});
    level.AssertSausage({5, 3, 6, 3, 1, Sausage::None});
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(4, 4, Right);
    // Staircase pair rolled together, staying offset and both flipping Rolled. (== commented 2b StackedStaircaseRoll.)
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(4, 4, Up);
    level.AssertSausage({4, 2, 5, 2, 0, Sausage::Rolled});
    level.AssertSausage({5, 2, 6, 2, 1, Sausage::Rolled});
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(4, 4, Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(5, 4, Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(6, 4, Right);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(6, 4, Down);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(6, 3, Down);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(6, 2, Down);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(6, 2, Right);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(5, 2, Right);
    level.AssertSausage({3, 2, 4, 2, 0, Sausage::Rolled});
    level.AssertSausage({4, 2, 5, 2, 1, Sausage::Rolled});
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(4, 2, Right);
    level.AssertSausage({2, 2, 3, 2, 0, Sausage::Rolled});
    level.AssertSausage({3, 2, 4, 2, 1, Sausage::Rolled});
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(3, 2, Right);
    level.AssertSausage({1, 2, 2, 2, 0, Sausage::Rolled});
    level.AssertSausage({2, 2, 3, 2, 1, Sausage::Rolled});
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(3, 2, Up);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(3, 1, Up);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(3, 1, Left);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(2, 1, Left);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(2, 1, Down);
    level.AssertSausage({1, 3, 2, 3, 0, Sausage::None});
    level.AssertSausage({2, 3, 3, 3, 1, Sausage::None});
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(2, 2, Down);
    level.AssertSausage({1, 4, 2, 4, 0, Sausage::Rolled});
    level.AssertSausage({2, 4, 3, 4, 1, Sausage::Rolled});
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(2, 2, Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(3, 2, Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(4, 2, Right);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(4, 2, Up);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(4, 3, Up);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(4, 4, Up);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(4, 5, Up);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(4, 5, Right);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(3, 5, Right);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(3, 4, Right);
    level.AssertSausage({2, 3, 3, 3, 0, Sausage::None});
  }

  MAKE_SYMMETRICAL_TEST(SausageHat) {
    TestSymmetryHelper level(symmetry, LevelType(10, 7, "arena",
      "         A"
      " _______ A"
      " _______ B"
      " >__?2__ B"
      " _______  "
      " _______  "
      "          ",
      {}, {}, { Sausage{1, 3, 2, 3, 1} }, { Tile::Over2 }));
    level.AssertPosition(1, 3, Right);
    level.AssertSausage({1, 3, 2, 3, 1, Sausage::None});
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(2, 3, Right);
    level.AssertSausageMoved(Right);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(2, 3, Down);
    level.AssertSausage({2, 3, 2, 4, 1, Sausage::None});
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(2, 3, Left);
    level.AssertSausage({1, 3, 2, 3, 1, Sausage::Swapped});
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(3, 3, Left);
    level.AssertSausageMoved(Right);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(3, 3, Up);
    level.AssertSausage({3, 2, 3, 3, 1, Sausage::Swapped});
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(3, 3, Right);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(2, 3, Right);
    level.AssertSausageMoved(Left);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(2, 3, Down);
    level.AssertSausage({2, 3, 3, 3, 1, Sausage::None});
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(2, 3, Left);
    level.AssertSausage({2, 3, 2, 4, 1, Sausage::None});
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(3, 3, Left);
    level.AssertSausageMoved(Right);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(4, 3, Left);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(3, 3, Left);
    level.AssertSausageMoved(Left, Sausage::Rolled);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(2, 3, Left);
    level.AssertSausageMoved(Left, Sausage::Rolled);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(1, 3, Left);
    level.AssertSausageMoved(Left, Sausage::Rolled);
    level.AssertMoveFails(Up);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(2, 3, Left);
    level.AssertSausageMoved(Right, Sausage::Rolled);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(2, 3, Down);
    level.AssertSausage({1, 3, 1, 4, 0, Sausage::None});
  }

  MAKE_SYMMETRICAL_TEST(SausageDoubleHat) {
    TestSymmetryHelper level(symmetry, LevelType(10, 7, "arena",
      "         A"
      " _______ A"
      " _______  "
      " >__?2__  "
      " _______  "
      " _______  "
      "          ",
      {}, {}, { Sausage{1, 3, 2, 3, 1}, Sausage{1, 3, 2, 3, 2} }, { Tile::Over2 }));
    level.AssertPosition(1, 3, Right);
    level.AssertSausage({1, 3, 2, 3, 1, Sausage::None});
    level.AssertSausage({1, 3, 2, 3, 2, Sausage::None});
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(2, 3, Right);
    level.AssertSausage({2, 3, 3, 3, 1, Sausage::None});
    level.AssertSausage({2, 3, 3, 3, 2, Sausage::None});
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(2, 3, Down);
    level.AssertSausage({2, 3, 2, 4, 1, Sausage::None});
    level.AssertSausage({2, 3, 2, 4, 2, Sausage::None});
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(2, 3, Left);
    level.AssertSausage({1, 3, 2, 3, 1, Sausage::Swapped});
    level.AssertSausage({1, 3, 2, 3, 2, Sausage::Swapped});
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(3, 3, Left);
    level.AssertSausage({2, 3, 3, 3, 1, Sausage::Swapped});
    level.AssertSausage({2, 3, 3, 3, 2, Sausage::Swapped});
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(3, 3, Up);
    level.AssertSausage({3, 2, 3, 3, 1, Sausage::Swapped});
    level.AssertSausage({3, 2, 3, 3, 2, Sausage::Swapped});
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(3, 3, Right);
    // (turn only -- the stack pivots as a unit; both halves are asserted at the next pose)
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(2, 3, Right);
    level.AssertSausage({2, 2, 2, 3, 1, Sausage::Swapped});
    level.AssertSausage({2, 2, 2, 3, 2, Sausage::Swapped});
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(2, 3, Down);
    level.AssertSausage({2, 3, 3, 3, 1, Sausage::None});
    level.AssertSausage({2, 3, 3, 3, 2, Sausage::None});
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(2, 3, Left);
    level.AssertSausage({2, 3, 2, 4, 1, Sausage::None});
    level.AssertSausage({2, 3, 2, 4, 2, Sausage::None});
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(3, 3, Left);
    level.AssertSausage({3, 3, 3, 4, 1, Sausage::None});
    level.AssertSausage({3, 3, 3, 4, 2, Sausage::None});
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(4, 3, Left);
    // (transfer onto the overhang -- unasserted in the single-hat original; both halves asserted next)
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(3, 3, Left);
    level.AssertSausage({2, 3, 2, 4, 1, Sausage::Rolled});
    level.AssertSausage({2, 3, 2, 4, 2, Sausage::Rolled});
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(2, 3, Left);
    level.AssertSausage({1, 3, 1, 4, 1, Sausage::None});
    level.AssertSausage({1, 3, 1, 4, 2, Sausage::None});
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(1, 3, Left);
    level.AssertSausage({0, 3, 0, 4, 1, Sausage::Rolled});
    level.AssertSausage({0, 3, 0, 4, 2, Sausage::Rolled});
    level.AssertMoveFails(Up);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(2, 3, Left);
    level.AssertSausage({1, 3, 1, 4, 1, Sausage::None});
    level.AssertSausage({1, 3, 1, 4, 2, Sausage::None});
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(2, 3, Down);
    level.AssertSausage({1, 3, 1, 4, 0, Sausage::None});
    level.AssertSausage({1, 3, 1, 4, 1, Sausage::None});
  }

};
