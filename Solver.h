#pragma once
#include "Types.h"
#include "Level.h"

struct Solver {
  Solver(Level* level);

  Vector<Direction> Solve(u32 moveLimit);

private:
  State* GetOrInsertState();

  Level* _level;
  u32 _moveLimit = 0;
};

