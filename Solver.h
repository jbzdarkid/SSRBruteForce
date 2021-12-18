#pragma once
#include "Types.h"
#include "Level.h"

struct Solver {
  Solver(Level* level);

  Vector<Direction> Solve(u32 moveLimit);

private:
  u8 SolveInternal(u8 depth);

  Level* _level;
  u32 _moveLimit = 0;
};

