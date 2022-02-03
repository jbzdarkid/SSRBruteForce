#include "Level.h"
#include <cstdio>
#include <intrin.h>

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

#define o(x) +1
  _cpmOut.movedSausages = Vector<bool>(SAUSAGES);
  _cpmOut.movedSausages.Resize(SAUSAGES);
#undef o

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

  if (sausages.size() > 0) {
    _sausages.Resize(0); // Extra sausages go first, because I said so.
    for (Sausage sausage : sausages) _sausages.Push(sausage);
  }
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
      s8 sausageNo = -1;
      for (int i=0; i<_sausages.Size(); i++) {
        const Sausage& sausage = _sausages[i];
        if ((sausage.x1 == x && sausage.y1 == y) 
         || (sausage.x2 == x && sausage.y2 == y)) {
          if (sausageNo == -1) sausageNo = i;
          else if (_sausages[sausageNo].z < sausage.z) sausageNo = i;
        }
      }
      char ladderCh = '\0';
      const char* dirChars = " UL  RD";
      for (const Ladder& ladder : _ladders) {
        if (ladder.x == x && ladder.y == y) ladderCh = dirChars[ladder.dir];
      }

      if (x == _stephen.x && y == _stephen.y) {
        if (_stephen.dir == Up)          putchar('^');
        else if (_stephen.dir == Down)   putchar('v');
        else if (_stephen.dir == Left)   putchar('<');
        else if (_stephen.dir == Right)  putchar('>');
        else assert(false);
      }
      else if (!_stephen.HasFork() && x == _stephen.forkX && y == _stephen.forkY) {
        putchar('+');
      }
      //else if (stephen.dir == Up && x == stephen.x && y == stephen.y - 1) row[x+1] = '|';
      //else if (stephen.dir == Down && x == stephen.x && y == stephen.y + 1) row[x+1] = '|';
      //else if (stephen.dir == Left && x == stephen.x - 1 && y == stephen.y) row[x+1] = '-';
      //else if (stephen.dir == Right && x == stephen.x + 1 && y == stephen.y) row[x+1] = '-';
      else if (sausageNo != -1) {
        if (_grid(x, y) == Empty)     putchar('A' + sausageNo);
        else                          putchar('a' + sausageNo);
      }
      else if (ladderCh != '\0')      putchar(ladderCh);
      else if (_grid(x, y) == Empty)  putchar(' ');
      else if (_grid(x, y) &  Grill)  putchar('#');
      else if (_grid(x, y) == Ground) putchar('_');
      else if (_grid(x, y) == Wall1)  putchar('1');
      else if (_grid(x, y) == Wall2)  putchar('2');
      else if (_grid(x, y) == Wall3)  putchar('3');
      else if (_grid(x, y) == Wall4)  putchar('4');
      else if (_grid(x, y) == Wall5)  putchar('5');
      else assert(false);
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
  if (_stephen.HasFork()) _sausageSpeared = GetSausage(_stephen.forkX, _stephen.forkY, _stephen.z);
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
#if _DEBUG
  u8 local;
  _stackStart = (u64)&local;
#endif
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
    if (_sausageSpeared != -1) {
      if (!HandleSpearedMotion(dir)) return false;
    } else if (!_stephen.HasFork()) {
      if (!HandleForklessMotion(dir)) return false;
    } else {
      if (!HandleDefaultMotion(dir)) return false;
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
        bool forkSupported = CanWalkOnto(_stephen.forkX, _stephen.forkY, _stephen.z);
        if (forkSupported) { // Stephen becomes forkless!
          _stephen.forkZ = _stephen.z;
          _stephen.forkDir = _stephen.dir;
          FAIL("Stephen cannot (yet) become forkless");
        }
      }
      _stephen.z--;
    }
  }

  return true;
}

bool Level::HandleSpearedMotion(Direction dir) {
  if (!CanPhysicallyMove(_stephen.forkX, _stephen.forkY, _stephen.z, dir)) {
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
      _stephen.forkZ = -1;
      _stephen.forkDir = None;
    }
  }
  return true;
}

