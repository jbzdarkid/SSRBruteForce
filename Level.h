#pragma once
#include "Types.h"
#include "WitnessRNG/StdLib.h"

enum Direction : u8 {
  None = 0,
  Up = 1,
  Left = 2,
  Jump = 3,
  Crouch = 4,
  Right = 5,
  Down = 6,
};
Direction Inverse(Direction dir);

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
    forkX = x_; forkY = y_; forkZ = z_;
    forkDir = None;

    if (dir == Up)         forkY--;
    else if (dir == Down)  forkY++;
    else if (dir == Left)  forkX--;
    else if (dir == Right) forkX++;
  }

  // If the fork has no direction, then stephen is holding it (its direction matches stephen's).
  // If it has any other value, then it is elsewhere in the world.
  inline bool HasFork() const { return forkDir == None; }
  bool operator==(const Stephen& other) const {
    // We keep states where stephen has a fork in a normalized form so that we can do a direct u64 comparison.
#if _DEBUG
    if (HasFork()) {
      assert(forkZ == z);
      assert(forkDir == None);
    }
    if (other.HasFork()) {
      assert(other.forkZ == other.z);
      assert(other.forkDir == None);
    }
#endif
    static_assert(sizeof(Stephen) == 8);
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
    Rolled = 16,
    Cook1 = Cook1A | Cook1B, // If (x1, y1) is cooked on both sides
    Cook2 = Cook2A | Cook2B, // If (x2, y2) is cooked on both sides
    FullyCooked = Cook1A | Cook1B | Cook2A | Cook2B,
  };

  u8 flags; // Typeless because otherwise we have to define |=, &=, etc.
  u16 _ = 0; // Padding

  inline bool IsVertical() const { return x1 == x2; }
  inline bool IsHorizontal() const { return y1 == y2; }
  inline bool IsRolled() const { return (flags & Rolled) != 0; }
  inline bool IsAt(s8 x_, s8 y_, s8 z_) const {
    if (z_ != z) return false;
    if (x_ == x1 && y_ == y1) return true;
    if (x_ == x2 && y_ == y2) return true;
    return false;
  }
  bool operator==(const Sausage& other) const {
    static_assert(sizeof(Sausage) == 8);
    u64 a = *(u64*)this;
    u64 b = *(u64*)&other;
    return a == b; // Assuming the padding bytes are always 0?
  }
  bool operator!=(const Sausage& other) const { return !(*this == other); }
};

#define SAUSAGES o(0) o(1) // o(2) // o(3) // o(4)
#define STAY_NEAR_THE_SAUSAGES 0
#define HASH_CACHING 1
#define SORT_SAUSAGE_STATE 0

struct State {
  Stephen stephen;

  // TODO: Try changing this to just be a Vector<Sausage>? It would save me a lot of headache, I think. But it might cost a lot of memory.
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

#if HASH_CACHING
  size_t hash = 0;
#endif

  bool operator==(const State& other) const;
  size_t Hash() const;
};

namespace std {
template<> struct hash<State> {
  size_t operator()(const State& state) const { 
#if HASH_CACHING
  return state.hash;
#else 
  return state.Hash();
#endif
  }
};
}

struct Level {
  enum Tile : u8 {
    Empty  = 0,
    Ground = 0b00000001,
    Wall1  = 0b00000011,
    Wall2  = 0b00000111,
    Over2  = 0b00000101,
    Wall3  = 0b00001111,
    Over3  = 0b00001001,
    Wall4  = 0b00011111,
    Wall5  = 0b00111111,
    Grill  = 0b01000000,
    Unused = 0b10000000,
  };

  Level(u8 width, u8 height, const char* name, const char* asciiGrid,
    const Stephen& stephen = {},
    std::initializer_list<Ladder> ladders = {},
    std::initializer_list<Sausage> sausages = {},
    std::initializer_list<Tile> tiles = {});
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
  bool HandleRotation(Direction dir);
  bool HandleParallelMotion(Direction dir);

  // CanPhysicallyMove is for when you want to check if motion is possible,
  // and if it isn't, stephen will enact a different kind of motion.
  // It has no side-effects.
  bool CanPhysicallyMove(s8 x, s8 y, s8 z, Direction dir, bool stephenIsRotating=false);
  bool CanPhysicallyMoveInternal(s8 x, s8 y, s8 z, Direction dir);
  // TODO: A comment
  void CheckForSausageCarry(Direction dir, s8 z, bool stephenIsRotating);
  // MoveThroughSpace is for when stephen is supposed to make a certain motion,
  // and if the motion fails, the move should not have been taken.
  // It may have side effects.
  bool MoveThroughSpace(s8 x, s8 y, s8 z, Direction dir, s8 stephenRotationDir=0, bool doDoubleMove=true);
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

  NArray<Tile> _grid;
  NArray<s8> _sausageGrid;
  s8 _width = 0;
  s8 _height = 0;
  s8 _sausageSpeared = -1;
  Stephen _start;
  Stephen _stephen;
  Vector<Sausage> _sausages;
  Vector<Ladder> _ladders;
  bool _interactive = false;
};
