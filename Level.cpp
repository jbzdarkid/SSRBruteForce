#include "Level.h"
#include <cstdio>
#include <intrin.h>

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
  SetState(&undoHistory[0]); // Restore the original state
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
  if (_stephen.HasFork()) _sausageSpeared = GetSausage(_stephen.forkX, _stephen.forkY, _stephen.forkZ);
}

const char* DIRS[] = {
  "None",
  "Up",
  "Left",
  "Jump",
  "Crouch",
  "Right",
  "Down",
};

#define FAIL(reason, ...) \
  { \
    if (_interactive) { \
      printf("[%d] Move was illegal: " reason "\n", __LINE__, ##__VA_ARGS__); \
    } \
    return false; \
  }

bool Level::WouldStephenStepOnGrill(const Stephen& stephen, Direction dir) {
  if (dir == Up)         return IsGrill(stephen.x, stephen.y - 1, stephen.z);
  else if (dir == Down)  return IsGrill(stephen.x, stephen.y + 1, stephen.z);
  else if (dir == Left)  return IsGrill(stephen.x - 1, stephen.y, stephen.z);
  else if (dir == Right) return IsGrill(stephen.x + 1, stephen.y, stephen.z);
  else {
    assert(false);
    FAIL("Stephen is moving in an invalid direction %d", dir);
  }
}

// Where possible, I prefer to check to see if we should call a function instead of using &handled.
bool Level::Move(Direction dir) {
  stackcheck_begin();
  /*
  s8 standingOnSausage = GetSausage(_stephen.x, _stephen.y, _stephen.z - 1);
  if (standingOnSausage != -1) {
    Sausage sausage = _sausages[standingOnSausage];
    bool handled = false;
    if (!HandleLogRolling(sausage, dir, handled)) return false;
    if (handled) return true;
  }

  bool handled = false;
  if (!HandleLadderMotion(dir, handled)) return false;

  if (!handled) {
    if (!_stephen.HasFork()) { // Need to check this first, because _sausageSpeared is re-used when stephen doesn't have his fork.
      if (!HandleForklessMotion(dir)) return false;
    } else { assert(_stephen.HasFork());
      if (_sausageSpeared != -1) {
        if (!HandleSpearedMotion(dir)) return false;
      } else if (dir == _stephen.dir || dir == Inverse(_stephen.dir)) {
        if (!HandleParallelMotion(dir)) return false;
      } else {
        if (!HandleRotation(dir)) return false;
      }
    }
  }

  if (IsGrill(_stephen.x, _stephen.y, _stephen.z)) {
    if (!HandleBurnedStep(dir)) return false;
  }
  */

  assert(_stephen.HasFork());
  if (_sausageSpeared != -1) {
    if (!HandleSpearedMotion(dir)) return false;
  } else if (dir == _stephen.dir || dir == Inverse(_stephen.dir)) {
    if (!HandleParallelMotion(dir)) return false;
  } else {
    if (!HandleRotation(dir)) return false;
  }

  // Hackity hack.
  Vector<char> sausagesToRemove;
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

  bool allOtherSausagesCooked = true;
  for (s8 i = 0; i < 32; i++) {
    if ((_sausages[i].flags & Sausage::Flags::FullyCooked) != Sausage::Flags::FullyCooked) {
      allOtherSausagesCooked = false;
      break;
    }
  }
  if (allOtherSausagesCooked && _stephen.x == 13 && _stephen.y == 3  && _stephen.dir == Left)   sausagesToRemove = {'g'};
  // if (_stephen.x == 12 && _stephen.y == 3  && _stephen.dir == Up)     sausagesToRemove = {'g'};

  for (char sausageToRemove : sausagesToRemove) {
    u8 sausageNo = (sausageToRemove > 'Z') ? sausageToRemove - 'a' + 26 : sausageToRemove - 'A';
    _sausages[sausageNo].z = -2;
    _sausages[sausageNo].flags = Sausage::Flags::FullyCooked;
  }

  return true;
}

bool Level::HandleLogRolling(const Sausage& sausage, Direction dir, bool& handled) {
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
    while (true) {
      bool stephenSupported = CanWalkOnto(_stephen.x, _stephen.y, _stephen.z);
      if (stephenSupported) break;
      // Log rolls are allowed to make stephen fall off of cliffs, but we've already checked that sausages cannot be lost.
      if (_stephen.z <= 0) FAIL("Stephen would fall below the world");

      if (_stephen.HasFork()) {
        if (!CanWalkOnto(_stephen.forkX, _stephen.forkY, _stephen.forkZ)) {
          _stephen.forkDir = _stephen.dir; // Fork disconnects, and remains where it was.
        }
      }
      _stephen.z--;
    }
  }

  return true;
}

