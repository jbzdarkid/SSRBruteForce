#pragma once
#include "LevelData.h"
#include "State.h"

// Thrown when a move reaches game behaviour we know we have NOT implemented. These sites used to return false, which
// pruned the branch and hid the gap: the RRT never explored it, so the divergence hunt could never report it and a
// clean run meant nothing there. Failing loudly instead makes each gap surface as a crash carrying the exact scenario,
// which is what a regression test needs. Callers that want to keep going (the explorer) catch it and report.
struct UnimplementedMove {
  const char* what;
};

// The move pipeline, following the staged "game-world" model in move-stages.md: classify the input, prove the whole
// motion is legal, move everything at once, then let the world react (gravity, then heat, then knock-ons, then outcome).
// Level derives from LevelData for the terrain queries (IsWall, CanWalkOnto, IsGrill, ...) and adds the move engine on
// top, plus state (de)serialization for the solver.
//
// Every motion is planned into a MovePlan and applied with a single Commit, so a refused move leaves the world
// untouched. The ladder is the one exception: it still steps the live state rung by rung (see move-stages.md).
struct Level : public LevelData {
  using LevelData::LevelData; // Inherit the constructor

  State GetState(bool sort = true) const;
  void SetState(const State& state);

#if OVERWORLD_HACK
  // An uncollected overworld sausage IS a wall, so the wall grid is derived from the current state and must be
  // re-derived after a move collects one. Do NOT reach for SetState(GetState()) to get this -- that round-trip also
  // copies the SORTED sausage array back over the live one, silently permuting slot identity.
  void RefreshOverworldWalls();
#endif

  // Takes a player input (one of the 4 cardinal directions) and simulates the game's response. Returns false if the
  // move is refused outright (a wall or the void blocks it). A move the game accepts returns true even when nothing
  // ends up moving -- a grill bounces Stephen back, a fork bonks against a wall mid-turn.
  bool Move(Direction dir);

  // Level-specific heuristic which returns false from losing states to reduce the total state count.
  bool (*heuristic)(const Level*, u32 depth) = nullptr;

private:
#if OVERWORLD_HACK
  u64 OverworldBits() const;              // which overworld sausages are collected, derived from the live world
  void ApplyOverworldBits(u64 bits);      // materialize those bits back into the wall grid
#endif
  // A move's entire planning scratch AND its working tableau -- created fresh on the stack by each leaf handler (see
  // NewPlan) and threaded by reference through the planners and the reaction passes, so a Move() owns NO mutable class
  // state (see move-stages.md "a move owns no class scratch"). |stephen|/|sausages| start as a copy of the live state and
  // are advanced -- by the planners (push/carry) then the reaction passes (Settle/CookMoved/DoubleMove) -- entirely off
  // to the side, so the live _stephen/_sausages stay untouched until the single Commit at the very end. |mask| marks
  // which sausages this move shifted; |doubleMoveMask|/|doubleMoveDir| record double-movers that owe one more tumble
  // (consumed by DoubleMove, which settles and cooks them itself); |rotating| is the one input the handler sets, when
  // the press is a turn, so the carry decision can treat Stephen's body as a wall.
  struct MovePlan {
    Stephen stephen;
    Sausage sausages[NUM_SAUSAGES];
    u64 mask = 0;
    u64 doubleMoveMask = 0;
    u64 rigidLoad = 0; // sausages moving as ONE rigid load with Stephen this move (speared+riders, head/fork hat+stack); co-movers must never be treated as obstacles to each other
    u64 stephenLoad = 0; // sausages Stephen's OWN carry places (his head/fork hats and their stacks): a push chain must not carry them, so no one has to undo its guess
    u64 noDoubleMove = 0; // sausages held rigidly by the fork: exactly one cell with Stephen, never an extra tumble
    s8 lodgedHost = -1; // sausage a still-detached fork is stuck inside: it rides that host down through gravity too
    Sausage lodgedPose{}; // that host's pose when the fork was tracked onto it, so later stages' motion can be replayed
    Direction doubleMoveDir[NUM_SAUSAGES] = {};
    bool rotating = false;
    bool airborne = false; // Stephen may end this move unsupported (a log roll off a ledge), so stage 5 drops him too
  };

