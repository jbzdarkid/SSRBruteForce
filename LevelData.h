#pragma once
#include <initializer_list>
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

class LevelData {
public:
  LevelData(u8 width, u8 height, const char* name, const char* asciiGrid,
    const Stephen& stephen = {},
    std::initializer_list<Ladder> ladders = {},
    std::initializer_list<Sausage> sausages = {},
    std::initializer_list<Tile> tiles = {});
  void Print() const;
  bool Won() const;

  s8 GetSausage(s8 x, s8 y, s8 z) const;
  bool IsWithinGrid(s8 x, s8 y, s8 z) const;
  bool IsWall(s8 x, s8 y, s8 z) const;
  bool CanWalkOnto(s8 x, s8 y, s8 z) const;
  bool IsGrill(s8 x, s8 y, s8 z) const;
  bool IsLadder(s8 x, s8 y, s8 z, Direction dir) const;

  const char* name;

protected:
  Stephen _stephen;
#define SAUSAGES o(0) o(1) // o(2) // o(3) // o(4)
  Vector<Sausage> _sausages;

private:
  u8 _width;
  u8 _height;
  NArray<Tile> _grid;
  Vector<Ladder> _ladders;
  Stephen _start;
};