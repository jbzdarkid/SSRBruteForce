#pragma once
#include "Types.h"
#include "WitnessRNG/StdLib.h"

enum Direction : u8 {
  Up = 1,
  Down = 2,
  Left = 4,
  Right = 8,
  Any = Up | Down | Left | Right,
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
    Cook1A = 1, // If (x1, y1) is cooked when not rolled over
    Cook1B = 2, // If (x1, y1) is cooked when rolled over
    Cook2A = 4, // If (x2, y2) is cooked when not rolled over
    Cook2B = 8, // If (x2, y2) is cooked when rolled over
    Horizontal = 16,
    Rolled = 32,
    FullyCooked = Cook1A | Cook1B | Cook2A | Cook2B,
  };

  u8 flags;

  inline bool IsHorizontal() { return (flags & Horizontal) != 0; }
  inline bool IsVertical() { return !IsHorizontal(); }
  inline bool IsRolled() { return (flags & Rolled) != 0; }
};

struct State {
  Stephen stephen;
  Sausage s0 = {};
  Sausage s1 = {};

  // Used to build the tree
  State* next = nullptr;
  State* u = nullptr;
  State* d = nullptr;
  State* l = nullptr;
  State* r = nullptr;
  u16 winDistance = 0xFFFF;

  bool operator==(const State& other) const;
};

u32 triple32_hash(u32 a, u32 b, u32 c, u32 d);

namespace std {
template<>
struct hash<State> {
  size_t operator()(const State& state) const {
    u32 a = *(u32*)&state.stephen;
    u32 b = *(u32*)&state.s0;
    u32 c = *(u32*)&state.s1;
    u32 d = state.s0.flags << 4 | state.s1.flags;
    return triple32_hash(a, b, c, d);
  }
};
}

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

  State GetState() const;
  void SetState(const State* state);

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
  Stephen _start = {};
  Vector<Sausage> sausages;
  Stephen stephen = {};
};
