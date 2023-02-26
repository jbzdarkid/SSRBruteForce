#include "Level.h"
#include <cstdio>
#include <intrin.h>

// Helper functions to check for infinite recursion. By taking the address of a stack-local variable,
// we can determine if the stack has grown _very_ large, and then pre-emptively kill execution.
// This will result in a smaller and easier callstack to debug.
static u64 stackStart;
void stackcheck_begin() {
#if _DEBUG
  u8 local;
  stackStart = (u64)&local;
#endif
}

void stackcheck() {
#if _DEBUG
  u8 local;
  u64 currentStack = (u64)&local;
  if (stackStart - currentStack > 0x10'000) {
    printf("Stack overflow detected! Execution paused.\n");
    assert(false);
    getchar();
  }
#endif
}

// Frequently used in FAIL() strings
constexpr const char* DIRS[] = {"None", "Up", "Left", "Jump", "Crouch", "Right", "Down"};

#define FAIL(reason, ...) \
  do { \
    if (_interactive) { \
      printf("[%d] Move was illegal: " reason "\n", __LINE__, ##__VA_ARGS__); \
    } \
    return false; \
  } while (0)

bool Level::InteractiveSolver() {
  _interactive = true;
  Print();
  printf("ULDR: ");
  Vector<State> undoHistory({GetState()});
  while (!Won()) {
    int ch = getchar();
    if (ch == '\n') {
      Print();
      printf("ULDR: ");
    } else if (ch == 'z' || ch == 'Z') {
      if (undoHistory.Size() > 1) { // Cannot pop the initial state
        undoHistory.Pop();
        SetState(&undoHistory[undoHistory.Size()-1]); // The state at the (new) end of the list
      }
    } else if (ch == 'q' || ch == 'Q') {
      printf("Reset level\n");
      SetState(&undoHistory[0]);
      undoHistory.Resize(1);
    } else {
      Direction dir = None;
      if (ch == 'u' || ch == 'U') dir = Up;
      else if (ch == 'd' || ch == 'D') dir = Down;
      else if (ch == 'l' || ch == 'L') dir = Left;
      else if (ch == 'r' || ch == 'R') dir = Right;
      if (dir != None) {
        if (Move(dir)) { // Valid move, add to the history
          undoHistory.Push(GetState());
        } else { // Invalid move, restore to fix side-effects
          SetState(&undoHistory[undoHistory.Size()-1]);
        }
      }
    }
  }

  printf("Level completed in %d moves\n", undoHistory.Size() - 1);
  _interactive = false;
  SetState(&undoHistory[0]); // Restore the initial state
  return true;
}

State Level::GetState() const {
  State s{_stephen};
  assert(sizeof(s.sausages) / sizeof(Sausage) == _sausages.Size());
#if SORT_SAUSAGE_STATE
  _sausages.SortedCopyIntoArray(s.sausages, sizeof(s.sausages), [](const Sausage& a, const Sausage& b) -> s8 {
    if (a.x1 > b.x1) return 1;
    if (a.y1 > b.y1) return 1;
    return -1;
  });
#else
  _sausages.CopyIntoArray(s.sausages, sizeof(s.sausages));
#endif

#if HASH_CACHING
  s.hash = s.Hash();
#endif
  return s;
}

void Level::SetState(const State* s) {
  _stephen = s->stephen;
  _sausages.CopyFromArray(s->sausages, sizeof(s->sausages));

  // Speared state is not saved, because it's recoverable. Memory > speed tradeoff.
  // TODO: Uhh, I think I'm more CPU bound these days? Not sure.
  if (_stephen.HasFork()) _sausageSpeared = GetSausage(_stephen.forkX, _stephen.forkY, _stephen.forkZ);
}

bool Level::Move(Direction dir) {
  stackcheck_begin();

  bool handled = false;
  if (!HandleLogRolling(dir, handled)) return false;
  if (!handled) {
    if (!HandleLadderMotion(dir, handled)) return false;
    if (!handled) {
      if (!HandleRotation(dir, handled)) return false;
      if (!handled) {
        if (!MoveStephenThroughSpace(dir)) return false;
      }
    }
  }

  // Can occur after (most) movements, so handle it commonly.
  if (!HandleBurnedStep(dir, handled)) return false;
  if (!HandleForkReconnect(dir, handled)) return false;

#if OVERWORLD_HACK // In the overworld, sausages disappear when you step on things. Not sure if this is the right place for this hack tbf.
  Vector<char> sausagesToRemove;
#if OVERWORLD_HACK == 1
  if (_stephen.x == 23 && _stephen.y == 25 && _stephen.dir == Up)     sausagesToRemove = {'A', 'B', 'C'};
  if (_stephen.x == 20 && _stephen.y == 21 && _stephen.dir == Right)  sausagesToRemove = {'D', 'E'};
  if (_stephen.x == 20 && _stephen.y == 20 && _stephen.dir == Left)   sausagesToRemove = {'F', 'G'};
  if (_stephen.x == 21 && _stephen.y == 14 && _stephen.dir == Left)   sausagesToRemove = {'H', 'I'};
  if (_stephen.x == 21 && _stephen.y == 12 && _stephen.dir == Right)  sausagesToRemove = {'J', 'K'};
  if (_stephen.x == 25 && _stephen.y == 11 && _stephen.dir == Left)   sausagesToRemove = {'L', 'M'};
  if (_stephen.x == 20 && _stephen.y == 7  && _stephen.dir == Left)   sausagesToRemove = {'N'};
  if (_stephen.x == 21 && _stephen.y == 6  && _stephen.dir == Right)  sausagesToRemove = {'O', 'P'};
  if (_stephen.x == 14 && _stephen.y == 13 && _stephen.dir == Down)   sausagesToRemove = {'Q'};
  if (_stephen.x == 12 && _stephen.y == 14 && _stephen.dir == Up)     sausagesToRemove = {'R'};
  if (_stephen.x == 11 && _stephen.y == 18 && _stephen.dir == Down)   sausagesToRemove = {'S', 'T'};
  if (_stephen.x == 5  && _stephen.y == 14 && _stephen.dir == Up)     sausagesToRemove = {'U', 'V'};
  if (_stephen.x == 4  && _stephen.y == 14 && _stephen.dir == Up)     sausagesToRemove = {'W', 'X'};
  if (_stephen.x == 3  && _stephen.y == 8  && _stephen.dir == Right)  sausagesToRemove = {'Y', 'Z', 'a'};
  if (_stephen.x == 10 && _stephen.y == 9  && _stephen.dir == Up)     sausagesToRemove = {'b', 'c'};
  if (_stephen.x == 9  && _stephen.y == 3  && _stephen.dir == Up)     sausagesToRemove = {'d', 'e', 'f'};
  if (_stephen.x == 13 && _stephen.y == 3  && _stephen.dir == Left)   sausagesToRemove = {'g'};
  s8 numRedSausages = 32;
#elif OVERWORLD_HACK == 2
  if (_stephen.x == 33 && _stephen.y == 3  && _stephen.dir == Right)  sausagesToRemove = {'A', 'B'};
  if (_stephen.x == 35 && _stephen.y == 7  && _stephen.dir == Down)   sausagesToRemove = {'C', 'D'};
  if (_stephen.x == 29 && _stephen.y == 18 && _stephen.dir == Right)  sausagesToRemove = {'E', 'F'};
  if (_stephen.x == 15 && _stephen.y == 27 && _stephen.dir == Right)  sausagesToRemove = {'G', 'H'};
  if (_stephen.x == 28 && _stephen.y == 33 && _stephen.dir == Up)     sausagesToRemove = {'I'};
  if (_stephen.x == 26 && _stephen.y == 30 && _stephen.dir == Down)   sausagesToRemove = {'J', 'K'};
  if (_stephen.x == 38 && _stephen.y == 40 && _stephen.dir == Right)  sausagesToRemove = {'L'};
  if (_stephen.x == 12 && _stephen.y == 37 && _stephen.dir == Right)  sausagesToRemove = {'M'};
  if (_stephen.x == 11 && _stephen.y == 31 && _stephen.dir == Right)  sausagesToRemove = {'N', 'O'};
  if (_stephen.x == 2  && _stephen.y == 34 && _stephen.dir == Right)  sausagesToRemove = {'P', 'Q'};
  if (_stephen.x == 18 && _stephen.y == 21 && _stephen.dir == Right)  sausagesToRemove = {'R'};
  s8 numRedSausages = 17;
#elif OVERWORLD_HACK == 3
  if (_stephen.x == 5  && _stephen.y == 15 && _stephen.dir == Down)   sausagesToRemove = {'A'};
  if (_stephen.x == 15 && _stephen.y == 13 && _stephen.dir == Down)   sausagesToRemove = {'B', 'C'};
  if (_stephen.x == 24 && _stephen.y == 21 && _stephen.dir == Right)  sausagesToRemove = {'F', 'G'};
  if (_stephen.x == 26 && _stephen.y == 4  && _stephen.dir == Right)  sausagesToRemove = {'H', 'I'};
  if (_stephen.x == 25 && _stephen.y == 11 && _stephen.dir == Up)     sausagesToRemove = {'J'};
  if (_stephen.x == 4  && _stephen.y == 22 && _stephen.dir == Left)   sausagesToRemove = {'K'};
  s8 numRedSausages = 10;
#endif

  for (char sausageToRemove : sausagesToRemove) {
    u8 sausageNo = (sausageToRemove > 'Z') ? sausageToRemove - 'a' + 26 : sausageToRemove - 'A';

    if (sausageNo == numRedSausages) {
      bool allOtherSausagesCooked = true;
      for (s8 i = 0; i < numRedSausages; i++) {
        if (_sausages[i].x1 > 0 && (_sausages[i].IsFullyCooked()) {
          allOtherSausagesCooked = false;
          break;
        }
      }
      if (!allOtherSausagesCooked) continue;
    }

    _sausages[sausageNo].z = -2;
    _sausages[sausageNo].flags = Sausage::Flags::FullyCooked;
  }
#endif

  return true;
}

bool Level::HandleLogRolling(Direction dir, bool& handled) {
  s8 standingOnSausage = GetSausage(_stephen.x, _stephen.y, _stephen.z - 1);
  if (standingOnSausage == -1) return true; // Not handled, but not useless
  Sausage sausage = _sausages[standingOnSausage];

  if (dir == Up && sausage.IsHorizontal() && (_stephen.dir == Up || _stephen.dir == Down)) {
    if (CanPhysicallyMove(sausage.x1, sausage.y1, sausage.z, Down)
     && CanPhysicallyMove(sausage.x2, sausage.y2, sausage.z, Down)) {
      if (!MoveThroughSpace(sausage.x1, sausage.y1, sausage.z, Down)) return false; // This *should* move the entire sausage.
      if (!MoveStephenThroughSpace(Down)) return false;
      handled = true;
    }
  } else if (dir == Down && sausage.IsHorizontal() && (_stephen.dir == Up || _stephen.dir == Down)) {
    if (CanPhysicallyMove(sausage.x1, sausage.y1, sausage.z, Up)
     && CanPhysicallyMove(sausage.x2, sausage.y2, sausage.z, Up)) {
      if (!MoveThroughSpace(sausage.x1, sausage.y1, sausage.z, Up)) return false; // This *should* move the entire sausage.
      if (!MoveStephenThroughSpace(Up)) return false;
      handled = true;
    }
  } else if (dir == Left && sausage.IsVertical() && (_stephen.dir == Left || _stephen.dir == Right)) {
    if (CanPhysicallyMove(sausage.x1, sausage.y1, sausage.z, Right)
     && CanPhysicallyMove(sausage.x2, sausage.y2, sausage.z, Right)) {
      if (!MoveThroughSpace(sausage.x1, sausage.y1, sausage.z, Right)) return false; // This *should* move the entire sausage.
      if (!MoveStephenThroughSpace(Right)) return false;
      handled = true;
    }
  } else if (dir == Right && sausage.IsVertical() && (_stephen.dir == Left || _stephen.dir == Right)) {
    if (CanPhysicallyMove(sausage.x1, sausage.y1, sausage.z, Left)
     && CanPhysicallyMove(sausage.x2, sausage.y2, sausage.z, Left)) {
      if (!MoveThroughSpace(sausage.x1, sausage.y1, sausage.z, Left)) return false; // This *should* move the entire sausage.
      if (!MoveStephenThroughSpace(Left)) return false;
      handled = true;
    }
  }
  if (handled) {
    // We've handled the lateral motion, so now we handle dropping stephen (and any sausages) down
    while (true) {
      bool stephenSupported = CanWalkOnto(_stephen.x, _stephen.y, _stephen.z);
      if (stephenSupported) break; // Landed safely
      if (_stephen.z <= 0) FAIL("Stephen would log roll off the world");

      // Handle fork disconnect (logroll where stephen is facing backwards)
      if (_stephen.HasFork()) {
        if (!CanWalkOnto(_stephen.forkX, _stephen.forkY, _stephen.forkZ)) {
          _stephen.forkDir = _stephen.dir; // Set the fork's direction to mark it as disconnected.
          assert(!_stephen.HasFork());
        }
      }
      _stephen.z--;
    }
  }

  return true;
}

bool Level::HandleLadderMotion(Direction dir, bool& handled) {
  bool climbUp = false;
  bool climbDown = false;
  if (_stephen.HasFork()) {
    if (dir == Up || dir == Down) {
      climbUp = climbDown = (_stephen.dir == Left || _stephen.dir == Right);
    } else if (dir == Left || dir == Right) {
      climbUp = climbDown = (_stephen.dir == Up || _stephen.dir == Down);
    }
  } else {
    climbUp = (_stephen.dir == dir);
    climbDown = (_stephen.dir == Inverse(dir));
  }

  if (climbUp) {
    // Keep climbing up while there is a ladder in our cell in the appropriate direction
    while (true) {
      if (!IsLadder(_stephen.x, _stephen.y, _stephen.z, dir)) break;
      if (!MoveStephenThroughSpace(Jump, true)) return false;
      handled = true;
    }
    if (handled) {
      if (!MoveStephenThroughSpace(dir)) return false;
      return true;
    }
  }

  if (climbDown) {
    if (dir == Up) {
      if (CanWalkOnto(_stephen.x, _stephen.y - 1, _stephen.z)) return true; // Ladder is blocked; use another movement system.
      if (  !IsLadder(_stephen.x, _stephen.y - 1, _stephen.z - 1, Inverse(dir))) return true; // No ladder present; use another movement system.
    } else if (dir == Down) {
      if (CanWalkOnto(_stephen.x, _stephen.y + 1, _stephen.z)) return true; // Ladder is blocked; use another movement system.
      if (  !IsLadder(_stephen.x, _stephen.y + 1, _stephen.z - 1, Inverse(dir))) return true; // No ladder present; use another movement system.
    } else if (dir == Left) {
      if (CanWalkOnto(_stephen.x - 1, _stephen.y, _stephen.z)) return true; // Ladder is blocked; use another movement system.
      if (  !IsLadder(_stephen.x - 1, _stephen.y, _stephen.z - 1, Inverse(dir))) return true; // No ladder present; use another movement system.
    } else if (dir == Right) {
      if (CanWalkOnto(_stephen.x + 1, _stephen.y, _stephen.z)) return true; // Ladder is blocked; use another movement system.
      if (  !IsLadder(_stephen.x + 1, _stephen.y, _stephen.z - 1, Inverse(dir))) return true; // No ladder present; use another movement system.
    }

    if (!MoveStephenThroughSpace(dir, true)) return false; // Move stephen over the ladder
    while (true) { // Descend while there is a ladder below us
      if (_stephen.z <= 0) FAIL("Stephen cannot descend through the floor");
      if (!MoveStephenThroughSpace(Crouch)) return false;
      if (CanWalkOnto(_stephen.x, _stephen.y, _stephen.z)) break; // If stephen is supported (by ground or sausage), he steps off the ladder.

      // There's air below us, check for another ladder
      if (!IsLadder(_stephen.x, _stephen.y, _stephen.z - 1, Inverse(dir))) FAIL("Stephen cannot stand on anything at the bottom of the ladder");
    }
    handled = true;
    return true;
  }

  return true;
}

bool Level::HandleBurnedStep(Direction dir, bool& handled) {
  if (!IsGrill(_stephen.x, _stephen.y, _stephen.z)) return true; // Not handled, but not useless
  handled = true;

  stackcheck(); // In some bugs, stephen can be standing in the middle of a grill, and could "bounce" between two grills.
  return Move(Inverse(dir));
}

bool Level::HandleForkReconnect(Direction dir, bool& handled) {
  if (!_stephen.HasFork() && _stephen.z == _stephen.forkZ && _stephen.dir == _stephen.forkDir) {
    bool reconnectFork = false;
    if (_stephen.dir == Up) {
      reconnectFork = (_stephen.x == _stephen.forkX && _stephen.y - 1 == _stephen.forkY);
    } else if (_stephen.dir == Down) {
      reconnectFork = (_stephen.x == _stephen.forkX && _stephen.y + 1 == _stephen.forkY);
    } else if (_stephen.dir == Left) {
      reconnectFork = (_stephen.x == _stephen.forkX - 1 && _stephen.y == _stephen.forkY);
    } else if (_stephen.dir == Right) {
      reconnectFork = (_stephen.x == _stephen.forkX + 1 && _stephen.y == _stephen.forkY);
    }
    if (reconnectFork) {
      handled = true;
      _stephen.forkDir = None;
    }
  }

  return true;
}

bool Level::HandleRotation(Direction dir, bool& handled) {
  if (dir == _stephen.dir || dir == Inverse(_stephen.dir)) return true; // Not handled, but not useless
  if (_sausageSpeared != -1) return true; // Stephen does not rotate while he has speared a sausage
  handled = true;

  // TODO: Handle sausage hat rotation here? It's currently in MTS which seems odd.
  if (!_stephen.HasFork()) { // Forkless motion is very simple. The if statements below are handling fork motion.
    _stephen.dir = dir;
    return true;
  }

  if (dir == Up) {
    s8 clockwise = (_stephen.dir == Left ? +1 : -1);
    if (!MoveThroughSpace( _stephen.forkX, _stephen.y - 1, _stephen.z, dir, clockwise)) return false;
    if (!CanPhysicallyMove(_stephen.x,     _stephen.y - 1, _stephen.z, Inverse(_stephen.dir), true)) return true; // Bonk
    _stephen.forkX = _stephen.x;
    _stephen.forkY = _stephen.y - 1;
    if (!MoveThroughSpace( _stephen.x,     _stephen.y - 1, _stephen.z, Inverse(_stephen.dir), clockwise, true, true)) return false;
    _stephen.dir = dir;
  } else if (dir == Down) {
    s8 clockwise = (_stephen.dir == Right ? +1 : -1);
    if (!MoveThroughSpace( _stephen.forkX, _stephen.y + 1, _stephen.z, dir, clockwise)) return false;
    if (!CanPhysicallyMove(_stephen.x,     _stephen.y + 1, _stephen.z, Inverse(_stephen.dir), true)) return true; // Bonk
    _stephen.forkX = _stephen.x;
    _stephen.forkY = _stephen.y + 1;
    if (!MoveThroughSpace( _stephen.x,     _stephen.y + 1, _stephen.z, Inverse(_stephen.dir), clockwise, true, true)) return false;
    _stephen.dir = dir;
  } else if (dir == Left) {
    s8 clockwise = (_stephen.dir == Down ? +1 : -1);
    if (!MoveThroughSpace( _stephen.x - 1, _stephen.forkY, _stephen.z, dir, clockwise)) return false;
    if (!CanPhysicallyMove(_stephen.x - 1, _stephen.y,     _stephen.z, Inverse(_stephen.dir), true)) return true; // Bonk
    _stephen.forkX = _stephen.x - 1;
    _stephen.forkY = _stephen.y;
    if (!MoveThroughSpace( _stephen.x - 1, _stephen.y,     _stephen.z, Inverse(_stephen.dir), clockwise, true, true)) return false;
    _stephen.dir = dir;
  } else if (dir == Right) {
    s8 clockwise = (_stephen.dir == Up ? +1 : -1);
    if (!MoveThroughSpace( _stephen.x + 1, _stephen.forkY, _stephen.z, dir, clockwise)) return false;
    if (!CanPhysicallyMove(_stephen.x + 1, _stephen.y,     _stephen.z, Inverse(_stephen.dir), true)) return true; // Bonk
    _stephen.forkX = _stephen.x + 1;
    _stephen.forkY = _stephen.y;
    if (!MoveThroughSpace( _stephen.x + 1, _stephen.y,     _stephen.z, Inverse(_stephen.dir), clockwise, true, true)) return false;
    _stephen.dir = dir;
  } else {
    assert(false);
    FAIL("Attempted to rotate stephen in direction %s", DIRS[dir]);
  }

  return true;
}

bool Level::Consider(s8 sausageNo) {
  if (sausageNo < 0) return false;
  u8 mask = (1 << sausageNo);
  assert(mask == (1 << sausageNo)); // Assert no truncation
  assert(sizeof(mask) == sizeof(data.consideredSausages)); // Assert correct size
  if (data.consideredSausages & mask) return false; // Already considered
  data.consideredSausages |= mask;
  return true;
}

bool Level::CanPhysicallyMove(s8 x, s8 y, s8 z, Direction dir, bool stephenIsRotating) {
  // Reset the struct rather than reallocating it.
  data.movedSausages.Resize(0);
  data.sausagesToDrop.Resize(0);
  data.sausageToSpear = -1;
  data.sausageHat = -1;
  data.consideredSausages = 0;
  data.sausagesToDoubleMove = 0;
  data.pushedFork = false;
  data.canPhysicallyMove = false;

  if (!CanPhysicallyMoveInternal(x, y, z, dir)) return false;
  data.canPhysicallyMove = true;
  CheckForSausageCarry(x, y, z, dir, stephenIsRotating);
  return true;
}

bool Level::CanPhysicallyMoveInternal(s8 x, s8 y, s8 z, Direction dir) {
  stackcheck();
  if (IsWall(x, y, z)) return false; // No, walls cannot move.

  s8 dx = 0;
  s8 dy = 0;
  s8 dz = 0;
  if (dir == Up)          dy = -1;
  else if (dir == Down)   dy = +1;
  else if (dir == Left)   dx = -1;
  else if (dir == Right)  dx = +1;
  else if (dir == Crouch) dz = -1; // Nothing can move down, ever.
  else if (dir == Jump)   dz = +1;
  else assert(false);

  // We check for a sausage first, because if the fork is inside a sausage we don't need think about its motion separately,
  // it simply rolls along with the sausage.
  s8 sausageNo = GetSausage(x, y, z);
  if (sausageNo == -1) {
    if (!_stephen.HasFork() && x == _stephen.forkX && y == _stephen.forkY && z == _stephen.forkZ) {
      data.pushedFork = true;
      if (_stephen.forkDir == dir) {
        data.sausageToSpear = GetSausage(x + dx, y + dy, z); // We don't need to add dz here because this only happens for UDLR.
      } else if (_stephen.forkDir == Inverse(dir)) {
        data.sausageToSpear = GetSausage(x - dx, y - dy, z); // We don't need to add dz here because this only happens for UDLR.
      } else {
        data.sausageToSpear = -1; // Can't push the fork into a sausage in this direction
      }
      return CanPhysicallyMoveInternal(x + dx, y + dy, z + dz, dir);
    }
    return true;
  }

#if OVERWORLD_HACK
  return false; // Sausages cannot move in the overworld
#endif

  if (!Consider(sausageNo)) return true; // Already analyzed
  if (data.sausageToSpear == -1) data.sausageToSpear = sausageNo; // If spearing is possible, the first sausage we encounter will be our spear target.
  Sausage sausage = _sausages[sausageNo];

  // Check if both of the sausage halves can move, recursively.
  if (!CanPhysicallyMoveInternal(sausage.x1 + dx, sausage.y1 + dy, sausage.z + dz, dir)) return false;
  if (!CanPhysicallyMoveInternal(sausage.x2 + dx, sausage.y2 + dy, sausage.z + dz, dir)) return false;
  data.movedSausages.Push(sausageNo);
  return true;
}

bool Level::IsSausageCarried(s8 x, s8 y, s8 z, Direction dir, bool stephenIsRotating, bool canDoubleMove) {
  s8 sausageNo = GetSausage(x, y, z+1);
  if (!Consider(sausageNo)) return false; // Invalid or already known to be moving
  Sausage sausage = _sausages[sausageNo];

  // Find the x and y which are not supported by our caller
  s8 otherX;
  s8 otherY;
  if (sausage.x1 == x && sausage.y1 == y) {
    otherX = sausage.x2;
    otherY = sausage.y2;
  } else { assert(sausage.x2 == x && sausage.y2 == y);
    otherX = sausage.x1;
    otherY = sausage.y1;
  }

  if (IsWall(otherX, otherY, z)) return false; // Other support is a wall

  bool thisSupportIsStephen = (_stephen.x == x && _stephen.y == y && _stephen.z == z);
  bool otherSupportIsStephen = (_stephen.x == otherX && _stephen.y == otherY && _stephen.z == z);
  if (stephenIsRotating && (thisSupportIsStephen || otherSupportIsStephen)) return false; // While stephen is rotating, he counts as a wall

  bool thisSupportIsFork = (_stephen.forkX == x && _stephen.forkY == y && _stephen.forkZ == z);
  bool otherSupportIsFork = (_stephen.forkX == otherX && _stephen.forkY == otherY && _stephen.forkZ == z);
  if (stephenIsRotating && _stephen.HasFork()
      && (thisSupportIsFork || otherSupportIsFork)) return false; // While stephen's fork is rotating, it acts like a wall in its final position
  if (otherSupportIsFork && !_stephen.HasFork() && !data.pushedFork) return false; // Disconnected forks act like a wall when not moving

  bool thisSupportIsSausage = !(thisSupportIsStephen || thisSupportIsFork); // Someone must've called us.
  s8 otherSausageNo = GetSausage(otherX, otherY, z);
  bool otherSupportIsSausage = (otherSausageNo != -1);

  if (otherSupportIsSausage && !data.movedSausages.Contains(otherSausageNo)) return false;  // Other support is a sausage which is not moving

  if ((thisSupportIsStephen && !otherSupportIsSausage && !otherSupportIsFork)
      || (otherSupportIsStephen && !thisSupportIsSausage && !thisSupportIsFork)) {
    data.sausageHat = sausageNo; // Supported by just stephen, so this is a hat
  }

  // Check for double-move
  // bool canDoubleMove = true;
  {
    if (otherSupportIsFork) canDoubleMove = false; // Being supported by the fork prevents a double-move
    if (otherSupportIsStephen) canDoubleMove = false; // Being supported by stephen prevents a double-move
  }

  // Preconditions for double-move were satisfied, i.e. not being supported by stephen or his fork.
  // Also, this checks that the primary support (i.e. our caller) is also facing perpendicular to the motion.
  if (canDoubleMove) {
    // Sausages can only double-move if they are in the same direction as the motion,
    // and if the sausage(s) that support them are perpendicular to the motion.
    if (dir == Up || dir == Down) {
      if (sausage.IsVertical() && (otherSausageNo == -1 || _sausages[otherSausageNo].IsHorizontal())) {
        data.sausagesToDoubleMove |= (1 << sausageNo);
      }
    } else { assert(dir == Left || dir == Right);
      if (sausage.IsHorizontal() && (otherSausageNo == -1 || _sausages[otherSausageNo].IsVertical())) {
        data.sausagesToDoubleMove |= (1 << sausageNo);
      }
    }
  }

  // If we've reached here, the other support is air or is also moving, so this sausage will move too.
  data.movedSausages.Push(sausageNo);
  return true;
}

void Level::CheckForSausageCarry(s8 x, s8 y, s8 z, Direction dir, bool stephenIsRotating) {
  if (dir == Crouch || dir == Jump) return; // Sausages can only be carried laterally (UDLR)

  bool anySausagesMoved;
  do {
    anySausagesMoved = false;
    for (s8 sausageNo : data.movedSausages) {
      Sausage sausage = _sausages[sausageNo];

      bool canDoubleMove = false;
      if (dir == Up || dir == Down) {
        if (sausage.IsHorizontal()) canDoubleMove = true;
      } else { assert(dir == Left || dir == Right);
        if (sausage.IsVertical()) canDoubleMove = true;
      }
      anySausagesMoved |= IsSausageCarried(sausage.x1, sausage.y1, sausage.z, dir, stephenIsRotating, canDoubleMove);
      anySausagesMoved |= IsSausageCarried(sausage.x2, sausage.y2, sausage.z, dir, stephenIsRotating, canDoubleMove);
    }
  } while (anySausagesMoved);
}

bool Level::MoveThroughSpace(s8 x, s8 y, s8 z, Direction dir, s8 stephenRotationDir, bool checkSausageCarry, bool doSausageRotation, bool doDoubleMove) {
  // TODO: Maybe cache & check the last CPM call? When we rotate, we make the same call twice in a row.
  // Actually we just want a function that we can safely call from HandleRotation -- i.e. it succeeds if CPM return false.
  // It's a little trickier than that, because we move the fork in between CPM and MTS while rotating (by design, used for fork carries).
  // The first CPM call really should be a lightweight one that just checks sausage pushing -- we just want to know if we bonked.
  CanPhysicallyMove(x, y, z, dir, stephenRotationDir != 0);
  // I think this is the correct code, I'm just not sure it's in the right spot.
  // eugh. really? I apparently have to reconstruct the original location of Stephen / his fork. This feels very gross. Sigh.
  if (checkSausageCarry) {
    IsSausageCarried(_stephen.x, _stephen.y, _stephen.z, dir, stephenRotationDir != 0, true);
    if (_stephen.HasFork()) IsSausageCarried(_stephen.forkX, _stephen.forkY, _stephen.forkZ, dir, stephenRotationDir != 0, true);
  }
  /*
  s8 sausageNo = GetSausage(_stephen.x, _stephen.y, _stephen.z + 1);
  if (sausageNo != -1) {
    Sausage sausage = _sausages[sausageNo];
    if (_stephen.x == sausage.x1 && _stephen.y == sausage.y1) {
      IsSausageCarried(sausage.x2, sausage.y2, _stephen.z, dir, stephenRotationDir != 0, false);
    } else { assert(_stephen.x == sausage.x2 && _stephen.y == sausage.y2);
      IsSausageCarried(sausage.x1, sausage.y1, _stephen.z, dir, stephenRotationDir != 0, false);
    }
  }
  */
  return MoveThroughSpaceInternal(x, y, z, dir, stephenRotationDir, doSausageRotation, doDoubleMove);
}

bool Level::MoveThroughSpace3(Direction dir, s8 stephenRotationDir) {
  // okay, wait
  // This function is for 'move a sausage that is on stephen's head'
  // So, if a sausage is supported by something else, it doesn't move.
  // Oh, we don't want all the machinery. I think.
  // Just this? Really?
  // "If there's a sausage here" (implicit in the carry check)
  // "And it's being carried" (supported only by other moving sausages)
  // "Then move it, without recomputing the movement list"
  bool forkCarry = IsSausageCarried(_stephen.forkX, _stephen.forkY, _stephen.forkZ, dir, stephenRotationDir != 0, false);
  bool headCarry = IsSausageCarried(_stephen.x,     _stephen.y,     _stephen.z,     dir, stephenRotationDir != 0, false);
  if (!forkCarry && !headCarry) return true; // Success but does nothing

  // pretty sure this x, y, z is for reporting only but it's worth a comment.
  return MoveThroughSpaceInternal(_stephen.x, _stephen.y, _stephen.z, dir, stephenRotationDir);
}

bool Level::MoveThroughSpaceInternal(s8 x, s8 y, s8 z, Direction dir, s8 stephenRotationDir, bool doSausageRotation, bool doDoubleMove) {
  bool canPhysicallyMove = data.canPhysicallyMove;
  if (!canPhysicallyMove) {
    // Stephen can only spear when he is moving forwards. (Note that we have already inverted |dir| if this is a log roll)
    bool canSpear = (_stephen.HasFork() && dir == _stephen.dir);
    if (canSpear && data.sausageToSpear != -1) {
      _sausageSpeared = data.sausageToSpear;
      // This location cannot move, but we can still move into it (by spearing).
      return true;
    }
    if (data.pushedFork && data.sausageToSpear != -1) {
      _sausageSpeared = data.sausageToSpear;
      // This location cannot move *but* the fork can move, which may allow the sausages to move.
      // This is a destructive action.
      if (dir == Up)         _stephen.forkY--;
      else if (dir == Down)  _stephen.forkY++;
      else if (dir == Left)  _stephen.forkX--;
      else if (dir == Right) _stephen.forkX++;
      // Then, try the move again, and if we still can't move, give up.
      canPhysicallyMove = CanPhysicallyMove(x, y, z, dir);
      if (!canPhysicallyMove) FAIL("Stephen's fork can stick into sausage %c but the move is still impossible", 'a' + data.sausageToSpear);
      // We successfully stuck the fork into a sausage, continue into the main block.
    } else {
      if (!_interactive) FAIL("");

      // This branch can be hit from a bunch of places. Let's try to give a useful error message.
      if (canSpear) FAIL("Stephen's fork cannot move in direction %s, and there is nothing to spear", DIRS[dir]);
      if (data.sausageToSpear != -1) FAIL("Cannot move sausage %c in direction %s", 'a' + data.sausageToSpear, DIRS[dir]);
      FAIL("Wall at [%d, %d, %d] cannot move %s (probably you pushed a fork into it)", x, y, z, DIRS[dir]);
    }
  }

  // The iteration order here is important -- we need to move lower sausages first so that stacked sausages drop properly.
  for (s8 sausageNo : data.movedSausages) {
    Sausage sausage = _sausages[sausageNo];

    // Move the sausage
    if (dir == Up) {
      sausage.y1--;
      sausage.y2--;
      if (!_stephen.HasFork() && sausageNo == _sausageSpeared) _stephen.forkY--;
    } else if (dir == Down) {
      sausage.y1++;
      sausage.y2++;
      if (!_stephen.HasFork() && sausageNo == _sausageSpeared) _stephen.forkY++;
    } else if (dir == Left) {
      sausage.x1--;
      sausage.x2--;
      if (!_stephen.HasFork() && sausageNo == _sausageSpeared) _stephen.forkX--;
    } else if (dir == Right) {
      sausage.x1++;
      sausage.x2++;
      if (!_stephen.HasFork() && sausageNo == _sausageSpeared) _stephen.forkX++;
    } else if (dir == Crouch) {
      sausage.z--;
      if (!_stephen.HasFork() && sausageNo == _sausageSpeared) _stephen.forkZ--;
    } else if (dir == Jump) {
      sausage.z++;
      if (!_stephen.HasFork() && sausageNo == _sausageSpeared) _stephen.forkZ++;
    } else {
      assert(false);
      FAIL("Cannot move sausage %c in direction %d", 'a' + sausageNo, dir);
    }

    if (_stephen.HasFork() && sausageNo == _sausageSpeared) {
      // Speared sausages do not roll nor fall
    } else if (sausageNo == data.sausageHat) {
      // Sausage hats similarly do not roll nor fall
    } else {
      // If the sausage rolled, it might also drop.
      // TODO: Sloppy. We should have this information during CanPhysicallyMove.
      data.sausagesToDrop.Push(sausageNo);

      // Check to see if the sausage rolls.
      // Also, check to see if the fork rolls -- note that it only rolls if it is perpendicular to the sausage (parallel to the direction of motion).
      if (sausage.IsHorizontal()) {
        if (dir == Up || dir == Down) {
          sausage.flags ^= Sausage::Flags::Rolled;
          if (!_stephen.HasFork() && sausageNo == _sausageSpeared && (_stephen.forkDir == Up || _stephen.forkDir == Down)) {
            _stephen.forkDir = Inverse(_stephen.forkDir);
          }
        }
      } else { assert(sausage.IsVertical());
        if (dir == Left || dir == Right) {
          sausage.flags ^= Sausage::Flags::Rolled;
          if (!_stephen.HasFork() && sausageNo == _sausageSpeared && (_stephen.forkDir == Left || _stephen.forkDir == Right)) {
            _stephen.forkDir = Inverse(_stephen.forkDir);
          }
        }
      }
    }

    _sausages[sausageNo] = sausage;
  }

  // And now we handle double-moves by just moving every marked sausage again.
  // TODO: Cooking two sides using a double move?
  if (doDoubleMove && data.sausagesToDoubleMove != 0) {
    // Make a copy since data will be overwritten after we call ourselves again.
    u8 sausagesToDoubleMove = data.sausagesToDoubleMove;
    for (s8 sausageNo=0; sausageNo<_sausages.Size(); sausageNo++) {
      if (sausagesToDoubleMove & (1 << sausageNo)) {
        Sausage sausage = _sausages[sausageNo];
        if (!MoveThroughSpace(sausage.x1, sausage.y1, sausage.z, dir, stephenRotationDir, false, false)) return false; // Avoid infinite-ish recursion
  
        // If any sausages moved as a part of this, they don't need to double-move (since they did just double-move).
        for (s8 sausageNo2 : data.movedSausages) sausagesToDoubleMove &= ~(1 << sausageNo2);
      }
    }
  }

  // The order here needs to be from bottom to top, fortunately this is the same order that we add sausages to the list in.
#if _DEBUG
  s8 z_ = -1;
  for (s8 sausageNo : data.sausagesToDrop) {
    Sausage sausage = _sausages[sausageNo];
    assert(sausage.z >= z_);
    z_ = sausage.z;
  }
#endif
  for (s8 sausageNo : data.sausagesToDrop) {
    Sausage sausage = _sausages[sausageNo];

    while (true) {
      bool supported = CanWalkOnto(sausage.x1, sausage.y1, sausage.z)
                    || CanWalkOnto(sausage.x2, sausage.y2, sausage.z);
      if (supported) break;
      if (sausage.z <= 0) FAIL("Sausage %c would fall below the world", 'a' + sausageNo);
      sausage.z--;
      if (!_stephen.HasFork() && sausageNo == _sausageSpeared) _stephen.forkZ--;
    }

    // Cook the sausage
    u8 sidesToCook = 0;
    if (IsGrill(sausage.x1, sausage.y1, sausage.z)) sidesToCook |= Sausage::Flags::Cook1A;
    if (IsGrill(sausage.x2, sausage.y2, sausage.z)) sidesToCook |= Sausage::Flags::Cook2A;

    if (sidesToCook != 0) {
      if (sausage.flags & Sausage::Flags::Rolled) sidesToCook *= 2; // Shift cooking flags to the rolled side
      if (sausage.flags & sidesToCook) FAIL("Sausage %c would burn", 'a' + sausageNo);
      sausage.flags |= sidesToCook;
    }

    _sausages[sausageNo] = sausage;
  }

  if (data.pushedFork) { // This boolean is only set if the fork is not inside a sausage.
    if (dir == Up) {
      _stephen.forkY--;
    } else if (dir == Down) {
      _stephen.forkY++;
    } else if (dir == Left) {
      _stephen.forkX--;
    } else if (dir == Right) {
      _stephen.forkX++;
    } else if (dir == Jump) {
      _stephen.forkZ++;
    } else if (dir == Crouch) {
      assert(false);
      FAIL("Cannot move disconnected fork in direction %d", dir);
    }
  }

  // TODO: What if dropping the fork means a sausage is unsupported? I'm really thinking about making a dummy sausage for the fork.
  // How do I usually compute supportability? Why can't this work like that?
  if (!_stephen.HasFork()) {
    while (true) {
      bool supported = CanWalkOnto(_stephen.forkX, _stephen.forkY, _stephen.forkZ);
      if (supported) break;
      if (_stephen.forkZ <= 0) FAIL("Disconnected fork would fall below the world");
      _stephen.forkZ--;
    }
  }

  // And now we handle rotation by another mess of if statements.
  // Rotation is made of two separate moves, and we only rotate sausages on the second one.
  if (doSausageRotation && data.sausageHat != -1) {
    assert(stephenRotationDir);
    Sausage& sausage = _sausages[data.sausageHat];
    while (true) { // Recurse until we stop finding things to rotate. We'll change sausage at the end of the loop.
      // Because x1 <= x2 and y1 <= y2, there are only 4 ways fo a sausage to be on stephen's head. In the ASCII art, stephen is in the middle.
      if (sausage.x1 == _stephen.x - 1) {
        assert(sausage.y1 == _stephen.y); // ___
        assert(sausage.x2 == _stephen.x); // 12_
        assert(sausage.y2 == _stephen.y); // ___
        if (stephenRotationDir == +1) {
          if (!CanPhysicallyMove(sausage.x1,     sausage.y1 - 1, sausage.z, Up)
           || !CanPhysicallyMove(sausage.x1 + 1, sausage.y1 - 1, sausage.z, Right)) break;
          if  (!MoveThroughSpace(sausage.x1,     sausage.y1 - 1, sausage.z, Up)) return false;
          if  (!MoveThroughSpace(sausage.x1 + 1, sausage.y1 - 1, sausage.z, Right)) return false;
          sausage.x1++;
          sausage.y1--;
        } else { assert(stephenRotationDir == -1);
          if (!CanPhysicallyMove(sausage.x1,     sausage.y1 + 1, sausage.z, Down)
           || !CanPhysicallyMove(sausage.x1 + 1, sausage.y1 + 1, sausage.z, Right)) break;
          if  (!MoveThroughSpace(sausage.x1,     sausage.y1 + 1, sausage.z, Down)) return false;
          if  (!MoveThroughSpace(sausage.x1 + 1, sausage.y1 + 1, sausage.z, Right)) return false;
          sausage.x1++;
          sausage.y2++;
        }
      } else if (sausage.x2 == _stephen.x + 1) {
        assert(sausage.x1 == _stephen.x); // ___
        assert(sausage.y1 == _stephen.y); // _12
        assert(sausage.y2 == _stephen.y); // ___
        if (stephenRotationDir == +1) {
          if (!CanPhysicallyMove(sausage.x2,     sausage.y2 + 1, sausage.z, Down)
           || !CanPhysicallyMove(sausage.x2 - 1, sausage.y2 + 1, sausage.z, Left)) break;
          if  (!MoveThroughSpace(sausage.x2,     sausage.y2 + 1, sausage.z, Down)) return false;
          if  (!MoveThroughSpace(sausage.x2 - 1, sausage.y2 + 1, sausage.z, Left)) return false;
          sausage.x2--;
          sausage.y2++;
        } else { assert(stephenRotationDir == -1);
          if (!CanPhysicallyMove(sausage.x2,     sausage.y2 - 1, sausage.z, Up)
           || !CanPhysicallyMove(sausage.x2 - 1, sausage.y2 - 1, sausage.z, Left)) break;
          if  (!MoveThroughSpace(sausage.x2,     sausage.y2 - 1, sausage.z, Up)) return false;
          if  (!MoveThroughSpace(sausage.x2 - 1, sausage.y2 - 1, sausage.z, Left)) return false;
          sausage.x2--;
          sausage.y1--;
        }
      } else if (sausage.y1 == _stephen.y - 1) {
        assert(sausage.x1 == _stephen.x); // _1_
        assert(sausage.x2 == _stephen.x); // _2_
        assert(sausage.y2 == _stephen.y); // ___
        if (stephenRotationDir == +1) {
          if (!CanPhysicallyMove(sausage.x1 + 1, sausage.y1,     sausage.z, Right)
           || !CanPhysicallyMove(sausage.x1 + 1, sausage.y1 + 1, sausage.z, Down)) break;
          if  (!MoveThroughSpace(sausage.x1 + 1, sausage.y1,     sausage.z, Right)) return false;
          if  (!MoveThroughSpace(sausage.x1 + 1, sausage.y1 + 1, sausage.z, Down)) return false;
          sausage.x2++;
          sausage.y1++;
        } else { assert(stephenRotationDir == -1);
          if (!CanPhysicallyMove(sausage.x1 - 1, sausage.y1,     sausage.z, Left)
           || !CanPhysicallyMove(sausage.x1 - 1, sausage.y1 + 1, sausage.z, Down)) break;
          if  (!MoveThroughSpace(sausage.x1 - 1, sausage.y1,     sausage.z, Left)) return false;
          if  (!MoveThroughSpace(sausage.x1 - 1, sausage.y1 + 1, sausage.z, Down)) return false;
          sausage.x1--;
          sausage.y1++;
        }
      } else if (sausage.y2 == _stephen.y + 1) {
        assert(sausage.x1 == _stephen.x); // ___
        assert(sausage.y1 == _stephen.y); // _1_
        assert(sausage.x2 == _stephen.x); // _2_
        if (stephenRotationDir == +1) {
          if (!CanPhysicallyMove(sausage.x2 - 1, sausage.y2,     sausage.z, Left)
           || !CanPhysicallyMove(sausage.x2 - 1, sausage.y2 - 1, sausage.z, Up)) break;
          if  (!MoveThroughSpace(sausage.x2 - 1, sausage.y2,     sausage.z, Left)) return false;
          if  (!MoveThroughSpace(sausage.x2 - 1, sausage.y2 - 1, sausage.z, Up)) return false;
          sausage.x1--;
          sausage.y2--;
        } else { assert(stephenRotationDir == -1);
          if (!CanPhysicallyMove(sausage.x2 + 1, sausage.y2,     sausage.z, Right)
           || !CanPhysicallyMove(sausage.x2 + 1, sausage.y2 - 1, sausage.z, Up)) break;
          if  (!MoveThroughSpace(sausage.x2 + 1, sausage.y2,     sausage.z, Right)) return false;
          if  (!MoveThroughSpace(sausage.x2 + 1, sausage.y2 - 1, sausage.z, Up)) return false;
          sausage.x2++;
          sausage.y2--;
        }
      } else {
        assert(false);
        FAIL("Not sure what the orientation is of sausage at [%d,%d,%d]", _stephen.x, _stephen.y, z);
      }

      // We got here, so the sausage rotated successfully.
      // TODO: Fork rotation. This is very complex; we need to handle forks directly on top of stephen,
      // as well as forks supported by rotating sausages. I think? Or do they just drop...
      // Wait, what happens if a sausage is only supported by a rotating sausage? I don't think I've seen this case. I really hope it just drops.
      // Probably we move this block up to the top and just recompute whatever is on stephen's head. Ugh, really?
      // We could also set a boolean for 'is the fork on stephen's head', I guess.
      s8 sausageNo = GetSausage(_stephen.x, _stephen.y, sausage.z+1);
      if (sausageNo == -1) break;
      sausage = _sausages[sausageNo]; // And we go again.
    }
  }

  return true;
}

bool Level::MoveStephenThroughSpace(Direction dir, bool ladderMotion) {
  if (_stephen.HasFork()) {
    // If there's a speared sausage, check to see if it gets unspeared.
    if (_sausageSpeared != -1) {
      if (!CanPhysicallyMove(_stephen.forkX, _stephen.forkY, _stephen.forkZ, dir)) {
        if (dir == Inverse(_stephen.dir)) _sausageSpeared = -1; // Unspear but continue motion
        else FAIL("Speared sausage cannot physically move %s", DIRS[dir]);
      }
    }

    // If there's a speared sausage, the movement starts from the fork (the spear point) rather than the spot in front of it,
    // since the spear point is the location of a sausage.
    if (_sausageSpeared != -1) {
      if (!MoveThroughSpace(_stephen.forkX, _stephen.forkY, _stephen.forkZ, dir)) return false;
    } else { assert(_sausageSpeared == -1);
      if (dir == Up) {
        if (!MoveThroughSpace(_stephen.forkX, _stephen.forkY - 1, _stephen.forkZ, dir)) return false;
      } else if (dir == Down) {
        if (!MoveThroughSpace(_stephen.forkX, _stephen.forkY + 1, _stephen.forkZ, dir)) return false;
      } else if (dir == Left) {
        if (!MoveThroughSpace(_stephen.forkX - 1, _stephen.forkY, _stephen.forkZ, dir)) return false;
      } else if (dir == Right) {
        if (!MoveThroughSpace(_stephen.forkX + 1, _stephen.forkY, _stephen.forkZ, dir)) return false;
      } else if (dir == Jump) {
        if (!MoveThroughSpace(_stephen.forkX, _stephen.forkY, _stephen.forkZ + 1, dir)) return false;
      } else if (dir == Crouch) {
        if (!MoveThroughSpace(_stephen.forkX, _stephen.forkY, _stephen.forkZ - 1, dir)) return false;
      } else { assert(false); FAIL("Stephen's fork cannot move %s", DIRS[dir]); }
    }
  }

  // Now we check if stephen's body can move into the new space
  if (dir == Up) {
    if (!MoveThroughSpace(_stephen.x, _stephen.y - 1, _stephen.z, dir, 0, true)) return false;
  } else if (dir == Down) {
    if (!MoveThroughSpace(_stephen.x, _stephen.y + 1, _stephen.z, dir, 0, true)) return false;
  } else if (dir == Left) {
    if (!MoveThroughSpace(_stephen.x - 1, _stephen.y, _stephen.z, dir, 0, true)) return false;
  } else if (dir == Right) {
    if (!MoveThroughSpace(_stephen.x + 1, _stephen.y, _stephen.z, dir, 0, true)) return false;
  } else if (dir == Jump) {
    if (!MoveThroughSpace(_stephen.x, _stephen.y, _stephen.z + 1, dir)) return false;
  } else if (dir == Crouch) {
    if (!MoveThroughSpace(_stephen.x, _stephen.y, _stephen.z - 1, dir)) return false;
  } else { assert(false); FAIL("Stephen's body cannot move %s", DIRS[dir]); }

  // All other objects have been moved, now we can move stephen into his new position.
  // Potentially destructive if stephen is unsupported after motion.
  if (dir == Up) {
    _stephen.y--;
    if (_stephen.HasFork()) _stephen.forkY--;
  } else if (dir == Down) {
    _stephen.y++;
    if (_stephen.HasFork()) _stephen.forkY++;
  } else if (dir == Left) {
    _stephen.x--;
    if (_stephen.HasFork()) _stephen.forkX--;
  } else if (dir == Right) {
    _stephen.x++;
    if (_stephen.HasFork()) _stephen.forkX++;
  } else if (dir == Crouch) {
    _stephen.z--;
    if (_stephen.HasFork()) _stephen.forkZ--;
  } else if (dir == Jump) {
    _stephen.z++;
    if (_stephen.HasFork()) _stephen.forkZ++;
  }

  // Ladder motion doesn't need to check supportability sometimes.
  if (!ladderMotion && !CanWalkOnto(_stephen.x, _stephen.y, _stephen.z)) FAIL("Stephen is unsupported if he moves %s", DIRS[dir]);

#if STAY_NEAR_THE_SAUSAGES > 0 // only if the sausages aren't all cooked?
  u16 distance = STAY_NEAR_THE_SAUSAGES * STAY_NEAR_THE_SAUSAGES;
  bool closeToAnySausage = false;
  u16 distanceToSausage;
#define o(i) \
  distanceToSausage = \
    (_sausages[i].x1 - _stephen.x) * (_sausages[i].x1 - _stephen.x) + \
    (_sausages[i].y1 - _stephen.y) * (_sausages[i].y1 - _stephen.y); \
  if (distanceToSausage <= distance) closeToAnySausage = true;

  SAUSAGES;
#undef o

  if (!closeToAnySausage) { FAIL("Stephen would move %d units away from all sausages", distance); }
#endif
  return true;
}
