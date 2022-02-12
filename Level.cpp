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

Level::Level(u8 width, u8 height, const char* name, const char* asciiGrid,
  const Stephen& stephen,
  std::initializer_list<Ladder> ladders,
  std::initializer_list<Sausage> sausages,
  std::initializer_list<Tile> tiles)
{
  this->name = name;
  _width = width;
  _height = height;
  _grid = NArray<Tile>(_width, _height);
  _grid.Fill(Tile::Empty);
  Vector<Tile> extraTiles(tiles);

  assert(width * height == strlen(asciiGrid));
  for (s32 i=0; i<width * height; i++) {
    s8 x = i % width;
    s8 y = i / width;
    char c = asciiGrid[i];
    if (c == '?') _grid(x, y) = extraTiles.PopValue();
    else if (c == ' ') _grid(x, y) = Empty;
    else if (c == '#') _grid(x, y) = (Tile)(Ground | Grill);
    else if (c == '$') _grid(x, y) = (Tile)(Wall1 | Grill);
    else if (c == '%') _grid(x, y) = (Tile)(Wall2 | Grill);
    else if (c == '_') _grid(x, y) = Ground;
    else if (c == '1') _grid(x, y) = Wall1;
    else if (c == '2') _grid(x, y) = Wall2;
    else if (c == '3') _grid(x, y) = Wall3;
    else if (c == '4') _grid(x, y) = Wall4;
    else if (c == '5') _grid(x, y) = Wall5;
    else if (c == 'U') { _grid(x, y) = Ground; _ladders.Push(Ladder{x, y, 0, Up}); }
    else if (c == 'D') { _grid(x, y) = Ground; _ladders.Push(Ladder{x, y, 0, Down}); }
    else if (c == 'L') { _grid(x, y) = Ground; _ladders.Push(Ladder{x, y, 0, Left}); }
    else if (c == 'R') { _grid(x, y) = Ground; _ladders.Push(Ladder{x, y, 0, Right}); }
    else if (c == '^') { _grid(x, y) = Ground; _stephen = Stephen(x, y, 0, Up); }
    else if (c == 'v') { _grid(x, y) = Ground; _stephen = Stephen(x, y, 0, Down); }
    else if (c == '<') { _grid(x, y) = Ground; _stephen = Stephen(x, y, 0, Left); }
    else if (c == '>') { _grid(x, y) = Ground; _stephen = Stephen(x, y, 0, Right); }
    else if (c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z') {
      int num;
      if (c >= 'A' && c <= 'Z') {
        _grid(x, y) = Empty;
        num = c - 'A';
      } else {
        _grid(x, y) = Ground;
        num = c - 'a';
      }

      // We need to do this here because, even if we don't *solve* levels
      // with less than the expected count, we still construct them.
      while (_sausages.Size() < num + 1) _sausages.Push({-127, -127, -127, -127, Sausage::Flags::None});
      if (_sausages[num].x1 == x-1 && _sausages[num].y1 == y) {
        _sausages[num].x2 = x;
        _sausages[num].y2 = y;
      } else if (_sausages[num].x1 == x && _sausages[num].y1 == y-1) {
        _sausages[num].x2 = x;
        _sausages[num].y2 = y;
      } else {
        _sausages[num].x1 = x;
        _sausages[num].y1 = y;
      }
    } else {
      printf("Couldn't parse character '%c' for puzzle '%s', giving up\n", c, name);
      return;
    }
  }
  if (stephen.x > -1) {
    _stephen = stephen;
    _start = stephen;
  } else if (_stephen.x > -1) {
    _start = _stephen;
  } else {
    printf("No stephen for puzzle '%s', giving up\n", name);
    return;
  }

  for (Sausage sausage : sausages) _sausages.Push(sausage);
  for (Sausage& sausage : _sausages) {
    if (sausage.y1 == sausage.y2) sausage.flags |= Sausage::Flags::Horizontal;
  }

  // Ladders from the grid, as 2D, may need height extensions.
  Vector<Ladder> extraLadders;
  for (const Ladder& ladder : _ladders) {
    Tile adjacentWall = Ground;
    s8 z = ladder.z + 1;
    while (true) {
      if (ladder.dir == Up) {
        if (!IsWall(ladder.x, ladder.y - 1, z)) break;
      } else if (ladder.dir == Down) {
        if (!IsWall(ladder.x, ladder.y + 1, z)) break;
      } else if (ladder.dir == Left) {
        if (!IsWall(ladder.x - 1, ladder.y, z)) break;
      } else if (ladder.dir == Right) {
        if (!IsWall(ladder.x + 1, ladder.y, z)) break;
      }
      extraLadders.Push(Ladder{ladder.x, ladder.y, z, ladder.dir});
      z++;
    }
  }
  _ladders.Append(extraLadders);

  // Ladders from the initializer list do not get the same treatment.
  for (const Ladder& ladder : ladders) _ladders.Push(ladder);

  assert(extraTiles.Size() == 0); // Assert that all excess tiles were consumed
}

