#include "Solver2.h"
#include "FrontierBuilder.h"

#include <filesystem>

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
    ProcessOneLayer(depth);

    if (_winningStateFound) {
      printf("Winning state found at depth %d!\n", depth);

      // Allow for 2 extra iterations to search for solutions which potentially take more moves, but are faster in realtime.
      maxDepth = std::min(maxDepth, depth + 2);
    }
  }

  // Step 2: Re-traverse the tree backwards to identify winning states.
  for (u32 depth = maxDepth - 1; depth > 0; depth--) {
    FindWinningStates(depth);
  }

  // Step 3: Now that we have a minimal set of winning states, DFS to identify the fastest realtime solution.
  std::vector<Direction> solution;
  FindFastestSolution(initialState, solution, 0);
  return _bestSolution;
}

void Solver::ProcessOneLayer(u32 depth) {
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

        if (!_level->Move(dir)) continue; // Discard illegal (losing) moves
        if (_level->heuristic && !_level->heuristic(_level, depth)) continue; // Discard heuristically-pruned moves

        cache.AddStateUnchecked(_level->GetState());
      }
    }
  }

  // Once we've finished iterating, process all the states to remove duplicates from the next layer.
  u64 newStates = cache.ProcessStates();

  std::cout << "Finished exploring depth " << depth << ", and found " << newStates << " new states.\n";
}

void Solver::FindWinningStates(u32 depth) {
  u32 newlyWinningStates = 0;
  for (u32 bucket = 0; bucket < _numBuckets; bucket++) {
    LayerCache<State> layer("depth", depth, "bucket", bucket);
    for (const State& state : layer) {
      for (Direction dir : { Up, Down, Left, Right }) {
        _level->SetState(state);

        // We will have multiple 'winning' depths, so it's possible that we find immediately winning states.
        if (_level->Won()) {
          _winningStates.emplace(state, depth);
          newlyWinningStates++;
          break;
        }

        if (!_level->Move(dir)) continue; // Discard illegal (losing) moves
        if (_level->heuristic && !_level->heuristic(_level, depth + 1)) continue;

        State newState = _level->GetState();
        auto search = _winningStates.find(newState);
        if (search == std::end(_winningStates)) continue; // Not a winning move

        // If any move is winning from this state, we can record it and move on.
        // We don't actually care about the sequence of moves yet, just that there is a winning move.
        _winningStates.emplace(state, depth);
        break;
      }
    }
  }

  std::cout << "Finished identifying winning states at depth " << depth << ", and found " << newlyWinningStates << " new winning states.\n";
}

void Solver::FindFastestSolution(const State& state, std::vector<Direction>& solution, u32 score) {
  _level->SetState(state);
  if (_level->Won()) {
    if (score < _bestScore) {
      _bestSolution = solution;
      _bestScore = score;
    }
    return;
  }

  for (Direction dir : { Up, Down, Left, Right }) {
    _level->SetState(state);

    if (!_level->Move(dir)) continue;

    State newState = _level->GetState();
    auto search = _winningStates.find(newState);
    if (search == std::end(_winningStates) || search->second != solution.size() + 1) continue; // Not a winning move, or not an optimal winning move.

    u32 scoreDelta = ComputeScore(state, dir, _level->GetState(false));
    solution.push_back(dir);
    FindFastestSolution(newState, solution, score + scoreDelta);
    solution.pop_back();
  }
}

Direction DirectionBetween(s8 x1, s8 y1, s8 x2, s8 y2) {
  if (x1 != x2) return x1 > x2 ? Right : Left;
  if (y1 != y2) return y1 > y2 ? Down : Up;
  return None;
}

u32 Solver::ComputeScore(const State& state, Direction dir, const State& newState) {
  u32 score = 1000; // By default, every accepted move costs 1 unit of time

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
      Direction rolled = DirectionBetween(before.x1, before.y1, after.x1, after.y1);
      if (rolled == None) rolled = DirectionBetween(before.x2, before.y2, after.x2, after.y2);
      sausageDirections |= 1u << rolled;
    }
    // -500 to account for the 'None' direction (from non-moving sausages)
    score += 500 * __popcnt(sausageDirections) - 500;
  }

  // Double-moves are full cost for each sausage that moves.
  for (s8 i = 0; i < NUM_SAUSAGES; i++) {
    const Sausage& before = state.sausages[i];
    const Sausage& after = newState.sausages[i];
    s8 distance = (after.x1 - before.x1) + (after.y1 - before.y1);
    if (distance < 0) distance = -distance;
    if (distance > 1) score += 1000 * (distance - 1);
  }

  // Ladder motion costs 1 beat per rung climbed
  s8 ladderDelta = newState.stephen.z - state.stephen.z;
  if (ladderDelta > 0) {
    score += 1000 * ladderDelta;
  }

  // Descending a ladder moves simultaneously with dropped sausages, so compute them together
  s8 maximumDrop = 0;
  if (ladderDelta < maximumDrop) maximumDrop = ladderDelta;
  for (s8 i = 0; i < NUM_SAUSAGES; i++) {
    s8 sausageDelta = newState.sausages[i].z - state.sausages[i].z;
    if (sausageDelta < maximumDrop) maximumDrop = sausageDelta;
  }
  score += 1000 * -maximumDrop;

  // TODO: This does not correctly handle logrolling
  Direction stephenMoved = DirectionBetween(state.stephen.x, state.stephen.y, newState.stephen.x, newState.stephen.y);
  if (dir == (Direction)(7 - stephenMoved)) score--; // Prefer backwards steps where possible as a tie break

  // TODO: Time motion w/ fork carry
  // TODO: Time motion as forkless -> rotations *and* lateral motion
  // TODO: Time motion when pushing a block

  return score;
}