#pragma once
#include "Types.h"
#include "LevelData.h"
#include "WitnessRNG/StdLib.h"

#define STAY_NEAR_THE_SAUSAGES 0
#define HASH_CACHING 1
#define SORT_SAUSAGE_STATE 0

struct State {
  Stephen stephen;

#define o(x) +1
  Sausage sausages[SAUSAGES];
#undef o

  // Used to build the tree, ergo not part of the hashing or comparison algos
  State* next = nullptr;
  State* u = nullptr;
  State* d = nullptr;
  State* l = nullptr;
  State* r = nullptr;
#define UNWINNABLE 0xFFFE
  u16 winDistance = UNWINNABLE;

#if HASH_CACHING
  size_t hash = 0;
#endif

  bool operator==(const State& other) const;
  size_t Hash() const;
};

namespace std {
template<> struct hash<State> {
  size_t operator()(const State& state) const { 
#if HASH_CACHING
  return state.hash;
#else 
  return state.Hash();
#endif
  }
};
}

struct Level : public LevelData {
  using LevelData::LevelData; // Inherit the constructor
  bool InteractiveSolver();

  State GetState() const;
  void SetState(const State* state);
  bool WouldStephenStepOnGrill(const Stephen& stephen, Direction dir); // Used by Solver to compute movement durations.

  // Returns true if the move succeeded
  // Returns false if the move was illegal for any reason
  bool Move(Direction dir);
private:
  // Move() helpers
  bool HandleLogRolling(const Sausage& sausage, Direction dir, bool& handled);
  bool HandleSpearedMotion(Direction dir);
  bool HandleLadderMotion(Direction dir, bool& handled);
  bool HandleBurnedStep(Direction dir);
  bool HandleForklessMotion(Direction dir);
  bool HandleRotation(Direction dir);
  bool HandleParallelMotion(Direction dir);
  // TODO: Rename, repurposed
  bool MoveThroughSpace3(Direction dir, s8 stephenRotationDir=0);

  // CanPhysicallyMove is for when you want to check if motion is possible,
  // and if it isn't, stephen will enact a different kind of motion.
  // It has no side-effects.
  bool CanPhysicallyMove(s8 x, s8 y, s8 z, Direction dir, bool stephenIsRotating=false);
  bool CanPhysicallyMoveInternal(s8 x, s8 y, s8 z, Direction dir);
  // When a sausage is supported by another moving object (sausage, stephen, or fork) they can be 'carried' along,
  // which means that they move but do not block the overall motion.
  bool IsSausageCarried(s8 x, s8 y, s8 z, Direction dir, bool stephenIsRotating, bool canDoubleMove);
  void CheckForSausageCarry(s8 x, s8 y, s8 z, Direction dir, bool stephenIsRotating);
  // MoveThroughSpace is for when stephen is supposed to make a certain motion,
  // and if the motion fails, the move should not have been taken.
  // It may have side effects.
  bool MoveThroughSpace(s8 x, s8 y, s8 z, Direction dir, s8 stephenRotationDir=0, bool doSausageRotation=false, bool doDoubleMove=true);
  bool MoveThroughSpace2(s8 x, s8 y, s8 z, Direction dir, s8 stephenRotationDir=0, bool doSausageRotation=false, bool doDoubleMove=true);
  // Similarly to MoveThroughSpace, MoveStephenThroughSpace is for actually moving stephen,
  // and we are just trying to figure out if that motion results in an invalid state,
  // such as losing or burning a sausage. The equivalent check function is CanWalkOnto.
  // It will have side effects even if the move is invalid.
  bool MoveStephenThroughSpace(Direction dir);

  s8 _sausageSpeared = -1;
  bool _interactive = false;
};