bool Level::HandleSpearedMotion(Direction dir) {
  if (!CanPhysicallyMove(_stephen.forkX, _stephen.forkY, _stephen.forkZ, dir)) {
    if (dir == Inverse(_stephen.dir)) _sausageSpeared = -1;
    else FAIL("Speared sausage cannot physically move %s", DIRS[dir]);
  }

  // Move stephen, which implicitly moves his fork, which implicitly moves any sausage in the same cell as his fork.
  if (!MoveStephenThroughSpace(dir)) return false;
  if (!CanWalkOnto(_stephen.x, _stephen.y, _stephen.z)) FAIL("Stephen is unsupported if he moves %s", DIRS[dir]);

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
      if (!MoveStephenThroughSpace(Jump)) return false;
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

    if (!MoveStephenThroughSpace(dir)) return false; // Move stephen over the ladder
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

bool Level::HandleBurnedStep(Direction dir) {
  stackcheck();
  return Move(Inverse(dir));
}

bool Level::HandleForklessMotion(Direction dir) {
  if (dir == _stephen.dir || dir == Inverse(_stephen.dir)) {
    if (!MoveStephenThroughSpace(dir)) return false;
  } else {
    _stephen.dir = dir;
  }

  if (_stephen.z == _stephen.forkZ && _stephen.dir == _stephen.forkDir) {
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
      _stephen.forkDir = None;
    }
  }
  return true;
}

bool Level::HandleParallelMotion(Direction dir) {
  if (!MoveStephenThroughSpace(dir)) return false;

  if (!CanWalkOnto(_stephen.x, _stephen.y, _stephen.z)) FAIL("Stephen would walk off of a cliff");

  return true;
}

bool Level::HandleRotation(Direction dir) {
  if (dir == Up) {
    s8 clockwise = (_stephen.dir == Left ? +1 : -1);
    if (!MoveThroughSpace( _stephen.forkX, _stephen.y - 1, _stephen.z, dir, clockwise)) return false;
    if (!CanPhysicallyMove(_stephen.x,     _stephen.y - 1, _stephen.z, Inverse(_stephen.dir), true)) return true; // Bonk
    _stephen.forkY = _stephen.y - 1;
    if (!MoveThroughSpace( _stephen.x,     _stephen.y - 1, _stephen.z, Inverse(_stephen.dir), clockwise, true)) return false;
    _stephen.dir = dir;
    _stephen.forkX = _stephen.x;
  } else if (dir == Down) {
    s8 clockwise = (_stephen.dir == Right ? +1 : -1);
    if (!MoveThroughSpace( _stephen.forkX, _stephen.y + 1, _stephen.z, dir, clockwise)) return false;
    if (!CanPhysicallyMove(_stephen.x,     _stephen.y + 1, _stephen.z, Inverse(_stephen.dir), true)) return true; // Bonk
    _stephen.forkY = _stephen.y + 1;
    if (!MoveThroughSpace( _stephen.x,     _stephen.y + 1, _stephen.z, Inverse(_stephen.dir), clockwise, true)) return false;
    _stephen.dir = dir;
    _stephen.forkX = _stephen.x;
  } else if (dir == Left) {
    s8 clockwise = (_stephen.dir == Down ? +1 : -1);
    if (!MoveThroughSpace( _stephen.x - 1, _stephen.forkY, _stephen.z, dir, clockwise)) return false;
    if (!CanPhysicallyMove(_stephen.x - 1, _stephen.y,     _stephen.z, Inverse(_stephen.dir), true)) return true; // Bonk
    _stephen.forkX = _stephen.x - 1;
    if (!MoveThroughSpace( _stephen.x - 1, _stephen.y,     _stephen.z, Inverse(_stephen.dir), clockwise, true)) return false;
    _stephen.dir = dir;
    _stephen.forkY = _stephen.y;
  } else if (dir == Right) {
    s8 clockwise = (_stephen.dir == Up ? +1 : -1);
    if (!MoveThroughSpace( _stephen.x + 1, _stephen.forkY, _stephen.z, dir, clockwise)) return false;
    if (!CanPhysicallyMove(_stephen.x + 1, _stephen.y,     _stephen.z, Inverse(_stephen.dir), true)) return true; // Bonk
    _stephen.forkX = _stephen.x + 1;
    if (!MoveThroughSpace( _stephen.x + 1, _stephen.y,     _stephen.z, Inverse(_stephen.dir), clockwise, true)) return false;
    _stephen.dir = dir;
    _stephen.forkY = _stephen.y;
  } else {
    assert(false);
    FAIL("Attempted to rotate stephen in direction %s", DIRS[dir]);
  }

  return true;
}

