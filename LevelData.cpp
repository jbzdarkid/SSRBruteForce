#include "LevelData.h"
#include <cstdio>

LevelData::LevelData(u8 width, u8 height, const char* name, const char* asciiGrid,
  const Stephen& stephen,
  std::initializer_list<Ladder> ladders,
  std::initializer_list<Sausage> sausages,
  std::initializer_list<Tile> tiles)
  : _width(width),
    _height(height),
    _grid(NArray<Tile>(_width, _height)),
    name(name)
{
  _grid.Fill(Tile::Empty);
  Vector<Tile> extraTiles(tiles);

  assert(width * height == strlen(asciiGrid));
  for (s32 i=0; i<width * height; i++) {
    s8 x = i % width;
    s8 y = i / width;
    char c = asciiGrid[i];
    if (c == '?') _grid(x, y) = extraTiles.PopValue();
    else if (c == ' ') _grid(x, y) = Empty;
    else if (c == '#') _grid(x, y) = GroundGrill;
    else if (c == '$') _grid(x, y) = Wall1Grill;
    else if (c == '%') _grid(x, y) = Wall2Grill;
    else if (c == '_') _grid(x, y) = Ground;
    else if (c == '1') _grid(x, y) = Wall1;
    else if (c == '2') _grid(x, y) = Wall2;
    else if (c == '3') _grid(x, y) = Wall3;
    else if (c == '4') _grid(x, y) = Wall4;
    else if (c == '5') _grid(x, y) = Wall5;
    else if (c == '6') _grid(x, y) = Wall6;
    else if (c == '7') _grid(x, y) = Wall7;
    else if (c == '8') _grid(x, y) = Wall8;
#if (OVERWORLD_HACK == 0 || OVERWORLD_HACK >= 2) // need to use these capital letters for sausages I mean not really but whatever
    else if (c == 'U') { _grid(x, y) = Ground; _ladders.Push(Ladder{x, y, 0, Up}); }
    else if (c == 'D') { _grid(x, y) = Ground; _ladders.Push(Ladder{x, y, 0, Down}); }
    else if (c == 'L') { _grid(x, y) = Ground; _ladders.Push(Ladder{x, y, 0, Left}); }
    else if (c == 'R') { _grid(x, y) = Ground; _ladders.Push(Ladder{x, y, 0, Right}); }
#endif
    else if (c == '^') { _grid(x, y) = Ground; _stephen = Stephen(x, y, 0, Up); }
    else if (c == 'v') { _grid(x, y) = Ground; _stephen = Stephen(x, y, 0, Down); }
    else if (c == '<') { _grid(x, y) = Ground; _stephen = Stephen(x, y, 0, Left); }
    else if (c == '>') { _grid(x, y) = Ground; _stephen = Stephen(x, y, 0, Right); }
    else if (c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z') {
      int num;
#if OVERWORLD_HACK
      if (c >= 'A' && c <= 'Z') {
        _grid(x, y) = Ground;
        num = c - 'A';
      } else {
        _grid(x, y) = Ground;
        num = c - 'a' + 26;
      }
#else
      if (c >= 'A' && c <= 'Z') {
        _grid(x, y) = Empty;
        num = c - 'A';
      } else {
        _grid(x, y) = Ground;
        num = c - 'a';
      }
#endif

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

    if (name[0] == 'O' && name[1] == 'v' && name[2] == 'e') { // Overworlds
      const char* dirs[] = {
        nullptr,
        "North",
        "West",
        nullptr,
        nullptr,
        "East",
        "South",
      };
      //printf("[%s] if (_stephen.x == %d && _stephen.y == %d && _stephen.dir == %s) sausagesToRemove = {};\n", name, _stephen.x, _stephen.y, dirs[_stephen.dir]);
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

  assert(extraTiles.Empty()); // Assert that all excess tiles were consumed
}

void LevelData::Print() const {
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
#if OVERWORLD_HACK
          if (sausageNo != -1 && sausageNo < 26) dynamic = 'A' + sausageNo;
          if (sausageNo != -1 && sausageNo >= 26) dynamic = 'a' + sausageNo - 26;
#else
          if (sausageNo != -1 && _grid(x, y) == Empty) dynamic = 'A' + sausageNo;
          if (sausageNo != -1 && _grid(x, y) != Empty) dynamic = 'a' + sausageNo;
#endif
        } else if (dynamic == ' ') { // Ladders are lower priority over basically everything else.
          for (const Ladder& ladder : _ladders) {
            if (ladder.x == x && ladder.y == y && ladder.z == z) {
              dynamic = " UL  RD"[ladder.dir];
              break;
            }
          }
        }
      }
      if (dynamic != ' ')                   putchar(dynamic);
      else if (_grid(x, y) == Empty)        putchar(' ');
      else if (_grid(x, y) == GroundGrill)  putchar('#');
      else if (_grid(x, y) == Wall1Grill)   putchar('$');
      else if (_grid(x, y) == Wall2Grill)   putchar('%');
      else if (_grid(x, y) == Ground)       putchar('_');
      else if (_grid(x, y) == Wall1)        putchar('1');
      else if (_grid(x, y) == Wall2)        putchar('2');
      else if (_grid(x, y) == Wall3)        putchar('3');
      else if (_grid(x, y) == Wall4)        putchar('4');
      else if (_grid(x, y) == Wall5)        putchar('5');
      else if (_grid(x, y) == Wall6)        putchar('6');
      else if (_grid(x, y) == Wall7)        putchar('7');
      else if (_grid(x, y) == Wall8)        putchar('8');
      else                                  putchar('?');
    }
    putchar('|');
    putchar('\n');
  }

  putchar('+');
  for (u8 x=0; x<_width; x++) putchar('-');
  putchar('+');
  putchar('\n');
}

