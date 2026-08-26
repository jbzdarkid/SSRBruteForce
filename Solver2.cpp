#include "Solver2.h"
#include "FrontierBuilder.h"

#include <filesystem>

// A gap in the engine must not take a long solve down with it. Those sites throw so the divergence hunt surfaces them
// loudly; here we swallow it and skip the edge, exactly as the old silent refusal did. On x64 entering a try costs
// nothing (table-driven unwind), so this is free unless it actually fires.
static bool TryMove(Level* level, Direction dir) {
  try {
    return level->Move(dir);
  } catch (const UnimplementedMove& gap) {
    static bool warned = false;
    if (!warned) { warned = true; printf("WARNING: skipping unimplemented move -- %s\n", gap.what); }
    return false;
  }
}

Solver::Solver(Level* level, u32 numBuckets) {
  _level = level;
  _numBuckets = numBuckets;
}

std::vector<Direction> Solver::Solve() {
  // Clean out any previous solve data (which likely had a different salt)
  std::filesystem::remove_all("cache");
  std::filesystem::create_directories("cache");

  // Step 1: BFS through all states, tracking states which are known 
  u32 maxDepth = 0xFFFF;
  State initialState = _level->GetState();

  // Initial layer with only one state
  {
    FrontierBuilder cache(0, _numBuckets);
    cache.AddStateUnchecked(initialState);
    cache.ProcessStates();
  }

  for (u32 depth = 1; depth < maxDepth; depth++) {
    u64 newStates = ProcessOneLayer(depth);
    if (newStates == 0) {
      std::cout << "Frontier exhausted at depth " << depth << " giving up.\n";
      break;
    }

    if (_winningStateFound) {
      printf("Winning state found at depth %d!\n", depth);

      // Allow for 2 extra iterations to search for solutions which potentially take more moves, but are faster in realtime.
      maxDepth = std::min(maxDepth, depth + 2);
    }
  }

  // Step 2: Re-traverse the tree backwards to identify winning states.
  for (s32 depth = maxDepth - 1; depth >= 0; depth--) {
    FindWinningStates(depth);
  }

  // Step 3: Now that we have a minimal set of winning states, DFS to identify the fastest realtime solution.
  std::vector<Direction> solution = FindFastestSolution(initialState);
  return solution;
}

u64 Solver::ProcessOneLayer(u32 depth) {
  FrontierBuilder cache(depth, _numBuckets);

  // First, iterate through all the states in the previous layer (bucketed by the top hash bits)
  // Note that we only do basic Move validation here, not deduplication.
  for (u32 bucket = 0; bucket < _numBuckets; bucket++) {
    LayerCache<State> previousLayer("depth", depth - 1, "bucket", bucket);
    for (const State& state : previousLayer) {
      for (Direction dir : { Up, Down, Left, Right }) {
        _level->SetState(state); // Sadly our solver is still not completely transactional.
        if (_level->Won()) {
          _winningStateFound = true; // No need to explore further past a winning state
          break;
        }

        if (!TryMove(_level, dir)) continue; // Discard illegal (losing) moves
        if (_level->heuristic && !_level->heuristic(_level, depth)) continue; // Discard heuristically-pruned moves

        cache.AddStateUnchecked(_level->GetState());
      }
    }
  }

  // Once we've finished iterating, process all the states to remove duplicates from the next layer.
  u64 newStates = cache.ProcessStates();

  std::cout << "Finished exploring depth " << depth << ", and found " << newStates << " new states.\n";
  return newStates;
}

void Solver::FindWinningStates(s32 depth) {
  u32 newlyWinningStates = 0;
  for (u32 bucket = 0; bucket < _numBuckets; bucket++) {
    LayerCache<State> layer("depth", depth, "bucket", bucket);
    for (const State& state : layer) {
      u32 bestCost = 0xFFFF'FFFF;

      for (Direction dir : { Up, Down, Left, Right }) {
        _level->SetState(state);

        // We will have multiple 'winning' depths, so it's possible that we find immediately winning states.
        if (_level->Won()) {
          _winningStates.emplace(state, 0);
          newlyWinningStates++;
          break;
        }

        if (!TryMove(_level, dir)) continue; // Discard illegal (losing) moves
        if (_level->heuristic && !_level->heuristic(_level, depth + 1)) continue;

        State newState = _level->GetState();
        auto search = _winningStates.find(newState);
        if (search == std::end(_winningStates)) continue; // Not a winning move

        // If any move is winning from this state, we record it and compute the overall cost.
        u32 cost = search->second + ComputeScore(state, dir, _level->GetState(false));
        if (cost < bestCost) bestCost = cost;
      }

      if (bestCost < 0xFFFF'FFFF) {
        _winningStates.emplace(state, bestCost);
        newlyWinningStates++;
      }
    }
  }

  std::cout << "Finished identifying winning states at depth " << depth << ", and found " << newlyWinningStates << " new winning states.\n";
}