bool Level::HandleDefaultMotion(Direction dir) {
  if (dir == Up) {
    if (_stephen.dir == Left) {
      if (!MoveThroughSpace( _stephen.x - 1, _stephen.y - 1, _stephen.z, Up)) return false;
      if (!CanPhysicallyMove(_stephen.x,     _stephen.y - 1, _stephen.z, Right)) return true; // Bonk
      if (!MoveThroughSpace( _stephen.x,     _stephen.y - 1, _stephen.z, Right)) return false;
      _stephen.dir = dir;
      _stephen.forkY--;
      _stephen.forkX++;
    } else if (_stephen.dir == Right) {
      if (!MoveThroughSpace( _stephen.x + 1, _stephen.y - 1, _stephen.z, Up)) return false;
      if (!CanPhysicallyMove(_stephen.x,     _stephen.y - 1, _stephen.z, Left)) return true; // Bonk
      if (!MoveThroughSpace( _stephen.x,     _stephen.y - 1, _stephen.z, Left)) return false;
      _stephen.dir = dir;
      _stephen.forkY--;
      _stephen.forkX--;
    } else {
      if (!MoveStephenThroughSpace(dir)) return false;
      if (!CanWalkOnto(_stephen.x, _stephen.y, _stephen.z)) FAIL("Stephen would walk off of a cliff");
    }
  } else if (dir == Down) {
    if (_stephen.dir == Left) {
      if (!MoveThroughSpace( _stephen.x - 1, _stephen.y + 1, _stephen.z, Down)) return false;
      if (!CanPhysicallyMove(_stephen.x,     _stephen.y + 1, _stephen.z, Right)) return true; // Bonk
      if (!MoveThroughSpace( _stephen.x,     _stephen.y + 1, _stephen.z, Right)) return false;
      _stephen.dir = dir;
      _stephen.forkY++;
      _stephen.forkX++;
    } else if (_stephen.dir == Right) {
      if (!MoveThroughSpace( _stephen.x + 1, _stephen.y + 1, _stephen.z, Down)) return false;
      if (!CanPhysicallyMove(_stephen.x,     _stephen.y + 1, _stephen.z, Left)) return true; // Bonk
      if (!MoveThroughSpace( _stephen.x,     _stephen.y + 1, _stephen.z, Left)) return false;
      _stephen.dir = dir;
      _stephen.forkY++;
      _stephen.forkX--;
    } else {
      if (!MoveStephenThroughSpace(dir)) return false;
      if (!CanWalkOnto(_stephen.x, _stephen.y, _stephen.z)) FAIL("Stephen would walk off of a cliff");
    }
  } else if (dir == Left) {
    if (_stephen.dir == Up) {
      if (!MoveThroughSpace( _stephen.x - 1, _stephen.y - 1, _stephen.z, Left)) return false;
      if (!CanPhysicallyMove(_stephen.x - 1, _stephen.y,     _stephen.z, Down)) return true; // Bonk
      if (!MoveThroughSpace( _stephen.x - 1, _stephen.y,     _stephen.z, Down)) return false;
      _stephen.dir = dir;
      _stephen.forkY++;
      _stephen.forkX--;
    } else if (_stephen.dir == Down) {
      if (!MoveThroughSpace( _stephen.x - 1, _stephen.y + 1, _stephen.z, Left)) return false;
      if (!CanPhysicallyMove(_stephen.x - 1, _stephen.y,     _stephen.z, Up)) return true; // Bonk
      if (!MoveThroughSpace( _stephen.x - 1, _stephen.y,     _stephen.z, Up)) return false;
      _stephen.dir = dir;
      _stephen.forkY--;
      _stephen.forkX--;
    } else {
      if (!MoveStephenThroughSpace(dir)) return false;
      if (!CanWalkOnto(_stephen.x, _stephen.y, _stephen.z)) FAIL("Stephen would walk off of a cliff");
    }
  } else if (dir == Right) {
    if (_stephen.dir == Up) {
      if (!MoveThroughSpace( _stephen.x + 1, _stephen.y - 1, _stephen.z, Right)) return false;
      if (!CanPhysicallyMove(_stephen.x + 1, _stephen.y,     _stephen.z, Down)) return true; // Bonk
      if (!MoveThroughSpace( _stephen.x + 1, _stephen.y,     _stephen.z, Down)) return false;
      _stephen.dir = dir;
      _stephen.forkY++;
      _stephen.forkX++;
    } else if (_stephen.dir == Down) {
      if (!MoveThroughSpace( _stephen.x + 1, _stephen.y + 1, _stephen.z, Right)) return false;
      if (!CanPhysicallyMove(_stephen.x + 1, _stephen.y,     _stephen.z, Up)) return true; // Bonk
      if (!MoveThroughSpace( _stephen.x + 1, _stephen.y,     _stephen.z, Up)) return false;
      _stephen.dir = dir;
      _stephen.forkY--;
      _stephen.forkX++;
    } else {
      if (!MoveStephenThroughSpace(dir)) return false;
      if (!CanWalkOnto(_stephen.x, _stephen.y, _stephen.z)) FAIL("Stephen would walk off of a cliff");
    }
  }
  return true;
}

