#include "Solver.h"
#include "Level.h"
#include <unordered_set>
#include <unordered_map>

Solver::Solver(Level* level) : _level(level) { }

std::vector<Direction> Solver::Solve() {
  printf("Solving %s\n", _level->name);

  State initialState = _level->GetState();

  _bestDepth = 0xFFFF;
  _visited.clear();
  _visited.reserve(0x7FFFFF);
  _winning.clear();

  // First, brute-force the problem space to find our best solution.
  bool victory = SolveRecursive(0);
  if (!victory) return {}; // Puzzle was somehow unsolvable.

  _visited = {}; // Regain (some) memory

  // DFS again, but now with the intent to find the fastest path.
  // We can afford to explore the entire state graph now,
  // since we know which states are winning or not.
  _level->SetState(&initialState);
  FindBestSolutionRecursive(0, 0);

  return std::move(_bestSolution);
}

bool Solver::SolveRecursive(u16 depth) {
  if (depth > _bestDepth) return false; // Slower than the best solution = not really a solution we care about.

  // If the node was already reached, return our cached result
  State state = _level->GetState();
  auto existing = _visited.insert(state);
  if (!existing.second) return _winning.contains(state);

  if (_level->Won()) {
    _winning.emplace(state);
    _bestDepth = depth; // if depth is greater than this we already exited.
    return true;
  }

  bool anyVictory = false;
  if (_level->Move(Up))     anyVictory |= SolveRecursive(depth + 1);
  _level->SetState(&state);
  if (_level->Move(Down))   anyVictory |= SolveRecursive(depth + 1);
  _level->SetState(&state);
  if (_level->Move(Left))   anyVictory |= SolveRecursive(depth + 1);
  _level->SetState(&state);
  if (_level->Move(Right))  anyVictory |= SolveRecursive(depth + 1);

  // If any child state was winning, update our state as winning, too.
  // Tail recursion will mark all other states on the winning path, too.
  if (anyVictory) _winning.emplace(state);
  return anyVictory;
}

void Solver::FindBestSolutionRecursive(u16 depth, u64 cost) {
  if (depth > _bestDepth) return;
  State state = _level->GetState();
  if (!_winning.contains(state)) return;

  // Check to see if we've already processed this winning node.
  // If we've been here before (at a lower cost) there's no need to explore it again.
  auto existing = _winningCosts.emplace(state, cost);
  if (!existing.second) { // True: Inserted, False: Element already exists
    if (existing.first->second <= cost) return;
    existing.first->second = cost; // This is a faster path to an existing node, update the cost and continue processing.
  }

  if (_level->Won()) {
    if (depth < _bestDepth) _bestDepth = depth;
    if (cost < _bestCost) {
      _bestCost = cost;
      _bestSolution = {_solution.begin(), _solution.end()};
    }
    return;
  }

  ComputePenaltyAndRecurse(state, Up, depth, cost);
  _level->SetState(&state);
  ComputePenaltyAndRecurse(state, Down, depth, cost);
  _level->SetState(&state);
  ComputePenaltyAndRecurse(state, Left, depth, cost);
  _level->SetState(&state);
  ComputePenaltyAndRecurse(state, Right, depth, cost);
}

// Compute the duration of this motion, then recurse back into FindBestSolutionRecursive
void Solver::ComputePenaltyAndRecurse(const State& state, Direction dir, u16 depth, u64 cost) {
  _level->Move(dir);
  State nextState = _level->GetState();

  // Speared state is not saved, because it's recoverable. Memory > speed tradeoff.
  // This is gross. It gets a little cleaner if I can use for-each, but not much.
  bool sausageSpeared = false;
  if (state.stephen.HasFork()) {
#define o(x) +1
    for (u8 i=0; i<SAUSAGES; i++) {
#undef o
      const Sausage& sausage = state.sausages[i];
      if (state.stephen.z != sausage.z) continue;
      if ((state.stephen.x == sausage.x1 && state.stephen.y == sausage.y1)
        || (state.stephen.x == sausage.x2 && state.stephen.y == sausage.y2)) {
        sausageSpeared = true;
        break;
      }
    }
  }
  if (!sausageSpeared) {
    cost += 160;

#define o(x) if (state.sausages[x] != nextState.sausages[x]) cost += 38;
    SAUSAGES;
#undef o
  } else { // Movements are faster while spearing a sausage
    cost += 158;

#define o(x) if (state.sausages[x] != nextState.sausages[x]) cost += 4;
    SAUSAGES;
#undef o
  }

  if (WouldStephenStepOnGrill(state.stephen, dir)) cost += 152; // TODO: Does this change while speared?
  // TODO: Does the sausage movement cost depend on your *current state* or the *next state*? I.e. if you unspear and roll a sausage behind you, do you pay for it?
  // TODO: Time sausage pushes as fork pushes (same latency as rotations?)
  // TODO: Time motion w/ sausage hat
  // TODO: Time motion w/ fork carry
  // TODO: Time motion as forkless -> rotations *and* lateral motion
  // TODO: Time motion when pushing a block
  // TODO: Ladder climbs while speared / non-speared?

  _solution.push_back(dir);
  FindBestSolutionRecursive(depth + 1, cost);
  _solution.pop_back();
}

bool Solver::WouldStephenStepOnGrill(Stephen stephen, Direction dir) const {
  if (dir == Up)         return _level->IsGrill(stephen.x, stephen.y - 1, stephen.z);
  else if (dir == Down)  return _level->IsGrill(stephen.x, stephen.y + 1, stephen.z);
  else if (dir == Left)  return _level->IsGrill(stephen.x - 1, stephen.y, stephen.z);
  else if (dir == Right) return _level->IsGrill(stephen.x + 1, stephen.y, stephen.z);
  assert(false);
  return false;
}