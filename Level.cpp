#include "Level.h"
#include <cstdio>

Level::Level(u8 width, u8 height, const char* asciiGrid) {
  _width = width;
  _height = height;
  _grid = NewDoubleArray2<Tile>(width, height);

  for (s32 i=0; i<width * height; i++) {
    s8 x = i % width;
    s8 y = i / width;
    char c = asciiGrid[i];
    if (c == ' ') _grid[x][y] = Tile::Empty;
    else if (c == '_') _grid[x][y] = Tile::Ground;
    else if (c == '#') _grid[x][y] = Tile::Grill;
    else if (c == '^') {
      _grid[x][y] = Tile::Ground;
      _stephen.x = x;
      _stephen.y = y;
      _stephen.dir = Up;
    } else if (c == 'v') {
      _grid[x][y] = Tile::Ground;
      _stephen.x = x;
      _stephen.y = y;
      _stephen.dir = Down;
    } else if (c == '<') {
      _grid[x][y] = Tile::Ground;
      _stephen.x = x;
      _stephen.y = y;
      _stephen.dir = Left;
    } else if (c == '>') {
      _grid[x][y] = Tile::Ground;
      _stephen.x = x;
      _stephen.y = y;
      _stephen.dir = Right;
    } else if (c >= '0' && c <= '9') {
      _grid[x][y] = Tile::Ground;
      // Note that there is some ambiguity here with sausages thinking their first half is at (0, 0)
      // but it's OK, because they will just be replaced once their first half is actually found.
      int num = c - '0';
      if (sausages.Size() > num) { // First sausage half was (maybe) already found
        if (sausages[num].x1 == x-1 && sausages[num].y1 == y) {
          sausages[num].x2 = x;
          sausages[num].y2 = y;
          sausages[num].flags |= Sausage::Flags::Horizontal;
        } else if (sausages[num].x1 == x && sausages[num].y1 == y-1) {
          sausages[num].x2 = x;
          sausages[num].y2 = y;
          sausages[num].flags &= ~Sausage::Flags::Horizontal;
        }
      } else {
        sausages.Ensure(num + 1);
        sausages.Resize(num + 1);
        sausages[num] = Sausage{x, y};
      }
    }
    _start = _stephen;
  }
}

Level::~Level() {
  DeleteDoubleArray2(_grid);
}

bool Level::Move(Direction dir) {
  if (_stephen.dir == Up) {
    if (dir == Left) {
      if (!MoveThroughSpace(_stephen.x - 1, _stephen.y - 1, Left)) return false;
      if (MoveThroughSpace(_stephen.x - 1, _stephen.y, Down)) _stephen.dir = Left;
    } else if (dir == Right) {
      if (!MoveThroughSpace(_stephen.x + 1, _stephen.y - 1, Right)) return false;
      if (MoveThroughSpace(_stephen.x + 1, _stephen.y, Down)) _stephen.dir = Right;
    } else if (dir == Up) {
      if (!CanWalkOnto(_stephen.x, _stephen.y - 1)) return false;
      if (MoveThroughSpace(_stephen.x, _stephen.y - 2, Up)) _stephen.y--;
    } else if (dir == Down) {
      if (!CanWalkOnto(_stephen.x, _stephen.y + 1)) return false;
      if (MoveThroughSpace(_stephen.x, _stephen.y + 1, Down)) _stephen.y++;
    }
  } else if (_stephen.dir == Down) {
    if (dir == Left) {
      if (!MoveThroughSpace(_stephen.x - 1, _stephen.y + 1, Left)) return false;
      if (MoveThroughSpace(_stephen.x - 1, _stephen.y, Up)) _stephen.dir = Left;
    } else if (dir == Right) {
      if (!MoveThroughSpace(_stephen.x + 1, _stephen.y + 1, Right)) return false;
      if (MoveThroughSpace(_stephen.x + 1, _stephen.y, Up)) _stephen.dir = Right;
    } else if (dir == Down) {
      if (!CanWalkOnto(_stephen.x, _stephen.y + 1)) return false;
      if (MoveThroughSpace(_stephen.x, _stephen.y + 2, Down)) _stephen.y++;
    } else if (dir == Up) {
      if (!CanWalkOnto(_stephen.x, _stephen.y - 1)) return false;
      if (MoveThroughSpace(_stephen.x, _stephen.y - 1, Up)) _stephen.y--;
    }
  } else if (_stephen.dir == Left) {
    if (dir == Up) {
      if (!MoveThroughSpace(_stephen.x - 1, _stephen.y - 1, Up)) return false;
      if (MoveThroughSpace(_stephen.x, _stephen.y - 1, Right)) _stephen.dir = Up;
    } else if (dir == Down) {
      if (!MoveThroughSpace(_stephen.x - 1, _stephen.y + 1, Down)) return false;
      if (MoveThroughSpace(_stephen.x, _stephen.y + 1, Right)) _stephen.dir = Down;
    } else if (dir == Left) {
      if (!CanWalkOnto(_stephen.x - 1, _stephen.y)) return false;
      if (MoveThroughSpace(_stephen.x - 2, _stephen.y, Left)) _stephen.x--;
    } else if (dir == Right) {
      if (!CanWalkOnto(_stephen.x + 1, _stephen.y)) return false;
      if (MoveThroughSpace(_stephen.x + 1, _stephen.y, Right)) _stephen.x++;
    }
  } else if (_stephen.dir == Right) {
    if (dir == Up) {
      if (!MoveThroughSpace(_stephen.x + 1, _stephen.y - 1, Up)) return false;
      if (MoveThroughSpace(_stephen.x, _stephen.y - 1, Left)) _stephen.dir = Up;
    } else if (dir == Down) {
      if (!MoveThroughSpace(_stephen.x + 1, _stephen.y + 1, Down)) return false;
      if (MoveThroughSpace(_stephen.x, _stephen.y + 1, Left)) _stephen.dir = Down;
    } else if (dir == Right) {
      if (!CanWalkOnto(_stephen.x + 1, _stephen.y)) return false;
      if (MoveThroughSpace(_stephen.x + 2, _stephen.y, Right)) _stephen.x++;
    } else if (dir == Left) {
      if (!CanWalkOnto(_stephen.x - 1, _stephen.y)) return false;
      if (MoveThroughSpace(_stephen.x - 1, _stephen.y, Left)) _stephen.x--;
    }
  }

  if (IsGrill(_stephen.x, _stephen.y)) { // Burned step
    if (dir == Up) Move(Down);
    else if (dir == Down) Move(Up);
    else if (dir == Left) Move(Right);
    else if (dir == Right) Move(Left);
  }

  return true;
}