struct CPMData {
  Vector<s8> movedSausages;
  Vector<s8> sausagesToDrop;
  s8 sausageToSpear = -1; // This applies to *all* situations where a fork gets stuck in a sausage.
  s8 sausageHat = -1;
  u8 consideredSausages = 0; // We have /considered/ if this sausage can physically move and added it to movedSausages
  u8 sausagesToDoubleMove = 0;
  bool pushedFork = false;
  bool canPhysicallyMove = false;
} data;

bool Level::CanPhysicallyMove(s8 x, s8 y, s8 z, Direction dir, bool stephenIsRotating) {
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

  assert(sausageNo < sizeof(data.consideredSausages) * 8);
  if (data.consideredSausages & (1 << sausageNo)) return true; // Already analyzed
  data.consideredSausages |= (1 << sausageNo);
  if (data.sausageToSpear == -1) data.sausageToSpear = sausageNo; // If spearing is possible, the first sausage we encounter will be our spear target.
  Sausage sausage = _sausages[sausageNo];
  return false; // HACK: sausages cannot move

  if (!CanPhysicallyMoveInternal(sausage.x1 + dx, sausage.y1 + dy, sausage.z + dz, dir)) return false;
  if (!CanPhysicallyMoveInternal(sausage.x2 + dx, sausage.y2 + dy, sausage.z + dz, dir)) return false;
  // Both of the sausage halves can move, so the sausage will move if the move succeeds.
  data.movedSausages.Push(sausageNo);
  return true;
}

