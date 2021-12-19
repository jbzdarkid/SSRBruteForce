#include "Level.h"
#include <cstdio>

Level::Level(u8 width, u8 height, const char* asciiGrid) {
  _width = width;
  _height = height;
  _grid = NewDoubleArray2<Tile>(width, height);
#define o(x) + 1
  sausages.Ensure(0 + SAUSAGES);
  // Push sausages which are very invalid so we don't accidentally find them.
  for (u8 i=0; i<0+SAUSAGES; i++) sausages.UnsafePush({-127, -127, -127, -127});
#undef o

  for (s32 i=0; i<width * height; i++) {
    s8 x = i % width;
    s8 y = i / width;
    char c = asciiGrid[i];
    if (c == ' ') _grid[x][y] = Tile::Empty;
    else if (c == '_') _grid[x][y] = Tile::Ground;
    else if (c == '#') _grid[x][y] = Tile::Grill;
    else if (c == '^') {
      _grid[x][y] = Tile::Ground;
      stephen.x = x;
      stephen.y = y;
      stephen.dir = Up;
    } else if (c == 'v') {
      _grid[x][y] = Tile::Ground;
      stephen.x = x;
      stephen.y = y;
      stephen.dir = Down;
    } else if (c == '<') {
      _grid[x][y] = Tile::Ground;
      stephen.x = x;
      stephen.y = y;
      stephen.dir = Left;
    } else if (c == '>') {
      _grid[x][y] = Tile::Ground;
      stephen.x = x;
      stephen.y = y;
      stephen.dir = Right;
    } else if (c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z') {
      int num;
      if (c >= 'A' && c <= 'Z') {
        _grid[x][y] = Tile::Empty;
        num = c - 'A';
      } else {
        _grid[x][y] = Tile::Ground;
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
    }
  }
  _start = stephen;
}

Level::~Level() {
  DeleteDoubleArray2(_grid);
}

bool Level::Move(Direction dir) {
  if (stephen.dir == Up) {
    if (dir == Left) {
      if (!MoveThroughSpace(stephen.x - 1, stephen.y - 1, Left)) return false;
      if (MoveThroughSpace(stephen.x - 1, stephen.y, Down)) stephen.dir = Left;
    } else if (dir == Right) {
      if (!MoveThroughSpace(stephen.x + 1, stephen.y - 1, Right)) return false;
      if (MoveThroughSpace(stephen.x + 1, stephen.y, Down)) stephen.dir = Right;
    } else if (dir == Up) {
      if (!CanWalkOnto(stephen.x, stephen.y - 1)) return false;
      if (MoveThroughSpace(stephen.x, stephen.y - 2, Up)) stephen.y--;
    } else if (dir == Down) {
      if (!CanWalkOnto(stephen.x, stephen.y + 1)) return false;
      if (MoveThroughSpace(stephen.x, stephen.y + 1, Down)) stephen.y++;
    }
  } else if (stephen.dir == Down) {
    if (dir == Left) {
      if (!MoveThroughSpace(stephen.x - 1, stephen.y + 1, Left)) return false;
      if (MoveThroughSpace(stephen.x - 1, stephen.y, Up)) stephen.dir = Left;
    } else if (dir == Right) {
      if (!MoveThroughSpace(stephen.x + 1, stephen.y + 1, Right)) return false;
      if (MoveThroughSpace(stephen.x + 1, stephen.y, Up)) stephen.dir = Right;
    } else if (dir == Down) {
      if (!CanWalkOnto(stephen.x, stephen.y + 1)) return false;
      if (MoveThroughSpace(stephen.x, stephen.y + 2, Down)) stephen.y++;
    } else if (dir == Up) {
      if (!CanWalkOnto(stephen.x, stephen.y - 1)) return false;
      if (MoveThroughSpace(stephen.x, stephen.y - 1, Up)) stephen.y--;
    }
  } else if (stephen.dir == Left) {
    if (dir == Up) {
      if (!MoveThroughSpace(stephen.x - 1, stephen.y - 1, Up)) return false;
      if (MoveThroughSpace(stephen.x, stephen.y - 1, Right)) stephen.dir = Up;
    } else if (dir == Down) {
      if (!MoveThroughSpace(stephen.x - 1, stephen.y + 1, Down)) return false;
      if (MoveThroughSpace(stephen.x, stephen.y + 1, Right)) stephen.dir = Down;
    } else if (dir == Left) {
      if (!CanWalkOnto(stephen.x - 1, stephen.y)) return false;
      if (MoveThroughSpace(stephen.x - 2, stephen.y, Left)) stephen.x--;
    } else if (dir == Right) {
      if (!CanWalkOnto(stephen.x + 1, stephen.y)) return false;
      if (MoveThroughSpace(stephen.x + 1, stephen.y, Right)) stephen.x++;
    }
  } else if (stephen.dir == Right) {
    if (dir == Up) {
      if (!MoveThroughSpace(stephen.x + 1, stephen.y - 1, Up)) return false;
      if (MoveThroughSpace(stephen.x, stephen.y - 1, Left)) stephen.dir = Up;
    } else if (dir == Down) {
      if (!MoveThroughSpace(stephen.x + 1, stephen.y + 1, Down)) return false;
      if (MoveThroughSpace(stephen.x, stephen.y + 1, Left)) stephen.dir = Down;
    } else if (dir == Right) {
      if (!CanWalkOnto(stephen.x + 1, stephen.y)) return false;
      if (MoveThroughSpace(stephen.x + 2, stephen.y, Right)) stephen.x++;
    } else if (dir == Left) {
      if (!CanWalkOnto(stephen.x - 1, stephen.y)) return false;
      if (MoveThroughSpace(stephen.x - 1, stephen.y, Left)) stephen.x--;
    }
  }

  if (IsGrill(stephen.x, stephen.y)) { // Burned step
    if (dir == Up) Move(Down);
    else if (dir == Down) Move(Up);
    else if (dir == Left) Move(Right);
    else if (dir == Right) Move(Left);
  }

  return true;
}

bool Level::Won() const {
  if (stephen.x != _start.x) return false;
  if (stephen.y != _start.y) return false;
  if (stephen.dir != _start.dir) return false;
  for (const Sausage& sausage : sausages) {
    if ((sausage.flags & Sausage::Flags::FullyCooked) != Sausage::Flags::FullyCooked) return false;
  }
  return true;
}

void Level::Print() const {
  char* row = new char[_width + 4];
  row[_width + 2] = '\n';
  row[_width + 3] = '\0';

  row[0] = '/';
  for (u8 x=0; x<_width; x++) row[x+1] = '-';
  row[_width + 1] = '\\';
  printf(row);

  row[0] = '|';
  row[_width + 1] = '|';
  for (u8 y=0; y<_height; y++) {
    for (u8 x=0; x<_width; x++) {
      s8 sausageNo = GetSausage(x, y);

      if (sausageNo != -1 && _grid[x][y] == Empty) row[x+1] = 'A' + (char)sausageNo;
      else if (sausageNo != -1)                    row[x+1] = 'a' + (char)sausageNo;
      //else if (stephen.dir == Up && x == stephen.x && y == stephen.y - 1) row[x+1] = '|';
      //else if (stephen.dir == Down && x == stephen.x && y == stephen.y + 1) row[x+1] = '|';
      //else if (stephen.dir == Left && x == stephen.x - 1 && y == stephen.y) row[x+1] = '-';
      //else if (stephen.dir == Right && x == stephen.x + 1 && y == stephen.y) row[x+1] = '-';
      else if (x == stephen.x && y == stephen.y) {
        if (stephen.dir == Up)          row[x+1] = '^';
        else if (stephen.dir == Down)   row[x+1] = 'v';
        else if (stephen.dir == Left)   row[x+1] = '<';
        else if (stephen.dir == Right)  row[x+1] = '>';
      }
      else if (_grid[x][y] == Empty)  row[x+1] = ' ';
      else if (_grid[x][y] == Ground) row[x+1] = '_';
      else if (_grid[x][y] == Grill)  row[x+1] = '#';
    }
    printf(row);
  }

  row[0] = '\\';
  for (u8 x=0; x<_width; x++) row[x+1] = '-';
  row[_width + 1] = '/';
  printf(row);

  delete[] row;
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

bool Level::MoveThroughSpace(s8 x, s8 y, Direction dir) {
  if (!CanTurnThrough(x, y)) return false;

  s8 sausageNo = GetSausage(x, y);
  if (sausageNo == -1) return true; // The move should succeed because there's nothing in the way.
  Sausage sausage = sausages[sausageNo];

  u8 sidesToCook = 0;

  // This is a little wrong. This code allows a sausage push which returns false to have side-effects.
  // For example, if one sausage pushes two different sausages, and only one them can move.
  if (dir == Up) {
    if (                          !MoveThroughSpace(sausage.x1, sausage.y1 - 1, dir)) return false;
    if (sausage.IsHorizontal() && !MoveThroughSpace(sausage.x2, sausage.y2 - 1, dir)) return false;
    if (                                    IsGrill(sausage.x1, sausage.y1 - 1)) sidesToCook |= Sausage::Flags::Cook1A;
    if (                                    IsGrill(sausage.x2, sausage.y2 - 1)) sidesToCook |= Sausage::Flags::Cook2A;
  } else if (dir == Down) {
    if (sausage.IsHorizontal() && !MoveThroughSpace(sausage.x1, sausage.y1 + 1, dir)) return false;
    if (                          !MoveThroughSpace(sausage.x2, sausage.y2 + 1, dir)) return false;
    if (                                    IsGrill(sausage.x1, sausage.y1 + 1)) sidesToCook |= Sausage::Flags::Cook1A;
    if (                                    IsGrill(sausage.x2, sausage.y2 + 1)) sidesToCook |= Sausage::Flags::Cook2A;
  } else if (dir == Left) {
    if (                          !MoveThroughSpace(sausage.x1 - 1, sausage.y1, dir)) return false;
    if (sausage.IsVertical()   && !MoveThroughSpace(sausage.x2 - 1, sausage.y2, dir)) return false;
    if (                                    IsGrill(sausage.x1 - 1, sausage.y1)) sidesToCook |= Sausage::Flags::Cook1A;
    if (                                    IsGrill(sausage.x2 - 1, sausage.y2)) sidesToCook |= Sausage::Flags::Cook2A;
  } else if (dir == Right) {
    if (sausage.IsVertical()   && !MoveThroughSpace(sausage.x1 + 1, sausage.y1, dir)) return false;
    if (                          !MoveThroughSpace(sausage.x2 + 1, sausage.y2, dir)) return false;
    if (                                    IsGrill(sausage.x1 + 1, sausage.y1)) sidesToCook |= Sausage::Flags::Cook1A;
    if (                                    IsGrill(sausage.x2 + 1, sausage.y2)) sidesToCook |= Sausage::Flags::Cook2A;
  }

  // The sausage can move (and we know which parts get cooked), move and cook

  if (dir == Up) {
    if (sausage.IsHorizontal()) sausage.flags ^= Sausage::Flags::Rolled;
    sausage.y1--;
    sausage.y2--;
  } else if (dir == Down) {
    if (sausage.IsHorizontal()) sausage.flags ^= Sausage::Flags::Rolled;
    sausage.y1++;
    sausage.y2++;
  } else if (dir == Left) {
    if (sausage.IsVertical()) sausage.flags ^= Sausage::Flags::Rolled;
    sausage.x1--;
    sausage.x2--;
  } else if (dir == Right) {
    if (sausage.IsVertical()) sausage.flags ^= Sausage::Flags::Rolled;
    sausage.x1++;
    sausage.x2++;
  }

  if (!IsSupported(sausage)) return false;

  if (sausage.IsRolled()) sidesToCook *= 2; // Shift cooking flags to the rolled side
  if ((sausage.flags & sidesToCook) > 0) return false; // Move would burn a side of the sausage
  sausage.flags |= sidesToCook;

  sausages[sausageNo] = sausage;
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

bool Level::CanWalkOnto(s8 x, s8 y) const {
  if (!IsWithinGrid(x, y)) return false;
  Tile cell = _grid[x][y];
  // Stephen *can* walk onto a grill, he just has to walk off again.
  // But, walking onto a grill can spear a sausage, so we handle that by forcing a move where he walks off.
  return cell == Ground || cell == Grill;
}

bool Level::CanTurnThrough(s8 x, s8 y) const {
  if (!IsWithinGrid(x, y)) return true;
  Tile cell = _grid[x][y];
  return cell == Empty || cell == Ground || cell == Grill;
}

bool Level::IsGrill(s8 x, s8 y) const {
  if (!IsWithinGrid(x, y)) return false;
  return _grid[x][y] == Grill;
}

bool Level::IsSupported(const Sausage& sausage) const {
  return CanWalkOnto(sausage.x1, sausage.y1) || CanWalkOnto(sausage.x2, sausage.y2);
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
static_assert(0 SAUSAGES <= 4); // We cannot create a u32 out of 5 u8s.
#undef o

  return hash;
}
