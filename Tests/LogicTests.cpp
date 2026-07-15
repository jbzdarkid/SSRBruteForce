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

  // 3-3 Cold Escarpment: a turn must not disturb a sausage merely perched on the fork's DESTINATION. A horizontal hat
  // spans a vertical base (its west end, at the turn's corner) and open air over the cell the fork swings into (its
  // east end), with a Wall2 just north of that east end. Turning west->north, the corner-sweep shoves the base out
  // north and tries to carry the hat with it, but the wall blocks the carry -- so the hat simply stays perched on the
  // fork tip at z=1; it is NOT speared/tipped down to the ground. (The old engine wrongly captured it during the
  // fork-dest sweep; Level2 keeps it put.)
  MAKE_SYMMETRICAL_TEST(TurnLeavesForkTipHatPerched) {
    TestSymmetryHelper level(symmetry, LevelType(10, 7, "arena",
      "          "
      "    _    _"
      "    _2   _"
      "    __    "
      "    _<    "
      "          "
      "          ",
      {}, {}, { Sausage{4, 3, 5, 3, 1}, Sausage{4, 2, 4, 3, 0}, Sausage{9, 1, 9, 2, 0} }));
    level.AssertPosition(5, 4, Left);
    level.AssertSausage({4, 3, 5, 3, 1, Sausage::None});          // hat: west end on the base, east end over the fork-dest
    level.AssertSausage({4, 2, 4, 3, 0, Sausage::None});          // vertical base at the turn's corner
    level.AssertMoveSucceeds(Up);                                 // turn west->north
    level.AssertPosition(5, 4, Up);                               // body stays; fork swings north to (5,3)
    level.AssertSausage({4, 3, 5, 3, 1, Sausage::None});          // hat UNCHANGED -- perched on the fork tip (carry wall-blocked)
    level.AssertSausage({4, 1, 4, 2, 0, Sausage::None});          // base swept one cell north
  }

  // 3-3 Cold Escarpment: backing up while speared is a DRAG, not an unspear, so the carry is unchanged -- a sausage on
  // Stephen's head rides along with him even though the base he drags slides in directly underneath it. Stephen faces
  // west with his fork speared in vertical base b; vertical hat a sits on his head (south end cantilevered). He presses
  // east (backs up): b is dragged east to right under a, but a's support (his head) never changed, so a rides east with
  // him rather than being deposited onto the passing base.
  MAKE_SYMMETRICAL_TEST(DragUnderHeadHatKeepsCarrying) {
    TestSymmetryHelper level(symmetry, LevelType(10, 7, "arena",
      "          "
      "         _"
      "         _"
      "   _<_    "
      "   ___    "
      "          "
      "          ",
      {}, {}, { Sausage{4, 3, 4, 4, 1}, Sausage{3, 3, 3, 4, 0}, Sausage{9, 1, 9, 2, 0} }));
    level.AssertPosition(4, 3, Left);
    level.AssertSausage({4, 3, 4, 4, 1, Sausage::None});          // hat on Stephen's head
    level.AssertSausage({3, 3, 3, 4, 0, Sausage::None});          // speared base
    level.AssertMoveSucceeds(Right);                              // back up east, dragging the speared base
    level.AssertPosition(5, 3, Left);                             // body backs to (5,3), still facing west; fork at (4,3)
    level.AssertSausage({5, 3, 5, 4, 1, Sausage::None});          // hat RIDES east with him (carry unchanged)
    level.AssertSausage({4, 3, 4, 4, 0, Sausage::None});          // base dragged east, ending right under the hat's old spot
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

  // 3-14 Cold Frustration move 86, stripped to the critical geometry: a horizontal rider bridges a vertical base (its
  // west end) and Stephen's fork (its east end, over a grill). Pressing forward steps Stephen's body onto that grill so
  // he BOUNCES straight back; the base -- jammed against the Wall1 to its west so it can't be pushed -- is speared and
  // dragged one cell east by the recoil onto the grill column (cooking both ends), and the rider rides east off the
  // fork onto Stephen's head. Stephen ends where he started.
  MAKE_SYMMETRICAL_TEST(RiderCarriedByMovingBaseOffFork) {
    TestSymmetryHelper level(symmetry, LevelType(10, 7, "arena",
      "          "
      " _____    "
      " _____    "
      " _1_#<   A"
      " ___#_   A"
      " _____    "
      "          ",
      {}, {}, { Sausage{3, 3, 3, 4, 0}, Sausage{3, 3, 4, 3, 1, Sausage::Rolled} }));
    level.AssertPosition(5, 3, Left);
    level.AssertSausage({3, 3, 3, 4, 0, Sausage::None});                       // vertical base
    level.AssertSausage({3, 3, 4, 3, 1, Sausage::Rolled});                     // rider: west end on base, east end on fork
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(5, 3, Left);                                          // Stephen bounced back to (5,3)
    level.AssertSausage({4, 3, 4, 4, 0, Sausage::Flags(Sausage::Cook1A | Sausage::Cook2A)}); // base dragged east onto the grill, cooked
    level.AssertSausage({4, 3, 5, 3, 1, Sausage::Rolled});                     // rider carried east onto Stephen's head
  }

  // 4-2 Toad's Folly move 72: Stephen climbs a Left-ladder up the east face of a Wall2 (the ladder auto-extends to the
  // wall's height), and a horizontal sausage resting on that wall-top -- one end on the Wall2, the other cantilevered
  // over the ladder cell -- is directly in the rising body's path. The reference LIFTS the sausage up one level as he
  // climbs (it becomes a hat on his head), then when he steps off the ladder top the hat would carry west but its
  // destination is a Wall5, so the carry is blocked and it stays put. Net: Stephen ends on the wall-top with the
  // sausage riding one level higher at the same (x,y). Stephen starts one cell south of the ladder (so his '^' shows in
  // the grid) and steps north onto the base rung into that start pose before climbing.
  MAKE_SYMMETRICAL_TEST(ClimbLadderLiftsRestingSausage) {
    TestSymmetryHelper level(symmetry, LevelType(10, 7, "arena",
      "          "
      " _____   A"
      " _2L__   A"
      " 52L__   B"
      " __^__   B"
      " _____    "
      "          ",
      {}, {}, { Sausage{2, 3, 3, 3, 2} }));
    level.AssertPosition(3, 4, Up);
    level.AssertSausage({2, 3, 3, 3, 2, Sausage::None});
    level.AssertMoveSucceeds(Up); // step north onto the ladder's bottom rung, into the start pose
    level.AssertPosition(3, 3, Up);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(2, 3, Up);                    // climbed to the Wall2 top, stepped one cell west
    level.AssertSausage({2, 3, 3, 3, 3, Sausage::None}); // lifted one level, same footprint (west carry wall-blocked)
  }

  // 4-2 Toad's Folly move 81: Stephen turns (facing East -> North) with a vertical sausage hatted on his head. The hat
  // pivots to horizontal, and its far end sweeps into a second sausage resting one cell east; the reference shoves that
  // sausage the way the far end is travelling (north), where it rolls off a Wall1 ledge and drops to the ground.
  MAKE_SYMMETRICAL_TEST(HatRotationShovesSausage) {
    TestSymmetryHelper level(symmetry, LevelType(10, 7, "arena",
      "          "
      " _____   A"
      " _____   A"
      " _____    "
      " __>_1    "
      " _____    "
      "          ",
      {}, {}, { Sausage{3, 4, 3, 5, 1}, Sausage{4, 4, 5, 4, 1} }));
    level.AssertPosition(3, 4, Right);
    level.AssertSausage({3, 4, 3, 5, 1, Sausage::None}); // vertical hat
    level.AssertSausage({4, 4, 5, 4, 1, Sausage::None}); // sausage resting east, one end on the Wall1
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(3, 4, Up);                      // Stephen turned in place
    level.AssertSausage({3, 4, 4, 4, 1, Sausage::None}); // hat pivoted to horizontal
    level.AssertSausage({4, 3, 5, 3, 0, Sausage::Rolled}); // shoved north, rolled off the ledge, dropped a level
  }

  // 4-4 Foul Fen move 34: Stephen (facing East, on a Wall2 top) presses South to climb DOWN a back-facing ladder onto
  // a lower ledge, with a sausage resting on top of his fork. The reference carries that fork-borne sausage down with
  // him (it slides one cell and drops a level, staying on the fork). Level2's descent only carried a *speared* sausage,
  // so the fork-borne one was left to fall straight down instead.
  MAKE_SYMMETRICAL_TEST(DescendLadderCarriesForkSausage) {
    TestSymmetryHelper level(symmetry, LevelType(10, 7, "arena",
      "          "
      " __1__   A"
      " _21__   A"
      " _11__   B"
      " _____   B"
      " _____    "
      "          ",
      Stephen{2, 2, 2, Right}, { Ladder{2, 3, 1, Up} },
      { Sausage{3, 1, 3, 2, 3} }));
    level.AssertPosition(2, 2, Right);
    level.AssertSausage({3, 1, 3, 2, 3, Sausage::None}); // resting on top of the fork
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(2, 3, Right);                   // descended one level, one cell south
    level.AssertSausage({3, 2, 3, 3, 2, Sausage::None}); // rode the fork down (slid south, dropped a level)
  }

  // 4-6 Gator Paddock move 84, stripped to the critical geometry: Stephen faces East with a vertical sausage hatted on
  // his head. Pressing East steps his body onto the grill ahead so he bounces straight back; the hat's forward carry is
  // blocked by the Over2Grill overhang (solid at head height, z1) to the NE, so the recoil rolls it one cell west off
  // his head, where it drops to the ground.
  MAKE_SYMMETRICAL_TEST(HatRollsOffDuringGrillBounce) {
    TestSymmetryHelper level(symmetry, LevelType(10, 7, "arena",
      "          "
      " ___?_   A"
      " __>#_   A"
      " _____   B"
      " _____   B"
      " _____    "
      "          ",
      {}, {},
      { Sausage{3, 1, 3, 2, 1} },
      { Tile::Over2Grill }));
    level.AssertPosition(3, 2, Right);
    level.AssertSausage({3, 1, 3, 2, 1, Sausage::None});
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(3, 2, Right);                    // bounced back off the grill
    level.AssertSausage({2, 1, 2, 2, 0, Sausage::Rolled}); // hat rolled one cell west and dropped a level
  }

  // 4-4 Foul Fen (off-path divergence found by DiffEngines), stripped to the critical geometry: Stephen stands on a
  // Wall2 top with a sausage speared on his fork (its far end braced on the neighbouring Wall2). Pressing South tries
  // to climb down the back-facing ladder below him, but lowering the speared sausage would drive it into the Wall2
  // column beneath the fork, so the whole move is refused. Level2 used to lower the sausage straight through the wall
  // because the descent only wall-checked the fork, not what it carries.
  MAKE_SYMMETRICAL_TEST(SpearedSausageBlocksLadderDescent) {
    TestSymmetryHelper level(symmetry, LevelType(10, 7, "arena",
      "          "
      " _2___   A"
      " 22___   A"
      " 1____   B"
      " _____   B"
      " _____    "
      "          ",
      Stephen{1, 2, 2, Right}, { Ladder{1, 3, 1, Up} },
      { Sausage{2, 1, 2, 2, 2} }));
    level.AssertPosition(1, 2, Right);
    level.AssertSausage({2, 1, 2, 2, 2, Sausage::None});
    level.AssertMoveFails(Down); // lowering the speared sausage into the wall column -- refused
  }

  // 4-4 Foul Fen (off-path divergence), stripped to the critical geometry: a sausage hatted on Stephen's head whose FAR
  // end rests on a Wall2 top (not open space) is anchored -- when Stephen turns, the hat stays put rather than pivoting.
  // Level2 used to pivot it because its "clean hat" test only rejected a far end resting on another sausage, not one
  // resting on wall terrain.
  MAKE_SYMMETRICAL_TEST(WallAnchoredHatDoesNotRotate) {
    TestSymmetryHelper level(symmetry, LevelType(10, 7, "arena",
      "          "
      " _2___   A"
      " _1___   A"
      " _____   B"
      " _____   B"
      " _____    "
      "          ",
      Stephen{2, 2, 1, Left}, {},
      { Sausage{2, 1, 2, 2, 2} }));
    level.AssertPosition(2, 2, Left);
    level.AssertSausage({2, 1, 2, 2, 2, Sausage::None});
    level.AssertMoveSucceeds(Down); // turn west -> south
    level.AssertPosition(2, 2, Down);                    // Stephen turned in place
    level.AssertSausage({2, 1, 2, 2, 2, Sausage::None}); // wall-anchored hat stays put
  }

  // 4-4 Foul Fen (off-path divergence): Stephen walks toward the grid's edge and his fork carries a sausage OFF the
  // edge, out over the void. It hangs there, held up by the fork -- Level2 used to refuse the move because its support
  // check dismissed the off-grid cell before noticing the fork beneath the sausage.
  MAKE_SYMMETRICAL_TEST(ForkSausageHangsOffEdge) {
    TestSymmetryHelper level(symmetry, LevelType(10, 7, "arena",
      "          "
      "      ___1"
      "      ___1"
      "      __11"
      "      ____"
      " AABB     "
      "          ",
      Stephen{8, 3, 1, Right}, {},
      { Sausage{9, 2, 9, 3, 2} }));
    level.AssertPosition(8, 3, Right);
    level.AssertSausage({9, 2, 9, 3, 2, Sausage::None});
    level.AssertMoveSucceeds(Right); // walk toward the edge; the fork carries the sausage off it
    level.AssertPosition(9, 3, Right);                     // stepped to the last column, fork now off-grid
    level.AssertSausage({10, 2, 10, 3, 2, Sausage::Rolled}); // sausage rolled off the edge, hanging on the fork
  }

  // 4-4 Foul Fen (off-path divergence): a "bridge" sausage spans Stephen's head and fork. He climbs a rung of the
  // ladder in his cell and steps off onto a Wall2 top; the bridge sausage rides up and North with him. Because it is
  // fork-borne (not a genuine head hat, which the fork disqualifies), the carry across its long axis ROLLS it. Level2
  // used to translate the carried sausage rigidly, leaving the Rolled flag unset.
  MAKE_SYMMETRICAL_TEST(ClimbLadderRollsBridgeSausage) {
    TestSymmetryHelper level(symmetry, LevelType(10, 7, "arena",
      "          "
      " _____   A"
      " _21__   A"
      " _11__   B"
      " _____   B"
      " _____    "
      "          ",
      Stephen{2, 3, 1, Right}, { Ladder{2, 3, 1, Up} },
      { Sausage{2, 3, 3, 3, 2} }));
    level.AssertPosition(2, 3, Right);
    level.AssertSausage({2, 3, 3, 3, 2, Sausage::None}); // bridge: west end on head, east end on fork
    level.AssertMoveSucceeds(Up); // climb the rung, step off North onto the Wall2 top
    level.AssertPosition(2, 2, Right);                    // rose a level, stepped one cell north; still facing east
    level.AssertSausage({2, 2, 3, 2, 3, Sausage::Rolled}); // rode up and north, rolling across its long axis
  }

  // 4-5 Crunchy Leaves (DiffEngines divergence): Stephen climbs a ladder carrying a GENUINE head hat (a vertical
  // sausage on his head, its south end cantilevered -- NOT fork-borne) which itself carries a horizontal rider perched
  // on its head end and cantilevered east. As he climbs and steps north, the head hat rides rigidly (it moves along its
  // own axis anyway), but the rider is carried across its long axis, so it ROLLS. Level2 used to translate the whole
  // head-hat stack rigidly (rollMask covered only the fork stack), leaving the rider's Rolled flag unset.
  MAKE_SYMMETRICAL_TEST(ClimbLadderRollsHeadHatRider) {
    TestSymmetryHelper level(symmetry, LevelType(10, 7, "arena",
      "          "
      "          "
      "  2       "
      " 11       "
      "  _       "
      "        _ "
      "        _ ",
      Stephen{2, 3, 1, Left}, { Ladder{2, 3, 1, Up} },
      { Sausage{2, 3, 2, 4, 2}, Sausage{2, 4, 3, 4, 3}, Sausage{8, 5, 8, 6, 0} }));
    level.AssertPosition(2, 3, Left);
    level.AssertSausage({2, 3, 2, 4, 2, Sausage::None}); // head hat: north end on head, south end cantilevered
    level.AssertSausage({2, 4, 3, 4, 3, Sausage::None}); // rider: west end on the head hat, east end cantilevered
    level.AssertMoveSucceeds(Up); // climb the rung and step off north onto the Wall2 top
    level.AssertPosition(2, 2, Left);                     // rose a level, stepped one cell north; still facing west
    level.AssertSausage({2, 2, 2, 3, 3, Sausage::None});   // head hat rode up+north along its own axis -- no roll
    level.AssertSausage({2, 3, 3, 3, 4, Sausage::Rolled}); // rider carried across its axis -- it rolls
  }

  // 4-2 Toad's Folly (DiffEngines divergence #1): Stephen stands on a Wall2 top facing NORTH, with a ladder immediately
  // to his EAST (descending the wall's east face) and a horizontal hat on his head whose far end cantilevers west over
  // the neighbouring Wall2 top. Pressing East steps him sideways onto the ladder and climbs him down to the ground; the
  // hat slides one cell east and drops onto the Wall2 top he was standing on, then STAYS there as he continues down,
  // because it now rests on the wall. Level2 used to keep the hat in its carried set for every rung, ramming its west
  // end into the Wall2 column at head height, and so refused the whole descent.
  MAKE_SYMMETRICAL_TEST(DescendLadderDepositsHatOnWall) {
    TestSymmetryHelper level(symmetry, LevelType(10, 7, "arena",
      "          "
      "       AA "
      "          "
      " 22L      "
      "          "
      "       BB "
      "          ",
      Stephen{2, 3, 2, Up}, {},
      { Sausage{1, 3, 2, 3, 3} }));
    level.AssertPosition(2, 3, Up);
    level.AssertSausage({1, 3, 2, 3, 3, Sausage::None}); // hat on head, far (west) end cantilevered over the Wall2 top
    level.AssertMoveSucceeds(Right); // step east onto the ladder and climb down the wall's east face
    level.AssertPosition(3, 3, Up);                      // reached the ground east of the wall, still facing north
    level.AssertSausage({2, 3, 3, 3, 2, Sausage::None}); // hat slid east and was deposited on the Wall2 top he left
  }

  // 4-2 Toad's Folly (DiffEngines divergence #2): a horizontal "bridge" sausage rests across Stephen's head (east end)
  // and his fork (west end) at head height, and a vertical sausage rides on the bridge's head end, cantilevered north
  // over open space. Walking forward carries the bridge one cell (along its own axis, so it doesn't roll) and carries
  // the rider along too -- but the rider is NOT part of the rigid hat (only one of its ends sits on the bridge), so it
  // rolls across its long axis. Level2 used to treat the whole stack as a rigid head hat and left the rider unrolled.
  MAKE_SYMMETRICAL_TEST(HatRiderRollsWhenCarriedAcrossAxis) {
    TestSymmetryHelper level(symmetry, LevelType(10, 7, "arena",
      "          "
      " _____   A"
      " _____   A"
      " _____    "
      " _____    "
      " _____    "
      "          ",
      Stephen{3, 3, 0, Left}, {},
      { Sausage{2, 3, 3, 3, 1}, Sausage{3, 2, 3, 3, 2} }));
    level.AssertPosition(3, 3, Left);
    level.AssertSausage({2, 3, 3, 3, 1, Sausage::None}); // bridge: west end on fork, east end on head
    level.AssertSausage({3, 2, 3, 3, 2, Sausage::None}); // rider: south end on the bridge, north end cantilevered
    level.AssertMoveSucceeds(Left); // walk forward; the bridge and its rider are carried one cell west
    level.AssertPosition(2, 3, Left);
    level.AssertSausage({1, 3, 2, 3, 1, Sausage::None});   // bridge slid west along its own axis -- no roll
    level.AssertSausage({2, 2, 2, 3, 2, Sausage::Rolled}); // rider carried west across its axis -- it rolls
  }

  // 4-3 Sludge Coast (DiffEngines divergence #1): Stephen's fork is speared into a vertical sausage; pressing sideways
  // (a perpendicular press, so no turn while speared) lunges his body onto a grill and drags the speared sausage along,
  // which shoves a neighbouring sausage one cell onto a second grill. The grill then bounces Stephen straight back,
  // pulling the speared sausage back with him -- but the shoved neighbour STAYS where the lunge pushed it (rolled and
  // branded on the grill). Level2 used to discard the whole lunge on the bounce, so the neighbour never moved.
  MAKE_SYMMETRICAL_TEST(SpearedBounceStillShovesNeighbor) {
    TestSymmetryHelper level(symmetry, LevelType(10, 7, "arena",
      "          "
      "   a    BB"
      "   a<     "
      "    #     "
      "    #     "
      "          "
      "          ",
      {}, {},
      { Sausage{3, 3, 4, 3, 0} }));
    level.AssertPosition(4, 2, Left);
    level.AssertSausage({3, 1, 3, 2, 0, Sausage::None}); // speared vertical sausage on the fork
    level.AssertSausage({3, 3, 4, 3, 0, Sausage::None}); // neighbour resting east, one end over the grill
    level.AssertMoveSucceeds(Down); // lunge south onto the grill and bounce back
    level.AssertPosition(4, 2, Left);                    // Stephen recoiled to where he started
    level.AssertSausage({3, 1, 3, 2, 0, Sausage::None}); // speared sausage yanked back unchanged
    level.AssertSausage({3, 4, 4, 4, 0, Sausage::Cook2B | Sausage::Rolled}); // neighbour shoved south, rolled + branded
  }

  // 3-1 Cold Jag (DiffEngines divergence #1): Stephen's fork is speared into a vertical sausage whose far end is pinned
  // against a Wall1, so pressing straight BACKWARD can't drag the sausage -- the fork instead pulls free (unspear) and
  // Stephen backs onto a grill. The grill bounces him forward again, and on the recoil his now-free fork shoves the
  // sausage one cell ahead (rolling it). Level2 used to shortcut a backward-unspear-onto-grill as "nothing moves", so
  // the sausage stayed frozen; the reference completes the step + bounce and shoves it.
  MAKE_SYMMETRICAL_TEST(UnspearBounceShovesSausage) {
    TestSymmetryHelper level(symmetry, LevelType(10, 7, "arena",
      "    BB CC "
      " ________ "
      " ___1a___ "
      " __#>a___ "
      " ________ "
      " ________ "
      "          "));
    level.AssertPosition(4, 3, Right);
    level.AssertSausage({5, 2, 5, 3, 0, Sausage::None});   // speared vertical sausage, far end pinned by the Wall1
    level.AssertMoveSucceeds(Left);                        // press backward: unspear, step onto the grill, bounce back
    level.AssertPosition(4, 3, Right);                     // recoiled to where he started
    level.AssertSausage({6, 2, 6, 3, 0, Sausage::Rolled}); // the freed sausage shoved one cell east by the recoil
  }

  // 3-2 Cold Finger (DiffEngines divergence #1): a sausage rides on the fork-tip while a two-high stack sits one cell
  // ahead, its far side pinned by a Wall3. Stepping forward would carry the fork-hat onto the stack, but the stack
  // can't be shoved further (the wall reaches z1), so the hat has nowhere to go -- it's LEFT BEHIND, and as Stephen's
  // body slides under it, it comes to rest on his head. Level2 used to ignore the blocked shove and carry the hat
  // anyway, landing it squarely on top of the stack (two sausages in the same cells -- an impossible overlap).
  MAKE_SYMMETRICAL_TEST(ForkHatLeftBehindWhenBlocked) {
    TestSymmetryHelper level(symmetry, LevelType(8, 6, "arena",
      "        "
      " ____   "
      "3____   "
      "3____   "
      " ____   "
      "        ",
      Stephen{3, 2, 0, Left}, {},
      { Sausage{1, 2, 1, 3, 0}, Sausage{1, 2, 1, 3, 1}, Sausage{2, 2, 2, 3, 1} }));
    level.AssertPosition(3, 2, Left);
    level.AssertSausage({2, 2, 2, 3, 1, Sausage::None}); // fork-hat riding the fork tip
    level.AssertMoveSucceeds(Left);                      // step forward: fork-hat's carry is wall-blocked
    level.AssertPosition(2, 2, Left);                    // Stephen advanced and speared the pinned base
    level.AssertSausage({1, 2, 1, 3, 0, Sausage::None}); // base pinned by the Wall1, unmoved
    level.AssertSausage({1, 2, 1, 3, 1, Sausage::None}); // rider on the base, unmoved
    level.AssertSausage({2, 2, 2, 3, 1, Sausage::None}); // hat left behind, now resting on Stephen's head (no overlap)
  }

  // 3-8 Cold Head (DiffEngines): Stephen is speared into a vertical sausage to his north while a horizontal sausage
  // rides on his HEAD. Pressing backward (south) moonwalks him one cell, dragging the speared base along -- and the
  // head hat must ride with him too. Level2's speared-motion path dragged the base but forgot the head hat, leaving it
  // floating a cell behind.
  MAKE_SYMMETRICAL_TEST(SpearedBackstepCarriesHeadHat) {
    TestSymmetryHelper level(symmetry, LevelType(9, 8, "arena",
      "         "
      "         "
      "  ____   "
      "  ____   "
      "  ____   "
      "  ____   "
      "  ____   "
      "         ",
      Stephen{4, 4, 0, Up}, {},
      { Sausage{4, 2, 4, 3, 0}, Sausage{3, 4, 4, 4, 1}, Sausage{2, 6, 3, 6, 0} }));
    level.AssertPosition(4, 4, Up);
    level.AssertSausage({4, 2, 4, 3, 0, Sausage::None}); // speared vertical base to his north
    level.AssertSausage({3, 4, 4, 4, 1, Sausage::None}); // hat riding his head
    level.AssertMoveSucceeds(Down);                      // moonwalk south, dragging base + head hat
    level.AssertPosition(4, 5, Up);
    level.AssertSausage({4, 3, 4, 4, 0, Sausage::None}); // base dragged one cell south
    level.AssertSausage({3, 5, 4, 5, 1, Sausage::None}); // head hat rode along (rigid, no roll)
  }

  // 3-8 Cold Head (DiffEngines): Stephen rides on top of a vertical sausage and log-rolls it west, while a second
  // sausage balances on his fork-tip a level above. The fork-hat must ride the roll with the fork. Level2's log-roll
  // carried a head hat but not a fork-hat, so the balanced sausage was left floating where it started.
  MAKE_SYMMETRICAL_TEST(LogRollCarriesForkHat) {
    TestSymmetryHelper level(symmetry, LevelType(8, 6, "arena",
      "        "
      "        "
      "  _____ "
      "  _____ "
      "        "
      "        ",
      Stephen{5, 3, 1, Left}, {},
      { Sausage{5, 2, 5, 3, 0}, Sausage{3, 3, 4, 3, 2}, Sausage{2, 2, 3, 2, 0} }));
    level.AssertPosition(5, 3, Left);
    level.AssertSausage({5, 2, 5, 3, 0, Sausage::None}); // the log Stephen rides
    level.AssertSausage({3, 3, 4, 3, 2, Sausage::None}); // sausage balanced on the fork-tip
    level.AssertMoveSucceeds(Right);                     // press east -> log rolls west, Stephen rides
    level.AssertPosition(4, 3, Left);
    level.AssertSausage({4, 2, 4, 3, 0, Sausage::Rolled}); // the log rolled one cell west
    level.AssertSausage({2, 3, 3, 3, 2, Sausage::None});   // fork-hat rode the roll west (slides along its axis)
  }

  // 3-11 Cold Terrace (DiffEngines): Stephen is speared into a horizontal sausage to his west and stands on a Up-ladder.
  // Pressing up climbs a rung, then steps off north -- but that step would ride the rigidly-speared sausage north into a
  // Wall2, which it can't enter, so the reference refuses the whole move. Level2 used to leave the speared sausage
  // behind against the wall (as if it were a loose rider) and complete the climb, reaching a state the game can't.
  MAKE_SYMMETRICAL_TEST(SpearedClimbRefusedWhenBaseHitsWall) {
    TestSymmetryHelper level(symmetry, LevelType(8, 6, "arena",
      "  2 1   "
      "  ___   "
      "  ___   "
      "      __"
      "      __"
      "        ",
      Stephen{4, 1, 0, Left}, { Ladder{4, 1, 0, Up} },
      { Sausage{2, 1, 3, 1, 0}, Sausage{6, 3, 7, 3, 0}, Sausage{6, 4, 7, 4, 0} }));
    level.AssertPosition(4, 1, Left);
    level.AssertSausage({2, 1, 3, 1, 0, Sausage::None}); // speared base to his west, its far end under a Wall2 column
    level.AssertMoveFails(Up);                           // climbing would ride the speared base north into the Wall2
    level.AssertPosition(4, 1, Left);                    // nothing moved
    level.AssertSausage({2, 1, 3, 1, 0, Sausage::None});
  }

  // 3-2 Cold Finger (DiffEngines): Stephen is speared into a vertical base carrying a vertical rider. Stepping forward
  // drags the base east (rigidly) and carries the rider east too -- and the rider slides into a neighbouring sausage
  // (cantilevered on a Wall1). The carry must PROPAGATE as a push: the neighbour is shoved east and, now over open
  // ground, falls a level. Level2 used to let the carried rider overlap the neighbour instead of shoving it.
  MAKE_SYMMETRICAL_TEST(CarriedRiderShovesNeighbor) {
    TestSymmetryHelper level(symmetry, LevelType(10, 6, "arena",
      "          "
      "   _______"
      "   _______"
      "     1____"
      "          "
      "          ",
      Stephen{3, 1, 0, Right}, {},
      { Sausage{4, 1, 4, 2, 0}, Sausage{4, 1, 4, 2, 1}, Sausage{5, 2, 5, 3, 1} }));
    level.AssertPosition(3, 1, Right);
    level.AssertSausage({4, 1, 4, 2, 0, Sausage::None}); // speared base
    level.AssertSausage({4, 1, 4, 2, 1, Sausage::None}); // rider on the base
    level.AssertSausage({5, 2, 5, 3, 1, Sausage::None}); // neighbour, its south end on a Wall1, north end cantilevered
    level.AssertMoveSucceeds(Right);                     // step east: base drags, rider carried into the neighbour
    level.AssertPosition(4, 1, Right);
    level.AssertSausage({5, 1, 5, 2, 0, Sausage::None});   // speared base rode east rigidly (no roll)
    level.AssertSausage({5, 1, 5, 2, 1, Sausage::Rolled}); // rider carried east across its axis -> rolled
    level.AssertSausage({6, 2, 6, 3, 0, Sausage::Rolled}); // neighbour shoved east and dropped a level
  }

  // 3-14 Cold Frustration (DiffEngines, isolated): Stephen is speared into a vertical sausage to his WEST and backs away
  // (presses east). The fork pulls the speared sausage east along with him; it rams a horizontal neighbour, which must
  // be shoved east too. This isolates the backward-drag push-propagation (no grill/bounce).
  MAKE_SYMMETRICAL_TEST(SpearedBackDragShovesNeighbor) {
    TestSymmetryHelper level(symmetry, LevelType(9, 7, "arena",
      "         "
      "         "
      "         "
      "  _____  "
      "  _____  "
      "  __     "
      "         ",
      Stephen{4, 3, 0, Left}, {},
      { Sausage{3, 3, 3, 4, 0}, Sausage{4, 4, 5, 4, 0}, Sausage{2, 5, 3, 5, 0} }));
    level.AssertPosition(4, 3, Left);
    level.AssertSausage({3, 3, 3, 4, 0, Sausage::None}); // speared base to his west
    level.AssertSausage({4, 4, 5, 4, 0, Sausage::None}); // neighbour one cell south-east
    level.AssertMoveSucceeds(Right);                     // back away east: fork drags the speared base into the neighbour
    level.AssertPosition(5, 3, Left);
    level.AssertSausage({4, 3, 4, 4, 0, Sausage::None}); // speared base dragged east (rigid)
    level.AssertSausage({5, 4, 6, 4, 0, Sausage::None}); // neighbour shoved east
  }

  // 3-14 Cold Frustration (DiffEngines): Stephen (unspeared) faces west with a grill directly ahead. Pressing west
  // steps his body onto the grill, spearing the vertical sausage beyond (pinned by a Wall1). The grill bounces him
  // straight back east, and on the recoil his fork drags the speared sausage east into a horizontal neighbour, which
  // must be shoved east too. Reproduces the neighbour-shove specifically in the grill-bounce recoil path.
  MAKE_SYMMETRICAL_TEST(GrillBounceDragShovesNeighbor) {
    TestSymmetryHelper level(symmetry, LevelType(9, 7, "arena",
      "         "
      "         "
      "         "
      "  1_#____"
      "  1______"
      "  __     "
      "         ",
      Stephen{5, 3, 0, Left}, {},
      { Sausage{3, 3, 3, 4, 0}, Sausage{4, 4, 5, 4, 0}, Sausage{2, 5, 3, 5, 0} }));
    level.AssertPosition(5, 3, Left);
    level.AssertSausage({3, 3, 3, 4, 0, Sausage::None});
    level.AssertSausage({4, 4, 5, 4, 0, Sausage::None});
    level.AssertMoveSucceeds(Left);   // step onto grill, spear, bounce back east dragging the base into the neighbour
    level.AssertPosition(5, 3, Left);
    level.AssertSausage({4, 3, 4, 4, 0, Sausage::Cook1A}); // speared base dragged east onto the grill column
    level.AssertSausage({5, 4, 6, 4, 0, Sausage::None});   // neighbour shoved east by the recoil
  }

  // 4-2 Toad's Folly (DiffEngines divergence #1): Stephen climbs a Left-ladder up the east face of a Wall2, carrying a
  // vertical bridge sausage on his head+fork and a horizontal rider resting across the bridge's head end and the Wall2
  // top to the west. He climbs to the wall-top and steps off west; the bridge rolls one cell west onto the wall, but
  // the rider's own west end would ram the Wall5 beyond -- so its carry is BLOCKED and it stays put (the bridge just
  // rolls under it), only rising with the climb. Level2 used to translate the whole carried stack rigidly, with no wall
  // check, and shoved the rider into the Wall5 column.
  MAKE_SYMMETRICAL_TEST(LadderStepOffRiderWallBlocked) {
    TestSymmetryHelper level(symmetry, LevelType(10, 7, "arena",
      "          "
      "          "
      "          "
      "        AA"
      "222L      "
      "252L      "
      "222_      ",
      Stephen{3, 5, 0, Down}, {},
      { Sausage{3, 5, 3, 6, 1}, Sausage{2, 5, 3, 5, 2} }));
    level.AssertPosition(3, 5, Down);
    level.AssertSausage({3, 5, 3, 6, 1, Sausage::None}); // vertical bridge on head + fork
    level.AssertSausage({2, 5, 3, 5, 2, Sausage::None}); // horizontal rider on the bridge + the Wall2 top
    level.AssertMoveSucceeds(Left); // climb the ladder to the wall-top and step off west
    level.AssertPosition(2, 5, Down);                      // ended on the Wall2 top, still facing south
    level.AssertSausage({2, 5, 2, 6, 3, Sausage::Rolled}); // bridge rolled one cell west onto the wall
    level.AssertSausage({2, 5, 3, 5, 4, Sausage::None});   // rider carry wall-blocked -- stayed put, just rose
  }

  // 4-5 Crunchy Leaves (DiffEngines divergence): Stephen descends a sideways ladder while speared into a stack; the
  // descent lowers the carried sausage rung by rung. When a fully-cooked sausage is dragged down onto a grill, pressing
  // a done face back onto the fire would burn it, so the whole descent is refused. Level2 used to never cook the
  // carried sausage during a descent, and so accepted the burning move.
  MAKE_SYMMETRICAL_TEST(SpearedDescentOntoGrillBurns) {
    TestSymmetryHelper level(symmetry, LevelType(10, 7, "arena",
      "          "
      "          "
      "          "
      "    ##__ A"
      "    ##   A"
      "      R1  "
      "          ",
      Stephen{7, 5, 1, Up}, {},
      { Sausage{7, 3, 7, 4, 0, Sausage::Flags(Sausage::Cook1A | Sausage::Cook1B | Sausage::Cook2A | Sausage::Cook2B)},
        Sausage{6, 4, 7, 4, 1, Sausage::Flags(Sausage::Cook1A | Sausage::Cook1B | Sausage::Cook2A | Sausage::Cook2B)} }));
    level.AssertPosition(7, 5, Up);
    level.AssertMoveFails(Left); // sideways descent would drag the fully-cooked sausage onto the grill and burn it -- refused
  }

  // 4-5 Crunchy Leaves (DiffEngines divergence, high-leverage): Stephen stands on a sausage and presses across its long
  // axis, log-rolling it and riding it one cell; a sausage resting on his head rides rigidly with him. Level2 used to
  // carry nothing on his head during a log roll, orphaning the head hat in place.
  MAKE_SYMMETRICAL_TEST(LogRollCarriesHeadHat) {
    TestSymmetryHelper level(symmetry, LevelType(11, 9, "arena",
      "___________"
      "___________"
      "___________"
      "___________"
      "___________"
      "___________"
      "___________"
      "aa_________"
      "___________",
      Stephen{5, 4, 1, Left}, {},
      { Sausage{5, 3, 5, 4, 0}, Sausage{5, 4, 6, 4, 2} }));
    level.AssertMoveSucceeds(Left);                        // press across the support -> log-roll east, riding the sausage
    level.AssertPosition(6, 4, Left);
    level.AssertSausage({6, 3, 6, 4, 0, Sausage::Rolled}); // support rolled east under him
    level.AssertSausage({6, 4, 7, 4, 2, Sausage::None});   // head-hat slid east with him
  }

  // 4-1 Wretch's Retreat (leading bothAcceptDiffer divergence): a horizontal base sausage carries a vertical rider on
  // one end (the rider's other end cantilevers over open ground). Stephen backs into the base, rolling it one cell
  // across its axis; because the base ROLLS and the rider is aligned with the motion with no stationary support, the
  // rider double-moves -- it tumbles TWO cells and, now unsupported, drops to the floor. Level2 used to judge the
  // double-move against Stephen's POST-step pose: since he steps under the rider's end, it wrongly looked "held" and
  // Level2 carried the rider only one cell (still elevated). The reference tests his PRE-step pose, so it tumbles.
  MAKE_SYMMETRICAL_TEST(RolledBaseDoubleMovesRider) {
    TestSymmetryHelper level(symmetry, LevelType(13, 8, "arena",
      "aa___________"
      "_____________"
      "_____________"
      "_____________"
      "_____________"
      "_____________"
      "_____________"
      "_____________",
      Stephen{10, 5, 0, Down}, {},
      { Sausage{10, 4, 11, 4, 0}, Sausage{10, 3, 10, 4, 1} }));
    level.AssertPosition(10, 5, Down);
    level.AssertMoveSucceeds(Up); // back north into the base -> it rolls one cell, the rider tumbles two and drops
    level.AssertPosition(10, 4, Down);
    level.AssertSausage({10, 3, 11, 3, 0, Sausage::Rolled}); // base rolled north one cell
    level.AssertSausage({10, 1, 10, 2, 0, Sausage::None});   // cantilevered rider double-moved two cells and dropped to z0
  }

  // 3-1 Cold Jag (leading divergence): Stephen stands on a support sausage, speared into a rider sausage that is
  // stacked on a THIRD sausage sitting beside the support. Pressing back log-rolls the support, whose roll chains into
  // the third sausage; the speared rider rides rigidly one cell with Stephen. Level2 used to move the rider TWICE --
  // once as a chain rider (which also rolled it) and once as the speared sausage -- landing it a cell too far, rolled.
  MAKE_SYMMETRICAL_TEST(SpearedRiderOnRolledChain) {
    TestSymmetryHelper level(symmetry, LevelType(12, 6, "arena",
      "____________"
      "____________"
      "____________"
      "____________"
      "____________"
      "____________",
      Stephen{10, 2, 1, Left}, {},
      { Sausage{9, 1, 9, 2, 0}, Sausage{10, 1, 10, 2, 0}, Sausage{9, 1, 9, 2, 1} }));
    level.AssertMoveSucceeds(Right);                       // press east -> log-roll the support west, riding it
    level.AssertPosition(9, 2, Left);
    level.AssertSausage({8, 1, 8, 2, 0, Sausage::Rolled}); // chain-pushed base rolled west
    level.AssertSausage({9, 1, 9, 2, 0, Sausage::Rolled}); // Stephen's support rolled west
    level.AssertSausage({8, 1, 8, 2, 1, Sausage::None});   // speared rider rides rigidly west by ONE (not two, not rolled)
  }

  // 3-8 Cold Head (leading divergence): Stephen is speared into a horizontal base sausage carrying a vertical rider on
  // its cantilevered far end. He climbs a sideways ladder, stepping the whole speared stack up onto a Wall1. The base
  // slides along its own axis (no roll), but the rider is carried across ITS axis and rolls. Level2 zeroed the entire
  // roll set whenever a sausage was speared, so the rider climbed the step without ever flipping to its rolled face.
  MAKE_SYMMETRICAL_TEST(SpearedClimbRollsRider) {
    TestSymmetryHelper level(symmetry, LevelType(10, 8, "arena",
      "__________"
      "__________"
      "__________"
      "__________"
      "____1_____"
      "___R1_____"
      "__________"
      "aa________",
      Stephen{3, 5, 0, Up}, {},
      { Sausage{2, 4, 3, 4, 0}, Sausage{2, 4, 2, 5, 1} }));
    level.AssertPosition(3, 5, Up);
    level.AssertMoveSucceeds(Right); // climb the ladder east, stepping the speared stack up onto the wall
    level.AssertPosition(4, 5, Up);
    level.AssertSausage({3, 4, 4, 4, 1, Sausage::None});   // speared base slid east along its axis onto the wall -- no roll
    level.AssertSausage({3, 4, 3, 5, 2, Sausage::Rolled}); // rider carried east across its axis -- it rolls
  }

  // Regression (3-8 Cold Head float bug): a ladder DESCENT must bring a rider DOWN with the speared base it rests on.
  // The reference's Crouch descent skipped CheckForSausageCarry, so the base rolled back down off the wall but the
  // rider was orphaned a level up (left floating at z=2); the fix drops it. Climb east onto the wall then descend back
  // west is a clean no-op -- the rider returns to z=1, never floating.
  MAKE_SYMMETRICAL_TEST(DescendLadderLowersRiderNoFloat) {
    TestSymmetryHelper level(symmetry, LevelType(10, 8, "arena",
      "__________"
      "__________"
      "__________"
      "__________"
      "____1_____"
      "___R1_____"
      "__________"
      "aa________",
      Stephen{3, 5, 0, Up}, {},
      { Sausage{2, 4, 3, 4, 0}, Sausage{2, 4, 2, 5, 1} }));
    level.AssertMoveSucceeds(Right); // climb east onto the wall: the speared base and its rider ride up, supported
    level.AssertSausage({3, 4, 4, 4, 1, Sausage::None});   // base on the wall at z=1
    level.AssertSausage({3, 4, 3, 5, 2, Sausage::Rolled}); // rider one level up at z=2, resting on the base
    level.AssertMoveSucceeds(Left);  // descend the ladder: the base rolls back down AND the rider comes with it
    level.AssertPosition(3, 5, Up);
    level.AssertSausage({2, 4, 3, 4, 0, Sausage::None});   // base back on the ground
    level.AssertSausage({2, 4, 2, 5, 1, Sausage::None});   // rider dropped back to z=1 -- NOT left floating at z=2
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
    level.AssertSausage({1, 3, 2, 3, 1, Sausage::None});
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(3, 3, Left);
    level.AssertSausageMoved(Right);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(3, 3, Up);
    level.AssertSausage({3, 2, 3, 3, 1, Sausage::None});
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
    level.AssertSausage({1, 3, 2, 3, 1, Sausage::None});
    level.AssertSausage({1, 3, 2, 3, 2, Sausage::None});
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(3, 3, Left);
    level.AssertSausage({2, 3, 3, 3, 1, Sausage::None});
    level.AssertSausage({2, 3, 3, 3, 2, Sausage::None});
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(3, 3, Up);
    level.AssertSausage({3, 2, 3, 3, 1, Sausage::None});
    level.AssertSausage({3, 2, 3, 3, 2, Sausage::None});
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(3, 3, Right);
    // (turn only -- the stack pivots as a unit; both halves are asserted at the next pose)
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(2, 3, Right);
    level.AssertSausage({2, 2, 2, 3, 1, Sausage::None});
    level.AssertSausage({2, 2, 2, 3, 2, Sausage::None});
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
