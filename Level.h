#pragma once
#include "Types.h"
#include "WitnessRNG/StdLib.h"

enum Direction : u8 {
  None = 0,
  Up = 1,
  Down = 2,
  Left = 4,
  Right = 8,
  Any = Up | Down | Left | Right,
};

struct Stephen {
  s8 x;
  s8 y;
  enum Flags {
    // Speared = 0x10,
  };
  union {
    Direction dir;
    Flags flags;
  };

  s8 sausageSpeared = -1;
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

  u8 flags; // TODO: Type?

  inline bool IsHorizontal() const { return (flags & Horizontal) != 0; }
  inline bool IsVertical() const { return !IsHorizontal(); }
  inline bool IsRolled() const { return (flags & Rolled) != 0; }
  bool operator==(const Sausage& other) const {
    return flags == other.flags && x1 == other.x1 && y1 == other.y1 && x2 == other.x2 && y2 == other.y2;
  }
};

#define SAUSAGES o(0) o(1) // o(2)

struct State {
  Stephen stephen;
#define o(x) Sausage s##x = {};
  SAUSAGES
#undef o

  // Used to build the tree, ergo not part of the hashing or comparison algos
  State* next = nullptr;
  State* u = nullptr;
  State* d = nullptr;
  State* l = nullptr;
  State* r = nullptr;
  u16 winDistance = 0xFFFF;
  u16 depth = 0;

  bool operator==(const State& other) const;
  u32 Hash() const;
};

namespace std {
template<> struct hash<State> {
  size_t operator()(const State& state) const { return state.Hash(); }
};
}

struct Level {
  enum Tile : u8 {
    Empty = 0,
    Ground = 1,
    Grill = 2,
    Wall = 3,
  };

  Level(u8 width, u8 height, const char* name, const char* asciiGrid);
  ~Level();
  void Print() const;
  bool InteractiveSolver();
  bool Won() const;

  State GetState() const;
  void SetState(const State* state);

  // Returns true if the move succeeded
  // Returns false if the move was illegal for any reason
  bool Move(Direction dir);

  const char* name;
  bool _explain = false;

private:
  Vector<s8> _movedSausages;
  bool CanPhysicallyMove(s8 x, s8 y, Direction dir, Vector<s8>* movedSausages = nullptr);
  // Returns true if the move was possible (the object was moved as a result)
  // Returns false if the move was impossible / blocked (may have side effects)
  bool MoveThroughSpace(s8 x, s8 y, Direction dir, bool spear=false);

  s8 GetSausage(s8 x, s8 y) const;
  bool IsWithinGrid(s8 x, s8 y) const;
  bool IsWall(s8 x, s8 y) const;
  bool CanTurnThrough(s8 x, s8 y) const;
  bool CanWalkOnto(s8 x, s8 y) const;
  bool IsGrill(s8 x, s8 y) const;

  Tile** _grid;
  s8 _width = 0;
  s8 _height = 0;
  Stephen _start = {};
  Stephen _stephen = {};
  Vector<Sausage> _sausages;
};