bool Level::Won() const {
  if (_stephen.x != _start.x) return false;
  if (_stephen.y != _start.y) return false;
  if (_stephen.dir != _start.dir) return false;
  for (Sausage sausage : sausages) {
    if ((sausage.flags & Sausage::Flags::FullyCooked) != Sausage::Flags::FullyCooked) return false;
  }
  return true;
}

void Level::Print() const {
  char* row = new char[_width + 4];
  row[_width + 2] = '\n';
  row[_width + 3] = '\0';

  row[0] = '+';
  for (u8 x=0; x<_width; x++) row[x+1] = '-';
  row[_width + 1] = '+';
  printf(row);

  row[0] = '|';
  row[_width + 1] = '|';
  for (u8 y=0; y<_height; y++) {
    for (u8 x=0; x<_width; x++) {
      s8 sausageNo = GetSausage(x, y);

      if (sausageNo != -1) row[x+1] = '0' + (char)sausageNo;
      else if (_stephen.dir == Up && x == _stephen.x && y == _stephen.y - 1) row[x+1] = '|';
      else if (_stephen.dir == Down && x == _stephen.x && y == _stephen.y + 1) row[x+1] = '|';
      else if (_stephen.dir == Left && x == _stephen.x - 1 && y == _stephen.y) row[x+1] = '-';
      else if (_stephen.dir == Right && x == _stephen.x + 1 && y == _stephen.y) row[x+1] = '-';
      else if (x == _stephen.x && y == _stephen.y) {
        if (_stephen.dir == Up)          row[x+1] = '^';
        else if (_stephen.dir == Down)   row[x+1] = 'v';
        else if (_stephen.dir == Left)   row[x+1] = '<';
        else if (_stephen.dir == Right)  row[x+1] = '>';
      }
      else if (_grid[x][y] == Empty)  row[x+1] = ' ';
      else if (_grid[x][y] == Ground) row[x+1] = '_';
      else if (_grid[x][y] == Grill)  row[x+1] = '#';
    }
    printf(row);
  }

  row[0] = '+';
  for (u8 x=0; x<_width; x++) row[x+1] = '-';
  row[_width + 1] = '+';
  printf(row);

  delete[] row;
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
    if (sausage.IsHorizontal() && !MoveThroughSpace(sausage.x2 - 1, sausage.y2, dir)) return false;
    if (                                    IsGrill(sausage.x1 - 1, sausage.y1)) sidesToCook |= Sausage::Flags::Cook1A;
    if (                                    IsGrill(sausage.x2 - 1, sausage.y2)) sidesToCook |= Sausage::Flags::Cook2A;
  } else if (dir == Right) {
    if (                          !MoveThroughSpace(sausage.x1 + 1, sausage.y1, dir)) return false;
    if (sausage.IsHorizontal() && !MoveThroughSpace(sausage.x2 + 1, sausage.y2, dir)) return false;
    if (                                    IsGrill(sausage.x1 + 1, sausage.y1)) sidesToCook |= Sausage::Flags::Cook1A;
    if (                                    IsGrill(sausage.x2 + 1, sausage.y2)) sidesToCook |= Sausage::Flags::Cook2A;
  }

  // Finally, success -- move and cook the sausage.

  if (dir == Up) {
    if (sausage.IsHorizontal()) sausage.flags ^= Sausage::Flags::Rolled;
    sausage.y1--;
    sausage.y2--;
  } else if (dir == Down) {
    if (sausage.IsHorizontal()) sausage.flags ^= Sausage::Flags::Rolled;
    sausage.y1++;
    sausage.y2++;
  } else if (dir == Left) {
    if (!sausage.IsHorizontal()) sausage.flags ^= Sausage::Flags::Rolled;
    sausage.x1--;
    sausage.x2--;
  } else if (dir == Right) {
    if (!sausage.IsHorizontal()) sausage.flags ^= Sausage::Flags::Rolled;
    sausage.x1++;
    sausage.x2++;
  }

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
  return x >= 0 && x <= _width - 1 && y >= 0 && y <= _width - 1;
}

bool Level::CanWalkOnto(s8 x, s8 y) const {
  if (!IsWithinGrid(x, y)) return false;
  return _grid[x][y] == Ground; // We'll deal with burned steps later, for now -- you cannot walk onto a grill.
}

bool Level::CanTurnThrough(s8 x, s8 y) const {
  if (!IsWithinGrid(x, y)) return true;
  return true; // We don't have elevation tiles yet, but some day
}

bool Level::IsGrill(s8 x, s8 y) const {
  if (!IsWithinGrid(x, y)) return false;
  return _grid[x][y] == Grill;
}
