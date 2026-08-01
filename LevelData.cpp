#include "LevelData.h"
#include <cstdio>
#include <vector>

LevelData::LevelData(u8 width, u8 height, const char* name, const char* asciiGrid,
  const Stephen& stephen,
  std::vector<Ladder> extraLadders,
  std::vector<Sausage> sausages,
  std::vector<SpecialTile> specialTiles)
  : _width(width),
    _height(height),
    _walls(NArray<u16>(_width, _height)),
    _grills(NArray<u16>(_width, _height)),
    _ladders(NArray<u16>(_width, _height, Direction::NUM_ENTRIES)),
    name(name)
{
  _walls.Fill(0);
  _grills.Fill(0);
  _ladders.Fill(0);

  std::vector<Ladder> ladders;

  assert(width * height == strlen(asciiGrid));
  for (s32 i=0; i<width * height; i++) {
    s8 x = i % width;
    s8 y = i / width;
    char c = asciiGrid[i];
    if      (c == ' ') { _walls(x, y) = 0b0000'0000; }
    else if (c == '#') { _walls(x, y) = 0b0000'0001; _grills(x, y) = 0b0000'0001; }
    else if (c == '$') { _walls(x, y) = 0b0000'0011; _grills(x, y) = 0b0000'0010; }
    else if (c == '%') { _walls(x, y) = 0b0000'0111; _grills(x, y) = 0b0000'0100; }
    else if (c == '_') { _walls(x, y) = 0b0000'0001; }
    else if (c == '1') { _walls(x, y) = 0b0000'0011; }
    else if (c == '2') { _walls(x, y) = 0b0000'0111; }
    else if (c == '3') { _walls(x, y) = 0b0000'1111; }
    else if (c == '4') { _walls(x, y) = 0b0001'1111; }
    else if (c == '5') { _walls(x, y) = 0b0011'1111; }
    else if (c == '6') { _walls(x, y) = 0b0111'1111; }
    else if (c == '7') { _walls(x, y) = 0b1111'1111; }
    else if (c == '8') { _walls(x, y) = 0b1'1111'1111; }
    else if (c == '?') {
        SpecialTile tile = specialTiles.back();
        _walls(x, y) = tile.walls;
        _grills(x, y) = tile.grills;
        specialTiles.pop_back();
    }
#if (OVERWORLD_HACK == 0 || OVERWORLD_HACK >= 2) // need to use these capital letters for sausages I mean not really but whatever
    else if (c == 'U') { _walls(x, y) = 0b0000'0001; ladders.push_back(Ladder{x, y, 0, Up}); }
    else if (c == 'D') { _walls(x, y) = 0b0000'0001; ladders.push_back(Ladder{x, y, 0, Down}); }
    else if (c == 'L') { _walls(x, y) = 0b0000'0001; ladders.push_back(Ladder{x, y, 0, Left}); }
    else if (c == 'R') { _walls(x, y) = 0b0000'0001; ladders.push_back(Ladder{x, y, 0, Right}); }
#endif
    else if (c == '^') { _walls(x, y) = 0b0000'0001; _stephen = Stephen(x, y, 0, Up); }
    else if (c == 'v') { _walls(x, y) = 0b0000'0001; _stephen = Stephen(x, y, 0, Down); }
    else if (c == '<') { _walls(x, y) = 0b0000'0001; _stephen = Stephen(x, y, 0, Left); }
    else if (c == '>') { _walls(x, y) = 0b0000'0001; _stephen = Stephen(x, y, 0, Right); }
    else if (c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z') {
      int num;
#if OVERWORLD_HACK
      if (c >= 'A' && c <= 'Z') {
        _walls(x, y) = 0b0000'0001;
        num = c - 'A';
      } else {
        _walls(x, y) = 0b0000'0001;
        num = c - 'a' + 26;
      }
#else
      if (c >= 'A' && c <= 'Z') {
        _walls(x, y) = 0b0000'0000;
        num = c - 'A';
      } else {
        _walls(x, y) = 0b0000'0001;
        num = c - 'a';
      }
#endif

      // We need to do this here because, even if we don't *solve* levels
      // with less than the expected count, we still construct them.
      assert(num <= _sausages.Size()); // There shouldn't be any gaps in level data or sausages out of order.
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

  // Ladders from the grid, as 2D, may need height extensions.
  for (const Ladder& ladder : ladders) {
    assert(_ladders(ladder.x, ladder.y, ladder.dir) == 0);

    for (s8 z = ladder.z; z < 17; z++) {
      assert(z < 16); // Maximum bitmask size
      _ladders(ladder.x, ladder.y, ladder.dir) |= (1 << z);

      if (ladder.dir == Up) {
        if (!IsWall(ladder.x, ladder.y - 1, z + 1)) break;
      } else if (ladder.dir == Down) {
        if (!IsWall(ladder.x, ladder.y + 1, z + 1)) break;
      } else if (ladder.dir == Left) {
        if (!IsWall(ladder.x - 1, ladder.y, z + 1)) break;
      } else if (ladder.dir == Right) {
        if (!IsWall(ladder.x + 1, ladder.y, z + 1)) break;
      }
    }
  }

  // Ladders from the initializer list do not get the same treatment.
  for (const Ladder& ladder : extraLadders) {
    _ladders(ladder.x, ladder.y, ladder.dir) |= (1 << ladder.z);
  }

  assert(specialTiles.empty()); // Assert that all excess tiles were consumed
}

void LevelData::Print() const {
  putchar('+');
  for (u8 x=0; x<_width; x++) putchar('-');
  putchar('+');
  putchar('\n');

  for (u8 y=0; y<_height; y++) {
    putchar('|');
    for (u8 x=0; x<_width; x++) {
      for (s8 z = 8; z >= 0; z--) {
        if (x == _stephen.x && y == _stephen.y && z == _stephen.z) {
          putchar(" ^<  >v"[_stephen.dir]);
        } else if (!_stephen.HasFork() && x == _stephen.forkX && y == _stephen.forkY && z == _stephen.forkZ) {
          putchar('+');
        } else if (GetSausage(x, y, z) != -1) {
          s8 sausageNo = GetSausage(x, y, z);
#if OVERWORLD_HACK
          if (sausageNo < 26) putchar('A' + sausageNo);
          if (sausageNo >= 26) putchar('a' + sausageNo - 26);
#else
          if (_walls(x, y) == 0) putchar('A' + sausageNo);
          if (_walls(x, y) != 0) putchar('a' + sausageNo);
#endif
        }
        else if (IsLadder(x, y, z, Up))     putchar('U');
        else if (IsLadder(x, y, z, Left))   putchar('L');
        else if (IsLadder(x, y, z, Right))  putchar('R');
        else if (IsLadder(x, y, z, Down))   putchar('D');
        else if (IsGrill(x, y, z))          putchar(" #$ %"[_grills(x, y)]);
        else if (IsWall(x, y, z))           putchar("_12345678"[z]);
        else {
          if (z > 0) continue; // Not yet handled, keep looking for something at a lower Z
          if (z == 0) putchar(' '); // Bottom of the world, put an empty cell
        }
        break; // Handled because we didn't enter the 'else' above
      }
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

bool LevelData::IsWithinGrid(s8 x, s8 y, s8 z) const {
  // return x >= 0 && x <= _width - 1 && y >= 0 && y <= _height - 1 && z >= 0;
  // This is an optimized version of the above; it uses signed->unsigned cast to avoid two branches
  return (u8)x < _width && (u8)y < _height && z >= 0;
}

bool LevelData::CanWalkOnto(s8 x, s8 y, s8 z) const {
  if (!IsWithinGrid(x, y, z)) return false;
  if (_walls(x, y) & (1 << z)) return true; // Stepping onto standable terrain at our current level
  // Stephen can also stand on a dropped fork or a sausage.
  if (!_stephen.HasFork() && _stephen.forkX == x && _stephen.forkY == y && _stephen.forkZ == z-1) return true;
  if (GetSausage(x, y, z-1) != -1) return true;
  return false;
}

bool LevelData::IsWall(s8 x, s8 y, s8 z) const {
  if (!IsWithinGrid(x, y, z)) return false;
  // A wall blocks level z iff the column is solid at z+1 (the block occupies z, you stand on top at z+1).
  return _walls(x, y) & (2 << z);
}

bool LevelData::IsGrill(s8 x, s8 y, s8 z) const {
  if (!IsWithinGrid(x, y, z)) return false;
  return _grills(x, y) & (1 << z); // There must be a grill at this Z index
}

bool LevelData::IsLadder(s8 x, s8 y, s8 z, Direction dir) const {
  if (!IsWithinGrid(x, y, z)) return false;
  u16 ladder = _ladders(x, y, dir);
  return ladder & (1 << z); // There must be a ladder at this Z index
}