  // Build a fresh plan seeded with the current game state -- |stephen| and every |sausages| slot copied from the live
  // _stephen/_sausages -- so the plan is a complete working tableau the move can advance without touching the object.
  MovePlan NewPlan() const;

  // The single mutation point of a Move: copy the fully-resolved plan (Stephen's pose + every sausage) into the live
  // game state. Reached only once every stage has succeeded, so it is unconditional -- there is nothing to roll back.
  void Commit(const MovePlan& plan);

  // True if a sausage end resting at (x,y,z) is held by STATIONARY support -- terrain (ground floor or wall-top), or a
  // sausage not in |moving|. Such an end keeps its sausage from being carried: the load slides out from under it and it
  // stays put. |self| is excluded so a sausage never anchors on itself.
  bool AnchoredAt(s8 x, s8 y, s8 z, s8 self, u64 moving) const;

  // A DETACHED fork is a passive rider in the reference (SubjectToPassiveForces / NeedsGround): it travels with whatever
  // it rests on and falls when nothing holds it up. CarryDetachedFork tracks it onto its support's new pose (run before
  // gravity, reading the pre-move layout to find that support); DropDetachedFork then lets it fall through the settled
  // tableau. A fork lodged INSIDE a sausage is held there and neither pass touches it.
  // True when a thrown fork lies back within Stephen's grasp -- one cell ahead in his facing, at his level, pointing the
  // way he faces (TryReattachFork). A fork facing crosswise is not picked up.
  bool ForkInReach(const Stephen& s) const;
  bool PlanHasOverlap(const MovePlan& plan) const;

  s8 RollTorsion(const MovePlan& plan, s8 idx, int depth = 0) const;
  bool TrackLodgedFork(MovePlan& plan) const;
  void CarryDetachedFork(MovePlan& plan) const;
  void DropDetachedFork(MovePlan& plan) const;

  // The shared reaction tail (stages 5-7 + commit) for every motion that plans one rigid step into |plan| and then lets
  // the world settle: MarkDoubleMoves -> Settle -> CookMoved -> DoubleMove -> Commit, reading the live _sausages/_stephen
  // as the pre-move layout. Returns false (refusing the move) if a sausage falls out of the world or would burn. Only
  // the ladder path, which interleaves its own fix-ups with these passes, does NOT use this.
  bool ReactAndCommit(MovePlan& plan);

  // GetSausage against the plan's working tableau (not the live, pre-move _sausages): the index of the sausage occupying
  // (x,y,z) in |plan.sausages|, or -1. Used by the reaction passes, which must see post-motion positions.
  s8 GetPlannedSausage(const MovePlan& plan, s8 x, s8 y, s8 z) const;

  // A press along Stephen's facing axis is an ordinary walk -- one cell forward (matching his facing) or backward
  // (opposing it), keeping his facing. The dispatch handler: sets |handled| and walks, or leaves |handled| false for a
  // perpendicular press so the turn takes it.
  bool HandleStepMotion(Direction dir, bool& handled);

  // A press perpendicular to Stephen's facing turns him in place, swinging the fork around his body to face |dir|. The
  // dispatch handler: sets |handled| and performs the turn, or leaves |handled| false for an along-axis press so the
  // walk takes it.
  bool HandleRotation(Direction dir, bool& handled);

  // Pivot a "clean hat" -- a sausage cantilevered on Stephen's head, plus any sausages stacked on it -- 90 degrees about
  // the head cell as he turns to |dir|. Records the rotated sausages into |plan| and |hatMask|. No-op unless a clean hat
  // is present (far half over open space, swing not wall-blocked).
  void PlanHatRotation(Direction dir, MovePlan& plan, u64& hatMask) const;

  // A sausage resting on Stephen's head or fork rides rigidly with a step (no roll); carries riders. Wall-blocked stays.
  void PlanHatCarry(s8 sausageNo, s8 dx, s8 dy, Direction dir, MovePlan& plan, u64 rigidMask) const;

