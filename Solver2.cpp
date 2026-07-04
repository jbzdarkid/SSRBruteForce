#include "Solver2.h"

#include <iostream>

Solver2::Solver2(Level* level) {
  _level = level;

  u64 numSlots = 1ull << 31; // 2^31 slots * (1 control byte + 8 hash bytes) ~= 19.3 GB
  _maxStateHashes = numSlots * 7 / 8; // Abseil's load factor is 7/8, at which point it rehashes
  _exploredStateHashes.reserve(_maxStateHashes); // Abseil will allocate a table that fits this many elements
  assert(_exploredStateHashes.capacity() == (1ull << 31) - 1);
}

std::vector<Direction> Solver2::Solve() {

  // Step 1: BFS through all states, tracking states which are known 
  u32 maxDepth = 0xFFFF;
  State2 initialState = _level->GetState2();

  {
    WritableLayerCache<State2> initialLayer(0);
    initialLayer.Add(initialState);
  } // Flushes at end of scope

  for (u32 depth = 1; depth < maxDepth; depth++) {
    ProcessOneLayer(depth);

    if (_winningStateFound) {
      printf("Winning state found at depth %d!\n", depth);

      // Allow for 2 extra interations to search for solutions which potentially take more moves, but are faster in realtime.
      maxDepth = std::min(maxDepth, depth + 2);
    }

    if (_exploredStateHashes.size() >= _maxStateHashes) {
      printf("We ran out of hashtable size and stopped storing new states.\n");
      if (_winningStateFound) break;
      return {};
    }
  }

  // Free the stage 1 scratch space
  _exploredStateHashes = {};

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
  ReadableLayerCache<State2> previousLayer(depth - 1);
  WritableLayerCache<State2> currentLayer(depth);

  do {
    const State2& state = previousLayer.Current();
    for (Direction dir : { Up, Down, Left, Right }) {
      _level->SetState2(state); // Sadly our solver is still not completely transactional.
      if (_level->Won()) {
        _winningStateFound = true; // No need to explore further past a winning state
        continue;
      }

      if (_exploredStateHashes.size() >= _maxStateHashes) continue; // Stop saving new states once we run out of capacity
      if (!_level->Move(dir)) continue; // Discard illegal (losing) moves

      State2 newState = _level->GetState2();
      bool inserted = _exploredStateHashes.insert(absl::HashOf(newState)).second;
      if (!inserted) continue;

      currentLayer.Add(newState);
    }
  } while (previousLayer.MoveNext());


  std::cout << "Finished exploring depth " << depth
    << " with " << currentLayer.Size()
    << " states. Total hashset size: " << _exploredStateHashes.size()
    << " / " << _maxStateHashes << "\n";
}

void Solver2::FindWinningStates(u32 depth) {
  ReadableLayerCache<State2> layer(depth);
  while (layer.MoveNext()) {
    const State2& state = layer.Current();
    for (Direction dir : { Up, Down, Left, Right }) {
      _level->SetState2(state);
      
      // We will have multiple 'winning' depths, so it's possible that we find immediately winning states.
      if (_level->Won()) {
        _winningStates.emplace(state, depth);
        break;
      }

      if (!_level->Move(dir)) continue; // Discard illegal (losing) moves

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