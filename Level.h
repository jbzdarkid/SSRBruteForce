#pragma once
#include "Types.h"
#include "WitnessRNG/StdLib.h"

enum Direction : u8 {
  Up = 0,
  Right = 1,
  Down = 2,
  Left = 3,
};

struct Stephen {
  s8 x;
  s8 y;
  Direction dir;
};

struct Sausage {
  // I expect x1, y1 to be the left- or upper- half of the sausage. This means future work if we rotate sausages.
  s8 x1;
  s8 y1;
  s8 x2;
  s8 y2;

  enum Flags : u8 {
    Horizontal = 1,
    Rolled = 2,
    Cook1A = 4, // If (x1, y1) is cooked when not rolled over
    Cook1B = 8, // If (x1, y1) is cooked when rolled over
    Cook2A = 16, // If (x2, y2) is cooked when not rolled over
    Cook2B = 32, // If (x2, y2) is cooked when rolled over
    FullyCooked = Cook1A | Cook1B | Cook2A | Cook2B,
  };

  u8 flags;

  inline bool IsHorizontal() { return (flags & Horizontal) != 0; }
  inline bool IsRolled() { return (flags & Rolled) != 0; }
};

struct Level {
  enum Tile : u8 {
    Empty = 0,
    Ground = 1,
    Grill = 2,
  };

  Level(u8 width, u8 height, const char* asciiGrid);
  ~Level();
  // Returns false if the move fails
  bool Move(Direction dir);
  bool Won() const;
  void Print() const;

  // Public for now, because some levels have complex starting positions, with sausages hanging over air
  Vector<Sausage> sausages;

private:
  bool MoveThroughSpace(s8 x, s8 y, Direction dir);

  s8 GetSausage(s8 x, s8 y) const;
  bool IsWithinGrid(s8 x, s8 y) const;
  bool CanTurnThrough(s8 x, s8 y) const;
  bool CanWalkOnto(s8 x, s8 y) const;
  bool IsGrill(s8 x, s8 y) const;

  Tile** _grid;
  s8 _width = 0;
  s8 _height = 0;
  Stephen _stephen = {};
  Stephen _start = {};
};