bool Level::CanPhysicallyMove(s8 x, s8 y, s8 z, Direction dir) {
  _cpmOut.movedSausages.Fill(false);
  _cpmOut.sausageToSpear = -1;
  _cpmOut.movedFork = false;
  return CanPhysicallyMoveInternal(x, y, z, dir);
}

bool Level::CanPhysicallyMoveInternal(s8 x, s8 y, s8 z, Direction dir) {
#if _DEBUG
  u8 local;
  u64 currentStack = (u64)&local;
  if (_stackStart - currentStack > 0x10'000) {
    printf("Stack overflow detected! Execution paused.\n");
    assert(false);
    getchar();
  }
#endif
  if (IsWall(x, y, z)) return false; // No, walls cannot move.

  s8 sausageNo = GetSausage(x, y, z);

  if (sausageNo == -1) { // There is not a sausage in this square
    if (!_stephen.HasFork() && x == _stephen.forkX && y == _stephen.forkY && z == _stephen.forkZ) {
      // There is a fork in this square
      _cpmOut.movedFork = true;

      // CanPhysicallyMoveInternal is pushing stephen's fork.
      // TODO: We need to decide (ala spearing) if this push is valid, or if it's valid but only if we spear the separated fork.
      if (dir == Up) {
        return CanPhysicallyMoveInternal(x, y - 1, z, dir);
      } else if (dir == Down) {
        return CanPhysicallyMoveInternal(x, y + 1, z, dir);
      } else if (dir == Left) {
        return CanPhysicallyMoveInternal(x - 1, y, z, dir);
      } else if (dir == Right) {
        return CanPhysicallyMoveInternal(x + 1, y, z, dir);
      } else if (dir == Jump) {
        return CanPhysicallyMoveInternal(x, y, z + 1, dir);
      } else if (dir == Crouch) {
        return false; // It's never possible for sausages to move down, so don't even bother.
      } else {
        assert(false); // Unknown direction
        FAIL("Attempted to physically move in an unknown direction");
      }
    }
  }

  if (_cpmOut.sausageToSpear == -1) _cpmOut.sausageToSpear = sausageNo;

  // We have already evaluated this sausage; it would've failed then.
  if (_cpmOut.movedSausages[sausageNo]) return true;
  _cpmOut.movedSausages[sausageNo] = true;

  Sausage sausage = _sausages[sausageNo];

  // Sausages can move if the tiles beyond them can move (either other sausages or empty space)
  if (dir == Up) {
    if (sausage.IsVertical()) {
      return CanPhysicallyMoveInternal(sausage.x1, sausage.y1 - 1, sausage.z, dir);
    } else {
      return CanPhysicallyMoveInternal(sausage.x1, sausage.y1 - 1, sausage.z, dir)
          && CanPhysicallyMoveInternal(sausage.x2, sausage.y2 - 1, sausage.z, dir);
    }
  } else if (dir == Down) {
    if (sausage.IsVertical()) {
      return CanPhysicallyMoveInternal(sausage.x2, sausage.y2 + 1, sausage.z, dir);
    } else {
      return CanPhysicallyMoveInternal(sausage.x1, sausage.y1 + 1, sausage.z, dir)
          && CanPhysicallyMoveInternal(sausage.x2, sausage.y2 + 1, sausage.z, dir);
    }
  } else if (dir == Left) {
    if (sausage.IsHorizontal()) {
      return CanPhysicallyMoveInternal(sausage.x1 - 1, sausage.y1, sausage.z, dir);
    } else {
      return CanPhysicallyMoveInternal(sausage.x1 - 1, sausage.y1, sausage.z, dir)
          && CanPhysicallyMoveInternal(sausage.x2 - 1, sausage.y2, sausage.z, dir);
    }
  } else if (dir == Right) {
    if (sausage.IsHorizontal()) {
      return CanPhysicallyMoveInternal(sausage.x2 + 1, sausage.y2, sausage.z, dir);
    } else {
      return CanPhysicallyMoveInternal(sausage.x1 + 1, sausage.y1, sausage.z, dir)
          && CanPhysicallyMoveInternal(sausage.x2 + 1, sausage.y2, sausage.z, dir);
    }
  } else if (dir == Jump) {
    return CanPhysicallyMoveInternal(sausage.x1, sausage.y1, sausage.z + 1, dir)
        && CanPhysicallyMoveInternal(sausage.x2, sausage.y2, sausage.z + 1, dir);
  } else if (dir == Crouch) {
    return false; // It's never possible for sausages to move down, so don't even bother.
  } else {
    assert(false); // Unknown direction
    FAIL("Attempted to physically move in an unknown direction");
  }
}

