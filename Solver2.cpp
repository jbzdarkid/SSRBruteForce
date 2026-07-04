#include "Solver2.h"

#include <fstream>
#include <string>

Solver2::Solver2(Level* level) {
  _level = level;
}

std::vector<Direction> Solver2::Solve() {

  // Step 1: BFS through all states, tracking states which are known 
  u32 maxDepth = 0xFFFF;
  State2 initialState = _level->GetState2();

  _currentLayer = WritableLayerCache<State2>(0); // Initial layer is at depth 0
  _currentLayer.Add(initialState);

  for (u32 depth = 1; depth < maxDepth; depth++) {
    // Swap so that we iterate nodes from the previous layer
    _currentLayer = WritableLayerCache<State2>(depth); // Flushes the _currentLayer to disk
    _previousLayer = ReadableLayerCache<State2>(depth - 1);

    ProcessOneLayer(depth);

    if (_winningStateFound) {
      // Allow for 2 extra interations to search for solutions which potentially take more moves, but are faster in realtime.
      maxDepth = std::min(maxDepth, depth + 2);
    }
  }

  // Free the stage 1 scratch space
  _exploredStateHashes = {};
  _currentLayer = {};
  _previousLayer = {};

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
  while (_previousLayer.MoveNext()) {
    const State2& state = _previousLayer.Current();
  // for (const State2& state : _previousLayer) {
    for (Direction dir : { Up, Down, Left, Right }) {
      _level->SetState2(state); // TODO: Should be partially avoidable once Move is itempotent on failure
      if (_level->Won()) {
        _winningStateFound = true; // No need to explore further past a winning state
        continue;
      }

      if (!_level->Move(dir)) continue; // Discard illegal (losing) moves

      State2 newState = _level->GetState2();
      bool inserted = _exploredStateHashes.insert(absl::HashOf(newState)).second;
      if (!inserted) continue;

      _currentLayer.Add(newState);
    }
  }
}

void Solver2::FindWinningStates(u32 depth) {
  std::vector<State2> layer = LoadLayerFromDisk(depth);
  for (const State2& state : layer) {
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

void Solver2::SaveLayerToDisk(const std::vector<State2>& layer, u32 depth) {
  std::ofstream out("layer_" + std::to_string(depth) + ".bin", std::ios::binary);
  out.write(reinterpret_cast<const char*>(layer.data()), layer.size() * sizeof(State2));
}

std::vector<State2> Solver2::LoadLayerFromDisk(u32 depth) {
  std::ifstream in("layer_" + std::to_string(depth) + ".bin", std::ios::binary | std::ios::ate);
  std::streamsize bytes = in.tellg();
  in.seekg(0);
  std::vector<State2> layer(bytes / sizeof(State2));
  in.read(reinterpret_cast<char*>(layer.data()), bytes);
  return layer;
}