std::vector<Direction> Solver::FindFastestSolution(const State& initialState) {
  auto start = _winningStates.find(initialState);
  if (start == std::end(_winningStates)) return {}; // unsolvable

  std::vector<Direction> solution;
  State state = initialState;
  s32 remainingCost = start->second;
  while (remainingCost > 0) {
    for (Direction dir : { Up, Down, Left, Right }) {
      _level->SetState(state);
      if (!TryMove(_level, dir)) continue;

      State newState = _level->GetState();
      auto search = _winningStates.find(newState);
      if (search == std::end(_winningStates)) continue; // Not a winning move

      // If this move's cost + the target node's cost is equal to our optimal cost, then this is an optimal move.
      s32 cost = search->second + ComputeScore(state, dir, _level->GetState(false));
      if (cost == remainingCost) {
        solution.push_back(dir);
        state = newState;
        remainingCost = search->second;
        break;
      }
    }
  }

  return solution;
}

Direction DirectionBetween(s8 x1, s8 y1, s8 x2, s8 y2) {
  if (x1 != x2) return x1 > x2 ? Right : Left;
  if (y1 != y2) return y1 > y2 ? Down : Up;
  return None;
}

s32 Solver::ComputeScore(const State& state, Direction dir, const State& newState) {
  s32 score = 1000; // By default, every accepted move costs 1 unit of time

  bool sidewaysPress = dir != state.stephen.dir && dir != (Direction)(7 - state.stephen.dir);

  s8 speared = -1;
  if (state.stephen.HasFork()) {
    for (s8 i = 0; i < NUM_SAUSAGES; i++) {
      if (state.sausages[i].IsAt(state.stephen.forkX, state.stephen.forkY, state.stephen.z)) {
        speared = i;
        break;
      }
    }
  }

  bool burnedStep = false;
  if (state.stephen == newState.stephen) {
    // We can trigger a burned step by moving forwards or backwards onto a grill, *or* by strafing while speared.
    if (!sidewaysPress || (sidewaysPress && speared != -1)) {
      if      (dir == Up)    burnedStep = _level->IsGrill(state.stephen.x,     state.stephen.y - 1, state.stephen.z);
      else if (dir == Down)  burnedStep = _level->IsGrill(state.stephen.x,     state.stephen.y + 1, state.stephen.z);
      else if (dir == Left)  burnedStep = _level->IsGrill(state.stephen.x - 1, state.stephen.y,     state.stephen.z);
      else if (dir == Right) burnedStep = _level->IsGrill(state.stephen.x + 1, state.stephen.y,     state.stephen.z);
    }
  }

  if (burnedStep) {
    score += 1000; // bounced off a grill for a full extra beat; the step's own sausage push is part of it (free)
  }
  
  bool turnedInPlace = sidewaysPress && speared == -1
    && state.stephen.x == newState.stephen.x
    && state.stephen.y == newState.stephen.y
    && state.stephen.z == newState.stephen.z;
  if (turnedInPlace) {
    // Stephen is turning in place (or bonking a turn); add 1/2 cost per unique direction a sausage rolled.
    // A perpendicular press that instead climbs a ladder or strafes moves his body, so it isn't a turn and rolls nothing.

    u32 sausageDirections = (1u << None);
    for (s8 i = 0; i < NUM_SAUSAGES; i++) {
      const Sausage& before = state.sausages[i];
      const Sausage& after = newState.sausages[i];
      // Mostly, the x1/y1 coordinate will identify a sausage's movement direciton.
      // However, when a sausage pivots, they will have different directions, so we skip computing the second direction in that case.
      Direction rolled = DirectionBetween(after.x1, after.y1, before.x1, before.y1);
      if (rolled == None) rolled = DirectionBetween(after.x2, after.y2, before.x2, before.y2);

      // If a sausage rolls in the direction stephen is facing *and* it drops, it doesn't accrue a directional cost (just a drop cost).
      if (after.z < before.z && rolled == newState.stephen.dir) continue;
      sausageDirections |= 1u << rolled;
    }
    // -500 to account for the 'None' direction (from non-moving sausages)
    score += 500 * __popcnt(sausageDirections) - 500;
  }

  // Count double-moves and drops for each sausage.
  s8 maximumSausageMove = 0;
  for (s8 i = 0; i < NUM_SAUSAGES; i++) {
    if (i == speared) continue; // Speared sausages do not accrue a falling/double-move cost.
    const Sausage& before = state.sausages[i];
    const Sausage& after = newState.sausages[i];

    s8 sausageMovement = 0;
    if (std::abs(before.x1 - after.x1) == 2 || std::abs(before.y1 - after.y1) == 2) sausageMovement++;
    sausageMovement += std::abs(newState.sausages[i].z - state.sausages[i].z);

    if (sausageMovement > maximumSausageMove) maximumSausageMove = sausageMovement;
  }
  score += 1000 * maximumSausageMove;

  s8 stephenMove = std::abs(newState.stephen.z - state.stephen.z);
  score += 1000 * stephenMove;

  // Detached fork drop (seems to run sequentially to the normal gravity steps)
  if (state.stephen.HasFork() && !newState.stephen.HasFork()) {
    s8 forkDelta = state.stephen.forkZ - newState.stephen.forkZ;
    if (forkDelta > 0) score += 1000 * forkDelta;
  }

  Direction stephenMoved = DirectionBetween(state.stephen.x, state.stephen.y, newState.stephen.x, newState.stephen.y);
  if (dir == (Direction)(7 - stephenMoved)) score--; // Prefer backwards steps where possible as a tie break

  return score;
}