bool Level::IsSausageCarried(s8 x, s8 y, s8 z, Direction dir, bool stephenIsRotating, bool canDoubleMove) {
  s8 sausageNo = GetSausage(x, y, z+1);
  if (sausageNo == -1 || data.consideredSausages & (1 << sausageNo)) return false; // Already analyzed
  data.consideredSausages |= (1 << sausageNo);
  Sausage sausage = _sausages[sausageNo];

  // Find the x and y which are not implicitly supported (by whoever our caller is)
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

  if (_stephen.forkX == otherX && _stephen.forkY == otherY && _stephen.forkZ == z) {
    if (!_stephen.HasFork() && !data.pushedFork) return false; // Sausage is resting on disconnected fork which is not moving
    if (_stephen.HasFork() && stephenIsRotating) { // Supported by fork while rotating (fork drop)
      data.sausagesToDrop.Push(sausageNo);
      return false;
    }
    canDoubleMove = false; // Supported by the fork which is also moving, but this prevents a double-move
  }
  if (_stephen.x == otherX && _stephen.y == otherY && _stephen.z == z) {
    // While stephen is rotating, he counts as a support.
    if (stephenIsRotating) return false;

    canDoubleMove = false; // Otherwise, the sausage moves with stephen, but he prevents a double move.
  }

  s8 otherSausageNo = GetSausage(otherX, otherY, z);
  if (otherSausageNo != -1 && !data.movedSausages.Contains(otherSausageNo)) return false; // Supported by another sausage which is not moving.

  if (_stephen.x == x && _stephen.y == y && _stephen.z == z) { // The primary support was stephen all along!
    if (otherSausageNo == -1) data.sausageHat = sausageNo; // And there is nothing else supporting this sausage
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

bool Level::MoveThroughSpace(s8 x, s8 y, s8 z, Direction dir, s8 stephenRotationDir, bool doSausageRotation, bool doDoubleMove) {
  // TODO: Maybe cache & check the last CPM call? When we rotate, we make the same call twice in a row.
  // Actually we just want a function that we can safely call from HandleRotation -- i.e. it succeeds if CPM return false.
  CanPhysicallyMove(x, y, z, dir, stephenRotationDir != 0);
  // Maybe this is just supposed to be x, y, z? I'm... not sure.
  // TODO: Still a little hacky. We don't want the tile *in front* of stephen, we want the one *above* him. But still, ugh.
  IsSausageCarried(_stephen.x, _stephen.y, _stephen.z, dir, stephenRotationDir != 0, false);
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
  return MoveThroughSpace2(x, y, z, dir, stephenRotationDir, doSausageRotation, doDoubleMove);
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
  return MoveThroughSpace2(_stephen.x, _stephen.y, _stephen.z, dir, stephenRotationDir);
}

bool Level::MoveThroughSpace2(s8 x, s8 y, s8 z, Direction dir, s8 stephenRotationDir, bool doSausageRotation, bool doDoubleMove) {
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
      // Else, we fall into the main block.
    } else {
      if (!_interactive) FAIL("");

      // This branch can be hit from a bunch of places. Let's try to give a useful error message.
      if (canSpear) FAIL("Stephen's fork cannot move in direction %s, and there is nothing to spear", DIRS[dir]);
      if (data.sausageToSpear != -1) FAIL("Cannot move sausage %c in direction %s", 'a' + data.sausageToSpear, DIRS[dir]);
      FAIL("Wall at [%d, %d, %d] cannot move %s (probably you pushed a fork into it)", x, y, z, DIRS[dir]);
    }
  }

  // The ordering here is important -- we need to move lower sausages first so that stacked sausages drop properly.
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
      data.sausagesToDrop.Append(sausageNo);

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
        if (!MoveThroughSpace(sausage.x1, sausage.y1, sausage.z, dir, stephenRotationDir, false)) return false; // Avoid infinite-ish recursion
  
        // If any sausages moved as a part of this, they don't need to double-move (since they did just double-move).
        for (s8 sausageNo2 : data.movedSausages) sausagesToDoubleMove &= ~(1 << sausageNo2);
      }
    }
  }

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

  if (!_stephen.HasFork()) { // TODO: What if dropping the fork means a sausage is unsupported? I'm really thinking about making a dummy sausage for the fork.
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

bool Level::MoveStephenThroughSpace(Direction dir) {
  if (_stephen.HasFork()) {
    // If there's a speared sausage, we move it directly, as opposed to the fork.
    // If it succeeds, the fork is clear to move (and we don't want to double-move the sausage).
    if (_sausageSpeared != -1) {
      if (!MoveThroughSpace(_stephen.forkX, _stephen.forkY, _stephen.z, dir)) return false;
    } else { assert(_sausageSpeared == -1);
      // If there's no speared sausage, we need to check the space the fork is moving into.
      if (dir == Up) {
        if (!MoveThroughSpace(_stephen.forkX, _stephen.forkY - 1, _stephen.forkZ, dir)) return false;
      } else if (dir == Down) {
        if (!MoveThroughSpace(_stephen.forkX, _stephen.forkY + 1, _stephen.forkZ, dir)) return false;
      } else if (dir == Left) {
        if (!MoveThroughSpace(_stephen.forkX - 1, _stephen.forkY, _stephen.forkZ, dir)) return false;
      } else if (dir == Right) {
        if (!MoveThroughSpace(_stephen.forkX + 1, _stephen.forkY, _stephen.forkZ, dir)) return false;
      } else { assert(false); FAIL("think about this later, but probably just z +- 1"); }
    }
  }
  if (dir == Up) {
    if (!MoveThroughSpace(_stephen.x, _stephen.y - 1, _stephen.z, dir)) return false;
  } else if (dir == Down) {
    if (!MoveThroughSpace(_stephen.x, _stephen.y + 1, _stephen.z, dir)) return false;
  } else if (dir == Left) {
    if (!MoveThroughSpace(_stephen.x - 1, _stephen.y, _stephen.z, dir)) return false;
  } else if (dir == Right) {
    if (!MoveThroughSpace(_stephen.x + 1, _stephen.y, _stephen.z, dir)) return false;
  } else { assert(false); FAIL("think about this later, but probably just z +- 1"); }

  if (dir == Up || dir == Down || dir == Left || dir == Right) {
    if (!MoveThroughSpace3(dir)) return false;
  }

  // All other objects have been moved, now we can move stephen into his new position.
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

#if STAY_NEAR_THE_SAUSAGES > 0 // only if the sausages aren't all cooked?
  u16 distanceToSausage0 =
    (_sausages[0].x1 - _stephen.x) * (_sausages[0].x1 - _stephen.x) +
    (_sausages[0].y1 - _stephen.y) * (_sausages[0].y1 - _stephen.y);
  u16 distanceToSausage1 =
    (_sausages[1].x1 - _stephen.x) * (_sausages[1].x1 - _stephen.x) +
    (_sausages[1].y1 - _stephen.y) * (_sausages[1].y1 - _stephen.y);
  u16 distanceToSausage2 =
    (_sausages[0].x2 - _stephen.x) * (_sausages[0].x2 - _stephen.x) +
    (_sausages[0].y2 - _stephen.y) * (_sausages[0].y2 - _stephen.y);
  u16 distanceToSausage3 =
    (_sausages[1].x2 - _stephen.x) * (_sausages[1].x2 - _stephen.x) +
    (_sausages[1].y2 - _stephen.y) * (_sausages[1].y2 - _stephen.y);
  u16 distance = STAY_NEAR_THE_SAUSAGES * STAY_NEAR_THE_SAUSAGES;
  if (distanceToSausage0 > distance && distanceToSausage1 > distance
    && distanceToSausage2 > distance && distanceToSausage3 > distance) {
    FAIL("Stephen would move %d units away from the sausages", distance);
  }
#endif
  return true;
}

bool State::operator==(const State& other) const {
  if (stephen != other.stephen) return false;
#define o(x) if (sausages[x] != other.sausages[x]) return false;
    SAUSAGES;
#undef o
  return true;
}

// From MSVC's type_traits
#if defined _WIN64
constexpr size_t _FNV_offset_basis = 14695981039346656037ULL;
constexpr size_t _FNV_prime        = 1099511628211ULL;
constexpr size_t GoldenRatio       = 0x9e3779b97f4a7c15;
#else
constexpr size_t _FNV_offset_basis = 2166136261U;
constexpr size_t _FNV_prime        = 16777619U;
constexpr size_t GoldenRatio       = 0x9e3779b9;
#endif

size_t msvc_hash_internal(u8* bytes, size_t length) {
  size_t h = _FNV_offset_basis;
  for (size_t i=0; i<length; i++) {
    h ^= bytes[i];
    h *= _FNV_prime;
  }
  return h;
}

size_t msvc_hash(u32 value) {
  return msvc_hash_internal((u8*)&value, sizeof(value));
}

size_t msvc_hash(u64 value) {
  return msvc_hash_internal((u8*)&value, sizeof(value));
}

void combine_hash(size_t& a, u64 b) {
  a ^= GoldenRatio + (a << 6) + (a >> 2) + msvc_hash(b);
}

/*
u32 triple32_hash(u32 x) {
  x ^= x >> 16;
  x *= 0x45d9f3bU;
  x ^= x >> 11;
  x *= 0xac4c1b51U;
  x ^= x >> 15;
  x *= 0x31848babU;
  x ^= x >> 14;
  return x;
}

void combine_hash(u32& a, u32 b) {
  a ^= 0x9e3779b9 + (a << 6) + (a >> 2) + triple32_hash(b);
}

size_t triple32_hash(u64 x) {
  u32 a = (u32)x;
  combine_hash(a, (u32)(x >> 32));
  return a;
}

void combine_hash(u32& a, u64 b) {
  combine_hash(a, (u32)b);
  combine_hash(a, (u32)(b >> 32));
}
*/

size_t State::Hash() const {
  static_assert(sizeof(Stephen) == 8);
  static_assert(sizeof(Sausage) == 8);
//   u32 hash = triple32_hash(*(u64*)&stephen);
// #define o(x) combine_hash(hash, *(u64*)&sausages[x]);
//   SAUSAGES
// #undef o

  size_t hash = msvc_hash(*(u64*)&stephen);
#define o(x) combine_hash(hash, *(u64*)&sausages[x]);
  SAUSAGES
#undef o

  return hash;
}

Direction Inverse(Direction dir) {
  assert(dir > 0 && dir < 7)
  return (Direction)(7 - dir);
}
