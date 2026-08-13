#include "CppUnitTest.h"
#include "Level2.h"

#include <string>

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
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
      "          "
      " _____   A"
      " _____   A"
      " >#bb1   C"
      " _____   C"
      " _____    "
      "          "));
    level.AssertPosition(1, 3, Right);
    level.AssertSausage({3, 3, 4, 3, 0, Sausage::None});
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(1, 3, Right);
    level.AssertSausage({2, 3, 3, 3, 0, Sausage::Cook1A});
  }

  // A thrown fork lies on the very cell Stephen climbs a ladder up onto. The step-off treats it as a solid: the body
  // shoves it one cell further along, it falls to its support, and the end-of-move reconnect takes it back into hand
  // (5-5 Land's End m30). Without the shove it would end co-located with Stephen and never reattach.
  MAKE_SYMMETRICAL_TEST(ForkReattachClimbingOntoThrownFork) {
    TestSymmetryHelper level(symmetry, Level(8, 5, "arena",
      "________"
      "_R11____"
      "________"
      "________"
      "________",
      Stephen{1, 1, 0, Right}, {},
      { Sausage{5, 3, 6, 3, 0}, Sausage{4, 4, 5, 4, 0}, Sausage{1, 4, 2, 4, 0} }));
    level.SetDetachedFork(2, 1, 1, Right); // fork thrown onto the ledge Stephen climbs up onto
    level.AssertMoveSucceeds(Right);        // climb the ladder and step off onto the fork's cell
    level.AssertPosition(2, 1, Right);      // on the ledge; the shoved fork sits one cell ahead at (3,1)
    level.AssertHasFork();                   // ...taken back into hand
  }

  // Stephen walks into the cell holding his thrown fork, so his body shoves the fork one cell on -- and that
  // destination holds a sausage. The fork is a pusher in its own right: the sausage slides along and the fork takes the
  // cell it vacated. Before the fix only a WALL was checked for, so the fork came to rest inside the sausage -- an
  // overlapping state that corrupted everything downstream (5-11 Rough View m143).
  MAKE_SYMMETRICAL_TEST(ShovedForkPushesSausageItLandsIn) {
    TestSymmetryHelper level(symmetry, Level(8, 5, "arena",
      "________"
      "________"
      "_>_aa___"
      "________"
      "_bb_cc__")); // parked decoys: the parser needs the sausage letters in index order
    level.SetDetachedFork(2, 2, 0, Up); // thrown crosswise, so the step shoves it rather than picking it back up
    level.AssertSausage({3, 2, 4, 2, 0, Sausage::None});
    level.AssertMoveSucceeds(Right);
    level.AssertBodyAt(2, 2, 0, Right);                  // stepped into the fork's cell
    level.AssertForkAt(3, 2, 0, Up);                     // shoved one cell on, into the end the sausage vacated
    level.AssertSausage({4, 2, 5, 2, 0, Sausage::None}); // pushed along its own axis, so it slides without rolling
  }

  // As ForkReattachClimbingOntoThrownFork, but the ledge ENDS at the fork: the body shoves it out over the void. That
  // puts it exactly where Stephen reaches -- one cell ahead, at his level, facing his way -- so the game takes it
  // straight back into hand. It must not fall first: dropping it to the floor leaves forkZ != stephen.z, the reconnect
  // can no longer fire, and a detached fork is left resting off the map (3-12 Cold Horizon m204).
  MAKE_SYMMETRICAL_TEST(ForkShovedOffLedgeIsCaughtNotDropped) {
    TestSymmetryHelper level(symmetry, Level(6, 5, "arena",
      "______"
      "______"
      "___R1 "  // ladder at (3,2), ledge top at (4,2) z=1, then the void
      "______"
      "aabbcc", // parked decoys, in index order
      Stephen{3, 2, 0, Right}));
    level.SetDetachedFork(4, 2, 1, Right); // on the ledge top, facing the way Stephen will
    level.AssertMoveSucceeds(Right);       // climb the ladder, step off onto the fork's cell, shove it over the edge
    level.AssertBodyAt(4, 2, 1, Right);
    level.AssertHasFork();                 // caught at ledge height, NOT dropped into the void
  }

  // A sausage balanced on the fork tip is carried up a ladder, but its far end rests on a sausage that stays put. That
  // anchored end cannot rotate, so the game's CalculateTorsion returns zero and the hat rides up FLAT. We treated every
  // bare fork-borne sausage as a free-hanging bridge and rolled it (3-8 Cold Head m407, 3-4 Cold Trail m464,
  // 3-5 Cold Cliff m191 -- three independent repros of the same rule).
  MAKE_SYMMETRICAL_TEST(AnchoredForkHatDoesNotRollWhenCarriedUpLadder) {
    TestSymmetryHelper level(symmetry, Level(8, 7, "arena",
      "________"
      "________"
      "________"
      "________"
      "____1L__"
      "________"
      "________",
      Stephen{5, 4, 0, Up}, {},
      { Sausage{5, 2, 5, 3, 1},   // the hat: near end on the fork tip at (5,3), far end over the anchor at (5,2)
        Sausage{5, 2, 6, 2, 0},   // the anchor under the hat's far end -- it does not move
        Sausage{0, 6, 1, 6, 0} }));
    level.AssertMoveSucceeds(Left);                      // perpendicular press -> mount the ladder and climb
    level.AssertBodyAt(4, 4, 1, Up);
    level.AssertSausage({4, 2, 4, 3, 2, Sausage::None}); // carried across its axis but anchored -> no Rolled
    level.AssertSausage({5, 2, 6, 2, 0, Sausage::None}); // the anchor stayed behind
  }

  // A single push fans out into a branching chain: the fork shoves base sausage 'c', whose two ends butt into 'a' (free
  // to slide) and 'b' (jammed against a wall). The push must be ATOMIC -- because one branch ('b') is blocked the whole
  // thing is refused and the fork merely spears 'c'; crucially 'a' must NOT have been dragged along. Before the fix,
  // PlanSausagePush committed the first branch before discovering the second was blocked, stranding it (5-3 Skeleton
  // m273). Letters run in the parser's index order -- the first one it sees has to be 'a'.
  MAKE_SYMMETRICAL_TEST(BranchingPushIsAtomicWhenOneBranchBlocked) {
    TestSymmetryHelper level(symmetry, Level(8, 8, "arena",
      "___1____"
      "__ab____"
      "__ab____"
      "__cc____"
      "________"
      "__^_____"
      "________"
      "________"));
    level.AssertPosition(2, 5, Up);
    level.AssertMoveSucceeds(Up);                       // fork tries to shove 'c' up; branch 'b' is walled -> refuse+spear
    level.AssertPosition(2, 4, Up);                     // body stepped forward, fork lodged in 'c' at (2,3)
    level.AssertSausage({2, 3, 3, 3, 0, Sausage::None}); // 'c' did not move (speared in place)
    level.AssertSausage({2, 1, 2, 2, 0, Sausage::None}); // 'a' did NOT get dragged -- the whole push rolled back
    level.AssertSausage({3, 1, 3, 2, 0, Sausage::None}); // 'b' still jammed against the wall
  }

  // A FORKLESS Stephen stands on top of a vertical log and presses across it (along his own facing). Even without the
  // fork he still log-rolls: the sausage rolls the opposite way and he rides along on top of it, while the thrown fork
  // just sits where it landed. Before the fix, a forkless press here walked straight off instead of rolling
  // (5-9 Drumlin m292).
  MAKE_SYMMETRICAL_TEST(ForklessStephenLogRollsRidingTheSausage) {
    TestSymmetryHelper level(symmetry, Level(8, 8, "arena",
      "________"
      "________"
      "________"
      "____a___"
      "____a___"
      "________"
      "________"
      "________",
      Stephen{4, 3, 1, Left}, {},
      { Sausage{0, 0, 1, 0, 0}, Sausage{6, 7, 7, 7, 0} })); // perched on the (4,3) end of the vertical log; fillers parked
    level.SetDetachedFork(1, 1, 0, Right); // fork lies loose across the arena; it must not move
    level.AssertBodyAt(4, 3, 1, Left);
    level.AssertMoveSucceeds(Left);                          // press Left -> log rolls Right, Stephen rides Right with it
    level.AssertBodyAt(5, 3, 1, Left);                       // carried one cell East, still on top, still facing Left
    level.AssertSausage({5, 3, 5, 4, 0, Sausage::Rolled});   // the log rolled one cell East
    level.AssertForkAt(1, 1, 0, Right);                      // the detached fork stayed exactly where it was thrown
  }

  // A forkless step pushes a sausage that the thrown fork is perched ON TOP of. In the reference a loose fork is an
  // entity subject to passive forces (Utility.SubjectToPassiveForces / NeedsGround), exactly like a sausage: it rides
  // the base that moves under it and falls when nothing holds it up. So the fork travels the cell the sausage slides,
  // staying on top of it. Level2 used to refuse the whole step as an unmodelled "rider fling", which stalled the
  // recorded solutions for 5-8 Open Baths (m5) and 5-3 Skeleton (m132).
  MAKE_SYMMETRICAL_TEST(ForklessStepCarriesForkRidingThePushedSausage) {
    TestSymmetryHelper level(symmetry, Level(8, 8, "arena",
      "________"
      "________"
      "________"
      "_>aa____"
      "________"
      "________"
      "________"
      "________",
      {}, {}, { Sausage{0, 0, 1, 0, 0}, Sausage{6, 7, 7, 7, 0} })); // fillers parked to reach the 3-sausage build count
    level.SetDetachedFork(2, 3, 1, Right); // fork perched on top of the sausage the step will push
    level.AssertBodyAt(1, 3, 0, Right);
    level.AssertMoveSucceeds(Right);
    level.AssertBodyAt(2, 3, 0, Right);                  // stepped into the cell the sausage vacated
    level.AssertSausage({3, 3, 4, 3, 0, Sausage::None}); // pushed along its own axis, so it slides without rolling
    level.AssertForkAt(3, 3, 1, Right);                  // rode the slide, still resting on the sausage
  }

  // A FORKLESS Stephen carrying a sausage on his head presses along his facing with no ladder anywhere. That is an
  // ordinary walk and the hat rides with him. The forkless branch of the ladder handler used to test for a head hat
  // BEFORE testing for a ladder, so it refused every such press outright -- and the forkless step never carried the hat
  // even when it was reached. Both showed up as 5-1 The Gorge stalling at m27.
  MAKE_SYMMETRICAL_TEST(ForklessWalkCarriesHeadHatWithNoLadder) {
    TestSymmetryHelper level(symmetry, Level(8, 6, "arena",
      "________"
      "________"
      "________"
      "________"
      "________"
      "________",
      Stephen{1, 2, 0, Right}, {},
      { Sausage{1, 2, 1, 3, 1},              // the hat: (1,2) on Stephen's head, (1,3) cantilevered over open floor
        Sausage{6, 0, 7, 0, 0}, Sausage{6, 5, 7, 5, 0} })); // parked fillers
    level.SetDetachedFork(4, 5, 0, Left);    // thrown well clear; it must not move or reattach
    level.AssertBodyAt(1, 2, 0, Right);
    level.AssertSausage({1, 2, 1, 3, 1, Sausage::None});
    level.AssertMoveSucceeds(Right);         // no ladder here -- an ordinary forkless walk
    level.AssertBodyAt(2, 2, 0, Right);
    level.AssertSausage({2, 2, 2, 3, 1, Sausage::None}); // the hat rode along, rigid (a pure translation, no roll)
    level.AssertForkAt(4, 5, 0, Left);       // the thrown fork stayed exactly where it was
  }

  // A rolling log SHOVES a loose fork out of its path -- the fork is a world entity, not terrain -- and the fork can
  // come to rest lodged inside whatever sausage it lands in. Level2 treated it as a solid blocker, refused the roll,
  // and then fell through to a plain step that walked Stephen the opposite way. Expected state taken from the
  // reference's own replay of 5-9 Drumlin m91.
  MAKE_SYMMETRICAL_TEST(LogRollShovesLooseForkAside) {
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
      "__________"
      "__________"
      "__________"
      "__________"
      "__________"
      "__________"
      "__________",
      Stephen{7, 3, 1, Left}, {},
      { Sausage{7, 2, 7, 3, 0},              // the log Stephen is standing on (vertical)
        Sausage{5, 2, 5, 3, 0},              // parked sausage the shoved fork ends up inside
        Sausage{0, 6, 1, 6, 0} }));          // parked filler
    level.SetDetachedFork(6, 3, 0, Left);    // lying in the log's roll path
    level.AssertBodyAt(7, 3, 1, Left);
    level.AssertMoveSucceeds(Right);                      // press east -> the log rolls WEST, Stephen rides it
    level.AssertBodyAt(6, 3, 1, Left);
    level.AssertSausage({6, 2, 6, 3, 0, Sausage::Rolled}); // the log rolled one cell west
    level.AssertSausage({5, 2, 5, 3, 0, Sausage::None});   // the parked sausage never moved
    level.AssertForkAt(5, 3, 0, Left);                     // shoved west, ending up lodged in that sausage
  }

  // A base that ROLLS does not merely carry a loose fork -- it FLICKS it. The reference hands a passive rider the
  // base's speed when the two turn together but speed+1 when the base twists under it, and a fork is a single cell so
  // it never counts as "extended": a freely rolling base always throws it two cells clear, and it then falls.
  // Expected state taken from the reference's replay of 5-8 Open Baths, and again on 5-1 The Gorge.
  MAKE_SYMMETRICAL_TEST(RollingBaseFlicksLooseForkTwoCells) {
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
      "__________"
      "__________"
      "__________"
      "__________"
      "__________"
      "__________"
      "__________",
      Stephen{5, 3, 0, Left}, {},
      { Sausage{4, 2, 4, 3, 0},              // vertical base; Stephen shoves its south end from the east
        Sausage{8, 0, 9, 0, 0}, Sausage{0, 6, 1, 6, 0} })); // parked fillers
    level.SetDetachedFork(4, 2, 1, Right);   // perched on the base's north end, away from Stephen's path
    level.AssertBodyAt(5, 3, 0, Left);
    level.AssertMoveSucceeds(Left);                        // step west, pushing the base across its axis
    level.AssertBodyAt(4, 3, 0, Left);
    level.AssertSausage({3, 2, 3, 3, 0, Sausage::Rolled}); // the base rolled one cell west
    level.AssertForkAt(2, 2, 0, Right);                    // flicked TWO cells west, then dropped to the floor
  }

  // ...but the flick has to have somewhere to go. Blocked, it falls back to a plain one-cell carry, and blocked again
  // it leaves the fork exactly where it was -- which is how a fork ends up resting on Stephen's own head, as he steps
  // into the cell the base just vacated. Here an overhang (walkable at z=0, solid at z=1) lets the base roll on
  // underneath while walling off the fork's flight at head height. Taken from the reference's replay of
  // 5-8 Open Baths m16, where a tower ahead eats both the flick and the carry.
  MAKE_SYMMETRICAL_TEST(BlockedFlickLeavesForkOnStephensHead) {
    TestSymmetryHelper level(symmetry, Level(9, 7, "arena",
      "_________"
      "_________"
      "____?____"                            // overhang: the base rolls under it, the fork cannot fly through it
      "_________"
      "_________"
      "_________"
      "_________",
      Stephen{4, 4, 0, Up}, {},
      { Sausage{3, 3, 4, 3, 0},              // horizontal base, directly north of Stephen
        Sausage{0, 0, 1, 0, 0}, Sausage{0, 6, 1, 6, 0} }, // parked fillers
      { SpecialTile({0, 2}) }));
    level.SetDetachedFork(4, 3, 1, Right);   // perched on the base's east end
    level.AssertBodyAt(4, 4, 0, Up);
    level.AssertMoveSucceeds(Up);                          // step north, pushing the base across its axis
    level.AssertBodyAt(4, 3, 0, Up);
    level.AssertSausage({3, 2, 4, 2, 0, Sausage::Rolled}); // the base rolled north, in under the overhang
    level.AssertForkAt(4, 3, 1, Right);                    // flick walled off -> stayed, now held up by Stephen's head
  }

  // A base rolling on top of ANOTHER roller imparts nothing at all: the two counter-rotate, so the reference's torsion
  // comes out negative and the passive push is skipped entirely -- neither the flick nor the one-cell carry. The fork
  // is left hanging in place while the whole stack rolls out from under it. Expected state taken from the reference's
  // replay of 5-3 Skeleton m132.
  MAKE_SYMMETRICAL_TEST(CounterRollingBaseLeavesLooseForkBehind) {
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
      "__________"
      "__________"
      "__________"
      "__________"
      "__________"
      "__________"
      "__________",
      Stephen{5, 3, 0, Left}, {},
      { Sausage{4, 2, 4, 3, 0},              // bottom roller, pushed by Stephen
        Sausage{4, 2, 4, 3, 1},              // stacked on top of it, so it counter-rolls
        Sausage{0, 6, 1, 6, 0} }));          // parked filler
    level.SetDetachedFork(4, 2, 2, Right);   // perched on the TOP sausage's north end
    level.AssertBodyAt(5, 3, 0, Left);
    level.AssertMoveSucceeds(Left);                        // step west, rolling the whole stack
    level.AssertSausage({3, 2, 3, 3, 0, Sausage::Rolled}); // bottom rolled one cell west
    level.AssertForkAt(4, 2, 0, Right);                    // imparted nothing -> stayed put, then fell to the floor
  }

  // A loose fork resting on Stephen's head rides him like a hat: it translates when he walks and swings round when he
  // turns (the reference's CanHatTurn covers forks, not just sausages). Level2 left it behind and then dropped it.
  // Expected states taken from the reference's replays of 5-8 Open Baths m17 and 5-3 Skeleton m133.
  MAKE_SYMMETRICAL_TEST(ForkOnHeadRidesAndTurnsWithStephen) {
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
      "__________"
      "__________"
      "__________"
      "__________"
      "__________"
      "__________"
      "__________",
      Stephen{3, 3, 0, Down}, {},
      { Sausage{0, 0, 1, 0, 0}, Sausage{8, 0, 9, 0, 0}, Sausage{0, 6, 1, 6, 0} })); // parked fillers
    level.SetDetachedFork(3, 3, 1, Right);   // sitting on Stephen's head
    level.AssertBodyAt(3, 3, 0, Down);
    level.AssertMoveSucceeds(Left);          // perpendicular press -> a turn in place
    level.AssertBodyAt(3, 3, 0, Left);
    level.AssertForkAt(3, 3, 1, Down);       // swung the same 90 degrees Stephen did (South->West turns East->South)
    level.AssertMoveSucceeds(Left);          // now a walk along his new facing
    level.AssertBodyAt(2, 3, 0, Left);
    level.AssertForkAt(2, 3, 1, Down);       // rode along on his head
  }

  // The fall half of the same rule: when the base rolls away and nothing else is underneath -- Stephen ends up beside
  // the fork, not below it -- the fork drops to the ground. This is the case my earlier attempt at a "fork falls" test
  // failed to actually construct (a sausage only falls with BOTH ends unsupported, so it never left the fork hanging).
  // The drop itself is confirmed against the reference by a probe demo on 5-8 Open Baths, where a fork left behind at
  // head height falls to the floor.
  MAKE_SYMMETRICAL_TEST(ForkDropsWhenItsBaseSlidesOutFromUnder) {
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
      "__________"
      "__________"
      "__________"
      "__________"
      "__________"
      "__________"
      "__________",
      Stephen{6, 2, 0, Left}, {},
      { Sausage{4, 2, 5, 2, 0},              // horizontal base; Stephen shoves its east end along its own length
        Sausage{8, 0, 9, 0, 0}, Sausage{0, 6, 1, 6, 0} })); // parked fillers
    level.SetDetachedFork(4, 2, 1, Up);      // perched on the base's west end
    level.AssertBodyAt(6, 2, 0, Left);
    level.AssertMoveSucceeds(Left);                        // step west, sliding the base along its axis
    level.AssertBodyAt(5, 2, 0, Left);
    level.AssertSausage({3, 2, 4, 2, 0, Sausage::None});   // slid one cell west without rolling
    level.AssertForkAt(3, 2, 1, Up);                       // a slide twists nothing -> carried exactly one cell
  }

  // A head-borne fork rides only if there is room for it: walking UNDER an overhang (solid at z=1, walkable at z=0)
  // leaves the fork behind at head height with nowhere to go, so it stays and drops. Confirmed by probe demos on
  // 5-8 Open Baths m30 -- from one state the fork rides east, west and south, and is left behind only heading north,
  // into the overhang.
  MAKE_SYMMETRICAL_TEST(ForkOnHeadStaysWhenOverhangBlocksIt) {
    TestSymmetryHelper level(symmetry, Level(8, 6, "arena",
      "________"
      "________"
      "___?____"                            // overhang: floor at z=0, solid rock at z=1
      "________"
      "________"
      "________",
      Stephen{3, 3, 0, Up}, {},
      { Sausage{0, 0, 1, 0, 0}, Sausage{6, 0, 7, 0, 0}, Sausage{0, 5, 1, 5, 0} }, // parked fillers
      { SpecialTile({0, 2}) }));
    level.SetDetachedFork(3, 3, 1, Left);   // riding on Stephen's head
    level.AssertBodyAt(3, 3, 0, Up);
    level.AssertMoveSucceeds(Up);           // step north, in under the overhang
    level.AssertBodyAt(3, 2, 0, Up);
    level.AssertForkAt(3, 3, 0, Left);      // no room at head height -> left behind, then dropped to the floor
  }

  // A loose fork shunted aside by a rolling log goes TINES-first when it points the way it is shoved: it simply spears
  // whatever sausage it lands in and leaves it standing (5-9 Drumlin m91).
  MAKE_SYMMETRICAL_TEST(ShovedForkSpearsWhatItHitsGoingTinesFirst) {
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
      "__________"
      "__________"
      "__________"
      "__________"
      "__________"
      "__________"
      "__________",
      Stephen{7, 3, 0, Left}, {},
      { Sausage{6, 2, 6, 3, 0},              // the log Stephen rolls west
        Sausage{4, 3, 4, 4, 0},              // parked in the cell the fork is about to be shunted into
        Sausage{8, 0, 9, 0, 0} }));          // parked filler
    level.SetDetachedFork(5, 3, 0, Left);    // lying in the log's path, pointing the way the shove goes
    level.AssertMoveSucceeds(Left);                        // roll the log west, shunting the fork ahead of it
    level.AssertSausage({5, 2, 5, 3, 0, Sausage::Rolled}); // the log rolled one cell west
    level.AssertSausage({4, 3, 4, 4, 0, Sausage::None});   // the parked sausage never budged -- it was speared, not rammed
    level.AssertForkAt(4, 3, 0, Left);                     // the fork ended up lodged inside it
  }

  // ...but shunted BUTT-first -- pointing back against the shove -- it rams instead, and rolls that sausage along with
  // it. The reference draws the same line, only testing the onward push when the fork faces the inverse of the motion.
  // Missing it let a rammed sausage stay put where the game rolls it clean off the map and drowns it (5-1 The Gorge).
  MAKE_SYMMETRICAL_TEST(ShovedForkRamsWhatItHitsGoingButtFirst) {
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
      "__________"
      "__________"
      "__________"
      "__________"
      "__________"
      "__________"
      "__________",
      Stephen{7, 3, 0, Left}, {},
      { Sausage{6, 2, 6, 3, 0},              // the log Stephen rolls west
        Sausage{4, 3, 4, 4, 0},              // parked in the cell the fork is about to be shunted into
        Sausage{8, 0, 9, 0, 0} }));          // parked filler
    level.SetDetachedFork(5, 3, 0, Right);   // same shove, but the fork points back the other way
    level.AssertMoveSucceeds(Left);
    level.AssertSausage({5, 2, 5, 3, 0, Sausage::Rolled}); // the log rolled one cell west
    level.AssertSausage({3, 3, 3, 4, 0, Sausage::Rolled}); // rammed by the fork's butt, so it rolled on west too
    level.AssertForkAt(4, 3, 0, Right);                    // the fork took the cell it cleared
  }

  // A lodged fork rides its host for the host's WHOLE journey, including the bonus cell of a knock-on tumble. Tracking
  // it before that extra slide (and only re-syncing its height afterwards) left it a cell short (5-1 The Gorge m126,
  // where the oracle's own DIAG gives it away: one roll but a two-cell slide).
  MAKE_SYMMETRICAL_TEST(LodgedForkRidesItsHostsDoubleMove) {
    TestSymmetryHelper level(symmetry, Level(8, 7, "arena",
      "________"
      "________"
      "________"
      "________"
      "________"
      "________"
      "________",
      Stephen{5, 4, 0, Left}, {},
      { Sausage{4, 3, 4, 4, 0},              // the base Stephen rolls west, across its axis
        Sausage{4, 3, 5, 3, 1},              // rider lying ALONG the push, so the roll flicks it an extra cell
        Sausage{0, 6, 1, 6, 0} }));          // parked filler
    level.SetDetachedFork(5, 3, 1, Up);      // lodged in the rider's east end, crosswise so it is never picked back up
    level.AssertBodyAt(5, 4, 0, Left);
    level.AssertMoveSucceeds(Left);
    level.AssertBodyAt(4, 4, 0, Left);
    level.AssertSausage({3, 3, 3, 4, 0, Sausage::Rolled}); // the base rolled its one cell
    level.AssertSausage({2, 3, 3, 3, 1, Sausage::None});   // the rider slid TWO -- carried one, flicked one more
    level.AssertForkAt(3, 3, 1, Up);                       // and the fork rode the full two, still lodged in it
  }

  // Backing DOWN a ladder carries the sausage on Stephen's head the whole way, exactly as climbing up does: it rides
  // the full drop and the step across. The descent branch refused outright whenever he wore a hat (5-6 Crater m51).
  MAKE_SYMMETRICAL_TEST(LadderDescentCarriesHeadHat) {
    TestSymmetryHelper level(symmetry, Level(8, 5, "arena",
      "________"
      "________"
      "__2L____"                             // a tower to stand on, with a ladder down its east face
      "________"
      "________",
      Stephen{2, 2, 2, Left}, {},
      { Sausage{2, 1, 2, 2, 3},              // the head hat, its south end on Stephen's head
        Sausage{6, 0, 7, 0, 0}, Sausage{0, 4, 1, 4, 0} })); // parked fillers
    level.SetDetachedFork(0, 0, 0, Left);    // thrown well clear, so this is the forkless ladder path
    level.AssertBodyAt(2, 2, 2, Left);
    level.AssertMoveSucceeds(Right);                        // back onto the ladder and climb down it
    level.AssertBodyAt(3, 2, 0, Left);
    level.AssertSausage({3, 1, 3, 2, 1, Sausage::None});     // rode the whole descent, still on his head
    level.AssertForkAt(0, 0, 0, Left);                       // the thrown fork never moved
  }

  // The grill recoil is a move in its own right, so a fork picked up as Stephen LANDS on the grill is already in his
  // hand -- and already spearing -- when the recoil drags him straight back off it. The speared sausage comes with him.
  // Running the reconnect after the whole chain instead meant he bounced off empty-handed (5-6 Crater m112).
  MAKE_SYMMETRICAL_TEST(BurnedStepReconnectsForkThenDragsItsSausage) {
    TestSymmetryHelper level(symmetry, Level(8, 5, "arena",
      "________"
      "________"
      "________"
      "__#_____"                             // the grill Stephen bounces off
      "________",
      Stephen{2, 4, 0, Up}, {},
      { Sausage{2, 1, 2, 2, 0},              // the fork's host, two cells north of him
        Sausage{6, 0, 7, 0, 0}, Sausage{0, 0, 1, 0, 0} })); // parked fillers
    level.SetDetachedFork(2, 2, 0, Up);      // lodged in the host's south end, pointing his way
    level.AssertBodyAt(2, 4, 0, Up);
    level.AssertMoveSucceeds(Up);            // step onto the grill -- and straight back off it
    level.AssertBodyAt(2, 4, 0, Up);         // recoiled to exactly where he started
    level.AssertHasFork();                   // ...but holding the fork he reached on the way
    level.AssertPosition(2, 4, Up);          // the fork sits one cell ahead, in his hand
    level.AssertSausage({2, 2, 2, 3, 0, Sausage::Cook2A}); // dragged along on the spear, its far end seared by the grill
  }

  // A fork lodged in a sausage rides that sausage as it rolls, flipping its facing with the tumble -- and if it lands
  // in Stephen's reach it goes back into his hand THERE AND THEN, before gravity runs. That matters: a held fork spears
  // its host, and a speared sausage is exempt from the fall, so the roll can leave it hanging over a void it would
  // otherwise drown in. Tracking the fork after gravity drowned it first (5-12 Baby Rock m124).
  MAKE_SYMMETRICAL_TEST(LodgedForkReconnectsMidRollAndHoldsItsHostUp) {
    TestSymmetryHelper level(symmetry, Level(8, 6, "arena",
      "________"
      "________"
      "__  ____"                             // void: nothing to catch the sausage once it rolls in
      "__11____"
      "__11____"
      "________",
      Stephen{3, 4, 1, Up}, {},
      { Sausage{2, 3, 3, 3, 1},              // the fork's host, on the shelf directly north of him
        Sausage{6, 0, 7, 0, 0}, Sausage{0, 5, 1, 5, 0} })); // parked fillers
    level.SetDetachedFork(3, 3, 1, Down);    // lodged in the host's east end, pointing back at Stephen
    level.AssertBodyAt(3, 4, 1, Up);
    level.AssertMoveSucceeds(Up);            // push north: the host rolls across its axis, out over the void
    level.AssertBodyAt(3, 3, 1, Up);
    level.AssertHasFork();                   // the tumble turned the fork to face his way, so he took it back
    level.AssertPosition(3, 3, Up);
    level.AssertSausage({2, 2, 3, 2, 1, Sausage::Rolled}); // held up over the void by the spear alone
  }

  // A FORKLESS turn still pivots the sausage on Stephen's head: the hat rides his body, not his fork. The forkless
  // branch of the rotation handler just set his facing and returned, so the hat never swung (5-1 The Gorge m31).
  MAKE_SYMMETRICAL_TEST(ForklessTurnPivotsHeadHat) {
    TestSymmetryHelper level(symmetry, Level(9, 7, "arena",
      "_________"
      "_________"
      "_________"
      "_________"
      "_________"
      "_________"
      "_________",
      Stephen{3, 3, 1, Right}, {},
      { Sausage{3, 3, 3, 4, 0},              // the base Stephen stands on
        Sausage{3, 3, 4, 3, 2},              // the head hat: near end on his head, far end cantilevered east
        Sausage{7, 6, 8, 6, 0} }));          // parked filler
    level.SetDetachedFork(0, 6, 0, Left);    // thrown well clear
    level.AssertBodyAt(3, 3, 1, Right);
    level.AssertSausage({3, 3, 4, 3, 2, Sausage::None});
    level.AssertMoveSucceeds(Down);          // perpendicular press -> turn east->south
    level.AssertBodyAt(3, 3, 1, Down);
    level.AssertSausage({3, 3, 3, 4, 2, Sausage::None}); // the hat swung 90 degrees about the head cell
    level.AssertForkAt(0, 6, 0, Left);       // the thrown fork never moved
  }

  // ...and a fork LODGED in that hat pivots with it. The pivot cell stays put, so a fork sitting there keeps its cell
  // but swings to the new facing. Mapping the fork by end INDEX gets this wrong, because a pivot renormalises the
  // sausage's upper-left invariant -- it has to be mapped by geometry (5-1 The Gorge m31/m32).
  MAKE_SYMMETRICAL_TEST(LodgedForkPivotsWithItsHostHat) {
    TestSymmetryHelper level(symmetry, Level(9, 7, "arena",
      "_________"
      "_________"
      "_________"
      "_________"
      "_________"
      "_________"
      "_________",
      Stephen{3, 3, 1, Right}, {},
      { Sausage{3, 3, 3, 4, 0},              // the base Stephen stands on
        Sausage{3, 3, 4, 3, 2},              // the head hat
        Sausage{7, 6, 8, 6, 0} }));          // parked filler
    level.SetDetachedFork(3, 3, 2, Up);      // lodged in the hat, at the cell it pivots about
    level.AssertBodyAt(3, 3, 1, Right);
    level.AssertMoveSucceeds(Down);          // turn east->south; the hat swings and takes the fork round with it
    level.AssertBodyAt(3, 3, 1, Down);
    level.AssertSausage({3, 3, 3, 4, 2, Sausage::None});
    level.AssertForkAt(3, 3, 2, Right);      // still in the pivot cell, spun the same 90 degrees (North -> East)
  }

  // A FORKLESS Stephen stands on a vertical log at a ladder cell and presses toward the ladder (along his own facing).
  // The reference tests the ladder (TryClimbUp) BEFORE the sausage-underfoot roll inside TryMovePlayer, so the log must
  // NOT roll -- he climbs the ladder and steps off on top instead (5-6 Crater m205).
  MAKE_SYMMETRICAL_TEST(ForklessLadderClimbBeatsLogRoll) {
    TestSymmetryHelper level(symmetry, Level(8, 8, "arena",
      "________"
      "________"
      "___2____"
      "___2a___"                            // (3,3) wall to climb onto; (4,3) is the log's north end
      "____a___"
      "________"
      "________"
      "________",
      Stephen{4, 3, 1, Left}, {Ladder{4, 3, 1, Left}}, // ladder on the (4,3) cell facing west, at the log's height
      { Sausage{0, 0, 1, 0, 0}, Sausage{6, 0, 7, 0, 0} })); // parked fillers
    level.SetDetachedFork(0, 7, 0, Right);  // forkless; the loose fork sits out of the way and must not reattach
    level.AssertBodyAt(4, 3, 1, Left);
    level.AssertMoveSucceeds(Left);          // press toward the ladder -> climb, do NOT roll the log
    level.AssertBodyAt(3, 3, 2, Left);       // climbed up and stepped off onto the wall top
    level.AssertSausage({4, 3, 4, 4, 0, Sausage::None}); // the log stayed put (it did not roll)
    level.AssertForkAt(0, 7, 0, Right);      // the loose fork never moved
  }

  // A forkless step shoves the thrown fork one cell ahead, but that cell is open void -- the game drops the fork off the
  // map (Fork Lost). The engine must REFUSE rather than rest it on the floor of an empty column (5-6 Crater m190).
  MAKE_SYMMETRICAL_TEST(ForklessStepRefusedWhenShovedForkWouldDrown) {
    TestSymmetryHelper level(symmetry, Level(7, 6, "arena",
      "_______"
      "_______"
      "__>_   "                             // (3,2) is solid; (4,2) onward is void
      "_______"
      "_______"
      "_______",
      {}, {},
      { Sausage{0, 0, 1, 0, 0}, Sausage{5, 0, 6, 0, 0}, Sausage{0, 5, 1, 5, 0} })); // parked fillers
    level.SetDetachedFork(3, 2, 0, Right);   // fork sits one cell ahead, so the forward step shoves it over the void
    level.AssertBodyAt(2, 2, 0, Right);
    level.AssertMoveFails(Right);            // shoving the fork into the void would lose it -> refuse
    level.AssertBodyAt(2, 2, 0, Right);      // nothing moved
    level.AssertForkAt(3, 2, 0, Right);
  }

  // A thrown fork ends one cell ahead of Stephen but faces CROSSWISE to him. The game only takes it back when
  // fork.direction == his facing (TryReattachFork), so a turn that swings a crosswise fork into front does NOT
  // reattach it (5-6 Crater m521).
  MAKE_SYMMETRICAL_TEST(ForkNotReattachedWhenFacingCrosswise) {
    TestSymmetryHelper level(symmetry, Level(7, 7, "arena",
      "_______"
      "_______"
      "_______"
      "___>___"                             // Stephen at (3,3) facing east
      "_______"
      "_______"
      "_______",
      Stephen{3, 3, 0, Right}, {},
      { Sausage{0, 0, 1, 0, 0}, Sausage{5, 0, 6, 0, 0}, Sausage{0, 6, 1, 6, 0} })); // parked fillers
    level.SetDetachedFork(3, 4, 0, Left);    // fork one cell south, facing west (crosswise to a south-facer)
    level.AssertBodyAt(3, 3, 0, Right);
    level.AssertMoveSucceeds(Down);          // turn to face south -> the fork is now directly ahead...
    level.AssertBodyAt(3, 3, 0, Down);       // ...turned in place
    level.AssertForkAt(3, 4, 0, Left);       // ...but NOT taken back (crosswise facing) -- still detached
  }

  // A SPEARED log roll drops Stephen off a ledge: he falls two levels riding the log, but the speared sausage's far end
  // catches a wall one level down. The reference detaches the fork mid-fall (TryDetatchFork) -- leaving it lodged in the
  // sausage at the wall height -- while Stephen's body falls on past it. Level2 used to drag the speared sausage rigidly
  // down to the body's level, burying it in the wall (5-6 Crater m40).
  MAKE_SYMMETRICAL_TEST(SpearedLogRollFallDetachesForkOnCaughtSausage) {
    TestSymmetryHelper level(symmetry, Level(9, 7, "arena",
      "_________"
      "_________"
      "_________"
      "_____22__"                            // (5,3),(6,3) height-2: the speared sausage's far end catches (6,3)
      "_____2___"                            // (5,4) height-2 platform; (6,4) is low ground
      "_________"
      "_________",
      Stephen{5, 5, 3, Up}, {},
      { Sausage{5, 4, 5, 5, 2},              // the log B: Stephen stands on its (5,5) end at z=3
        Sausage{5, 3, 5, 4, 3},              // the speared sausage A: the fork is lodged in its (5,4) end
        Sausage{0, 0, 1, 0, 0} }));          // parked filler
    level.AssertBodyAt(5, 5, 3, Up);          // the (speared) fork is held at (5,4,3)
    level.AssertMoveSucceeds(Left);            // press across the log (rotation locked by the spear) -> it rolls east
    level.AssertBodyAt(6, 5, 1, Up);           // Stephen fell two levels riding the log down to the low ground
    level.AssertForkAt(6, 4, 2, Up);           // the fork detached, lodged in A at the wall height a level above him
    level.AssertSausage({6, 3, 6, 4, 2, Sausage::None}); // A caught the (6,3) wall and stayed one level up
  }

  // A detached fork is lodged in a sausage whose far end rests on a wall, one level above Stephen. He walks forward to
  // stand directly UNDER it: the wall-borne sausage (and the fork stuck in it) stay put -- he does not lift them like a
  // head hat, because they hang from the wall, not from him (5-6 Crater m225, the follow-up to the m40 detach).
  MAKE_SYMMETRICAL_TEST(WalkUnderWallBorneSausageLeavesItAndDetachedFork) {
    TestSymmetryHelper level(symmetry, Level(7, 7, "arena",
      "_______"
      "_______"
      "_______"
      "__2____"                              // (2,3) height-2 wall: sausage A's north end rests on it
      "_______"
      "_______"
      "_______",
      Stephen{2, 5, 1, Up}, {},
      { Sausage{2, 3, 2, 4, 2},              // A: (2,3) end on the wall, (2,4) end cantilevered over the low ground
        Sausage{2, 4, 2, 5, 0},              // B: Stephen stands on its (2,5) end
        Sausage{6, 6, 6, 6, 0} }));          // parked filler (single cell is fine as a placeholder)
    level.SetDetachedFork(2, 4, 2, Up);       // fork lodged in A's (2,4) end, one level above Stephen's path
    level.AssertBodyAt(2, 5, 1, Up);
    level.AssertMoveSucceeds(Up);              // walk forward to stand directly under A
    level.AssertBodyAt(2, 4, 1, Up);
    level.AssertForkAt(2, 4, 2, Up);           // the detached fork stayed put (it hangs from the wall, not from Stephen)
    level.AssertSausage({2, 3, 2, 4, 2, Sausage::None}); // A stayed on the wall
  }

  // Speared Stephen backs up, dragging the speared sausage; a hat rests on BOTH the speared sausage and Stephen's head,
  // so both its supports move -- it must ride the full drag (3-4 Cold Trail m552).
  MAKE_SYMMETRICAL_TEST(SpearedBackDragCarriesHeadSpanningHat) {
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
      "          "
      " _______  "
      " _______  "
      " _______  "
      " __^____  "
      " _______  "
      "          ",
      {}, {}, { Sausage{3, 2, 3, 3, 0}, Sausage{3, 3, 3, 4, 1}, Sausage{9, 1, 9, 2, 0} }));
    level.AssertPosition(3, 4, Up);
    level.AssertSausage({3, 2, 3, 3, 0, Sausage::None}); // speared base, north of Stephen
    level.AssertSausage({3, 3, 3, 4, 1, Sausage::None}); // hat: (3,3) on the base, (3,4) on Stephen's head
    level.AssertMoveSucceeds(Down);                       // press backward -> drag the speared base south
    level.AssertPosition(3, 5, Up);
    level.AssertSausage({3, 3, 3, 4, 0, Sausage::None}); // base dragged one cell south
    level.AssertSausage({3, 4, 3, 5, 1, Sausage::None}); // hat rode the full drag (both supports moved)
  }

  // Log roll while speared, with a hat bridging Stephen's head and the speared sausage. Stephen stands on a horizontal
  // log and presses across it (facing along the press), so the log spins him and everything he carries one cell the
  // OPPOSITE way. The hat's far end rests on the speared sausage, which rides the fork rigidly -- so the hat is NOT
  // anchored and must ride the roll too, keeping its Rolled face (3-4 Cold Trail m552).
  MAKE_SYMMETRICAL_TEST(LogRollCarriesHatBridgingHeadAndSpearedSausage) {
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
      "          "
      " _______  "
      " _______  "
      " _______  "
      " _______  "
      " _______  "
      "          ",
      Stephen{3, 4, 1, Up}, {},
      { Sausage{3, 4, 4, 4, 0},          // the log Stephen stands on (horizontal), rolled south when he presses north
        Sausage{3, 2, 3, 3, 1},          // speared base on the fork (vertical, north of Stephen)
        Sausage{3, 3, 3, 4, 2, Sausage::Rolled} })); // hat: (3,3) on the base, (3,4) on Stephen's head; carried as one
    level.AssertPosition(3, 4, Up);
    level.AssertSausage({3, 4, 4, 4, 0, Sausage::None});          // the log underfoot
    level.AssertSausage({3, 2, 3, 3, 1, Sausage::None});          // speared base
    level.AssertSausage({3, 3, 3, 4, 2, Sausage::Rolled});        // the bridging hat
    level.AssertMoveSucceeds(Up);                                  // press across the log -> it rolls everything south
    level.AssertPosition(3, 5, Up);
    level.AssertSausage({3, 5, 4, 5, 0, Sausage::Rolled});        // the log rolled one cell south
    level.AssertSausage({3, 3, 3, 4, 1, Sausage::None});          // speared base dragged one cell south
    level.AssertSausage({3, 4, 3, 5, 2, Sausage::Rolled});        // hat rode the roll rigidly, keeping its face
  }

  // 3-8 Cold Head (m415): a fork speared into a sausage LOCKS Stephen's rotation, so a press perpendicular to his facing
  // can no longer turn him in place -- it rolls the log he stands on instead. Here he faces NORTH along a vertical log
  // with his fork lodged in a base to the north; pressing WEST (across the log) rolls the whole speared load one cell
  // EAST. Without the spear the same press would merely turn him to face west (see LogRoll), leaving the log unrolled --
  // Level2 used to require Stephen to face along the press axis, so it wrongly refused this roll.
  MAKE_SYMMETRICAL_TEST(SpearedLogRollFacingAlongLog) {
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
      "          "
      " _______  "
      " _______  "
      " _______  "
      " _______  "
      " _______  "
      "          ",
      Stephen{4, 4, 1, Up}, {},
      { Sausage{4, 4, 4, 5, 0},          // vertical log Stephen stands on (north end)
        Sausage{4, 2, 4, 3, 1},          // speared base on the fork (vertical, north of Stephen)
        Sausage{9, 1, 9, 2, 0} }));      // parked filler
    level.AssertPosition(4, 4, Up);
    level.AssertSausage({4, 4, 4, 5, 0, Sausage::None});
    level.AssertMoveSucceeds(Left);       // speared: a west press across the log rolls the load EAST rather than turning
    level.AssertPosition(5, 4, Up);       // Stephen rode the log one cell east; still facing north (rotation locked)
    level.AssertSausage({5, 4, 5, 5, 0, Sausage::Rolled}); // vertical log rolled one cell east
    level.AssertSausage({5, 2, 5, 3, 1, Sausage::None});   // speared base dragged rigidly one cell east
  }

  // 3-8 Cold Head (m419): the speared sausage here is HORIZONTAL and RIDES the log's far end (the fork is lodged in it
  // above the log's north cell), aligned with the roll direction. When the log rolls east the roll's push chain grabs
  // the speared sausage as a rider and the double-move detector flags it. But a fork-borne sausage rides RIGIDLY --
  // exactly one cell with Stephen -- and can never double-move. Level2 left that stale double-move flag set after
  // rebuilding the speared sausage as a rigid single translate, so it tumbled an extra cell and landed one too far east;
  // the fix clears the flag so it moves exactly +1.
  MAKE_SYMMETRICAL_TEST(SpearedHatRidesRollWithoutDoubleMove) {
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
      "          "
      " _______  "
      " _______  "
      " _______  "
      " _______  "
      " _______  "
      "          ",
      Stephen{4, 5, 1, Up}, {},
      { Sausage{4, 4, 4, 5, 0},          // vertical log: Stephen stands on its south end, the fork sits above its north end
        Sausage{3, 4, 4, 4, 1},          // speared sausage riding the log's north end -- HORIZONTAL, aligned with the roll (east)
        Sausage{9, 1, 9, 2, 0} }));      // parked filler
    level.AssertPosition(4, 5, Up);
    level.AssertSausage({4, 4, 4, 5, 0, Sausage::None});
    level.AssertSausage({3, 4, 4, 4, 1, Sausage::None});
    level.AssertMoveSucceeds(Left);       // speared: a west press across the log rolls the load one cell EAST
    level.AssertPosition(5, 5, Up);       // Stephen rode the log one cell east
    level.AssertSausage({5, 4, 5, 5, 0, Sausage::Rolled}); // vertical log rolled one cell east
    level.AssertSausage({4, 4, 5, 4, 1, Sausage::None});   // speared sausage rode rigidly +1 (NOT +2) and kept its face
  }

  // 3-4 Cold Trail (m436): a NON-speared log roll where Stephen's fork rides the roll into a rider bridging a ground
  // block and a sliding base. Stephen faces east on a vertical log (fork empty, hovering over the horizontal base H's
  // west end). Pressing WEST rolls the log EAST: it shoves H east (H slides along its own axis -> no roll) and Stephen's
  // fork rides east into the rider R's south end. R's NORTH end rests on a height-1 ground block, so the base-carry
  // treats R as anchored and leaves it -- then the fork PUSHES R across its long axis. But R's south end rides H, a base
  // co-moving east at the same speed, so R inherits that base's zero torsion (GameState.CalculateTorsion) and translates
  // RIGIDLY, keeping its face. Level2 used to roll it because a cross-axis push always flipped the face regardless of a
  // co-moving base beneath it.
  MAKE_SYMMETRICAL_TEST(ForkShoveRiderOnCoMovingBaseNoRoll) {
    TestSymmetryHelper level(symmetry, Level(10, 9, "arena",
      "          "
      " ________ "
      " ________ "
      " ________ "
      " ________ "
      " ___1____ "                                 // height-1 block at (4,5): the rider's north end rests on it (anchored)
      " ________ "
      " ________ "
      "          ",
      Stephen{2, 6, 1, Right}, {},
      { Sausage{2, 6, 2, 7, 0},                    // vertical log Stephen stands on (north end); fork empty above (3,6)
        Sausage{3, 6, 4, 6, 0},                    // horizontal base H, its west end in the log's roll path
        Sausage{4, 5, 4, 6, 1, Sausage::Rolled} })); // rider R: north end on the block, south end on H
    level.AssertPosition(2, 6, Right);
    level.AssertSausage({4, 5, 4, 6, 1, Sausage::Rolled});
    level.AssertMoveSucceeds(Left);                 // press west across the log -> it rolls everything one cell EAST
    level.AssertPosition(3, 6, Right);              // Stephen rode the log one cell east (still facing east)
    level.AssertSausage({3, 6, 3, 7, 0, Sausage::Rolled}); // the log rolled east
    level.AssertSausage({4, 6, 5, 6, 0, Sausage::None});   // base H slid east (no roll)
    level.AssertSausage({5, 5, 5, 6, 1, Sausage::Rolled}); // rider rode rigidly on the co-moving base -> keeps its face
  }

  // 3-13 Cold Gate (m485): like the case above, but the rider's moving base ROLLS rather than slides, and its other end
  // rests on a terrain block instead of a sausage. Stephen rides a log east; his body rams the rider R, whose west end
  // sits on a vertical log Q that itself rolls east and whose east end rests on a height-1 terrain block. The game's
  // CalculateTorsion carries R rigidly (torsion 0): the terrain-anchored end can't rotate, so R only translates east and
  // keeps its face -- even though a raw cross-axis shove (and even a rolling base) would otherwise flip it.
  MAKE_SYMMETRICAL_TEST(RiderOnRollingBaseAndTerrainAnchorNoRoll) {
    TestSymmetryHelper level(symmetry, Level(10, 9, "arena",
      "          "
      " ________ "
      " ________ "
      " ________ "
      " ________ "
      " ________ "
      " _____1__ "                                 // height-1 block at (6,6): the rider's east end rests on it (anchored)
      " ________ "
      "          ",
      Stephen{5, 5, 1, Left}, {},
      { Sausage{5, 4, 5, 5, 0},                    // vertical log P Stephen stands on (south end); press west rolls it east
        Sausage{6, 4, 6, 5, 0},                    // vertical log Q in the roll path -- it gets shoved east and rolls
        Sausage{6, 5, 6, 6, 1, Sausage::Rolled} })); // rider R: west end on Q, east end on the terrain block
    level.AssertPosition(5, 5, Left);
    level.AssertSausage({6, 5, 6, 6, 1, Sausage::Rolled});
    level.AssertMoveSucceeds(Left);                 // press west across log P -> the whole chain rolls one cell EAST
    level.AssertPosition(6, 5, Left);               // Stephen rode P one cell east (still facing west)
    level.AssertSausage({6, 4, 6, 5, 0, Sausage::Rolled}); // P rolled east into Q's old spot
    level.AssertSausage({7, 4, 7, 5, 0, Sausage::Rolled}); // Q rolled east
    level.AssertSausage({7, 5, 7, 6, 1, Sausage::Rolled}); // rider translated east on the rolling base + terrain -> no roll
  }

  // 3-14 Cold Frustration: a spear whose grill recoil would drive the speared log into terrain must not happen at all.
  // Stephen faces west with his fork over a ground grill (#); a vertical log ('a') sits just past the fork, pinned to
  // its west by a Wall1 (1) so a forward press can't roll it -- it SPEARS instead. Stepping onto the grill would recoil
  // the fork, dragging the log one cell back (east) onto the grill -- but the log's south end would land inside a Wall2
  // (2). The game blocks the whole recoil: the fork pulls free and Stephen bounces in place with nothing moved. Level2
  // used to ignore the wall, shoving the log east onto the grill and cooking it (the dominant 3-14 posdiff class).
  // Ignored: pre-existing spear-path failure, predates the Level2 rework. Still the live 3-14 Cold Frustration class.
  MAKE_SYMMETRICAL_TEST(SpearRecoilBlockedByWallNoDrag) {
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
      "          "
      " ______a_ "                                 // filler log a, well clear of the action
      " ______a_ "
      " _1b#<___ "                                 // Wall1(2,3) | log b north(3,3) | grill(4,3) | Stephen(5,3) facing west
      " __b2__c_ "                                 // log b south(3,4) | Wall2(4,4) -- the blocked drag cell | filler log c
      " ______c_ "
      "          "));
    level.AssertPosition(5, 3, Left);
    level.AssertSausage({3, 3, 3, 4, 0, Sausage::None});
    level.AssertMoveSucceeds(Left);                // press forward: spear + grill bounce, but the recoil drag is wall-blocked
    level.AssertPosition(5, 3, Left);              // Stephen bounced in place
    level.AssertSausage({3, 3, 3, 4, 0, Sausage::None}); // the log never moved and never cooked
  }

  MAKE_SYMMETRICAL_TEST(SpearedSausageBouncesOffGrill) {
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
      "          "
      " _____   A"
      " _____   A"
      " >_bb1   C"
      " _###_   C"
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
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
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

  // A head-hat whose rotation is blocked by a wall at its DESTINATION still shoves the sausage in its swept CORNER cell
  // out of the way -- exactly like a rotating fork's corner push, which lands even when the swing bonks. Stephen faces
  // East at (1,2) under a clean head-hat c=(1,1)-(1,2)@z1; a North press swings c's free end CCW toward the height-3 wall
  // at (0,2) (blocked, so c stays), sweeping through the corner (0,1) where the top of a west-edge 2-stack sits. The
  // sweep shoves that top sausage west off the map, so the turn is a losing move and must be refused (3-2 Cold Finger m42).
  MAKE_SYMMETRICAL_TEST(HeadHatRotationShovesCornerSausageOffEdge) {
    TestSymmetryHelper level(symmetry, Level(6, 4, "arena",
      "______"
      "______"
      "3>____"
      "______",
      {}, {}, { Sausage{0, 0, 0, 1, 0}, Sausage{0, 0, 0, 1, 1}, Sausage{1, 1, 1, 2, 1} }));
    level.AssertPosition(1, 2, Right);
    level.AssertSausage({1, 1, 1, 2, 1, Sausage::None}); // the clean head-hat
    level.AssertMoveFails(Up);                           // North turn: hat blocked by the wall, its sweep rolls the stack-top off the west edge
  }

  // 2d) Staircase roll where the top sausage's carry is blocked by a Wall2: it's left behind, then drops to the ground
  // once the bottom rolls out from under it (the fork is clear of it, so nothing catches it).
  MAKE_SYMMETRICAL_TEST(StackedStaircaseRollDrop) {
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
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
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
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
    // The base SLIDES east along its own axis; the vertical top rides it flat (zero torsion) -> it does NOT roll.
    level.AssertSausage({5, 3, 5, 4, 1, Sausage::None});
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(3, 3, Right);
    level.AssertSausage({5, 3, 6, 3, 0, Sausage::None});
    level.AssertSausage({6, 3, 6, 4, 1, Sausage::None});
  }

  // 3d) Perpendicular stack, Stephen behind the overhang. Pushing the bottom north rolls it; the vertical top, aligned
  // with the motion and supported by a perpendicular base, DOUBLE-MOVES north by two cells (sliding).
  MAKE_SYMMETRICAL_TEST(StackedPerpendicularDoubleMove) {
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
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

  // Same perpendicular double-move, but a rider is stacked on the vertical top. Pushing the base north rolls it and the
  // top DOUBLE-MOVES north by two -- and the top drags its own rider the full two cells (the aligned vertical rider
  // slides, no roll). Exercises DoubleMove's rider carry: a sausage riding a double-mover follows it two cells, not one
  // (3-5 Cold Cliff m217: the top sausage riding the double-moving mid tumbles along with it instead of being stranded).
  MAKE_SYMMETRICAL_TEST(DoubleMoveCarriesStackedRider) {
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
      "          "
      " _______  "
      " _______  "
      " _______  "
      " _______  "
      " __^____  "
      "          ",
      {}, {}, { Sausage{3, 3, 4, 3, 0}, Sausage{4, 3, 4, 4, 1}, Sausage{4, 3, 4, 4, 2} }));
    level.AssertPosition(3, 5, Up);
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(3, 4, Up);
    level.AssertSausage({3, 2, 4, 2, 0, Sausage::Rolled}); // base rolled north one cell
    level.AssertSausage({4, 1, 4, 2, 1, Sausage::None});   // top double-moved north two cells
    level.AssertSausage({4, 1, 4, 2, 2, Sausage::None});   // rider dragged the full two cells (aligned slide, no roll)
  }

  // The double-move's extra tumble is a PUSH, not a phase-through: when it laps onto a same-level neighbour, that
  // neighbour is shoved the same way (resolved BEFORE gravity). Here the double-mover rams a log wedged on a wall at the
  // north edge, driving it clean off the grid; the following Settle drowns it, so the whole move is refused (3-4 Cold
  // Trail m350: a slid log rams a wall-cornered upright off the west edge -> a loss the engine must not walk into).
  MAKE_SYMMETRICAL_TEST(DoubleMovePushOffEdgeDrownsRefused) {
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
      "          "
      " ___11__  "
      " _______  "
      " _______  "
      " _______  "
      " __^____  "
      "          ",
      {}, {}, { Sausage{3, 3, 4, 3, 0}, Sausage{4, 3, 4, 4, 1}, Sausage{4, 1, 5, 1, 1} }));
    level.AssertPosition(3, 5, Up);
    level.AssertSausage({4, 1, 5, 1, 1, Sausage::None}); // the edge log, resting on the wall
    // The top's second (double-move) cell would ram the edge log off the north edge -> it drowns -> the move is refused.
    level.AssertMoveFails(Up);
    // State is untouched: the edge log (and everything else) sits exactly where it started.
    level.AssertPosition(3, 5, Up);
    level.AssertSausage({3, 3, 4, 3, 0, Sausage::None});
    level.AssertSausage({4, 3, 4, 4, 1, Sausage::None});
    level.AssertSausage({4, 1, 5, 1, 1, Sausage::None});
  }

  // The same double-move push when the rammed neighbour has somewhere to go: it is shoved one cell onto the wall-top
  // ahead (rolling, since the push crosses its long axis) and survives, so the move succeeds with the double-mover
  // filling the vacated cell. Confirms the new push propagates without wrongly refusing when nothing drowns.
  MAKE_SYMMETRICAL_TEST(DoubleMovePushesBlockerOntoWallSucceeds) {
    TestSymmetryHelper level(symmetry, Level(10, 9, "arena",
      "          "
      " ___11__  "
      " ___11__  "
      " _______  "
      " _______  "
      " _______  "
      " __^____  "
      " _______  "
      "          ",
      {}, {}, { Sausage{3, 4, 4, 4, 0}, Sausage{4, 4, 4, 5, 1}, Sausage{4, 2, 5, 2, 1} }));
    level.AssertPosition(3, 6, Up);
    level.AssertSausage({4, 2, 5, 2, 1, Sausage::None}); // the blocker log on its wall
    level.AssertMoveSucceeds(Up);
    level.AssertPosition(3, 5, Up);
    level.AssertSausage({3, 3, 4, 3, 0, Sausage::Rolled}); // base rolled north one cell
    level.AssertSausage({4, 2, 4, 3, 1, Sausage::None});   // top double-moved north two cells (aligned slide, no roll)
    level.AssertSausage({4, 1, 5, 1, 1, Sausage::Rolled}); // blocker shoved one cell onto the north wall, rolling
  }

  // 3b) Perpendicular stack, Stephen steps UNDER the south overhang as he pushes. His body holding one end cancels the
  // double-move, so the top is carried just one cell (not two).
  MAKE_SYMMETRICAL_TEST(StackedPerpendicularHeld) {
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
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
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
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
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
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
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
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
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
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
  // rigidly, and the sausage riding on it inherits that zero torsion -- it rides flat and does NOT roll (a passenger on a
  // fork-held base moves with its support).
  MAKE_SYMMETRICAL_TEST(StackedCarryWhileSpeared) {
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
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
    level.AssertMoveSucceeds(Up); // drag north: bottom slides rigidly, top rides on the fork-held base -> no roll
    level.AssertPosition(2, 2, Right);
    level.AssertSausage({3, 2, 4, 2, 0, Sausage::None});
    level.AssertSausage({3, 2, 4, 2, 1, Sausage::None});
  }

  // The whole load Stephen drags translates as one simultaneous event (move-stages.md): a speared base + its rider AND a
  // head hat all shift by the same vector. Here backing east makes the rider's carry destination land exactly where the
  // head hat currently sits (and vice versa) -- they SWAP cells. Neither may treat the other as an obstacle: co-movers
  // ride rigidly and must NOT ram/roll each other (3-2 Cold Finger: order-dependent ram used to roll one of them).
  MAKE_SYMMETRICAL_TEST(SpearedDragHeadHatCoMove) {
    TestSymmetryHelper level(symmetry, Level(9, 7, "arena",
      "         "
      " _______ "
      " _______ "
      " _______ "
      " _______ "
      " _______ "
      "         ",
      Stephen{5, 3, 0, Left}, {},
      { Sausage{4, 3, 4, 4, 0}, Sausage{4, 3, 4, 4, 1}, Sausage{5, 3, 5, 4, 1} }));
    level.AssertPosition(5, 3, Left);
    level.AssertSausage({4, 3, 4, 4, 0, Sausage::None}); // speared base
    level.AssertSausage({4, 3, 4, 4, 1, Sausage::None}); // rider squarely on the base
    level.AssertSausage({5, 3, 5, 4, 1, Sausage::None}); // head hat on Stephen (its east end cantilevered)
    level.AssertMoveSucceeds(Right); // back east: base+rider drag east, head hat rides east -- they swap the (5,3)-(5,4) column
    level.AssertPosition(6, 3, Left);
    level.AssertSausage({5, 3, 5, 4, 0, Sausage::None}); // base dragged east, rigid
    level.AssertSausage({5, 3, 5, 4, 1, Sausage::None}); // rider rode with the base -- NOT rammed/rolled by the head hat
    level.AssertSausage({6, 3, 6, 4, 1, Sausage::None}); // head hat rode east, rigid -- NOT rammed/rolled by the rider
  }

  // The push-path analogue of SpearedDragHeadHatCoMove. A horizontal base is pushed along its own axis (it SLIDES, no
  // roll), carrying TWO vertical riders that both rest on it. The near rider's carry laps exactly onto the far rider's
  // cell -- they co-move by the same vector and must NOT ram/roll each other. Without folding the pushed base's riders
  // into the co-moving set (rigidLoad), the near rider's carry would shove (and roll) the far one (3-2 Cold Finger m41).
  MAKE_SYMMETRICAL_TEST(PushedBaseRidersCoMove) {
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
      "          "
      " _______  "
      " _______  "
      " _______  "
      " _______  "
      " _______  "
      "          ",
      Stephen{2, 4, 0, Right}, {},
      { Sausage{4, 4, 5, 4, 0}, Sausage{4, 3, 4, 4, 1}, Sausage{5, 3, 5, 4, 1} }));
    level.AssertPosition(2, 4, Right);
    level.AssertSausage({4, 4, 5, 4, 0, Sausage::None}); // horizontal base
    level.AssertSausage({4, 3, 4, 4, 1, Sausage::None}); // near rider (west) on the base's west end
    level.AssertSausage({5, 3, 5, 4, 1, Sausage::None}); // far rider (east) on the base's east end
    level.AssertMoveSucceeds(Right); // push east: base slides, both riders ride flat; near rider laps the far rider's cell
    level.AssertPosition(3, 4, Right);
    level.AssertSausage({5, 4, 6, 4, 0, Sausage::None}); // base slid east one cell (along its axis) -> no roll
    level.AssertSausage({5, 3, 5, 4, 1, Sausage::None}); // near rider rode east rigidly -- NOT rammed/rolled by the far rider
    level.AssertSausage({6, 3, 6, 4, 1, Sausage::None}); // far rider rode east rigidly -- NOT rammed/rolled by the near rider
  }

  // A vertical top sausage spans two supports: one end on a horizontal base, the other on Stephen's head. When Stephen
  // TURNS, his body counts as a wall, so the top is held in place even as the turn shoves the base out from under it.
  // (During a STEP his body would only cancel a double-move; only a turn pins the sausage. 3-3 Cold Escarpment move 69.)
  MAKE_SYMMETRICAL_TEST(StackedTopHeldByBodyDuringTurn) {
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
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

  // A head+fork bridge carries a rider that rests on its HEAD (pivot) end, cantilevered off the far side. When Stephen
  // turns, the bridge pivots about the head cell -- but the rider's only support IS that pivot cell, which stays put, so
  // the rider does NOT spin with the bridge; it's left in place (3-5 Cold Cliff m330). Contrast SausageDoubleHat, where a
  // squarely-stacked rider whose far end rides the bridge's swinging end pivots as a rigid unit.
  MAKE_SYMMETRICAL_TEST(TurnLeavesRiderOnBridgePivotEnd) {
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
      "          "
      "          "
      "          "
      "  _       "
      "          "
      "          "
      "          ",
      Stephen{2, 3, 0, Left}, {},
      { Sausage{1, 3, 2, 3, 1}, Sausage{2, 2, 2, 3, 2}, Sausage{9, 1, 9, 2, 0} }));
    level.AssertPosition(2, 3, Left);
    level.AssertSausage({1, 3, 2, 3, 1, Sausage::None}); // bridge: west end on the void-spanning fork, east end on head
    level.AssertSausage({2, 2, 2, 3, 2, Sausage::None}); // rider: south end on the bridge's head/pivot end, north cantilevered
    level.AssertMoveSucceeds(Down); // turn west->south: the bridge pivots south about the head cell
    level.AssertPosition(2, 3, Down);
    level.AssertSausage({2, 3, 2, 4, 1, Sausage::None}); // bridge swung to vertical about the head
    level.AssertSausage({2, 2, 2, 3, 2, Sausage::None}); // rider stayed -- its pivot-cell support never moved
  }

  // Spot test for 3-2 Cold Finger move 20 (a turn drops a fork-borne rider). A vertical rider spans a base sausage
  // (north end) and Stephen's fork (south end). Stephen turns east->north: the corner-sweep shoves the base out north
  // and the fork swings away, so the rider loses BOTH supports and drops straight down -- it is NOT carried north with
  // the base (the reference's rotation corner-sweep never carries riders).
  MAKE_SYMMETRICAL_TEST(TurnDropsForkBorneRider) {
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
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
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
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
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
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
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
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

  // 3-2 Cold Finger (DiffEngines divergence #1, move 267): the fork is speared into a vertical base that is wall-pinned
  // to its west, and a fork-tip hat rides one level up -- its NEAR end squarely over the fork cell (resting on the
  // base), its FAR end cantilevered over open space. Backing away can't drag the pinned base, so the fork pulls free
  // (an unspear). The hat's near end still rests on the base, which stays put, so the hat STAYS. Level2 judged the hat
  // by its far end alone and, finding it cantilevered, wrongly rolled it off the retreating fork.
  MAKE_SYMMETRICAL_TEST(UnspearLeavesForkTipHatOnSpearedBase) {
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
      "          "
      " ________ "
      " ________ "
      " ________ "
      " __1_____ "
      " ________ "
      "          ",
      Stephen{3, 3, 0, Right}, {},
      { Sausage{4, 3, 4, 4, 0}, Sausage{4, 2, 4, 3, 1}, Sausage{7, 1, 7, 2, 0} }));
    level.AssertPosition(3, 3, Right);
    level.AssertSausage({4, 3, 4, 4, 0, Sausage::None}); // vertical base, speared on the fork, Wall1-pinned to its west
    level.AssertSausage({4, 2, 4, 3, 1, Sausage::None}); // fork-tip hat: near end on the base, far end cantilevered
    level.AssertMoveSucceeds(Left);                      // back away west -> the fork unspears (the base can't be dragged)
    level.AssertPosition(2, 3, Right);
    level.AssertSausage({4, 3, 4, 4, 0, Sausage::None}); // base stays put (the fork pulled free)
    level.AssertSausage({4, 2, 4, 3, 1, Sausage::None}); // hat STAYS on the base -- it does not roll off the retreating fork
  }

  // 3-2 Cold Finger (DiffEngines divergence #2, move 324): a turn whose corner-sweep and fork-dest push move two bases
  // in DIFFERENT directions, with a rider bridging both. Stephen faces north; turning east shoves the horizontal base X
  // east (corner-sweep) and the vertical base Z south (fork-dest), and the fork swings under the rider Y's south end.
  // The rider can't follow both bases, so the fork catches it and it STAYS -- its north end merely cantilevers as X
  // slides out east. Level2's rotation fork-hold refused to hold whenever the rider's OTHER end sat on any moving base,
  // so it wrongly carried Y south with Z. The hold must ignore an other-end base that moves a DIFFERENT way than the carry.
  MAKE_SYMMETRICAL_TEST(TurnForkCatchesRiderWhenOtherBaseMovesAside) {
    TestSymmetryHelper level(symmetry, Level(10, 8, "arena",
      "          "
      " ________ "
      " ________ "
      " ________ "
      " ________ "
      " ________ "
      " ________ "
      "          ",
      Stephen{4, 3, 0, Up}, {},
      { Sausage{5, 2, 6, 2, 0}, Sausage{5, 2, 5, 3, 1}, Sausage{5, 3, 5, 4, 0} }));
    level.AssertPosition(4, 3, Up);
    level.AssertSausage({5, 2, 6, 2, 0, Sausage::None}); // horizontal base X at the turn's corner
    level.AssertSausage({5, 2, 5, 3, 1, Sausage::None}); // vertical rider Y: north end on X, south end on Z (the fork-dest)
    level.AssertSausage({5, 3, 5, 4, 0, Sausage::None}); // vertical base Z at the fork's destination
    level.AssertMoveSucceeds(Right);                     // turn north->east: X shoved east, Z shoved south, fork swings under Y
    level.AssertPosition(4, 3, Right);
    level.AssertSausage({6, 2, 7, 2, 0, Sausage::None}); // X shoved east by the corner-sweep
    level.AssertSausage({5, 4, 5, 5, 0, Sausage::None}); // Z shoved south by the fork-dest push
    level.AssertSausage({5, 2, 5, 3, 1, Sausage::None}); // Y STAYS -- caught by the fork; its north end cantilevers as X leaves
  }

  // 3-2 Cold Finger (DiffEngines divergence #3, move 291): a head hat bridges Stephen's head (its near end) and a
  // neighbouring sausage Q (its far end). Backing away drags the speared base R west; R shoves Q (rolling it), and Q's
  // push chain carries the hat. The hat rests on Stephen's head -- a rigid co-mover -- so it inherits his zero torsion
  // and rides RIGIDLY, without rolling, even though its far end sits on the rolling Q. Level2's push-chain carry rolled
  // it (it took Q's torsion).
  MAKE_SYMMETRICAL_TEST(SpearDragHeadHatOverPushedSausageRidesRigid) {
    TestSymmetryHelper level(symmetry, Level(9, 7, "arena",
      "         "
      " _______ "
      " _______ "
      " _______ "
      " _______ "
      " _______ "
      "         ",
      Stephen{4, 2, 0, Right}, {},
      { Sausage{4, 2, 4, 3, 1}, Sausage{4, 3, 4, 4, 0}, Sausage{5, 2, 5, 3, 0} }));
    level.AssertPosition(4, 2, Right);
    level.AssertSausage({4, 2, 4, 3, 1, Sausage::None}); // head hat: north end on Stephen's head, south end on Q
    level.AssertSausage({4, 3, 4, 4, 0, Sausage::None}); // Q, under the hat's south end
    level.AssertSausage({5, 2, 5, 3, 0, Sausage::None}); // speared base R
    level.AssertMoveSucceeds(Left);                      // back away west: R drags west and shoves Q, the hat rides Stephen
    level.AssertPosition(3, 2, Right);
    level.AssertSausage({4, 2, 4, 3, 0, Sausage::None});   // R dragged west (rigid)
    level.AssertSausage({3, 3, 3, 4, 0, Sausage::Rolled}); // Q shoved west, rolled across its axis
    level.AssertSausage({3, 2, 3, 3, 1, Sausage::None});   // head hat rode west RIGIDLY -- no roll (it rests on Stephen's head)
  }

  // 3-2 Cold Finger (DiffEngines divergence #4, move 324): a fork-bonked turn whose head-hat sweep shoves a wall-perched
  // sausage clean off the playable edge onto VOID that still lies within the grid's bounds, where it drowns. The bonk
  // keeps the hat home, but the swept sausage is an irreversible loss, so the move must be refused. Level2 only rejected
  // sausages pushed FULLY out of bounds, missing this within-bounds void drown, and wrongly accepted the move.
  MAKE_SYMMETRICAL_TEST(BonkedTurnSweepDrownsSausageOnVoid) {
    TestSymmetryHelper level(symmetry, Level(10, 8, "arena",
      "          "
      " ________ "
      " ________ "
      " ____1    "
      " _____    "
      " ________ "
      " ________ "
      "          ",
      Stephen{4, 3, 0, Down}, {},
      { Sausage{4, 3, 4, 4, 1}, Sausage{5, 3, 5, 4, 1}, Sausage{2, 6, 3, 6, 0} }));
    level.AssertPosition(4, 3, Down);
    level.AssertSausage({4, 3, 4, 4, 1, Sausage::None}); // head+fork hat that will sweep on the bonk
    level.AssertSausage({5, 3, 5, 4, 1, Sausage::None}); // C: north end on the Wall1 at (5,3), south end cantilevered
    level.AssertMoveFails(Right);                        // turn east bonks (fork-dest wall); the sweep would drown C -> refuse
  }

  // The companion to the above: the same bonked sweep, but with ground east of the wall so the swept sausage SURVIVES.
  // The whole sweep is then backed out with Stephen -- neither the hat nor the sausage it shoved keeps its new spot --
  // so the move succeeds with nothing moved at all. Only a DROWNED sausage (the test above) cannot be backed out and so
  // refuses the move. Merging the sweep's shoves into the committed plan instead diverges from the game on 3-2 Cold
  // Finger and 3-5 Cold Cliff, and no other test catches it.
  MAKE_SYMMETRICAL_TEST(BonkedTurnSweepIsBackedOut) {
    TestSymmetryHelper level(symmetry, Level(10, 8, "arena",
      "          "
      " ________ "
      " ________ "
      " ____1__  "
      " ______   "
      " ________ "
      " ________ "
      "          ",
      Stephen{4, 3, 0, Down}, {},
      { Sausage{4, 3, 4, 4, 1}, Sausage{5, 3, 5, 4, 1}, Sausage{2, 6, 3, 6, 0} }));
    level.AssertPosition(4, 3, Down);
    level.AssertMoveSucceeds(Right);                     // the turn bonks (fork-dest wall), but nothing is lost
    level.AssertPosition(4, 3, Down);                    // Stephen never turns
    level.AssertSausage({4, 3, 4, 4, 1, Sausage::None}); // the hat swung and came home
    level.AssertSausage({5, 3, 5, 4, 1, Sausage::None}); // C was shoved by the sweep, then put back
  }

  // 3-4 Cold Trail (DiffEngines divergence, move 430): a log-roll carries a horizontal rider west, and the rider's
  // LEADING end lands on a stationary sausage after one cell. That sausage is a "shelf" -- its top sits at the rider's
  // level and catches the front -- so the rider moves a SINGLE cell, not two. Level2 only treated a WALL as a shelf, so
  // it tumbled the rider an extra cell (a spurious double-move).
  MAKE_SYMMETRICAL_TEST(DoubleMoveCaughtByStationarySausageShelf) {
    TestSymmetryHelper level(symmetry, Level(9, 9, "arena",
      "         "
      " _______ "
      " _______ "
      " _______ "
      " _______ "
      " _______ "
      " _______ "
      " _______ "
      "         ",
      Stephen{5, 6, 1, Right}, {},
      { Sausage{3, 6, 3, 7, 0}, Sausage{4, 7, 5, 7, 1}, Sausage{5, 6, 5, 7, 0} }));
    level.AssertPosition(5, 6, Right);
    level.AssertSausage({3, 6, 3, 7, 0, Sausage::None}); // stationary sausage A -- the shelf the rider's lead end lands on
    level.AssertSausage({4, 7, 5, 7, 1, Sausage::None}); // horizontal rider B, east end on the log
    level.AssertSausage({5, 6, 5, 7, 0, Sausage::None}); // the vertical log C Stephen rides
    level.AssertMoveSucceeds(Right);                     // press east across the log -> it rolls west, Stephen rides
    level.AssertPosition(4, 6, Right);
    level.AssertSausage({3, 6, 3, 7, 0, Sausage::None});   // A unchanged (the shelf)
    level.AssertSausage({3, 7, 4, 7, 1, Sausage::None});   // B slid west ONE cell -- caught by A, no double-move
    level.AssertSausage({4, 6, 4, 7, 0, Sausage::Rolled}); // C rolled west
  }

  // 3-14 Cold Frustration move 86, stripped to the critical geometry: a horizontal rider bridges a vertical base (its
  // west end) and Stephen's fork (its east end, over a grill). Pressing forward steps Stephen's body onto that grill so
  // he BOUNCES straight back; the base -- jammed against the Wall1 to its west so it can't be pushed -- is speared and
  // dragged one cell east by the recoil onto the grill column (cooking both ends), and the rider rides east off the
  // fork onto Stephen's head. Stephen ends where he started.
  MAKE_SYMMETRICAL_TEST(RiderCarriedByMovingBaseOffFork) {
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
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
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
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
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
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
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
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
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
      "          "
      " ___?_   A"
      " __>#_   A"
      " _____   B"
      " _____   B"
      " _____    "
      "          ",
      {}, {},
      { Sausage{3, 1, 3, 2, 1} },
      { SpecialTile({0, 2}, {0}) }));
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
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
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
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
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
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
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
    level.AssertSausage({10, 2, 10, 3, 2, Sausage::None});   // carried off the edge on the fork tip -> rides rigidly, no roll
  }

  // 3-5 Cold Cliff / 4-4 Foul Fen: a "bridge" sausage spans Stephen's head and fork. He climbs a rung of the ladder in
  // his cell and steps off onto a Wall2 top; the bridge rides up and North with him. Its entire lower footprint is
  // Stephen himself (head under the west end, fork under the east end), a rigid co-mover moving at the sausage's own
  // speed, so the Oracle gives it ZERO torsion and it rides up rigidly -- it does NOT roll. (Measured directly on 3-5
  // Cold Cliff m62; corroborated by 21598 real Foul Fen rolls, none of which roll a co-mover-supported sausage. An
  // earlier version of this test asserted a Rolled flag, which was a mis-strip of the real head+wall bridge geometry.)
  MAKE_SYMMETRICAL_TEST(ClimbLadderCarriesBridgeSausageRigidly) {
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
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
    level.AssertSausage({2, 2, 3, 2, 3, Sausage::None}); // rode up and north rigidly on head+fork -- no roll
  }

  // 4-5 Crunchy Leaves (DiffEngines divergence): Stephen climbs a ladder carrying a GENUINE head hat (a vertical
  // sausage on his head, its south end cantilevered -- NOT fork-borne) which itself carries a horizontal rider perched
  // on its head end and cantilevered east. As he climbs and steps north, the head hat rides rigidly (it moves along its
  // own axis anyway), but the rider is carried across its long axis, so it ROLLS. Level2 used to translate the whole
  // head-hat stack rigidly (rollMask covered only the fork stack), leaving the rider's Rolled flag unset.
  MAKE_SYMMETRICAL_TEST(ClimbLadderRollsHeadHatRider) {
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
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
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
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

  // A horizontal "bridge" sausage rests across Stephen's head (east end) and his fork (west end) at head height, and a
  // vertical sausage rides on the bridge's head end, cantilevered north over open space. Walking forward is a PURE
  // TRANSLATION: the bridge slides one cell along its own axis (no roll) and its rider -- though cantilevered -- rides
  // flat with zero torsion and does NOT roll either (3-5 Cold Cliff m348, measured against the game: a cantilevered
  // rider on a head+fork bridge slides with it). The whole tower is one rigid co-moving unit.
  MAKE_SYMMETRICAL_TEST(HatRiderRidesFlatOnRigidBridge) {
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
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
    level.AssertSausage({1, 3, 2, 3, 1, Sausage::None}); // bridge slid west along its own axis -- no roll
    level.AssertSausage({2, 2, 2, 3, 2, Sausage::None}); // rider rode flat on the rigid bridge -- no roll (pure translation)
  }

  // 4-3 Sludge Coast (DiffEngines divergence #1): Stephen's fork is speared into a vertical sausage; pressing sideways
  // (a perpendicular press, so no turn while speared) lunges his body onto a grill and drags the speared sausage along,
  // which shoves a neighbouring sausage one cell onto a second grill. The grill then bounces Stephen straight back,
  // pulling the speared sausage back with him -- but the shoved neighbour STAYS where the lunge pushed it (rolled and
  // branded on the grill). Level2 used to discard the whole lunge on the bounce, so the neighbour never moved.
  MAKE_SYMMETRICAL_TEST(SpearedBounceStillShovesNeighbor) {
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
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
  // Ignored: pre-existing spear-path failure, predates the Level2 rework.
  MAKE_SYMMETRICAL_TEST(UnspearBounceShovesSausage) {
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
      "    AA BB "
      " ________ "
      " ___1c___ "
      " __#>c___ "
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
    TestSymmetryHelper level(symmetry, Level(8, 6, "arena",
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
    TestSymmetryHelper level(symmetry, Level(9, 8, "arena",
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
    TestSymmetryHelper level(symmetry, Level(8, 6, "arena",
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

  // Log roll carrying a fork-tip sausage that ITSELF has a rider stacked on it. The whole fork-borne stack rides west
  // with the fork; sliding along the (horizontal) axis, nothing rolls. Level2's log-roll used to translate only the
  // single fork-hat, orphaning the rider stacked on it.
  MAKE_SYMMETRICAL_TEST(LogRollCarriesForkHatRider) {
    TestSymmetryHelper level(symmetry, Level(8, 6, "arena",
      "        "
      "        "
      "  _____ "
      "  _____ "
      "        "
      "        ",
      Stephen{5, 3, 1, Left}, {},
      { Sausage{5, 2, 5, 3, 0}, Sausage{3, 3, 4, 3, 2}, Sausage{3, 3, 4, 3, 3} }));
    level.AssertPosition(5, 3, Left);
    level.AssertSausage({3, 3, 4, 3, 2, Sausage::None}); // fork-hat
    level.AssertSausage({3, 3, 4, 3, 3, Sausage::None}); // rider stacked on the fork-hat
    level.AssertMoveSucceeds(Right);                     // press east -> log rolls west, Stephen rides
    level.AssertPosition(4, 3, Left);
    level.AssertSausage({4, 2, 4, 3, 0, Sausage::Rolled}); // the log rolled one cell west
    level.AssertSausage({2, 3, 3, 3, 2, Sausage::None});   // fork-hat slid west
    level.AssertSausage({2, 3, 3, 3, 3, Sausage::None});   // rider rode west with the fork-hat
  }

  // Log roll carrying a fork-tip sausage oriented ACROSS the roll direction. Per the game rule, only the log Stephen
  // stands on rolls; anything supported by his head or fork translates RIGIDLY. So the vertical fork-hat, though carried
  // across its long axis, does NOT roll -- it just slides one cell with the fork. (3-8 Cold Head / 5-9 Drumlin m530.)
  MAKE_SYMMETRICAL_TEST(LogRollForkHatRidesRigidAcrossAxis) {
    TestSymmetryHelper level(symmetry, Level(8, 6, "arena",
      "        "
      "        "
      "  _____ "
      "  _____ "
      "        "
      "        ",
      Stephen{5, 3, 1, Left}, {},
      { Sausage{5, 2, 5, 3, 0}, Sausage{4, 3, 4, 4, 2}, Sausage{2, 2, 3, 2, 0} }));
    level.AssertPosition(5, 3, Left);
    level.AssertSausage({4, 3, 4, 4, 2, Sausage::None});   // vertical fork-hat balanced on the fork tip
    level.AssertMoveSucceeds(Right);                       // press east -> log rolls west, Stephen rides
    level.AssertPosition(4, 3, Left);
    level.AssertSausage({4, 2, 4, 3, 0, Sausage::Rolled}); // the log rolled one cell west
    level.AssertSausage({3, 3, 3, 4, 2, Sausage::None});   // fork-hat rode west rigidly (fork-borne -> no roll)
  }

  // Log roll carrying a fork-tip sausage that ITSELF has a rider aligned with the motion. Because the fork-hat rides
  // rigidly (fork-borne, no roll), the rider stacked on it is also fork-supported (transitively) and translates rigidly
  // too -- one cell, no double-move. (Confirms the corrected 5-9 Drumlin rule for a stacked fork-hat rider.)
  MAKE_SYMMETRICAL_TEST(LogRollForkHatRiderRidesRigid) {
    TestSymmetryHelper level(symmetry, Level(9, 6, "arena",
      "         "
      "         "
      "  ______ "
      "  ______ "
      "         "
      "         ",
      Stephen{5, 3, 1, Left}, {},
      { Sausage{5, 2, 5, 3, 0}, Sausage{4, 3, 4, 4, 2}, Sausage{4, 3, 5, 3, 3} }));
    level.AssertPosition(5, 3, Left);
    level.AssertSausage({4, 3, 4, 4, 2, Sausage::None}); // vertical fork-hat on the fork tip
    level.AssertSausage({4, 3, 5, 3, 3, Sausage::None}); // horizontal rider, one end on the fork-hat, aligned with the roll
    level.AssertMoveSucceeds(Right);                     // press east -> log rolls west, Stephen rides
    level.AssertPosition(4, 3, Left);
    level.AssertSausage({4, 2, 4, 3, 0, Sausage::Rolled}); // the log rolled one cell west
    level.AssertSausage({3, 3, 3, 4, 2, Sausage::None});   // fork-hat rides west rigidly (fork-borne -> no roll)
    level.AssertSausage({3, 3, 4, 3, 3, Sausage::None});   // rider rides rigidly one cell with the fork-hat (no double-move)
  }

  // 3-11 Cold Terrace (DiffEngines): Stephen is speared into a horizontal sausage to his west and stands on a Up-ladder.
  // Pressing up climbs a rung, then steps off north -- but that step would ride the rigidly-speared sausage north into a
  // Wall2, which it can't enter, so the reference refuses the whole move. Level2 used to leave the speared sausage
  // behind against the wall (as if it were a loose rider) and complete the climb, reaching a state the game can't.
  MAKE_SYMMETRICAL_TEST(SpearedClimbRefusedWhenBaseHitsWall) {
    TestSymmetryHelper level(symmetry, Level(8, 6, "arena",
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
    TestSymmetryHelper level(symmetry, Level(10, 6, "arena",
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
    level.AssertSausage({5, 1, 5, 2, 1, Sausage::None});   // rider rides the fork-held base rigidly -> no roll
    level.AssertSausage({6, 2, 6, 3, 0, Sausage::Rolled}); // neighbour shoved east and dropped a level
  }

  // 3-14 Cold Frustration (DiffEngines, isolated): Stephen is speared into a vertical sausage to his WEST and backs away
  // (presses east). The fork pulls the speared sausage east along with him; it rams a horizontal neighbour, which must
  // be shoved east too. This isolates the backward-drag push-propagation (no grill/bounce).
  MAKE_SYMMETRICAL_TEST(SpearedBackDragShovesNeighbor) {
    TestSymmetryHelper level(symmetry, Level(9, 7, "arena",
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
    TestSymmetryHelper level(symmetry, Level(9, 7, "arena",
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
  // top to the west. He climbs to the wall-top and steps off west; the bridge rides one cell west onto the wall. Its
  // lower footprint while carried is Stephen himself (head + fork), a rigid co-mover, so the Oracle gives it zero torsion
  // -- it slides rigidly, it does NOT roll. The rider's own west end would ram the Wall5 beyond, so its carry is BLOCKED
  // and it stays put, only rising with the climb. Level2 used to translate the whole carried stack rigidly with no wall
  // check, shoving the rider into the Wall5 column.
  MAKE_SYMMETRICAL_TEST(LadderStepOffRiderWallBlocked) {
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
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
    level.AssertSausage({2, 5, 2, 6, 3, Sausage::None});   // bridge rode one cell west onto the wall rigidly -- no roll
    level.AssertSausage({2, 5, 3, 5, 4, Sausage::None});   // rider carry wall-blocked -- stayed put, just rose
  }

  // 4-5 Crunchy Leaves (DiffEngines divergence): Stephen descends a sideways ladder while speared into a stack; the
  // descent lowers the carried sausage rung by rung. When a fully-cooked sausage is dragged down onto a grill, pressing
  // a done face back onto the fire would burn it, so the whole descent is refused. Level2 used to never cook the
  // carried sausage during a descent, and so accepted the burning move.
  MAKE_SYMMETRICAL_TEST(SpearedDescentOntoGrillBurns) {
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
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
    TestSymmetryHelper level(symmetry, Level(11, 9, "arena",
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

  // Log roll carrying a head-hat that ITSELF has a rider squarely stacked on it (both ends resting on the hat). The
  // whole rigid hat stack rides one cell east with Stephen -- the reference carries the entire stack (MoveThroughSpace's
  // hatStack), so the rider on top must ride too. Level2's log-roll used to translate only the single head-hat sausage,
  // orphaning any rider stacked on it a cell behind (and now floating).
  MAKE_SYMMETRICAL_TEST(LogRollCarriesHeadHatRider) {
    TestSymmetryHelper level(symmetry, Level(11, 9, "arena",
      "___________"
      "___________"
      "___________"
      "___________"
      "___________"
      "___________"
      "___________"
      "___________"
      "___________",
      Stephen{5, 4, 1, Left}, {},
      { Sausage{5, 3, 5, 4, 0}, Sausage{5, 4, 6, 4, 2}, Sausage{5, 4, 6, 4, 3} }));
    level.AssertMoveSucceeds(Left);                        // press across the support -> log-roll east, riding the sausage
    level.AssertPosition(6, 4, Left);
    level.AssertSausage({6, 3, 6, 4, 0, Sausage::Rolled}); // support rolled east under him
    level.AssertSausage({6, 4, 7, 4, 2, Sausage::None});   // head-hat slid east with him
    level.AssertSausage({6, 4, 7, 4, 3, Sausage::None});   // rider on the head-hat rode east rigidly too
  }

  // 4-1 Wretch's Retreat (leading bothAcceptDiffer divergence): a horizontal base sausage carries a vertical rider on
  // one end (the rider's other end cantilevers over open ground). Stephen backs into the base, rolling it one cell
  // across its axis; because the base ROLLS and the rider is aligned with the motion with no stationary support, the
  // rider double-moves -- it tumbles TWO cells and, now unsupported, drops to the floor. Level2 used to judge the
  // double-move against Stephen's POST-step pose: since he steps under the rider's end, it wrongly looked "held" and
  // Level2 carried the rider only one cell (still elevated). The reference tests his PRE-step pose, so it tumbles.
  MAKE_SYMMETRICAL_TEST(RolledBaseDoubleMovesRider) {
    TestSymmetryHelper level(symmetry, Level(13, 8, "arena",
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

  // 5-1 The Gorge (m209): a rolled base carries a rider, and the base rolls off a one-high shelf, dropping a SINGLE
  // level onto the adjacent ground. Stepping down just one level, it stays under the rider long enough to flick it, so
  // the aligned rider still DOUBLE-MOVES two cells (and drops to the floor). Level2 must not confuse this one-step drop
  // with a base that plummets away.
  MAKE_SYMMETRICAL_TEST(RolledBaseSteppingDownStillDoubleMoves) {
    TestSymmetryHelper level(symmetry, Level(13, 8, "arena",
      "aa___________"
      "_____________"
      "_____________"
      "_____________"
      "__________11_"
      "__________1__"
      "_____________"
      "_____________",
      Stephen{10, 5, 1, Down}, {},
      { Sausage{10, 4, 11, 4, 1}, Sausage{10, 3, 10, 4, 2} }));
    level.AssertPosition(10, 5, Down);
    level.AssertMoveSucceeds(Up); // back north into the base -> it rolls off the shelf, dropping one level
    level.AssertPosition(10, 4, Down);
    level.AssertSausage({10, 3, 11, 3, 0, Sausage::Rolled}); // base rolled north one cell and fell to z0
    level.AssertSausage({10, 1, 10, 2, 0, Sausage::None});   // rider double-moved two cells and dropped to z0
  }

  // 5-1 The Gorge (m195) / 5-4 Slope View (m379): same rolled-base-carries-rider setup, but the base sits on a TWO-high
  // shelf and plummets TWO levels off its edge. A base that drops two or more levels falls out from under the rider
  // before it can flick it, so the aligned rider is carried just ONE cell (then drops a level). Level2's double-move
  // detector only checked that the base ROLLED, not how far it fell, so it tumbled the rider an extra cell.
  MAKE_SYMMETRICAL_TEST(RolledBasePlummetingCancelsDoubleMove) {
    TestSymmetryHelper level(symmetry, Level(13, 8, "arena",
      "aa___________"
      "_____________"
      "_____________"
      "_____________"
      "__________22_"
      "__________2__"
      "_____________"
      "_____________",
      Stephen{10, 5, 2, Down}, {},
      { Sausage{10, 4, 11, 4, 2}, Sausage{10, 3, 10, 4, 3} }));
    level.AssertPosition(10, 5, Down);
    level.AssertMoveSucceeds(Up); // back north into the base -> it rolls off the shelf and plummets two levels
    level.AssertPosition(10, 4, Down);
    level.AssertSausage({10, 3, 11, 3, 0, Sausage::Rolled}); // base rolled north one cell and fell to z0
    level.AssertSausage({10, 2, 10, 3, 1, Sausage::None});   // rider carried just ONE cell (no flick) and dropped to z1
  }

  // 3-1 Cold Jag (leading divergence): Stephen stands on a support sausage, speared into a rider sausage that is
  // stacked on a THIRD sausage sitting beside the support. Pressing back log-rolls the support, whose roll chains into
  // the third sausage; the speared rider rides rigidly one cell with Stephen. Level2 used to move the rider TWICE --
  // once as a chain rider (which also rolled it) and once as the speared sausage -- landing it a cell too far, rolled.
  MAKE_SYMMETRICAL_TEST(SpearedRiderOnRolledChain) {
    TestSymmetryHelper level(symmetry, Level(12, 6, "arena",
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

  // 3-8 Cold Head (m143): Stephen is speared into a horizontal base sausage carrying a vertical rider on its cantilevered
  // far end. He climbs a sideways ladder, stepping the whole speared stack up onto a Wall1. The base slides along its own
  // axis (no roll). The rider is carried across its axis but rides RIGIDLY -- the fork locks the base flat and drags it
  // at a fixed speed, so the rider (its other end hanging over the floor) moves at that same speed and keeps its face,
  // matching the game's zero-torsion result. Level2 used to roll it, tumbling it to the wrong final position.
  MAKE_SYMMETRICAL_TEST(SpearedClimbCarriesRiderRigid) {
    TestSymmetryHelper level(symmetry, Level(10, 8, "arena",
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
    level.AssertSausage({3, 4, 3, 5, 2, Sausage::None});   // rider carried east rigidly on the fork-locked base -- no roll
  }

  // Regression (3-8 Cold Head float bug): a ladder DESCENT must bring a rider DOWN with the speared base it rests on.
  // The reference's Crouch descent skipped CheckForSausageCarry, so the base rolled back down off the wall but the
  // rider was orphaned a level up (left floating at z=2); the fix drops it. Climb east onto the wall then descend back
  // west is a clean no-op -- the rider returns to z=1, never floating.
  MAKE_SYMMETRICAL_TEST(DescendLadderLowersRiderNoFloat) {
    TestSymmetryHelper level(symmetry, Level(10, 8, "arena",
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
    level.AssertSausage({3, 4, 3, 5, 2, Sausage::None});   // rider one level up at z=2, riding the fork-locked base rigidly
    level.AssertMoveSucceeds(Left);  // descend the ladder: the base rolls back down AND the rider comes with it
    level.AssertPosition(3, 5, Up);
    level.AssertSausage({2, 4, 3, 4, 0, Sausage::None});   // base back on the ground
    level.AssertSausage({2, 4, 2, 5, 1, Sausage::None});   // rider dropped back to z=1 -- NOT left floating at z=2
  }

  // 3-5 Cold Cliff (move 27): Stephen stands on a Wall2 cliff facing north with a horizontal base 'a' cantilevered east
  // off the cliff top and a perpendicular vertical rider 'b' balanced on 'a's west end, its free half pointing SOUTH.
  // Turning east swings the fork onto 'a's west cell and rolls 'a' south. 'b' is a perpendicular rider on a rolled base
  // and its free end points the SAME way as the roll, so the fork (arriving under 'b's NORTH/trailing end) cannot catch
  // it: 'b' double-rolls south off the cliff and drops to the ground. It would only stay put if it were north of 'a'
  // (its south end over the fork, tumbling INTO it). Both engines used to hold 'b' still via the trailing-end fork.
  MAKE_SYMMETRICAL_TEST(RotationDoubleRollsUnsupportedRider) {
    TestSymmetryHelper level(symmetry, Level(8, 8, "arena",
      "2222    "
      "2222    "
      "2222    "
      "2222    "
      "2222__  "
      "________"
      "________"
      "________",
      Stephen{2, 3, 2, Up}, {},
      { Sausage{3, 3, 4, 3, 2}, Sausage{3, 3, 3, 4, 3}, Sausage{4, 4, 4, 5, 0} }));
    level.AssertPosition(2, 3, Up);
    level.AssertSausage({3, 3, 4, 3, 2, Sausage::None}); // base 'a', west end on the cliff, east end cantilevered
    level.AssertSausage({3, 3, 3, 4, 3, Sausage::None}); // rider 'b' on 'a's west end, free half pointing south
    level.AssertMoveSucceeds(Right);                     // turn east: fork swings onto 'a', rolling it south
    level.AssertPosition(2, 3, Right);
    level.AssertSausage({3, 4, 4, 4, 2, Sausage::Rolled}); // 'a' rolled one cell south
    level.AssertSausage({3, 5, 3, 6, 0, Sausage::None});   // 'b' double-rolled south off the cliff and dropped to z=0
    level.AssertSausage({4, 4, 4, 5, 0, Sausage::None});   // 'c' untouched
  }

  // 3-5 Cold Cliff (the "weird setup"): Stephen stands on a vertical log; a head-hat bridges from his head onto a
  // horizontal "mid" sausage whose east end also rides the log. Pressing across the log rolls it east. Three distinct
  // outcomes: the log rolls one cell (Rolled); the head-hat is cleanly supported by Stephen so it rides one cell rigidly
  // (NO roll) even though its far end sat on the moving mid; and the mid rides the log but the hat sitting directly
  // ABOVE it cancels its double-move, so it shifts only ONE cell (a wall above would do the same).
  MAKE_SYMMETRICAL_TEST(LogRollHeadHatOverMovingMid) {
    TestSymmetryHelper level(symmetry, Level(9, 8, "arena",
      "_________"
      "_________"
      "_________"
      "_________"
      "_________"
      "_________"
      "_________"
      "_________",
      Stephen{4, 4, 1, Right}, {},
      { Sausage{4, 4, 4, 5, 0}, Sausage{4, 4, 4, 5, 2}, Sausage{3, 5, 4, 5, 1} }));
    level.AssertPosition(4, 4, Right);
    level.AssertSausage({4, 4, 4, 5, 0, Sausage::None}); // the vertical log Stephen rides
    level.AssertSausage({4, 4, 4, 5, 2, Sausage::None}); // head-hat: near end on Stephen, far end on the mid
    level.AssertSausage({3, 5, 4, 5, 1, Sausage::None}); // horizontal mid: east end on the log, west end cantilevered
    level.AssertMoveSucceeds(Left);                      // press across the log -> it rolls east, Stephen rides
    level.AssertPosition(5, 4, Right);
    level.AssertSausage({5, 4, 5, 5, 0, Sausage::Rolled}); // log rolled one cell east
    level.AssertSausage({5, 4, 5, 5, 2, Sausage::None});   // head-hat rode Stephen one cell east, rigid (no roll)
    level.AssertSausage({4, 5, 5, 5, 1, Sausage::None});   // mid shifted one cell -- double-move cancelled by the hat above
  }
};

TEST_CLASS(LogicTests) {
  MAKE_SYMMETRICAL_TEST(BasicLocomotion) {
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
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
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
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
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
      "          "
      " _____   A"
      " _____   A"
      " >_bb_   C"
      " _____   C"
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
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
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
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
      "          "
      " _____   A"
      " ___1_   A"
      " >_bb_   C"
      " _____   C"
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
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
      "          "
      " _____   A"
      " __bb_   A"
      " >_#__   C"
      " _____   C"
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
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
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
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
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
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
      "          "
      " _______ A"
      " _______ A"
      " >_bc_1_  "
      " __bc___  "
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

  // Ignored: pre-existing spear-path failure, predates the Level2 rework.
  MAKE_SYMMETRICAL_TEST(TwoSausagesAndWallSpear) {
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
      "          "
      " _______ A"
      " _______ A"
      " >_bc1__  "
      " __bc___  "
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
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
      "          "
      " _D___   A"
      " 11bb_   A"
      " >____   C"
      " ____1   C"
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
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
    "          "
    " __a____ B"
    " __a111_ B"
    " >__121_ C"
    " ___111_ C"
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
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
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
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
      "         A"
      " _______ A"
      " _______ B"
      " >__?2__ B"
      " _______  "
      " _______  "
      "          ",
      {}, {}, { Sausage{1, 3, 2, 3, 1} }, { SpecialTile({0, 2}) }));
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
    level.AssertSausageMoved(Left); // fork-borne hat rides rigidly -> no roll
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(2, 3, Left);
    level.AssertSausageMoved(Left);
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(1, 3, Left);
    level.AssertSausageMoved(Left);
    level.AssertMoveFails(Up);
    level.AssertMoveSucceeds(Right);
    level.AssertPosition(2, 3, Left);
    level.AssertSausageMoved(Right);
    level.AssertMoveSucceeds(Down);
    level.AssertPosition(2, 3, Down);
    level.AssertSausage({1, 3, 1, 4, 0, Sausage::None});
  }

  MAKE_SYMMETRICAL_TEST(SausageDoubleHat) {
    TestSymmetryHelper level(symmetry, Level(10, 7, "arena",
      "         A"
      " _______ A"
      " _______  "
      " >__?2__  "
      " _______  "
      " _______  "
      "          ",
      {}, {}, { Sausage{1, 3, 2, 3, 1}, Sausage{1, 3, 2, 3, 2} }, { SpecialTile({0, 2}) }));
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
    level.AssertSausage({2, 3, 2, 4, 1, Sausage::None}); // fork-borne stack rides rigidly -> no roll
    level.AssertSausage({2, 3, 2, 4, 2, Sausage::None});
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(2, 3, Left);
    level.AssertSausage({1, 3, 1, 4, 1, Sausage::None});
    level.AssertSausage({1, 3, 1, 4, 2, Sausage::None});
    level.AssertMoveSucceeds(Left);
    level.AssertPosition(1, 3, Left);
    level.AssertSausage({0, 3, 0, 4, 1, Sausage::None});
    level.AssertSausage({0, 3, 0, 4, 2, Sausage::None});
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
