#pragma once
#include <ostream>
#include "WitnessRNG/StdLib.h"

#define OVERWORLD_HACK 0
#ifndef SAUSAGES // Overwritten by scripts. Defaults to 3 for testing.
    #define SAUSAGES o(0) o(1) o(2)
#endif

constexpr int NUM_SAUSAGES =
#define o(x) +1
SAUSAGES
#undef o
;

enum Direction : u8 {
  None = 0,
  Up = 1,
  Left = 2,
  Jump = 3,
  Crouch = 4,
  Right = 5,
  Down = 6,

  NUM_ENTRIES,
};

extern const char* DIR_NAMES[]; // defined in Main.cpp; indexed by Direction

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
    static_assert(sizeof(Stephen) == sizeof(u64));
    u64 a = *(u64*)this;
    u64 b = *(u64*)&other;
    return a == b;
  }
  bool operator!=(const Stephen& other) const { return !(*this == other); }

  friend std::ostream& operator<<(std::ostream& o, const Stephen& s) {
    return o << (int)s.x << ' ' << (int)s.y << ' ' << (int)s.z << ' ' << DIR_NAMES[s.dir] << ' '
             << (int)s.forkX << ' ' << (int)s.forkY << ' ' << (int)s.forkZ << ' ' << DIR_NAMES[s.forkDir];
  }
};

struct Ladder {
  s8 x = -1;
  s8 y = -1;
  s8 z = -1;
  Direction dir;
};

enum SpecialTile : u8 {
  Over2,
  Over3,
  Over2Grill,
};

struct Sausage {
  // x1, y1 will always refer to the the left- or upper- half of the sausage.
  // This does cause some extra work while rotating, but makes comparison and referencing much easier.
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
    FullyCooked = Cook1A | Cook1B | Cook2A | Cook2B, // If all 4 sides are cooked
  };

  u8 flags; // Typeless because otherwise we have to define |=, &=, etc.
  u16 _ = 0; // Unused, padding

  inline bool IsVertical() const { return x1 == x2; }
  inline bool IsHorizontal() const { return y1 == y2; }
  inline bool IsRolled() const { return (flags & Rolled) != 0; }
  inline bool IsAt(s8 x_, s8 y_, s8 z_) const {
    if (z_ != z) return false;
    if (x_ == x1 && y_ == y1) return true;
    if (x_ == x2 && y_ == y2) return true;
    return false;
  }
  inline bool IsFullyCooked() const { return (flags & FullyCooked) == FullyCooked; }
  // The compiler optimizes the std::pair reasonably well here.
  inline std::pair<s8, s8> OtherEnd(s8 x, s8 y) const {
    if (x1 == x && y1 == y) return {x2, y2};
    return {x1, y1};
  }
  // Used during a hat rotation along with swapping the ends.
  inline void SwapCookBits() {
    u8 cook1 = flags & Cook1;
    u8 cook2 = flags & Cook2;
    flags = (flags & ~(u8)FullyCooked) | (u8)(cook1 << 2) | (u8)(cook2 >> 2);
  }
  bool operator==(const Sausage& other) const {
    static_assert(sizeof(Sausage) == 8);
    u64 a = *(u64*)this;
    u64 b = *(u64*)&other;
    return a == b; // Assuming the padding bytes are always 0
  }
  bool operator!=(const Sausage& other) const { return !(*this == other); }

  friend std::ostream& operator<<(std::ostream& o, const Sausage& s) {
    o << (int)s.x1 << ' ' << (int)s.y1 << ' ' << (int)s.x2 << ' ' << (int)s.y2 << ' ' << (int)s.z;
    if (s.flags & Sausage::Flags::Cook1A) o << " Cook1A";
    if (s.flags & Sausage::Flags::Cook1B) o << " Cook1B";
    if (s.flags & Sausage::Flags::Cook2A) o << " Cook2A";
    if (s.flags & Sausage::Flags::Cook2B) o << " Cook2B";
    if (s.flags & Sausage::Flags::Rolled) o << " Rolled";
    if (s.flags == 0) o << " NoFlags";
    return o;
  }
};

class LevelData {
public:
  friend class TestSymmetryHelper;

  LevelData(u8 width, u8 height, const char* name, const char* asciiGrid,
    const Stephen& stephen = {},
    std::vector<Ladder> ladders = {},
    std::vector<Sausage> sausages = {},
    std::vector<SpecialTile> specialTiles = {});
  void Print() const;
  bool Won() const;

  // Frequently used for level heuristics.
  const Vector<Sausage>& GetSausages() const { return _sausages; }
  const Stephen& GetStephen() const { return _stephen; }

  // Used by Solver2 to compute traversal costs
  bool IsGrill(s8 x, s8 y, s8 z) const;

  const char* name;

protected: // Used in the Level engine
  inline std::pair<s8, s8> Delta(Direction dir) const {
    if (dir == Up)    return { 0, -1 };
    if (dir == Down)  return { 0, +1 };
    if (dir == Left)  return { -1, 0 };
    if (dir == Right) return { +1, 0 };
    return { 0, 0 };
  }
  inline Direction Inverse(Direction dir) const { return (Direction)(7 - dir); }

  s8 GetSausage(s8 x, s8 y, s8 z) const;
  bool IsWithinGrid(s8 x, s8 y, s8 z) const;
  bool CanWalkOnto(s8 x, s8 y, s8 z) const;
  bool IsWall(s8 x, s8 y, s8 z) const;
  bool IsLadder(s8 x, s8 y, s8 z, Direction dir) const;

  Stephen _stephen;
  Vector<Sausage> _sausages;

private:
  u8 _width;
  u8 _height;
  NArray<u16> _walls;
  NArray<u16> _grills;
  NArray<u8> _ladders;
  Stephen _start;
};