bool Level::MoveThroughSpace(s8 x, s8 y, s8 z, Direction dir, bool spear) {
  bool canPhysicallyMove = CanPhysicallyMove(x, y, z, dir);

  if (!canPhysicallyMove) {
    if (spear && _cpmOut.sausageToSpear != -1) {
      _sausageSpeared = _cpmOut.sausageToSpear;
      // This location cannot move, but we can still move into it (by spearing).
      return true;
    }

    if (!_interactive) return false;

    // This branch can be hit from a bunch of places. Let's try to give a useful error message.
    if (spear) FAIL("Stephen's fork cannot move in direction %s, and there is nothing to spear", DIRS[dir]);
    if (_cpmOut.sausageToSpear != -1) FAIL("Cannot move sausage %c in direction %s", 'a' + _cpmOut.sausageToSpear, DIRS[dir]);
    FAIL("Wall at [%d, %d, %d] cannot move %s (probably you pushed a fork into it)", x, y, z, DIRS[dir]);
  }

  // Currently not worrying about side-effects. Wouldn't be too hard to prevent, though:
  // Just copy the sausages into a temp array, then swap the temp and current arrays.
  for (s8 sausageNo = 0; sausageNo < _cpmOut.movedSausages.Size(); sausageNo++) {
    // TODO: I'm really hoping that order doesn't matter here. But it might, if sausages are stacked on each other?
    if (!_cpmOut.movedSausages[sausageNo]) continue;
    Sausage sausage = _sausages[sausageNo];

    bool sausageHasFork = !_stephen.HasFork()
      && sausage.z == _stephen.forkZ
      && ((sausage.x1 == _stephen.forkX && sausage.y1 == _stephen.forkY)
       || (sausage.x2 == _stephen.forkX && sausage.y2 == _stephen.forkY));

    // Move the sausage
    if (dir == Up) {
      sausage.y1--;
      sausage.y2--;
      if (sausageHasFork) _stephen.forkY--;
    } else if (dir == Down) {
      sausage.y1++;
      sausage.y2++;
      if (sausageHasFork) _stephen.forkY++;
    } else if (dir == Left) {
      sausage.x1--;
      sausage.x2--;
      if (sausageHasFork) _stephen.forkX--;
    } else if (dir == Right) {
      sausage.x1++;
      sausage.x2++;
      if (sausageHasFork) _stephen.forkX++;
    } else if (dir == Jump) {
      sausage.z++;
      if (sausageHasFork) _stephen.forkZ--;
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
          if (sausageHasFork && (_stephen.forkDir == Up || _stephen.forkDir == Down)) _stephen.forkDir = Inverse(_stephen.forkDir);
        }
      } else { // sausage.IsVertical()
        if (dir == Left || dir == Right) {
          sausage.flags ^= Sausage::Flags::Rolled;
          if (sausageHasFork && (_stephen.forkDir == Left || _stephen.forkDir == Right)) _stephen.forkDir = Inverse(_stephen.forkDir);
        }
      }

      while (true) {
        bool supported = CanWalkOnto(sausage.x1, sausage.y1, sausage.z)
                      || CanWalkOnto(sausage.x2, sausage.y2, sausage.z);
        if (supported) break;
        if (sausage.z <= 0) FAIL("Sausage %c would fall below the world", 'a' + sausageNo);
        sausage.z--;
        if (sausageHasFork) _stephen.forkZ--;
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

  if (_cpmOut.movedFork) { // This boolean is only set if the fork is not inside a sausage.
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

  return true;
}

bool Level::MoveStephenThroughSpace(Direction dir) {
  // If there's a speared sausage, we need to move it first, and it will check the space it's moving into.
  // If it succeeds, the fork is clear to move (and we don't want to double-move the sausage).
  if (_sausageSpeared != -1 && _stephen.HasFork()) {
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
  } else if (dir == Jump) {
    _stephen.z++;
  }

  // If there's no speared sausage, we need to check the space the fork is moving into.
  if (_sausageSpeared == -1 && _stephen.HasFork()) {
    bool spear = (dir == _stephen.dir);
    if (!MoveThroughSpace(_stephen.forkX, _stephen.forkY, _stephen.z, dir, spear)) return false;
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

#define o(i) \
  { \
    const Sausage& sausage = _sausages[i]; \
    if (z == sausage.z) { \
      if (x == sausage.x1 && y == sausage.y1) return i; \
      if (x == sausage.x2 && y == sausage.y2) return i; \
    } \
  }
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