  // Common to every motion (Level1's HandleBurnedStep): a body left resting on a grill is on hot ground, so it recoils
  // straight back the way it came as its own fresh move. Sets |handled| when it fires. The recoil is forced, so a
  // refused one refuses the whole press: |preMove| is the tableau from before the first half committed, restored so the
  // chained pair is as atomic as a single move.
  bool HandleBurnedStep(Direction dir, const MovePlan& preMove, bool& handled);

  // Standing on top of a sausage and pressing across its long axis log-rolls it: the sausage (and Stephen riding it)
  // rolls one cell the OPPOSITE way, then Stephen settles if he rolled off an edge. Sets |handled| when it fires;
  // leaves it false (a press along the axis, or a blocked roll) so the rest of the pipeline can take the move.
  bool HandleLogRolling(Direction dir, bool& handled);

  // A ladder lets Stephen change levels: climbing up its matching face, or descending a back-facing ladder one level
  // down. He carries a speared sausage with him. Sets |handled| when it fires; leaves it false when there's no ladder
  // to use so ordinary walking/turning can take the move.
  bool HandleLadderMotion(Direction dir, bool& handled);

  // Raise/lower Stephen's body and fork by |dz| levels -- the rigid motion of a ladder rung. |carried| is the set that
  // rides with him (grown here when a climb-up shoves a resting sausage upward); such body/fork-shoved sausages are also
  // recorded in |hatMask| so the step-off treats them as a head hat rather than a rigid rider. Returns false if the
  // fork/body would move into a wall, or into a sausage that itself can't rise (which blocks the climb).
  bool LiftStephen(s8 dz, u64& carried, u64& hatMask);

  // Shove sausage |sausageNo| (and everything stacked transitively on top of it) up by |dz| as Stephen climbs into it:
  // record the whole tower into |carried| and |hatMask|. Returns false if any of them would rise into a wall.
  bool LiftSausageStack(s8 sausageNo, s8 dz, u64& carried, u64& hatMask);

  // The mask of |base| plus every sausage stacked transitively on top of it (growing through EITHER end) -- everything
  // that rides along with |base|, rigid or not, whether or not it is free to leave. This is the tower a fork or a hat
  // carries; for the co-moving load a carry must not treat as an obstacle, use MovingLoad. Returns 0 for |base| == -1.
  u64 CarriedMask(s8 base) const;

  // The sausages that ACTUALLY ride |base| when it translates: |base| plus every sausage stacked transitively on it
  // whose own ends are all free to leave -- neither propped on terrain nor on a sausage that stays behind. Grown to a
  // fixed point, since a rider only comes free once whatever sits under its other end is itself known to be riding.
  // This is the honest co-moving set for |rigidLoad| (move-stages.md: motion is one event): CarriedMask folds in every
  // sausage stacked above regardless, but one anchored on a wall-top or a stationary neighbour is left behind as the
  // base slides out from under it, so it is NOT a co-mover and must still be treated as an obstacle. Returns 0 for -1.
  u64 MovingLoad(s8 base) const;

  // How GrowRigidStack treats an end with no stack sausage directly beneath it (below == -1):
  enum class Cantilever {
    None, // never rigid -- the end must rest squarely on a stack member (a pure both-ends stack)
    Air,  // an end cantilevered over genuine open space (below == -1 && !IsWall) still rides rigidly
  };
  // Grow a rigid (non-rolling) stack from |seed|: repeatedly add any sausage whose two ends are EACH either resting on
  // a stack member or an allowed cantilever (see |cant|), with at least one end on a stack member. This is the shared
  // "which carried sausages ride flat" primitive for steps and ladder climbs; a squarely-stacked rider joins, a
  // cantilevered rider joins only under Cantilever::Air (a pure translation, or a fork-locked speared base).
  u64 GrowRigidStack(u64 seed, Cantilever cant) const;

