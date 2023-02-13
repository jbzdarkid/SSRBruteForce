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
    else if (c == '#') _grid(x, y) = (Tile)(Ground | Grill);
    else if (c == '$') _grid(x, y) = (Tile)(Wall1 | Grill);
    else if (c == '%') _grid(x, y) = (Tile)(Wall2 | Grill);
    else if (c == '_') _grid(x, y) = Ground;
    else if (c == '1') _grid(x, y) = Wall1;
    else if (c == '2') _grid(x, y) = Wall2;
    else if (c == '3') _grid(x, y) = Wall3;
    else if (c == '4') _grid(x, y) = Wall4;
    else if (c == '5') _grid(x, y) = Wall5;
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

  assert(extraTiles.Size() == 0); // Assert that all excess tiles were consumed
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
      if (dynamic !=  ' ')            putchar(dynamic);
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

bool LevelData::Won() const {
  if (_stephen.x == 1 && _stephen.y == 1) return true; // also hack
#if !OVERWORLD_HACK
  if (_stephen != _start) return false;
#endif
  for (Sausage sausage : _sausages) {
    if ((sausage.flags & Sausage::Flags::FullyCooked) != Sausage::Flags::FullyCooked) return false;
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

bool LevelData::IsWithinGrid(s8 x, s8 y, s8 z) const {
  return x >= 0 && x <= _width - 1 && y >= 0 && y <= _height - 1 && z >= 0;
}

bool LevelData::IsWall(s8 x, s8 y, s8 z) const {
  if (!IsWithinGrid(x, y, z)) return false;
  u8 cell = _grid(x, y);
  // if _stephen.z == 0 then you are blocked by Wall1. if _stephen.z == 1 then you are blocked by Wall2.
  return (cell & (Ground << 1 << z)) != 0;
}

bool LevelData::CanWalkOnto(s8 x, s8 y, s8 z) const {
  if (!IsWithinGrid(x, y, z)) return false;
  u8 cell = _grid(x, y);
  // Here's one of the places where we take advantage of the overhang bitmask.
  // Note that this *only* checks for ground at our feet, not if we're walking into a wall.
  // if _stephen.z == 0 then you can walk onto Ground. if _stephen.z == 1 then you can walk onto Wall1
  if ((cell & (Ground << z)) != 0) return true; // Stepping onto ground at our current level
  if (!_stephen.HasFork() && _stephen.forkX == x && _stephen.forkY == y && _stephen.forkZ == z) return true; // Stepping onto a fork
  if (GetSausage(x, y, z-1) != -1) return true; // Stepping onto a sausage
  return false;
}

bool LevelData::IsGrill(s8 x, s8 y, s8 z) const {
  if (!IsWithinGrid(x, y, z)) return false;
  u8 cell = _grid(x, y);
  if ((cell & Grill) == 0) return false;
  return (cell & (Ground << z)) != 0; // There is ground at this level
}

bool LevelData::IsLadder(s8 x, s8 y, s8 z, Direction dir) const {
  for (const Ladder& ladder : _ladders) {
    if (ladder.x == x && ladder.y == y && ladder.z == z && ladder.dir == dir) return true;
  }
  return false;
}