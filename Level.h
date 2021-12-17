#pragma once
#include "Types.h"
#include "WitnessRNG/StdLib.h"

enum Direction : u8 {
  Up = 0,
  Right = 1,
  Down = 2,
  Left = 3,
};

struct Sausage {
  s8 x;
  s8 y;
  Direction dir;
  
  enum Cooked : u8 {
    None = 0,
    UpBottom = 1,
    UpTop = 2,
    DownBottom = 4,
    DownTop = 8,
  } cooked;
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
  void Print() const;

private:
  bool MoveForkThroughSpace(s8 x, s8 y, Direction dir);

  s8 GetSausage(s8 x, s8 y) const;
  bool IsWithinGrid(s8 x, s8 y) const;
  bool CanTurnThrough(s8 x, s8 y) const;
  bool CanWalkOnto(s8 x, s8 y) const;

  Tile** _grid;
  s8 _width = 0;
  s8 _height = 0;
  Sausage _stephen = {};
  Vector<Sausage> _sausages;
};