  // The mask of |base| plus every sausage SQUARELY stacked on it -- one whose BOTH ends rest on stack members
  // (transitively). This is the rigid unit that rides without rolling (a head hat and anything squarely stacked on it);
  // a cantilevered rider (only one end on the stack) is excluded, so it rolls when carried across its axis. Returns 0
  // for |base| == -1. Contrast CarriedMask, which grows through EITHER end (everything that rides along, rigid or not).
  u64 FullySupportedStack(s8 base) const;

  // Step Stephen one cell in |dir| while keeping his facing, as part of a ladder climb -- either OFF the top of a ladder
  // onto a ledge (|ladderMotion| false: the body needs footing), or OUT over a ladder at the start of a descent
  // (|ladderMotion| true: the body may hang). Unlike a rigid slide, the fork and body PUSH any sausage at their
  // destination (the reference routes ladder motion through MoveStephenThroughSpace -- the fork
  // shoves a sausage as he steps onto/over the ledge), then gravity and heat resolve. |carried| is the sausage riding on
  // the fork (it translates rigidly with him, -1 if none). Returns false on a wall, a missing ledge, or an immovable push.
  bool StepOffLadder(Direction dir, u64 carried, bool ladderMotion, u64 hatMask = 0, u64 rollMask = 0, s8 speared = -1);

  // If Stephen's fork is lodged in a sausage, drag it rigidly (the speared-motion dispatch handler). Sets |handled| and
  // performs the drag; leaves |handled| false when nothing is speared so the step/turn classification can take over.
  // Drag Stephen and the sausage speared on his fork rigidly one cell in |dir| (any direction; his facing is
  // unchanged). The speared sausage is the one at the fork's cell; it translates with the fork and never rolls.
  bool HandleSpearedMotion(Direction dir, bool& handled);

  // Compute (without committing) the result of pushing sausage |sausageNo| one cell in |dir|. The push chains: a
  // sausage in the way is pushed the same direction too (recursively). |plan| accumulates every sausage that shifts --
  // plan.sausages holds their new states, plan.mask marks which indices moved. Each sausage slides along its long axis
  // or rolls across it (cooking is deferred to CookMoved). Returns false if any sausage in the chain would lodge in a
  // wall or topple into the void (|plan| may be left partially filled; commit only on success).
  bool PlanSausagePush(s8 sausageNo, Direction dir, MovePlan& plan, const Stephen* mover = nullptr) const;

  // Carry a sausage stacked on top of one that just moved (the old engine's "sausage carry", IsSausageCarried): it
  // shifts by the same horizontal displacement (so it stays on top) and rolls across its own long axis. Recurses for
  // sausages stacked on it. A carried sausage whose destination is a wall is left where it is (the parent move still
  // proceeds); the later Settle pass drops it if its support has gone. Motion only -- gravity and cooking come after.
  // Double-move is NOT decided here; the central MarkDoubleMoves detector resolves it post-hoc. A turn (plan.rotating) treats Stephen's body as a wall.
  bool PlanSausageCarry(s8 sausageNo, s8 dx, s8 dy, Direction dir, MovePlan& plan, const Stephen* mover = nullptr, bool rigid = false, bool baseDragRolled = false) const;

  // Stage 5 (gravity). Drop any disturbed sausage in |plan|'s working tableau whose ends have lost their support,
  // bottom-up to a fixed point. Only sausages this move touched -- those in |movedMask|, plus any that were stacked on
  // them in the pre-move layout |preMove| -- are subject to the fall, so a sausage that was already floating (a parked
  // decoy over the void) is left alone. Sausages flagged in |exclude| are skipped entirely (a pending double-move owns
  // its own fall). |movedMask| accumulates the sausages that fell (so they get cooked at their landing cell). A sausage
  // speared on the held fork is exempt. Returns false if a sausage falls out of the bottom of the world -- the move is
  // then refused (the plan is discarded uncommitted).
  // When |plan.airborne| is set, Stephen falls in the SAME fixed point (see StephenFallTick), so a sausage riding his
  // head or fork tracks him down as ordinary gravity rather than needing a bespoke vertical carry.
  bool Settle(MovePlan& plan, u64& movedMask, const Sausage* preMove, const Stephen& prevStephen, u64 exclude = 0);