void Level::Print() const {
  putchar('+');
  for (u8 x=0; x<_width; x++) putchar('-');
  putchar('+');
  putchar('\n');

  for (u8 y=0; y<_height; y++) {
    putchar('|');
    for (u8 x=0; x<_width; x++) {

      char dynamic = ' ';
      for (s8 z = 0; z < 8; z++) {
        if (x == _stephen.x && y == _stephen.y && z == _stephen.z) {
          dynamic = " ^<  >v"[_stephen.dir];
        } else if (!_stephen.HasFork() && x == _stephen.forkX && y == _stephen.forkY && z == _stephen.forkZ) {
          dynamic = '+';
        } else if (GetSausage(x, y, z) != -1) {
          s8 sausageNo = GetSausage(x, y, z);
          if (sausageNo != -1 && _grid(x, y) == Empty) dynamic = 'A' + sausageNo;
          if (sausageNo != -1 && _grid(x, y) != Empty) dynamic = 'a' + sausageNo;
        } else if (dynamic == ' ') { // Ladders are lower priority over basically everything else.
          for (const Ladder& ladder : _ladders) {
            if (ladder.x == x && ladder.y == y && ladder.z == z) {
              dynamic = " UL  RD"[ladder.dir];
              break;
            }
          }
        }
      }
      if (dynamic !=  ' ')              putchar(dynamic);
      else if (_grid(x, y) == Empty)  putchar(' ');
      else if (_grid(x, y) &  Grill)  putchar('#');
      else if (_grid(x, y) == Ground) putchar('_');
      else if (_grid(x, y) == Wall1)  putchar('1');
      else if (_grid(x, y) == Wall2)  putchar('2');
      else if (_grid(x, y) == Wall3)  putchar('3');
      else if (_grid(x, y) == Wall4)  putchar('4');
      else if (_grid(x, y) == Wall5)  putchar('5');
      else                            putchar('?');
    }
    putchar('|');
    putchar('\n');
  }

  putchar('+');
  for (u8 x=0; x<_width; x++) putchar('-');
  putchar('+');
  putchar('\n');
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

bool Level::Won() const {
  if (_stephen != _start) return false;
  for (const Sausage& sausage : _sausages) {
    if ((sausage.flags & Sausage::Flags::FullyCooked) != Sausage::Flags::FullyCooked) return false;
  }
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
    if (!MoveThroughSpace( _stephen.forkX, _stephen.y - 1, _stephen.z, dir,                   true)) return false;
    if (!CanPhysicallyMove(_stephen.x,     _stephen.y - 1, _stephen.z, Inverse(_stephen.dir), true)) return true; // Bonk
    _stephen.forkY = _stephen.y - 1;
    if (!MoveThroughSpace( _stephen.x,     _stephen.y - 1, _stephen.z, Inverse(_stephen.dir), true)) return false;
    _stephen.dir = dir;
    _stephen.forkX = _stephen.x;
  } else if (dir == Down) {
    if (!MoveThroughSpace( _stephen.forkX, _stephen.y + 1, _stephen.z, dir,                   true)) return false;
    if (!CanPhysicallyMove(_stephen.x,     _stephen.y + 1, _stephen.z, Inverse(_stephen.dir), true)) return true; // Bonk
    _stephen.forkY = _stephen.y + 1;
    if (!MoveThroughSpace( _stephen.x,     _stephen.y + 1, _stephen.z, Inverse(_stephen.dir), true)) return false;
    _stephen.dir = dir;
    _stephen.forkX = _stephen.x;
  } else if (dir == Left) {
    if (!MoveThroughSpace( _stephen.x - 1, _stephen.forkY, _stephen.z, dir,                   true)) return false;
    if (!CanPhysicallyMove(_stephen.x - 1, _stephen.y,     _stephen.z, Inverse(_stephen.dir), true)) return true; // Bonk
    _stephen.forkX = _stephen.x - 1;
    if (!MoveThroughSpace( _stephen.x - 1, _stephen.y,     _stephen.z, Inverse(_stephen.dir), true)) return false;
    _stephen.dir = dir;
    _stephen.forkY = _stephen.y;
  } else if (dir == Right) {
    if (!MoveThroughSpace( _stephen.x + 1, _stephen.forkY, _stephen.z, dir,                   true)) return false;
    if (!CanPhysicallyMove(_stephen.x + 1, _stephen.y,     _stephen.z, Inverse(_stephen.dir), true)) return true; // Bonk
    _stephen.forkX = _stephen.x + 1;
    if (!MoveThroughSpace( _stephen.x + 1, _stephen.y,     _stephen.z, Inverse(_stephen.dir), true)) return false;
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
  u8 consideredSausages = 0;
  u8 sausagesToDoubleMove = 0;
  bool pushedFork = false;
} data;

bool Level::CanPhysicallyMove(s8 x, s8 y, s8 z, Direction dir, bool stephenIsRotating) {
  data.movedSausages.Resize(0);
  data.sausagesToDrop.Resize(0);
  data.sausageToSpear = -1;
  data.consideredSausages = 0;
  data.sausagesToDoubleMove = 0;
  data.pushedFork = false;

  if (!CanPhysicallyMoveInternal(x, y, z, dir)) return false;
  CheckForSausageCarry(dir, z, stephenIsRotating);
  return true;
}

bool Level::CanPhysicallyMoveInternal(s8 x, s8 y, s8 z, Direction dir /*, CPMData& data */) {
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

  if (!CanPhysicallyMoveInternal(sausage.x1 + dx, sausage.y1 + dy, sausage.z + dz, dir)) return false;
  if (!CanPhysicallyMoveInternal(sausage.x2 + dx, sausage.y2 + dy, sausage.z + dz, dir)) return false;
  // Both of the sausage halves can move, so the sausage will move if the move succeeds.
  data.movedSausages.Push(sausageNo);
  return true;
}

void Level::CheckForSausageCarry(Direction dir, s8 z, bool stephenIsRotating) {
  if (dir == Crouch || dir == Jump) return; // Sausages can only be carried laterally (UDLR)

  bool anySausagesMoved;
  do {
    z++;
    anySausagesMoved = false;

    // It's probably more efficient to iterate by asking about sausages that are above sausages which moved.
    // But it's also a whole lot more work.
    for (s8 sausageNo = 0; sausageNo < _sausages.Size(); sausageNo++) {
      Sausage sausage = _sausages[sausageNo];
      if (sausage.z != z) continue; // Pedantic layer-by-layer nonsense

      if (IsWall(sausage.x1, sausage.y1, sausage.z-1)
       || IsWall(sausage.x2, sausage.y2, sausage.z-1)) continue; // Sausage is resting on a wall

      bool sausageCanDoubleMove = true;

      if (sausage.IsAt(_stephen.forkX, _stephen.forkY, _stephen.forkZ+1)) {
        if (!_stephen.HasFork() && !data.pushedFork) continue; // Sausage is resting on disconnected fork which is not moving
        if (_stephen.HasFork() && stephenIsRotating) continue; // Supported by fork while rotating (fork drop)
        sausageCanDoubleMove = false;
      } else if (sausage.IsAt(_stephen.x, _stephen.y, _stephen.z+1)) {
        if (stephenIsRotating) continue; // Supported by stephen while rotating (sausage hat) [TODO]. Note: I don't think sausage hats bonk.
        sausageCanDoubleMove = false;
      }

      // TODO: What if a sausage is supported by sausages which are dropping?
      // What if it's a bit of both? -> Pretty sure it doesn't move in this case, because the 'dropping' sausage is static.

      s8 restingOn1 = GetSausage(sausage.x1, sausage.y1, sausage.z-1);
      if (restingOn1 != -1 && !data.movedSausages.Contains(restingOn1)) continue; // Supported by another sausage which is not moving.
      s8 restingOn2 = GetSausage(sausage.x2, sausage.y2, sausage.z-1);
      if (restingOn2 != -1 && !data.movedSausages.Contains(restingOn2)) continue; // Supported by another sausage which is not moving.

      // NB: We are calling the internal method here to avoid resetting the data structure.
      if (!CanPhysicallyMoveInternal(sausage.x1, sausage.y1, sausage.z, dir)) {
        // We know that this sausage *should've* been carried -- all of its supports were moving.
        // If it couldn't move, that means (probably) all of its supports are gone, so we potentially need to drop it.
        data.sausagesToDrop.Push(sausageNo);
        continue;
      }
      // Otherwise, it was already added to data.sausagesToMove by CanPhysicallyMoveInternal.
      anySausagesMoved = true;
      if (!sausageCanDoubleMove) continue; // Preconditions for double-move, i.e. not being supported by stephen or his fork.

      // Sausages can only double-move if they are in the same direction as the motion,
      // and if the sausage(s) that support them are perpendicular to the motion.
      if (dir == Up || dir == Down) {
        if (sausage.IsVertical()
          && (restingOn1 == -1 || _sausages[restingOn1].IsHorizontal())
          && (restingOn2 == -1 || _sausages[restingOn2].IsHorizontal())) {
          data.sausagesToDoubleMove |= (1 << sausageNo);
        }
      } else if (dir == Left || dir == Right) {
        if (sausage.IsHorizontal()
          && (restingOn1 == -1 || _sausages[restingOn1].IsVertical())
          && (restingOn2 == -1 || _sausages[restingOn2].IsVertical())) {
          data.sausagesToDoubleMove |= (1 << sausageNo);
        }
      }
    } // End for loop
  } while (anySausagesMoved);
}

bool Level::MoveThroughSpace(s8 x, s8 y, s8 z, Direction dir, bool stephenIsRotating, bool doDoubleMove) {
  // TODO: Maybe cache & check the last CPM call? Most of the time we run the same exact call twice.
  bool canPhysicallyMove = CanPhysicallyMove(x, y, z, dir, stephenIsRotating);

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
      if (!_interactive) return false;

      // This branch can be hit from a bunch of places. Let's try to give a useful error message.
      if (canSpear) FAIL("Stephen's fork cannot move in direction %s, and there is nothing to spear", DIRS[dir]);
      if (data.sausageToSpear != -1) FAIL("Cannot move sausage %c in direction %s", 'a' + data.sausageToSpear, DIRS[dir]);
      FAIL("Wall at [%d, %d, %d] cannot move %s (probably you pushed a fork into it)", x, y, z, DIRS[dir]);
    }
  }

  // The ordering here is important -- we need to move lower sausages first so that they drop in the same order.
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
    } else {
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

      while (true) {
        bool supported = CanWalkOnto(sausage.x1, sausage.y1, sausage.z)
                      || CanWalkOnto(sausage.x2, sausage.y2, sausage.z);
        if (supported) break;
        if (sausage.z <= 0) FAIL("Sausage %c would fall below the world", 'a' + sausageNo);
        sausage.z--;
        if (!_stephen.HasFork() && sausageNo == _sausageSpeared) _stephen.forkZ--;
      }
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

    // If nothing has gone wrong, save the updated sausage
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

    while (true) {
      bool supported = CanWalkOnto(_stephen.forkX, _stephen.forkY, _stephen.forkZ);
      if (supported) break;
      if (_stephen.forkZ <= 0) FAIL("Disconnected fork would fall below the world");
      _stephen.forkZ--;
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
    _sausages[sausageNo] = sausage;
  }

  // And now we handle double-moves by just moving every marked sausage again.
  if (doDoubleMove && data.sausagesToDoubleMove != 0) {
    // Make a copy since data will be overwritten after we call ourselves again.
    u8 sausagesToDoubleMove = data.sausagesToDoubleMove;
    for (s8 i=0; i<_sausages.Size(); i++) {
      if (sausagesToDoubleMove & (1 << i)) {
        Sausage sausage = _sausages[i];
        MoveThroughSpace(sausage.x1, sausage.y1, sausage.z, dir, false, false); // Avoid infinite-ish recursion
        for (s8 sausageNo : data.movedSausages) sausagesToDoubleMove &= ~(1 << sausageNo);
      }
    }
  }

  return true;
}

