#pragma once
#include "Types.h"
#include "WitnessRNG/StdLib.h"

enum Direction : u8 {
  None = 0,
  Up = 1,
  Down = 2,
  Left = 3,
  Right = 4,
  Crouch = 5,
  Jump = 6,
};

struct Stephen {
  s8 x;
  s8 y;
  s8 z;
  Direction dir;

  s8 forkX;
  s8 forkY;
  s8 forkZ;
  Direction forkDir;

  Stephen() : Stephen(-1, -1, 0, None) {}
  Stephen(s8 x_, s8 y_, s8 z_, Direction dir_) {
    x = x_; y = y_; z = z_;
    dir = dir_;
    forkX = x_; forkY = y_; forkZ = -1;
    forkDir = None;
  }

  // If forkZ is negative, then stephen has his fork -- if it is positive, the fork is disconnected.
  inline bool HasFork() const { return forkZ < 0; }
  bool operator==(const Stephen& other) const {
    // We keep states where stephen has a fork in a normalized form so that we can do a direct u64 comparison.
#if _DEBUG
    if (HasFork()) {
      assert(forkZ == -1);
      assert(forkDir == None);
    }
    if (other.HasFork()) {
      assert(other.forkZ == -1);
      assert(other.forkDir == None);
    }
#endif
    u64 a = *(u64*)this;
    u64 b = *(u64*)&other;
    return a == b;
  }
  bool operator!=(const Stephen& other) const { return !(*this == other); }
};

struct Ladder {
  s8 x = -1;
  s8 y = -1;
  s8 z = -1;
  Direction dir;
};

struct Sausage {
  // I expect x1, y1 to be the left- or upper- half of the sausage. This means future work if we rotate sausages.
  s8 x1;
  s8 y1;
  s8 x2;
  s8 y2;
  s8 z = 0;

  enum Flags : u8 {
    None = 0,
    Cook1A = 1, // If (x1, y1) is cooked when not rolled over
    Cook1B = 2, // If (x1, y1) is cooked when rolled over
    Cook2A = 4, // If (x2, y2) is cooked when not rolled over
    Cook2B = 8, // If (x2, y2) is cooked when rolled over
    Horizontal = 16,
    Rolled = 32,
    Cook1 = Cook1A | Cook1B, // If (x1, y1) is cooked on both sides
    Cook2 = Cook2A | Cook2B, // If (x2, y2) is cooked on both sides
    FullyCooked = Cook1A | Cook1B | Cook2A | Cook2B,
  };

  u8 flags; // Typeless because otherwise we have to define |=, &=, etc.
  u8 _[2] = {0}; // Padding

  inline bool IsHorizontal() const { return (flags & Horizontal) != 0; }
  inline bool IsVertical() const { return !IsHorizontal(); }
  inline bool IsRolled() const { return (flags & Rolled) != 0; }
  bool operator==(const Sausage& other) const {
    return flags == other.flags && x1 == other.x1 && y1 == other.y1 && x2 == other.x2 && y2 == other.y2 && z == other.z;
  }
  bool operator!=(const Sausage& other) const { return !(*this == other); }
};

#define SAUSAGES o(0) // o(1) o(2)
#define STAY_NEAR_THE_SAUSAGES 0

struct State {
  Stephen stephen;

  // TODO: Try changing this to just be a Vector<Sausage>? It would save me a lot of headache, I think.
#define o(x) +1
  Sausage sausages[SAUSAGES];
#undef o

  // Used to build the tree, ergo not part of the hashing or comparison algos
  State* next = nullptr;
  State* u = nullptr;
  State* d = nullptr;
  State* l = nullptr;
  State* r = nullptr;
#define UNWINNABLE 0xFFFE
  u16 winDistance = UNWINNABLE;

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
    Ground = 0b1,
    Wall1  = 0b11,
    Wall2  = 0b111,
    Wall3  = 0b1111,
    Grill = 16,
  };

  Level(u8 width, u8 height, const char* name, const char* asciiGrid, const Stephen& stephen, std::initializer_list<Sausage> sausages = {}, std::initializer_list<Ladder> ladders = {});
  Level(u8 width, u8 height, const char* name, const char* asciiGrid);
  ~Level();
  void Print() const;
  bool InteractiveSolver();
  bool Won() const;

  State GetState() const;
  void SetState(const State* state);
  bool WouldStephenStepOnGrill(const Stephen& stephen, Direction dir); // Used by Solver to compute movement durations.

  // Returns true if the move succeeded
  // Returns false if the move was illegal for any reason
  bool Move(Direction dir);

  const char* name;
private:
  // Move() helpers
  bool HandleLogRolling(const Sausage& sausage, Direction dir, bool& handled);
  bool HandleSpearedMotion(Direction dir);
  bool HandleLadderMotion(Direction dir, bool& handled);
  bool HandleBurnedStep(Direction dir);
  bool HandleForklessMotion(Direction dir);
  bool HandleDefaultMotion(Direction dir);

  Vector<s8> _movedSausages;
  // CanPhysicallyMove is for when you want to check if motion is possible,
  // and if it isn't, stephen will enact a different kind of motion.
  // It has no side-effects.
  bool CanPhysicallyMove(s8 x, s8 y, s8 z, Direction dir, Vector<s8>* movedSausages=nullptr, s8* sausageWithFork=nullptr);
  // MoveThroughSpace is for when stephen is supposed to make a certain motion,
  // and if the motion fails, the move should not have been taken.
  // It may have side effects.
  bool MoveThroughSpace(s8 x, s8 y, s8 z, Direction dir, bool spear=false);
  // Similarly to MoveThroughSpace, MoveStephenThroughSpace is for actually moving stephen,
  // and we are just trying to figure out if that motion results in an invalid state,
  // such as losing or burning a sausage. The equivalent check function is CanWalkOnto.
  // It will have side effects even if the move is invalid.
  bool MoveStephenThroughSpace(Direction dir);

  // Also move helpers, but smaller (and const)
  s8 GetSausage(s8 x, s8 y, s8 z) const;
  bool IsWithinGrid(s8 x, s8 y, s8 z) const;
  bool IsWall(s8 x, s8 y, s8 z) const;
  bool CanWalkOnto(s8 x, s8 y, s8 z) const;
  bool IsGrill(s8 x, s8 y, s8 z) const;
  bool IsLadder(s8 x, s8 y, s8 z, Direction dir) const;

  Tile** _grid;
  s8 _width = 0;
  s8 _height = 0;
  s8 _sausageSpeared = -1;
  Stephen _start;
  Stephen _stephen;
  Vector<Sausage> _sausages;
  Vector<Ladder> _ladders;
  bool _interactive = false;
};
