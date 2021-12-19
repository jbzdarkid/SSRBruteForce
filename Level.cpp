#include "Level.h"
#include <cstdio>

Level::Level(u8 width, u8 height, const char* name, const char* asciiGrid) {
  this->name = name;
  _width = width;
  _height = height;
  _grid = NewDoubleArray2<Tile>(width, height);

#define o(x) +1
  while (sausages.Size() < SAUSAGES + 1) sausages.Push({-127, -127, -127, -127, 0});
  _movedSausages = Vector<s8>(SAUSAGES);
#undef o

  for (s32 i=0; i<width * height; i++) {
    s8 x = i % width;
    s8 y = i / width;
    char c = asciiGrid[i];
    if (c == ' ') _grid[x][y] = Empty;
    else if (c == '_') _grid[x][y] = Ground;
    else if (c == '#') _grid[x][y] = Grill;
    else if (c == '1') _grid[x][y] = Wall;
    else if (c == '^' || c == 'v' || c == '<' || c == '>') {
      _grid[x][y] = Ground;
      stephen.x = x;
      stephen.y = y;
      if (c == '^')      stephen.dir = Up;
      else if (c == 'v') stephen.dir = Down;
      else if (c == '<') stephen.dir = Left;
      else if (c == '>') stephen.dir = Right;
    } else if (c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z') {
      int num;
      if (c >= 'A' && c <= 'Z') {
        _grid[x][y] = Empty;
        num = c - 'A';
      } else {
        _grid[x][y] = Ground;
        num = c - 'a';
      }

      if (sausages[num].x1 == x-1 && sausages[num].y1 == y) {
        sausages[num].x2 = x;
        sausages[num].y2 = y;
        sausages[num].flags |= Sausage::Flags::Horizontal;
      } else if (sausages[num].x1 == x && sausages[num].y1 == y-1) {
        sausages[num].x2 = x;
        sausages[num].y2 = y;
        sausages[num].flags &= ~Sausage::Flags::Horizontal;
      } else {
        sausages[num].x1 = x;
        sausages[num].y1 = y;
      }
    } else assert(false); // Unknown tile
  }
  _start = stephen;
}

Level::~Level() {
  DeleteDoubleArray2(_grid);
}

