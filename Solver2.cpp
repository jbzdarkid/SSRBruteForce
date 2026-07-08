#include "Solver2.h"
#include "FrontierBuilder.h"

#include <filesystem>

Solver2::Solver2(Level* level, u32 numBuckets) {
  _level = level;
  _numBuckets = numBuckets;
}

std::vector<Direction> Solver2::Solve() {
  // Clean out any previous solve data (which likely had a different salt)
  std::filesystem::remove_all("cache");
  std::filesystem::create_directories("cache");

  // Step 1: BFS through all states, tracking states which are known 
  u32 maxDepth = 0xFFFF;
  State2 initialState = _level->GetState2();

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

void Solver2::ProcessOneLayer(u32 depth) {
  FrontierBuilder cache(depth, _numBuckets);

  // First, iterate through all the states in the previous layer (bucketed by the top hash bits)
  // Note that we only do basic Move validation here, not deduplication.
  for (u32 bucket = 0; bucket < _numBuckets; bucket++) {
    LayerCache<State2> previousLayer("depth", depth - 1, "bucket", bucket);
    for (const State2& state : previousLayer) {
      for (Direction dir : { Up, Down, Left, Right }) {
        _level->SetState2(state); // Sadly our solver is still not completely transactional.
        if (_level->Won()) {
          _winningStateFound = true; // No need to explore further past a winning state
          break;
        }

        if (!_level->Move(dir)) continue; // Discard illegal (losing) moves
        if (_level->heuristic && !_level->heuristic(_level)) continue; // Discard heuristically-pruned moves

        cache.AddStateUnchecked(_level->GetState2());
      }
    }
  }

  // Once we've finished iterating, process all the states to remove duplicates from the next layer.
  u64 newStates = cache.ProcessStates();

  std::cout << "Finished exploring depth " << depth << ", and found " << newStates << " new states.\n";
}

void Solver2::FindWinningStates(u32 depth) {
  for (u32 bucket = 0; bucket < _numBuckets; bucket++) {
    LayerCache<State2> layer("depth", depth, "bucket", bucket);
    for (const State2& state : layer) {
      for (Direction dir : { Up, Down, Left, Right }) {
        _level->SetState2(state);
      
        // We will have multiple 'winning' depths, so it's possible that we find immediately winning states.
        if (_level->Won()) {
          _winningStates.emplace(state, depth);
          break;
        }

        if (!_level->Move(dir)) continue; // Discard illegal (losing) moves
        if (_level->heuristic && !_level->heuristic(_level)) continue;

        State2 newState = _level->GetState2();
        auto search = _winningStates.find(newState);
        if (search == std::end(_winningStates)) continue; // Not a winning move

        // If any move is winning from this state, we can record it and move on.
        // We don't actually care about the sequence of moves yet, just that there is a winning move.
        _winningStates.emplace(state, depth);
        break;
      }
    }
  }
}

void Solver2::FindFastestSolution(const State2& state, std::vector<Direction>& solution, u32 score) {
  _level->SetState2(state);
  if (_level->Won()) {
    if (score < _bestScore) {
      _bestSolution = solution;
      _bestScore = score;
    }
    return;
  }

  for (Direction dir : { Up, Down, Left, Right }) {
    _level->SetState2(state);

    if (!_level->Move(dir)) continue;

    State2 newState = _level->GetState2();
    auto search = _winningStates.find(newState);
    if (search == std::end(_winningStates) || search->second != solution.size() + 1) continue; // Not a winning move, or not an optimal winning move.

    u32 scoreDelta = ComputeScore(state, dir, newState);
    solution.push_back(dir);
    FindFastestSolution(newState, solution, score + scoreDelta);
    solution.pop_back();
  }
}

u32 Solver2::ComputeScore(const State2& state, Direction dir, const State2& newState) {
  u32 score = 0;

  // Speared state is not saved, because it's recoverable. Memory > speed tradeoff.
  bool sausageSpeared = false;
  if (state.stephen.HasFork()) {
    for (const Sausage& sausage : state.sausages) {
      if (sausage.IsAt(state.stephen.x, state.stephen.y, state.stephen.z)) {
        sausageSpeared = true;
        break;
      }
    }
  }
  if (!sausageSpeared) {
    score += 160'000;

#define o(x) if (state.sausages[x] != newState.sausages[x]) score += 38'000;
    SAUSAGES
#undef o
  } else { // Movements are faster while spearing a sausage
    score += 158'000;

#define o(x) if (state.sausages[x] != newState.sausages[x]) score += 4'000;
    SAUSAGES
#undef o
  }

  bool burnedStep = false;
  if (dir == Up)         burnedStep = _level->IsGrill(state.stephen.x, state.stephen.y - 1, state.stephen.z);
  else if (dir == Down)  burnedStep = _level->IsGrill(state.stephen.x, state.stephen.y + 1, state.stephen.z);
  else if (dir == Left)  burnedStep = _level->IsGrill(state.stephen.x - 1, state.stephen.y, state.stephen.z);
  else if (dir == Right) burnedStep = _level->IsGrill(state.stephen.x + 1, state.stephen.y, state.stephen.z);
  if (burnedStep) score += 152'000; // TODO: Does this change while speared?

  // TODO: Does the sausage movement cost depend on your *current state* or the *next state*? I.e. if you unspear and roll a sausage behind you, do you pay for it?
  // TODO: Time sausage pushes as fork pushes (same latency as rotations?)
  // TODO: Time motion w/ sausage hat
  // TODO: Time motion w/ fork carry
  // TODO: Time motion as forkless -> rotations *and* lateral motion
  // TODO: Time motion when pushing a block
  // TODO: Ladder climbs while speared / non-speared?

  // TODO: This does not correctly handle logrolling
  // Instead of counting backward steps separately, just lower the score (as a "reward").
  if (state.stephen.dir == Up && dir == Down)         score--;
  else if (state.stephen.dir == Down && dir == Up)    score--;
  else if (state.stephen.dir == Left && dir == Right) score--;
  else if (state.stephen.dir == Right && dir == Left) score--;

  return score;
}