bool LevelData::Won() const {
  if (winOverride) return winOverride(this);
//  return (_stephen.x == 7 && _stephen.y == 13 && _stephen.dir == Up
//      && ((_sausages[0].x1 == 7 && _sausages[0].y1 == 9 && _sausages[1].x1 == 7 && _sausages[1].y1 == 11)
//          || (_sausages[1].x1 == 7 && _sausages[1].y1 == 9 && _sausages[0].x1 == 7 && _sausages[0].y1 == 11)));
#if !OVERWORLD_HACK
  if (_stephen != _start) return false;
#endif
  for (Sausage sausage : _sausages) {
    if (!sausage.IsFullyCooked()) return false;
  }
  return true;
}

s8 LevelData::GetSausage(s8 x, s8 y, s8 z) const {
  if (z < 0) return -1;

#define o(i) if (_sausages[i].IsAt(x, y, z)) return (i);
  SAUSAGES;
#undef o

  return -1;
}

int LevelData::NumSausages() const {
    return _sausages.Size();
}

bool LevelData::IsWithinGrid(s8 x, s8 y, s8 z) const {
  return x >= 0 && x <= _width - 1 && y >= 0 && y <= _height - 1 && z >= 0;
}

bool LevelData::IsWall(s8 x, s8 y, s8 z) const {
  if (!IsWithinGrid(x, y, z)) return false;
  u8 cell = _grid(x, y);

  // Special walls (Wall6-8) encode height in the low 2 bits: 5 + (cell & 0b11).
  if (cell & Tile::Special) return z < 5 + (cell & 0b11);

  // Normal walls (Wall1-5): mask off the Grill/Special flag bits first so they don't alias the
  // height test at z==5/6. z==0 is blocked by Wall1, z==1 by Wall2, etc.
  return ((cell & 0b00111111) & (Ground << 1 << z)) != 0;
}

bool LevelData::CanWalkOnto(s8 x, s8 y, s8 z) const {
  if (!IsWithinGrid(x, y, z)) return false;
  u8 cell = _grid(x, y);
  // Here's one of the places where we take advantage of the overhang bitmask.
  // Note that this *only* checks for ground at our feet, not if we're walking into a wall.

  // Special walls (Wall6-8) are solid through their height and walkable on top.
  if (cell & Tile::Special) {
    if (z <= 5 + (cell & 0b11)) return true;
  }

  // Normal walls/overhangs: mask off Grill/Special flag bits so they don't alias the height test.
  // z==0 walks onto Ground, z==1 onto Wall1, etc.
  if (((cell & 0b00111111) & (Ground << z)) != 0) return true; // Stepping onto ground at our current level
  if (!_stephen.HasFork() && _stephen.forkX == x && _stephen.forkY == y && _stephen.forkZ == z) return true; // Stepping onto a fork
  if (GetSausage(x, y, z-1) != -1) return true; // Stepping onto a sausage
  return false;
}

bool LevelData::IsGrill(s8 x, s8 y, s8 z) const {
  if (!IsWithinGrid(x, y, z)) return false;
  u8 cell = _grid(x, y);
  if ((cell & Grill) == 0) return false;

  // Special walls (Wall6-8): grill applies through their height.
  if (cell & Tile::Special) return z < 5 + (cell & 0b11);

  // The Grill flag is one bit shared by the whole column, but a grill is a hazardous *surface*.
  // Resolve it to the lowest exposed standable level (solid with open space above), so an overhang
  // (Over2/Over3) grills its waterline floor, not the safe ledge on top (Cold Gate move 47).
  u8 mask = cell & 0b00111111;
  for (u8 surface = 0; surface < 6; surface++) {
    if ((mask & (Ground << surface)) && !(mask & (Ground << (surface + 1)))) {
      return z == surface; // The first exposed surface from the bottom is the grill.
    }
  }
  return false;
}

bool LevelData::IsLadder(s8 x, s8 y, s8 z, Direction dir) const {
  for (const Ladder& ladder : _ladders) {
    if (ladder.x == x && ladder.y == y && ladder.z == z && ladder.dir == dir) return true;
  }
  return false;
}