bool Level::MoveStephenThroughSpace(Direction dir) {
  // If there's a speared sausage, we need to move it first, and it will check the space it's moving into.
  // If it succeeds, the fork is clear to move (and we don't want to double-move the sausage).
  if (_stephen.HasFork() && _sausageSpeared != -1) {
    if (!MoveThroughSpace(_stephen.forkX, _stephen.forkY, _stephen.z, dir)) return false;
  }

  // This function is allowed side effects, so we can move stephen's body before calling MoveThroughSpace
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

  // If there's no speared sausage, we need to check the space the fork is moving into.
  if (_sausageSpeared == -1 && _stephen.HasFork()) {
    if (!MoveThroughSpace(_stephen.forkX, _stephen.forkY, _stephen.forkZ, dir)) return false;
  }
  if (!MoveThroughSpace(_stephen.x, _stephen.y, _stephen.z, dir)) return false;

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

s8 Level::GetSausage(s8 x, s8 y, s8 z) const {
  if (z < 0) return -1;

#define o(i) if (_sausages[i].IsAt(x, y, z)) return i;
  SAUSAGES;
#undef o

  return -1;
}

bool Level::IsWithinGrid(s8 x, s8 y, s8 z) const {
  return x >= 0 && x <= _width - 1 && y >= 0 && y <= _height - 1 && z >= 0;
}

bool Level::IsWall(s8 x, s8 y, s8 z) const {
  if (!IsWithinGrid(x, y, z)) return false;
  u8 cell = _grid(x, y);
  // At z=0, we are blocked if the cell has the second bit set, i.e. (cell & 0b10).
  return cell & (Ground << 1 << z);
}

bool Level::CanWalkOnto(s8 x, s8 y, s8 z) const {
  if (!IsWithinGrid(x, y, z)) return false;
  u8 cell = _grid(x, y);
  // If you are at z=0, you can walk onto anything at ground level. If you are at z=1, you can walk onto Wall1
  if ((cell & (Ground << z)) != 0) return true; // Ground at our current level
  if (GetSausage(x, y, z-1) != -1) return true; // Sausage at our current level
  return false;
}

bool Level::IsGrill(s8 x, s8 y, s8 z) const {
  if (!IsWithinGrid(x, y, z)) return false;
  u8 cell = _grid(x, y);
  if ((cell & Grill) == 0) return false;
  return (cell & (Ground << z)) != 0; // There is ground at this level
}

bool Level::IsLadder(s8 x, s8 y, s8 z, Direction dir) const {
  for (const Ladder& ladder : _ladders) {
    if (ladder.x == x && ladder.y == y && ladder.z == z && ladder.dir == dir) return true;
  }
  return false;
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
