#pragma once
#include "LevelData.h"
#include "State.h"
#include "WitnessRNG/StdLib.h"

struct Level : public LevelData {
  using LevelData::LevelData; // Inherit the constructor

  // The starting point whenever you write an automated solver -- this function allows the user to
  // manually input the moves they'd like to make. This also is a critical debugging tool, since it
  // allows us to reproduce a specific behavior.
  bool InteractiveSolver();

  // Serialize/deserialize the current state, used for backtracking algorithms.
  State GetState() const;
  void SetState(const State* state);

  // The main entry point -- this takes a player input (any of the 4 cardinal directions) and
  // simulates the game's behavior by moving stephen, his fork, and the sausages around the level.
  // This function returns false if moves is "useless", i.e. it would cause an immediate loss
  // or zero change in state (walking into a wall).
  bool Move(Direction dir);
private:
  // These 4 functions handle the different ways stephen can move on level terrain
  // Much like the parent Move function, their return value indicates a useless move.
  bool HandleLogRolling(Direction dir, bool& handled);
  bool HandleLadderMotion(Direction dir, bool& handled);
  bool HandleRotation(Direction dir, bool& handled);
  bool HandleBurnedStep(Direction dir, bool& handled);
  bool HandleForkReconnect(Direction dir, bool& handled);
  // TODO: Rename, repurposed
  bool MoveThroughSpace3(Direction dir, s8 stephenRotationDir=0);

  // This function checks if *whatever* is at |x, y, z| can move in a direction |dir|
  // For example, air can always move, walls can never move, and most everything else
  // will check the cell in front of it.
  // This function has no direct side-effects, but does evaluate the entirety of the movement,
  // and updates the below struct with the potential results of the call.
  struct CPMData {
    Vector<s8> movedSausages;
    Vector<s8> sausagesToRoll;
    Vector<s8> sausagesToDrop;
    s8 sausageToSpear = -1; // This applies to *all* situations where a fork gets stuck in a sausage.
    s8 sausageHat = -1;
    u8 consideredSausages = 0; // We have /considered/ if this sausage can physically move and added it to movedSausages if applicable
    u8 sausagesToDoubleMove = 0;
    bool pushedFork = false;
    bool canPhysicallyMove = false;
  } data;
  bool Consider(s8 sausageNo); // Helper around data.consideredSausages
  bool CanPhysicallyMove(s8 x, s8 y, s8 z, Direction dir, bool stephenIsRotating=false);
  // Recursive internal function which actually does the heavy lifting.
  bool CanPhysicallyMoveInternal(s8 x, s8 y, s8 z, Direction dir);
  // Another internal helper because sausage carrying is complicated
  bool IsSausageCarried(s8 x, s8 y, s8 z, Direction dir, bool stephenIsRotating, bool canDoubleMove);
  void CheckForSausageCarry(s8 x, s8 y, s8 z, Direction dir, bool stephenIsRotating);

  // The counterpart to CanPhysicallyMove, MoveThroughSpace actually enacts the proposed movement.
  // Note that in some cases a move will have side effects but not result in stephen moving.
  // As with CPM, a return value of false indicates a useless movement.
  bool MoveThroughSpace(s8 x, s8 y, s8 z, Direction dir, s8 stephenRotationDir=0, bool checkSausageCarry=false, bool doSausageRotation=false, bool doDoubleMove=true);
  bool MoveThroughSpaceInternal(s8 x, s8 y, s8 z, Direction dir, s8 stephenRotationDir=0, bool doSausageRotation=false, bool doDoubleMove=true);

  // This function handles movement of stephen (and his fork) common to all the above functions.
  // A return value of false indicates a useless move.
  bool MoveStephenThroughSpace(Direction dir, bool ladderMotion=false);

  // Saves which sausage the fork is currently stuck in (-1 if not stuck).
  // *technically* this should live on Stephen, but it would make that > sizeof(u64).
  s8 _sausageSpeared = -1;
  bool _interactive = false; // Set to true while in the InteractiveSolver, allows us to emit nice errors

  inline Direction Inverse(Direction dir) {
    assert(dir > 0 && dir < 7);
    return (Direction)(7 - dir);
  }
};
