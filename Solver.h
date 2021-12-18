#pragma once
#include "Types.h"
#include "Level.h"

struct Solver {
  Solver(Level* level);

  Vector<Direction> Solve(u32 moveLimit);

private:
  u16 SolveInternal(u16 depth);

  Level* _level;
  u32 _moveLimit = 0;
};