void Level::Print() const {
  putchar('+');
  for (u8 x=0; x<_width; x++) putchar('-');
  putchar('+');
  putchar('\n');

  for (u8 y=0; y<_height; y++) {
    putchar('|');
    for (u8 x=0; x<_width; x++) {
      s8 sausageNo = GetSausage(x, y);

      if (sausageNo != -1 && _grid[x][y] == Empty) putchar('A' + (char)sausageNo);
      else if (sausageNo != -1)                    putchar('a' + (char)sausageNo);
      //else if (stephen.dir == Up && x == stephen.x && y == stephen.y - 1) row[x+1] = '|';
      //else if (stephen.dir == Down && x == stephen.x && y == stephen.y + 1) row[x+1] = '|';
      //else if (stephen.dir == Left && x == stephen.x - 1 && y == stephen.y) row[x+1] = '-';
      //else if (stephen.dir == Right && x == stephen.x + 1 && y == stephen.y) row[x+1] = '-';
      else if (x == stephen.x && y == stephen.y) {
        if (stephen.dir == Up)          putchar('^');
        else if (stephen.dir == Down)   putchar('v');
        else if (stephen.dir == Left)   putchar('<');
        else if (stephen.dir == Right)  putchar('>');
      }
      else if (_grid[x][y] == Empty)  putchar(' ');
      else if (_grid[x][y] == Ground) putchar('_');
      else if (_grid[x][y] == Grill)  putchar('#');
      else if (_grid[x][y] == Wall)   putchar('1') ;
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
  _explain = true;
  Print();
  printf("ULDR: ");
  Vector<State> undoHistory({GetState()});
  while (!Won()) {
    int ch = getchar();
    if (ch == '\n') {
      Print();
      printf("ULDR: ");
      continue;
    }
    else if (ch == 'z' || ch == 'Z') {
      if (undoHistory.Size() > 1) { // Cannot pop the initial state
        undoHistory.Pop();
        SetState(&undoHistory[undoHistory.Size()-1]); // The state at the (new) end of the list
      }
    }
    else if (ch == 'q' || ch == 'Q') return false;

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

  printf("Level completed in %d moves\n", undoHistory.Size());
  return true;
}

bool Level::Won() const {
  if (stephen.x != _start.x) return false;
  if (stephen.y != _start.y) return false;
  if (stephen.dir != _start.dir) return false;
#define o(x) if ((sausages[x].flags & Sausage::Flags::FullyCooked) != Sausage::Flags::FullyCooked) return false;
  SAUSAGES
#undef o
  return true;
}

State Level::GetState() const {
  State s;
  s.stephen = stephen;
#define o(x) s.s##x = sausages[x];
  SAUSAGES
#undef o
  return s;
}

void Level::SetState(const State* s) {
  stephen = s->stephen;
#define o(x) sausages[x] = s->s##x;
  SAUSAGES
#undef o
}

#if _DEBUG
const char* dirs = " UD L   R";
#define EXPLAIN(reason)  if (_explain) printf("Stephen cannot move %c because sausage %c at (%d, %d) %s\n", dirs[dir], 'a' + sausageNo, x, y, reason)
#define EXPLAIN2(reason) if (_explain) printf("(%d, %d) cannot move %c because %s\n", x, y, dirs[dir], reason);
#define EXPLAIN3(reason) if (_explain) printf("Stephen cannot move %c because %s\n", dirs[dir], reason);
#else
#define EXPLAIN(reason)
#define EXPLAIN2(reason)
#define EXPLAIN3(reason)
#endif

// No longer promising 'no side effects', now just returns "was move valid"
bool Level::Move(Direction dir) {
  if (stephen.sausageSpeared == -1) {
    if (stephen.dir == Up) {
      if (dir == Left) {
        if (!MoveThroughSpace(stephen.x - 1, stephen.y - 1, Left)) return false;
        if (!MoveThroughSpace(stephen.x - 1, stephen.y, Down)) return false;
        stephen.dir = Left;
      } else if (dir == Right) {
        if (!MoveThroughSpace(stephen.x + 1, stephen.y - 1, Right)) return false;
        if (!MoveThroughSpace(stephen.x + 1, stephen.y, Down)) return false;
        stephen.dir = Right;
      } else if (dir == Up) {
        if (!CanWalkOnto(stephen.x, stephen.y - 1)) { EXPLAIN3("he would walk off a cliff"); return false; }
        if (!MoveThroughSpace(stephen.x, stephen.y - 2, Up, true)) return false;
        stephen.y--;
      } else if (dir == Down) {
        if (!CanWalkOnto(stephen.x, stephen.y + 1)) { EXPLAIN3("he would walk off a cliff"); return false; }
        if (!MoveThroughSpace(stephen.x, stephen.y + 1, Down)) return false;
        stephen.y++;
      }
    } else if (stephen.dir == Down) {
      if (dir == Left) {
        if (!MoveThroughSpace(stephen.x - 1, stephen.y + 1, Left)) return false;
        if (!MoveThroughSpace(stephen.x - 1, stephen.y, Up)) return false;
        stephen.dir = Left;
      } else if (dir == Right) {
        if (!MoveThroughSpace(stephen.x + 1, stephen.y + 1, Right)) return false;
        if (!MoveThroughSpace(stephen.x + 1, stephen.y, Up)) return false;
        stephen.dir = Right;
      } else if (dir == Down) {
        if (!CanWalkOnto(stephen.x, stephen.y + 1)) { EXPLAIN3("he would walk off a cliff"); return false; }
        if (!MoveThroughSpace(stephen.x, stephen.y + 2, Down, true)) return false;
        stephen.y++;
      } else if (dir == Up) {
        if (!CanWalkOnto(stephen.x, stephen.y - 1)) { EXPLAIN3("he would walk off a cliff"); return false; }
        if (!MoveThroughSpace(stephen.x, stephen.y - 1, Up)) return false;
        stephen.y--;
      }
    } else if (stephen.dir == Left) {
      if (dir == Up) {
        if (!MoveThroughSpace(stephen.x - 1, stephen.y - 1, Up)) return false;
        if (!MoveThroughSpace(stephen.x, stephen.y - 1, Right)) return false;
        stephen.dir = Up;
      } else if (dir == Down) {
        if (!MoveThroughSpace(stephen.x - 1, stephen.y + 1, Down)) return false;
        if (!MoveThroughSpace(stephen.x, stephen.y + 1, Right)) return false;
        stephen.dir = Down;
      } else if (dir == Left) {
        if (!CanWalkOnto(stephen.x - 1, stephen.y)) { EXPLAIN3("he would walk off a cliff"); return false; }
        if (!MoveThroughSpace(stephen.x - 2, stephen.y, Left, true))  return false;
        stephen.x--;
      } else if (dir == Right) {
        if (!CanWalkOnto(stephen.x + 1, stephen.y)) { EXPLAIN3("he would walk off a cliff"); return false; }
        if (!MoveThroughSpace(stephen.x + 1, stephen.y, Right))  return false;
        stephen.x++;
      }
    } else if (stephen.dir == Right) {
      if (dir == Up) {
        if (!MoveThroughSpace(stephen.x + 1, stephen.y - 1, Up)) return false;
        if (!MoveThroughSpace(stephen.x, stephen.y - 1, Left)) return false;
        stephen.dir = Up;
      } else if (dir == Down) {
        if (!MoveThroughSpace(stephen.x + 1, stephen.y + 1, Down)) return false;
        if (!MoveThroughSpace(stephen.x, stephen.y + 1, Left)) return false;
        stephen.dir = Down;
      } else if (dir == Right) {
        if (!CanWalkOnto(stephen.x + 1, stephen.y)) { EXPLAIN3("he would walk off a cliff"); return false; }
        if (!MoveThroughSpace(stephen.x + 2, stephen.y, Right, true)) return false;
        stephen.x++;
      } else if (dir == Left) {
        if (!CanWalkOnto(stephen.x - 1, stephen.y)) { EXPLAIN3("he would walk off a cliff"); return false; }
        if (!MoveThroughSpace(stephen.x - 1, stephen.y, Left)) return false;
        stephen.x--;
      }
    }
  } else if (stephen.sausageSpeared != -1) {
    // First, see if stephen can move
    if (dir == Up) {
      if (!CanPhysicallyMove(stephen.x, stephen.y - 1, dir)) return false;
    } else if (dir == Down) {
      if (!CanPhysicallyMove(stephen.x, stephen.y + 1, dir)) return false;
    } else if (dir == Left) {
      if (!CanPhysicallyMove(stephen.x - 1, stephen.y, dir)) return false;
    } else if (dir == Right) {
      if (!CanPhysicallyMove(stephen.x + 1, stephen.y, dir)) return false;
    }
    // Then, check to see if our sausage gets unspeared
    if (dir == Up && stephen.dir == Down) {
      if (!CanPhysicallyMove(stephen.x, stephen.y + 1, dir)) stephen.sausageSpeared = -1;
    } else if (dir == Down && stephen.dir == Up) {
      if (!CanPhysicallyMove(stephen.x, stephen.y - 1, dir)) stephen.sausageSpeared = -1;
    } else if (dir == Left && stephen.dir == Right) {
      if (!CanPhysicallyMove(stephen.x + 1, stephen.y, dir)) stephen.sausageSpeared = -1;
    } else if (dir == Right && stephen.dir == Left) {
      if (!CanPhysicallyMove(stephen.x - 1, stephen.y, dir)) stephen.sausageSpeared = -1;
    }
    // If the sausage is still speared, try to move it, and fail the movement is invalid
    if (stephen.sausageSpeared != -1) {
      if (stephen.dir == Up) {
        if (!MoveThroughSpace(stephen.x, stephen.y - 1, dir)) return false;
      } else if (stephen.dir == Down) {
        if (!MoveThroughSpace(stephen.x, stephen.y + 1, dir)) return false;
      } else if (stephen.dir == Left) {
        if (!MoveThroughSpace(stephen.x - 1, stephen.y, dir)) return false;
      } else if (stephen.dir == Right) {
        if (!MoveThroughSpace(stephen.x + 1, stephen.y, dir)) return false;
      }
    }
    // Finally, move stephen, now that our sausage has moved out of the way
    if (dir == Up) {
      if (!CanWalkOnto(stephen.x, stephen.y - 1)) { EXPLAIN3("he would walk off a cliff"); return false; }
      if (!MoveThroughSpace(stephen.x, stephen.y - 1, Up)) return false;
      stephen.y--;
    } else if (dir == Down) {
      if (!CanWalkOnto(stephen.x, stephen.y + 1)) { EXPLAIN3("he would walk off a cliff"); return false; }
      if (!MoveThroughSpace(stephen.x, stephen.y + 1, Down)) return false;
      stephen.y++;
    } else if (dir == Left) {
      if (!CanWalkOnto(stephen.x - 1, stephen.y)) { EXPLAIN3("he would walk off a cliff"); return false; }
      if (!MoveThroughSpace(stephen.x - 1, stephen.y, Left)) return false;
      stephen.x--;
    } else if (dir == Right) {
      if (!CanWalkOnto(stephen.x + 1, stephen.y)) { EXPLAIN3("he would walk off a cliff"); return false; }
      if (!MoveThroughSpace(stephen.x + 1, stephen.y, Right)) return false;
      stephen.x++;
    }
  }

  if (IsGrill(stephen.x, stephen.y)) { // Stepped onto a grill, forcibly step back
    bool success = false;
    if (dir == Up) success = Move(Down);
    else if (dir == Down) success = Move(Up);
    else if (dir == Left) success = Move(Right);
    else if (dir == Right) success = Move(Left);
    if (!success) {
      EXPLAIN3("because the corresponding burned step does not succeed");
      return false;
    }
  }

  return true;
}

bool Level::CanPhysicallyMove(s8 x, s8 y, Direction dir, Vector<s8>* movedSausages) {
  if (IsWall(x, y)) return false; // No, walls cannot move.

  s8 sausageNo = GetSausage(x, y);
  if (sausageNo == -1) return true; // Empty spaces can move.
  if (movedSausages) {
    for (s8 i : *movedSausages) {
      // We have already evaluated this sausage; it would've failed then.
      if (i == sausageNo) return true;
    }
    movedSausages->UnsafePush(sausageNo);
  }
  Sausage sausage = sausages[sausageNo];

  // Sausages can move if the tiles beyond them can move (either other sausages or empty space)
  if (dir == Up) {
    return                        CanPhysicallyMove(sausage.x1, sausage.y1 - 1, dir, movedSausages)
      && (sausage.IsVertical() || CanPhysicallyMove(sausage.x2, sausage.y2 - 1, dir, movedSausages));
  } else if (dir == Down) {
    return                        CanPhysicallyMove(sausage.x2, sausage.y2 + 1, dir, movedSausages)
      && (sausage.IsVertical() || CanPhysicallyMove(sausage.x1, sausage.y1 + 1, dir, movedSausages));
  } else if (dir == Left) {
    return                          CanPhysicallyMove(sausage.x1 - 1, sausage.y1, dir, movedSausages)
      && (sausage.IsHorizontal() || CanPhysicallyMove(sausage.x2 - 1, sausage.y2, dir, movedSausages));
  } else if (dir == Right) {
    return                          CanPhysicallyMove(sausage.x2 + 1, sausage.y2, dir, movedSausages)
      && (sausage.IsHorizontal() || CanPhysicallyMove(sausage.x1 + 1, sausage.y1, dir, movedSausages));
  } else {
    assert(false); // Unknown direction
    return false;
  }
}

bool Level::MoveThroughSpace(s8 x, s8 y, Direction dir, bool spear) {
  _movedSausages.Resize(0);
  bool canPhysicallyMove = CanPhysicallyMove(x, y, dir, &_movedSausages);

  if (!canPhysicallyMove) {
    if (spear && _movedSausages.Size() > 0) {
      stephen.sausageSpeared = _movedSausages[0];
      // This location cannot move, but we can stil move into it (by spearing).
      return true;
    }
    // This location cannot move (because some part of it pushes into a wall).
    return false;
  }

  // Currently not worrying about side-effects. Wouldn't be too hard to prevent, though.
  for (s8 sausageNo : _movedSausages) {
    Sausage sausage = sausages[sausageNo];

    // Move the sausage
    if (dir == Up) {
      sausage.y1--;
      sausage.y2--;
    } else if (dir == Down) {
      sausage.y1++;
      sausage.y2++;
    } else if (dir == Left) {
      sausage.x1--;
      sausage.x2--;
    } else if (dir == Right) {
      sausage.x1++;
      sausage.x2++;
    }

    // Speared sausages do not roll nor fall off cliffs
    if (sausageNo != stephen.sausageSpeared) {
      // Check if the sausage would fall off a cliff
      if (!CanWalkOnto(sausage.x1, sausage.y1)
       && !CanWalkOnto(sausage.x2, sausage.y2)) {
        return false;
      }

      // Roll the sausage
      if ((sausage.IsHorizontal() && (dir == Up || dir == Down))
        || (sausage.IsVertical() && (dir == Left || dir == Right))) {
        sausage.flags ^= Sausage::Flags::Rolled;
      }
    }

    // Cook the sausage
    u8 sidesToCook = 0;
    if (IsGrill(sausage.x1, sausage.y1)) sidesToCook |= Sausage::Flags::Cook1A;
    if (IsGrill(sausage.x2, sausage.y2)) sidesToCook |= Sausage::Flags::Cook2A;

    if (sidesToCook > 0) {
      if (sausage.flags & Sausage::Flags::Rolled) sidesToCook *= 2; // Shift cooking flags to the rolled side
      if (sausage.flags & sidesToCook) return false; // If any of the sides are already cooked
      sausage.flags |= sidesToCook;
    }

    // If nothing has gone wrong, save the updated sausage
    sausages[sausageNo] = sausage;
  }

  return true;
}

s8 Level::GetSausage(s8 x, s8 y) const {
  for (u8 i=0; i<sausages.Size(); i++) {
    Sausage sausage = sausages[i];
    if ((x == sausage.x1 && y == sausage.y1) || (x == sausage.x2 && y == sausage.y2)) return i;
  }
  return -1;
}

bool Level::IsWithinGrid(s8 x, s8 y) const {
  return x >= 0 && x <= _width - 1 && y >= 0 && y <= _height - 1;
}

bool Level::IsWall(s8 x, s8 y) const {
  if (!IsWithinGrid(x, y)) return false;
  Tile cell = _grid[x][y];
  return cell == Wall;
}

bool Level::CanTurnThrough(s8 x, s8 y) const {
  return !IsWall(x, y);
}

bool Level::CanWalkOnto(s8 x, s8 y) const {
  if (!IsWithinGrid(x, y)) return false;
  Tile cell = _grid[x][y];
  // Stephen *can* walk onto a grill, he just has to walk off again.
  // But, walking onto a grill can spear a sausage, so we handle that by forcing a move where he walks off.
  return cell == Ground || cell == Grill;
}

bool Level::IsGrill(s8 x, s8 y) const {
  if (!IsWithinGrid(x, y)) return false;
  return _grid[x][y] == Grill;
}

bool State::operator==(const State& other) const {
  return stephen.x == other.stephen.x
      && stephen.y == other.stephen.y
      && stephen.dir == other.stephen.dir
#define o(x) && s##x == other.s##x
    SAUSAGES;
#undef o
}

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
  b = triple32_hash(b);
  a += (b << 6) + (b >> 2);
}

u32 State::Hash() const {
    u32 hash = triple32_hash(*(u32*)&stephen);
#define o(x) combine_hash(hash, *(u32*)&s##x);
  SAUSAGES
#undef o
#define o(x) | (s##x.flags << (4*x))
    combine_hash(hash, 0 SAUSAGES);
#undef o

#define o(x) + 1
static_assert(SAUSAGES <= 4); // We cannot create a u32 out of 5 u8s.
#undef o

  return hash;
}