  // Stage 5 applied to Stephen himself, one level per call, so his fall interleaves with the sausages' in Settle's fixed
  // point. His body drops while unsupported, dragging the fork and any sausage speared on it -- until that sausage
  // catches, when the body drops PAST it and the fork is left lodged there. An unspeared held fork doesn't ride his hand
  // down: it settles onto its own support, no lower than the body. |fell| reports whether anything moved. Returns false
  // if he drops out of the bottom of the world. A no-op unless |plan.airborne|.
  bool StephenFallTick(MovePlan& plan, bool& fell) const;

  // Central double-move DETECTOR -- the single place every path resolves double-move. Double-move is fundamentally
  // about ROLLING: a sausage left balanced across a perpendicular base that just rolled keeps tumbling one more cell.
  // Scan the just-moved tableau once against the pre-move layout |preMove|, inferring each sausage's own move direction
  // from its displacement (so one detector serves step, spear, log-roll, rotation and ladder alike, even when a plan
  // mixes push directions), and record every such rider into |plan|'s doubleMoveMask/doubleMoveDir. Every move path
  // calls this after its motion and before Settle to get uniform double-move handling.
  void MarkDoubleMoves(MovePlan& plan, const Sausage* preMove) const;

  // Stage 7 (double-move). A sausage left balanced across a perpendicular base that just rolled keeps tumbling one more
  // cell -- recorded (during a carry, or by MarkDoubleMoves) in |plan|'s doubleMoveMask/doubleMoveDir. Run that extra
  // tumble here, after the primary move has fully settled and cooked, as its own little motion -> settle -> cook on
  // |plan|'s tableau. Returns false if it burns or falls out of the world.
  bool DoubleMove(MovePlan& plan);

  // Shove sausage |sausageNo| (and whatever it rams, transitively) one cell by (dx,dy) on |plan|'s tableau, at its own
  // level -- the horizontal push a double-move's extra tumble makes when it laps onto a same-z neighbour, resolved
  // BEFORE gravity. |protect| pins
  // sausages that must never be pushed (the double-move's own group); pushed sausages are OR'd into |pushed|. Returns
  // false only when the chain wall-bottoms (the tumble is then stopped); a shove off the world is allowed here and left
  // for the following Settle to drown (which refuses the move).
  bool PushPlanned(MovePlan& plan, s8 sausageNo, s8 dx, s8 dy, u64 protect, u64& pushed) const;

  // Stage 6 (heat). Cook every sausage in |plan|'s tableau flagged in |movedMask| (those that moved or settled this
  // turn) at its resting cell, skipping any in |defer| -- a pending double-move browns at the end of its extra tumble,
  // not here. Returns false if a sausage would burn (an already-cooked face touching a grill again), refusing the move.
  bool CookMoved(MovePlan& plan, u64 movedMask, u64 defer = 0);

  // True if a sausage end resting at (x,y,z) has something solid under it, judged against |plan|'s working tableau:
  // terrain (ground floor or a wall-top), a sausage below in |plan.sausages|, or Stephen's body or held fork (at the
  // plan's post-move pose) directly beneath. Used by Settle to decide what floats, and by the log-roll ride to find
  // Stephen's footing as he drops.
  bool SausageSupported(const MovePlan& plan, s8 x, s8 y, s8 z) const;

  // Cook any of |sausage|'s ends that are resting on a grill, recording which face browned. Returns false if a face
  // that is already cooked would touch the grill again -- the sausage would burn.
  bool CookSausage(Sausage& sausage) const;

  // True if translating sausage |sausageNo| one cell in |dir| drives it (or its push chain) into a wall, as opposed to
  // merely pushing it out over the void. This exists only for the rigid speared sausage in HandleSpearedMotion, whose
  // advance can't go through PlanSausagePush (it must not roll): a wall-blocked advance unspears (the fork backs free),
  // a void one rides out. For an ordinary push, PlanSausagePush already refuses exactly on a wall, so no separate check
  // is needed.
  bool SausageBlocked(s8 sausageNo, Direction dir) const;
};
