#include "Level2.h"

bool Level2::Move(Direction dir) {
  bool handled = false;
  if (!HandleLogRolling(dir, handled)) return false;
  if (!handled) {
    if (!HandleLadderMotion(dir, handled)) return false;
    if (!handled) {
      if (!HandleSpearedMotion(dir, handled)) return false;
      if (!handled) {
        if (!HandleStepMotion(dir, handled)) return false;
        if (!handled) {
          if (!HandleRotation(dir, handled)) return false;
        }
      }
    }
  }

  // Can occur after (most) movements, so handle it commonly.
  if (!HandleBurnedStep(dir, handled)) return false;
  // TODO: HandleForkReconnect probably lives here?

  return true;
}

bool Level2::HandleBurnedStep(Direction dir, bool& handled) {
  // Stage 8 (outcomes): a body that comes to rest on a grill recoils straight back the way it came, as its own move.
  // Applied commonly so EVERY motion (step, ladder climb/descent, log-roll, spear) bounces consistently.
  if (!IsGrill(_stephen.x, _stephen.y, _stephen.z)) return true;
  handled = true;
  return Move(Inverse(dir));
}

bool Level2::HandleSpearedMotion(Direction dir, bool& handled) {
  // The fork lodged in a sausage means Stephen drags it rigidly; if nothing is speared, leave |handled| false so the
  // step/turn classification in Move() takes the press instead.
  // The speared sausage is whatever the fork is currently lodged in.
  s8 sausageNo = GetSausage(_stephen.forkX, _stephen.forkY, _stephen.forkZ);
  if (sausageNo == -1) return true;
  handled = true;

  s8 dx, dy;
  Delta(dir, dx, dy);
  s8 z = _stephen.z;
  s8 bodyX = _stephen.x + dx;
  s8 bodyY = _stephen.y + dy;

  // --- Validate / plan (nothing is mutated until the commit at the very end) ---

  // Stephen always needs footing wherever he steps.
  if (IsWall(bodyX, bodyY, z) || !CanWalkOnto(bodyX, bodyY, z)) return false;

  // The speared sausage rides along rigidly -- it translates with the fork and never rolls.
  Sausage moved = _sausages[sausageNo];
  s8 x1 = moved.x1 + dx;
  s8 y1 = moved.y1 + dy;
  s8 x2 = moved.x2 + dx;
  s8 y2 = moved.y2 + dy;

  // If the sausage can't follow -- an end, or a sausage further down its push chain, is pinned against a wall -- then
  // backing away unspears it (the fork pulls free, leaving the sausage put). Driving it into the block any other way
  // is refused.
  MovePlan plan = NewPlan();
  bool unspear = SausageBlocked(sausageNo, dir);
  if (unspear) {
    if (dir != Inverse(_stephen.dir)) return false;
    // Pulling free is a backward step, so the body still pushes any sausage at its destination (the speared one
    // stays put). If that sausage can't move, the move is refused.
    s8 bodySausage = GetSausage(bodyX, bodyY, z);
    if (bodySausage != -1 && bodySausage != sausageNo)
      if (!PlanSausagePush(bodySausage, dir, plan)) return false;
  } else {
    // The dragged sausage pushes any sausage it runs into the same way (a chain); those obey the normal push rules
    // (roll, need footing) and refuse the drag if one can't move. Mark this sausage moving first so the chain never
    // tries to push it back.
    plan.mask |= (1 << sausageNo);
    s8 ahead1 = GetSausage(x1, y1, z);
    if (ahead1 != -1 && ahead1 != sausageNo && !(plan.mask & (1 << ahead1)))
      if (!PlanSausagePush(ahead1, dir, plan)) return false;
    s8 ahead2 = GetSausage(x2, y2, z);
    if (ahead2 != -1 && ahead2 != sausageNo && ahead2 != ahead1 && !(plan.mask & (1 << ahead2)))
      if (!PlanSausagePush(ahead2, dir, plan)) return false;

    moved.x1 = x1;
    moved.y1 = y1;
    moved.x2 = x2;
    moved.y2 = y2;
    // The sausage needs no ground -- it's held up by the fork -- so it can ride out over the void. Cooking waits for
    // CookMoved at the commit, which browns it at the cell it ends on, like every other moved sausage.
    plan.sausages[sausageNo] = moved;

    // A sausage riding on top of the speared one is carried along the drag (and rolls across its own axis), just like
    // a normal push carries its riders. Use Stephen's post-move pose so the body/fork test sees where he ends up.
    {
      const Sausage& spearedSausage = _sausages[sausageNo];
      Stephen mover = _stephen;
      mover.x = bodyX;
      mover.y = bodyY;
      mover.forkX += dx;
      mover.forkY += dy;
      s8 aboveA = GetSausage(spearedSausage.x1, spearedSausage.y1, spearedSausage.z + 1);
      s8 aboveB = GetSausage(spearedSausage.x2, spearedSausage.y2, spearedSausage.z + 1);
      if (aboveA != -1 && aboveA != sausageNo && !(plan.mask & (1 << aboveA)))
        if (!PlanSausageCarry(aboveA, dx, dy, dir, plan, &mover, false)) return false;
      if (aboveB != -1 && aboveB != sausageNo && aboveB != aboveA && !(plan.mask & (1 << aboveB)))
        if (!PlanSausageCarry(aboveB, dx, dy, dir, plan, &mover, false)) return false;
    }

    // Stephen's body also shoulders aside any sausage standing where it steps, pushing it the same way.
    s8 bodySausage = GetSausage(bodyX, bodyY, z);
    if (bodySausage != -1 && bodySausage != sausageNo && !(plan.mask & (1 << bodySausage)))
      if (!PlanSausagePush(bodySausage, dir, plan)) return false;
  }

  // --- Commit --- Stepping onto a grill makes Stephen recoil to where he started. A backward unspear there just pulls
  // the fork free and nothing moves; otherwise the speared sausage lunged onto the grill and was yanked back, branded
  // -- a special case handled on its own.
  if (IsGrill(bodyX, bodyY, z)) {
    if (unspear) return true;
    return BounceSpearedSausageOffGrill(sausageNo, moved);
  }

  // Off a grill: body and fork shift and the speared sausage (plus anything it pushed) rides along; a backward unspear
  // may instead have pushed a sausage out of the body's way. Gravity then settles, and heat browns everything that
  // moved -- the speared sausage included, at the cell the fork dragged it to (it rides the fork, so gravity skips it).
  plan.stephen.x = bodyX;
  plan.stephen.y = bodyY;
  plan.stephen.forkX += dx;
  plan.stephen.forkY += dy;

  // Stages 5-7 (gravity -> heat -> knock-on) + the single commit, all on |plan|. Nothing in the live state has moved
  // yet, so _sausages/_stephen ARE the pre-move layout the gravity pass needs -- pass them straight in (no snapshot).
  u16 movedMask = plan.mask;
  if (!Settle(plan, movedMask, _sausages.begin(), _stephen, plan.doubleMoveMask)) return false;
  if (!CookMoved(plan, movedMask, plan.doubleMoveMask)) return false;
  if (!DoubleMove(plan)) return false;
  Commit(plan);
  return true;
}

bool Level2::BounceSpearedSausageOffGrill(s8 sausageNo, Sausage lunged) {
  // Stephen recoiled off the grill, so the speared sausage keeps its original cell -- but it lunged onto the grill on
  // the way out and back, so it earns whatever brand that excursion cooks in. Cook the lunged pose, then leave the
  // sausage where it started with only those flags updated.
  if (!CookSausage(lunged)) return false; // a cooked face hitting the grill again would burn
  _sausages[sausageNo].flags = lunged.flags;
  return true;
}

bool Level2::HandleLogRolling(Direction dir, bool& handled) {
  // Log rolling only applies when Stephen is standing directly on top of a sausage.
  s8 onSausage = GetSausage(_stephen.x, _stephen.y, _stephen.z - 1);
  if (onSausage == -1) return true;

  // It fires only when he walks ACROSS the sausage's long axis (and is already facing along that press axis). The
  // sausage spins backward underfoot, so it -- and Stephen with it -- rolls one cell the OPPOSITE way to the press.
  Sausage sausage = _sausages[onSausage];
  bool across = (sausage.IsHorizontal() && (dir == Up || dir == Down) && (_stephen.dir == Up || _stephen.dir == Down))
             || (sausage.IsVertical()   && (dir == Left || dir == Right) && (_stephen.dir == Left || _stephen.dir == Right));
  if (!across) return true; // a press along the axis is just an ordinary step -- let the pipeline handle it
  Direction roll = Inverse(dir);

  // A sausage already speared on the fork rides along with Stephen; capture it before any motion so the ride can drag
  // it rigidly (3-8 Cold Head: he log-rolls the sausage underfoot while a second sausage is lodged on his fork).
  s8 speared = _stephen.HasFork() ? GetSausage(_stephen.forkX, _stephen.forkY, _stephen.forkZ) : -1;

  // Roll the sausage (and anything it pushes) one cell. If it can't roll there -- a wall, or it would fall out of the
  // world -- this isn't a legal log roll; fall through so the rest of the pipeline can refuse or reinterpret the press.
  MovePlan plan = NewPlan();
  if (!PlanSausagePush(onSausage, roll, plan)) return true;

  // Stephen rides the roll: his body and fork translate one cell with the sausage. Neither may ride into a wall -- the
  // reference moves him via MoveStephenThroughSpace, whose fork/body passes refuse a wall destination (3-12 Cold
  // Horizon: the fork would ride into a Wall2). Once the sausage can roll, a blocked ride refuses the whole move.
  s8 dx, dy;
  Delta(roll, dx, dy);
  s8 newBodyX = _stephen.x + dx, newBodyY = _stephen.y + dy;
  s8 newForkX = _stephen.forkX + dx, newForkY = _stephen.forkY + dy;
  if (IsWall(newBodyX, newBodyY, _stephen.z) || IsWall(newForkX, newForkY, _stephen.forkZ)) return false;

  // That same ride shoulders any sausage standing where his fork or body lands, pushing it the roll direction (3-8 Cold
  // Head: the body rides into a sausage and shoves it clear). Use his post-ride pose so a rider is judged correctly.
  Stephen mover = _stephen;
  mover.x = newBodyX; mover.y = newBodyY;
  mover.forkX = newForkX; mover.forkY = newForkY;
  s8 forkDest = GetSausage(newForkX, newForkY, _stephen.forkZ);
  if (forkDest != -1 && forkDest != onSausage && forkDest != speared && !(plan.mask & (1 << forkDest))) {
    // The fork shoves the sausage in its path -- unless a wall blocks the shove, in which case the fork SPEARS it
    // instead, riding into its cell and lodging there (3-8 Cold Head: the fork rides into a sausage braced on a Wall2).
    // PlanSausagePush refuses (leaving the plan untouched) exactly when the shove hits a wall, which is the spear case,
    // so a refused push simply falls through to the implicit spear.
    PlanSausagePush(forkDest, roll, plan, &mover);
  }
  s8 bodyDest = GetSausage(newBodyX, newBodyY, _stephen.z);
  if (bodyDest != -1 && bodyDest != onSausage && bodyDest != speared && !(plan.mask & (1 << bodyDest)))
    if (!PlanSausagePush(bodyDest, roll, plan, &mover)) return false;

  // --- Commit --- the sausage rolls (with anything it pushes), gravity settles it, then Stephen rides on top and drops.
  // Nothing in the live state has moved yet, so _sausages/_stephen ARE the pre-move layout Settle needs.
  u16 movedMask = plan.mask;
  if (!Settle(plan, movedMask, _sausages.begin(), _stephen, plan.doubleMoveMask)) return false;

  // A sausage speared on the fork rides rigidly with Stephen (it never rolls). A wall in its path pulls the fork free
  // only when the roll runs opposite his facing (an unspear); any other blocked drag refuses the move.
  if (speared != -1) {
    Sausage& spearedSausage = plan.sausages[speared];
    if (IsWall(spearedSausage.x1 + dx, spearedSausage.y1 + dy, spearedSausage.z) || IsWall(spearedSausage.x2 + dx, spearedSausage.y2 + dy, spearedSausage.z)) {
      if (roll != Inverse(_stephen.dir)) return false;
      speared = -1; // unspear: the fork pulls free and the sausage stays put
    } else {
      spearedSausage.x1 += dx; spearedSausage.y1 += dy; spearedSausage.x2 += dx; spearedSausage.y2 += dy;
      movedMask |= (1 << speared);
    }
  }
  plan.stephen.x += dx;
  plan.stephen.y += dy;
  plan.stephen.forkX += dx;
  plan.stephen.forkY += dy;
  while (!SausageSupported(plan, plan.stephen.x, plan.stephen.y, plan.stephen.z)) {
    if (plan.stephen.z <= 0) return false;
    plan.stephen.z--;
    plan.stephen.forkZ--;
    if (speared != -1) plan.sausages[speared].z--; // the speared sausage rides down with the fork
  }
  if (!CookMoved(plan, movedMask, plan.doubleMoveMask)) return false;
  if (!DoubleMove(plan)) return false;
  Commit(plan);
  handled = true;
  return true;
}

bool Level2::HandleLadderMotion(Direction dir, bool& handled) {
  // With a fork, a vertical (Up/Down) ladder is mounted by pressing up/down while facing sideways, and a horizontal
  // (Left/Right) ladder while facing up/down. Forkless, Stephen simply climbs the ladder he faces along.
  bool climb;
  if (_stephen.HasFork()) {
    if (dir == Up || dir == Down) climb = (_stephen.dir == Left || _stephen.dir == Right);
    else                          climb = (_stephen.dir == Up || _stephen.dir == Down);
  } else {
    climb = (_stephen.dir == dir || _stephen.dir == Inverse(dir));
  }
  if (!climb) return true;

  s8 speared = GetSausage(_stephen.forkX, _stephen.forkY, _stephen.forkZ);
  // A sausage riding on Stephen's HEAD rides with him up or down the ladder (3-14 Cold Frustration m96/m98).
  s8 headHat = GetSausage(_stephen.x, _stephen.y, _stephen.z + 1);

  // Climb up: while a ladder facing the press direction is in our cell, rise a rung; then step off its top in |dir|.
  if (IsLadder(_stephen.x, _stephen.y, _stephen.z, dir)) {
    // The fork carries whatever sits on it up the ladder: a speared sausage (lodged at the fork's level), or -- if the
    // fork is empty -- a sausage resting directly on top of the fork, which the rising fork lifts along (3-7 Cold Plateau).
    s8 carried = speared != -1 ? speared : GetSausage(_stephen.forkX, _stephen.forkY, _stephen.forkZ + 1);
    u16 carriedMask = CarriedMask(carried) | CarriedMask(headHat);
    while (IsLadder(_stephen.x, _stephen.y, _stephen.z, dir))
      if (!LiftStephen(+1, carriedMask)) return false;
    if (!StepOffLadder(dir, carriedMask, false)) return false;
    handled = true;
    return true;
  }

  // Descend: the cell ahead has no footing at our level, but a back-facing ladder one level down leads to a surface.
  s8 dx, dy;
  Delta(dir, dx, dy);
  s8 ax = _stephen.x + dx, ay = _stephen.y + dy;
  if (CanWalkOnto(ax, ay, _stephen.z)) return true;                  // there's a ledge ahead -- ordinary walking handles it
  if (!IsLadder(ax, ay, _stephen.z - 1, Inverse(dir))) return true;  // nothing to climb down -- fall through
  u16 carriedMask = CarriedMask(speared) | CarriedMask(headHat);
  if (!StepOffLadder(dir, carriedMask, true)) return false;             // step out over the ladder (hanging, no footing yet)
  while (true) {
    if (_stephen.z <= 0) return false;
    if (!LiftStephen(-1, carriedMask)) return false;
    if (CanWalkOnto(_stephen.x, _stephen.y, _stephen.z)) break;     // reached a surface -- step off
    if (!IsLadder(_stephen.x, _stephen.y, _stephen.z - 1, Inverse(dir))) return false; // ladder ran out mid-air
  }
  handled = true;
  return true;
}

bool Level2::LiftStephen(s8 dz, u16 carried) {
  // The fork can't ram into a wall, nor crouch/rise into a sausage it isn't carrying (a sausage resting on solid
  // ground won't yield, so the climb is blocked).
  s8 newForkZ = _stephen.forkZ + dz;
  if (IsWall(_stephen.forkX, _stephen.forkY, newForkZ)) return false;
  s8 blocking = GetSausage(_stephen.forkX, _stephen.forkY, newForkZ);
  if (blocking != -1 && !(carried & (1 << blocking))) return false;

  _stephen.z += dz;
  _stephen.forkZ += dz;
  for (int i = 0; i < _sausages.Size(); i++)
    if (carried & (1 << i)) _sausages[i].z += dz;
  return true;
}

u16 Level2::CarriedMask(s8 base) const {
  if (base == -1) return 0;
  u16 mask = (1 << base);
  bool grew = true;
  while (grew) {
    grew = false;
    for (int i = 0; i < _sausages.Size(); i++) {
      if (mask & (1 << i)) continue;
      const Sausage& s = _sausages[i];
      s8 belowA = GetSausage(s.x1, s.y1, s.z - 1), belowB = GetSausage(s.x2, s.y2, s.z - 1);
      if ((belowA != -1 && (mask & (1 << belowA))) || (belowB != -1 && (mask & (1 << belowB)))) { mask |= (1 << i); grew = true; }
    }
  }
  return mask;
}

bool Level2::StepOffLadder(Direction dir, u16 carried, bool ladderMotion) {
  s8 dx, dy;
  Delta(dir, dx, dy);
  s8 bodyX = _stephen.x + dx, bodyY = _stephen.y + dy, bodyZ = _stephen.z;
  s8 forkX = _stephen.forkX + dx, forkY = _stephen.forkY + dy, forkZ = _stephen.forkZ;

  // Neither body nor fork may step into a wall. Stepping OFF a ladder top needs real footing for the body; stepping OUT
  // over a ladder to begin a descent lets the body hang (the crouch that follows lowers him to a surface).
  if (IsWall(bodyX, bodyY, bodyZ) || IsWall(forkX, forkY, forkZ)) return false;
  if (!ladderMotion && !CanWalkOnto(bodyX, bodyY, bodyZ)) return false;

  // The fork (and body) shove any sausage standing where they land -- everything but the stack Stephen is carrying. Use
  // Stephen's post-step pose so a rider on a pushed sausage is judged against where he ends up.
  MovePlan plan = NewPlan();
  Stephen mover = _stephen;
  mover.x = bodyX; mover.y = bodyY;
  mover.forkX = forkX; mover.forkY = forkY;
  s8 forkSausage = GetSausage(forkX, forkY, forkZ);
  if (forkSausage != -1 && !(carried & (1 << forkSausage)))
    if (!PlanSausagePush(forkSausage, dir, plan, &mover)) return false;
  s8 bodySausage = GetSausage(bodyX, bodyY, bodyZ);
  if (bodySausage != -1 && !(carried & (1 << bodySausage)) && !(plan.mask & (1 << bodySausage)))
    if (!PlanSausagePush(bodySausage, dir, plan, &mover)) return false;

  // --- Commit --- pushed sausages land in |plan|, Stephen (and the stack he carries) translates rigidly, then settle + cook.
  plan.stephen.x = bodyX;
  plan.stephen.y = bodyY;
  plan.stephen.forkX = forkX;
  plan.stephen.forkY = forkY;
  for (int i = 0; i < _sausages.Size(); i++) {
    if (!(carried & (1 << i))) continue;
    Sausage& cs = plan.sausages[i];
    cs.x1 += dx; cs.y1 += dy; cs.x2 += dx; cs.y2 += dy;
  }
  // Nothing in the live state has moved yet, so _sausages/_stephen ARE the pre-move layout Settle needs.
  u16 movedMask = plan.mask;
  if (!Settle(plan, movedMask, _sausages.begin(), _stephen)) return false;
  if (!CookMoved(plan, movedMask)) return false;
  Commit(plan);
  return true;
}

bool Level2::HandleStepMotion(Direction dir, bool& handled) {
  // A press along Stephen's facing axis is an ordinary walk (forward or backward); a perpendicular press is a turn, so
  // leave |handled| false and let HandleRotation take it.
  if (dir != _stephen.dir && dir != Inverse(_stephen.dir)) return true;
  handled = true;

  s8 dx, dy;
  Delta(dir, dx, dy);
  s8 bodyX = _stephen.x + dx;
  s8 bodyY = _stephen.y + dy;
  s8 forkX = _stephen.forkX + dx;
  s8 forkY = _stephen.forkY + dy;
  s8 z = _stephen.z;
  bool forward = (dir == _stephen.dir);

  // --- Validate / plan (nothing is mutated until the commit at the very end) ---

  // The fork leads the way: it may hang out over the void, but it cannot bury itself in a wall.
  if (IsWall(forkX, forkY, z)) return false;
  // The body cannot walk into a wall, and it needs solid footing wherever it lands
  // (the void at the edge of the playfield is not footing).
  if (IsWall(bodyX, bodyY, z)) return false;
  if (!CanWalkOnto(bodyX, bodyY, z)) return false;

  // The leading cell -- the fork's when walking forward, the body's when backing up -- may hold a sausage that the
  // step pushes one cell along. Plan the push now; commit it only at the end.
  s8 sausageNo = GetSausage(forward ? forkX : bodyX, forward ? forkY : bodyY, z);
  MovePlan plan = NewPlan();
  bool spear = false;
  // Where Stephen's body and fork will END UP. A sausage carried by the push checks against this final pose to see
  // whether his body or fork ends up holding it (which catches a falling sausage and cancels a double-move).
  Stephen mover = _stephen;
  mover.x = bodyX; mover.y = bodyY;
  mover.forkX = forkX; mover.forkY = forkY;
  // A successful push records itself in |plan|; on refusal we either spear or reject.
  if (sausageNo != -1 && !PlanSausagePush(sausageNo, dir, plan, &mover)) {
    if (forward) {
      // PlanSausagePush only refuses when the push chain bottoms out against a wall (a push over the void succeeds and
      // the sausage falls later, in Settle). Walking FORWARD into such a wall-blocked sausage spears it: the fork lodges
      // in the sausage. Normally the sausage stays put and Stephen steps up onto its old cell -- but if his body would
      // bounce off a grill (below), the recoil drags the freshly-speared sausage back onto the grill instead.
      spear = true;
    } else {
      // Backing into a wall-blocked sausage is refused.
      return false;
    }
  }

  // Stepping onto a grill bounces Stephen straight back to where he started; any push or spear still stands.
  bool bodyOnGrill = IsGrill(bodyX, bodyY, z);

  // A spear whose step bounces off a grill leaves the fork lodged in the sausage as it recoils, dragging the sausage
  // one cell back (opposite the press) onto the fork's grill cell, where the landing face browns. Plan it now so a
  // would-be burn refuses the whole move without mutating anything.
  Sausage spearDragged;
  bool spearDrag = false;
  if (spear && bodyOnGrill) {
    spearDragged = _sausages[sausageNo];
    spearDragged.x1 -= dx;
    spearDragged.y1 -= dy;
    spearDragged.x2 -= dx;
    spearDragged.y2 -= dy;
    if (!CookSausage(spearDragged)) return false;
    spearDrag = true;

    // The recoil drags the freshly-speared sausage one cell back onto the fork's grill; any sausage riding on it comes
    // along by that same displacement (a rigid drag -- no roll, no double-move). Mark the base moved so the carry
    // doesn't treat it as a stationary anchor, and carry from the base's ORIGINAL footprint. Stephen stays put, so the
    // carry judges support against his current pose (mover = nullptr -> live _stephen). (3-14 Cold Frustration m86.)
    plan.mask |= (1 << sausageNo);
    const Sausage& base = _sausages[sausageNo];
    s8 riderA = GetSausage(base.x1, base.y1, base.z + 1);
    s8 riderB = GetSausage(base.x2, base.y2, base.z + 1);
    if (riderA != -1 && riderA != sausageNo && !(plan.mask & (1 << riderA)))
      PlanSausageCarry(riderA, -dx, -dy, Inverse(dir), plan, nullptr, false);
    if (riderB != -1 && riderB != sausageNo && riderB != riderA && !(plan.mask & (1 << riderB)))
      PlanSausageCarry(riderB, -dx, -dy, Inverse(dir), plan, nullptr, false);
  }

  // --- Commit --- everything is planned into |plan|; the live state moves only when ReactAndCommit succeeds.
  s8 headHat = GetSausage(_stephen.x, _stephen.y, z + 1);
  s8 forkHat = GetSausage(_stephen.forkX, _stephen.forkY, z + 1);
  if (spearDrag) plan.sausages[sausageNo] = spearDragged;
  if (!bodyOnGrill) {
    plan.stephen.x = bodyX;
    plan.stephen.y = bodyY;
    plan.stephen.forkX = forkX;
    plan.stephen.forkY = forkY;
    // A sausage on Stephen's HEAD is a hat: it rides rigidly and never rolls (the reference exempts data.sausageHat).
    // One resting on the FORK, though, rolls when the step carries it across its long axis (3-7 Cold Plateau m50).
    // A rider stacked on either inherits that base's roll behavior (the stack is rigid): head-hat stacks never roll.
    PlanHatCarry(headHat, dx, dy, dir, plan, false);
    PlanHatCarry(forkHat, dx, dy, dir, plan, true);
  }

  // Stages 5-7 (gravity -> heat -> knock-on) + the single commit, all on |plan|. Nothing in the live state has moved
  // yet, so _sausages/_stephen ARE the pre-move layout the gravity pass needs -- pass them straight in (no snapshot).
  u16 movedMask = plan.mask;
  // The spear-dragged base already browned on the fork's grill (above); keep CookMoved from re-cooking (and burning) it.
  u16 preCooked = plan.doubleMoveMask | (spearDrag ? (u16)(1 << sausageNo) : 0);
  if (!Settle(plan, movedMask, _sausages.begin(), _stephen, plan.doubleMoveMask)) return false;
  if (!CookMoved(plan, movedMask, preCooked)) return false;
  if (!DoubleMove(plan)) return false;
  Commit(plan);
  return true;
}

bool Level2::HandleRotation(Direction dir, bool& handled) {
  // A press perpendicular to Stephen's facing turns him in place; a press along his facing axis is a walk, so leave
  // |handled| false and let HandleStepMotion take it.
  if (dir == _stephen.dir || dir == Inverse(_stephen.dir)) return true;
  handled = true;

  MovePlan plan = NewPlan();
  plan.rotating = true;
  // The fork swings from its current cell to the perpendicular one; the corner it sweeps through is the diagonal
  // between them -- the fork's destination, offset back along the old facing.
  s8 dx, dy;
  Delta(dir, dx, dy);
  s8 forkX = _stephen.x + dx;
  s8 forkY = _stephen.y + dy;

  Delta(_stephen.dir, dx, dy);
  s8 cornerX = forkX + dx;
  s8 cornerY = forkY + dy;

  s8 z = _stephen.z;

  // --- Validate / plan (nothing is mutated until the commit at the very end) ---

  // A rider carried by a pushed sausage checks whether Stephen's body/fork ends up beneath it (the rotation-hold guard
  // in PlanSausageCarry). During a turn the fork ends at its swung-to cell, so seed the carry checks with that POST-turn
  // pose -- otherwise a sausage the fork swings directly under is wrongly carried off its new support instead of being
  // left resting on the fork (3-11 Cold Terrace move 69: the fork swings under a cantilevered sausage).
  Stephen mover = _stephen;
  mover.dir = dir;
  mover.forkX = forkX;
  mover.forkY = forkY;

  // A wall in the corner blocks the swing outright;
  // a sausage there must be pushable (along the new facing) or the turn is refused.
  if (IsWall(cornerX, cornerY, z)) return false;
  s8 cornerSausage = GetSausage(cornerX, cornerY, z);
  if (cornerSausage != -1) {
    if (!PlanSausagePush(cornerSausage, dir, plan)) return false;
  }
  u16 cornerMask = plan.mask; // the corner push lands even when Stephen bonks instead of turning

  // At the fork's destination, a sausage is pushed away from where the fork came from (the inverse of the old facing).
  // A wall-blocked sausage (or a bare wall) bonks -- the turn is abandoned but the corner push stands; a sausage pushed
  // out over the void rides out and is dropped (the move refused if it falls out of the world) by the later Settle.
  bool bonk = false;
  s8 forkSausage = GetSausage(forkX, forkY, z);
  if (forkSausage != -1 && !(plan.mask & (1 << forkSausage))) {
    // (If the corner push already swept this sausage along, that push covered it.) PlanSausagePush refuses only when the
    // fork-dest sausage is wall-blocked, so a refused push here means the turn bonks: keep the corner push, drop this chain.
    if (!PlanSausagePush(forkSausage, Inverse(_stephen.dir), plan, &mover)) {
      bonk = true;
      plan.mask = cornerMask; // keep only the corner push
    }
  } else if (forkSausage == -1 && IsWall(forkX, forkY, z)) {
    bonk = true;
  }

  // A sausage sitting on Stephen's head pivots 90 degrees with him, swinging about the head cell -- but only a "clean
  // hat" (the far half cantilevered over open space). If the far half rests on another sausage it stays put (carried).
  // The whole stack above it pivots too. Plan it now; it lands only if Stephen actually turns (no bonk).
  u16 hatMask = 0;
  if (!bonk) PlanHatRotation(dir, plan, hatMask);

  // --- Commit --- the planned pushes are in |plan|; Stephen swings to his new facing only if he didn't bonk.
  if (!bonk) {
    plan.stephen.dir = dir;
    plan.stephen.forkX = forkX;
    plan.stephen.forkY = forkY;
  }

  // Stages 5-7 (gravity -> heat -> knock-on) + the single commit, all on |plan|. Nothing in the live state has moved
  // yet, so _sausages/_stephen ARE the pre-move layout the gravity pass needs -- pass them straight in (no snapshot).
  u16 movedMask = plan.mask;
  if (!Settle(plan, movedMask, _sausages.begin(), _stephen, plan.doubleMoveMask)) return false;
  if (!CookMoved(plan, movedMask, plan.doubleMoveMask)) return false;
  if (!DoubleMove(plan)) return false;
  Commit(plan);
  return true;
}

void Level2::PlanHatRotation(Direction dir, MovePlan& plan, u16& hatMask) const {
  s8 headX = _stephen.x, headY = _stephen.y, z = _stephen.z;
  s8 hatNo = GetSausage(headX, headY, z + 1);
  if (hatNo == -1) return;

  // Only a "clean hat" pivots: the far half (the end not on the head) must be cantilevered over open space, not
  // resting on another sausage -- in which case it's held in place (carried) instead.
  {
    const Sausage& hat = _sausages[hatNo];
    bool firstOnHead = (hat.x1 == headX && hat.y1 == headY);
    s8 farX = firstOnHead ? hat.x2 : hat.x1;
    s8 farY = firstOnHead ? hat.y2 : hat.y1;
    if (GetSausage(farX, farY, z) != -1) return;
  }

  // Rotation taking the old facing onto the new one; apply it to every end's offset about the head cell. The whole
  // stack on the head pivots together.
  s8 odx, ody, ndx, ndy;
  Delta(_stephen.dir, odx, ody);
  Delta(dir, ndx, ndy);
  bool cw = (odx * ndy - ody * ndx) > 0;
  for (s8 no = hatNo, sz = z + 1; no != -1 && !(plan.mask & (1 << no)); no = GetSausage(headX, headY, ++sz)) {
    Sausage s = _sausages[no];
    s8 ox1 = s.x1, oy1 = s.y1, ox2 = s.x2, oy2 = s.y2;
    auto rot = [&](s8& x, s8& y) { s8 ox = x - headX, oy = y - headY; if (cw) { x = headX - oy; y = headY + ox; } else { x = headX + oy; y = headY - ox; } };
    rot(s.x1, s.y1);
    rot(s.x2, s.y2);
    // A wall at a destination cell blocks the swing; so does one in the corner an end sweeps through (the cell diagonally
    // between its old and new spot = old + new - head). Either way the whole hat stays put. (3-14 Cold Frustration m101,
    // where the far half would sweep through a 2-tall Wall2's upper cell.)
    if (IsWall(s.x1, s.y1, s.z) || IsWall(s.x2, s.y2, s.z)
     || IsWall(ox1 + s.x1 - headX, oy1 + s.y1 - headY, s.z)
     || IsWall(ox2 + s.x2 - headX, oy2 + s.y2 - headY, s.z)) return; // swing blocked -> leave the hat put
    if (s.x1 > s.x2 || (s.x1 == s.x2 && s.y1 > s.y2)) { // restore upper-left; physical halves now swap slots
      s8 tx = s.x1; s.x1 = s.x2; s.x2 = tx;
      s8 ty = s.y1; s.y1 = s.y2; s.y2 = ty;
      u8 cook1 = s.flags & Sausage::Cook1, cook2 = s.flags & Sausage::Cook2;
      s.flags = (s.flags & ~(u8)Sausage::FullyCooked) | (u8)(cook1 << 2) | (u8)(cook2 >> 2);
      s.flags ^= Sausage::Swapped; // the two halves exchanged slots
    }
    plan.sausages[no] = s;
    plan.mask |= (1 << no);
    hatMask |= (1 << no);
  }
}

void Level2::PlanHatCarry(s8 sausageNo, s8 dx, s8 dy, Direction dir, MovePlan& plan, bool rolls) const {
  // A sausage riding on Stephen's head/fork translates with him. A head hat never rolls; a fork-borne sausage (and any
  // rider) does roll when carried across its long axis -- the caller passes |rolls| accordingly. A wall in its path
  // leaves it behind; otherwise it shifts and drags any sausage stacked on it the same way. But if its other (non-head)
  // end rests on a non-moving sausage OR on wall terrain it's anchored and stays put -- only the clean hat rides.
  if (sausageNo == -1 || (plan.mask & (1 << sausageNo))) return;
  Sausage s = _sausages[sausageNo];
  s8 belowA = GetSausage(s.x1, s.y1, s.z - 1);
  s8 belowB = GetSausage(s.x2, s.y2, s.z - 1);
  // Anchored if its far end rests on a NON-MOVING support -- a stationary sausage below, or wall terrain (mirrors the
  // reference's IsSausageCarried IsWall(otherEnd, z) check). The support end (on Stephen's head/fork) never trips the
  // wall test: its z-1 is Stephen's own occupied level, which is never a wall -- so checking both ends is safe.
  if ((belowA != -1 && belowA != sausageNo && !(plan.mask & (1 << belowA)))
   || (belowB != -1 && belowB != sausageNo && !(plan.mask & (1 << belowB)))
   || IsWall(s.x1, s.y1, s.z - 1) || IsWall(s.x2, s.y2, s.z - 1)) return; // anchored on a non-moving sausage or wall
  if (IsWall(s.x1 + dx, s.y1 + dy, s.z) || IsWall(s.x2 + dx, s.y2 + dy, s.z)) return;
  s8 aboveA = GetSausage(s.x1, s.y1, s.z + 1);
  s8 aboveB = GetSausage(s.x2, s.y2, s.z + 1);
  // Another sausage standing where it shifts gets pushed the same way first.
  s8 destA = GetSausage(s.x1 + dx, s.y1 + dy, s.z);
  if (destA != -1 && destA != sausageNo && !(plan.mask & (1 << destA))) PlanSausagePush(destA, dir, plan);
  s8 destB = GetSausage(s.x2 + dx, s.y2 + dy, s.z);
  if (destB != -1 && destB != sausageNo && destB != destA && !(plan.mask & (1 << destB))) PlanSausagePush(destB, dir, plan);
  s.x1 += dx; s.y1 += dy; s.x2 += dx; s.y2 += dy;
  if (rolls && (s.IsHorizontal() ? (dy != 0) : (dx != 0))) s.flags ^= Sausage::Rolled; // a carry across the long axis rolls
  plan.sausages[sausageNo] = s;
  plan.mask |= (1 << sausageNo);
  // A rider is part of the same rigid stack, so it inherits this base's roll behavior: riders on a head hat ride
  // without rolling (the whole hat is rigid), while riders on a fork-borne base roll with it across their own axis.
  PlanHatCarry(aboveA, dx, dy, dir, plan, rolls);
  if (aboveB != aboveA) PlanHatCarry(aboveB, dx, dy, dir, plan, rolls);
}

bool Level2::PlanSausagePush(s8 sausageNo, Direction dir, MovePlan& plan, const Stephen* mover) const {
  Sausage sausage = _sausages[sausageNo];
  s8 dx, dy;
  Delta(dir, dx, dy);
  s8 z = sausage.z;
  s8 x1 = sausage.x1 + dx;
  s8 y1 = sausage.y1 + dy;
  s8 x2 = sausage.x2 + dx;
  s8 y2 = sausage.y2 + dy;

  // A sausage cannot be pushed into a wall.
  if (IsWall(x1, y1, z) || IsWall(x2, y2, z)) return false;

  // Push chain: another sausage standing where an end is headed gets pushed the same way first; if it can't move, the
  // whole chain is refused. (Skip our own other end -- a slide along the axis -- and anything already pushed.)
  s8 ahead1 = GetSausage(x1, y1, z);
  if (ahead1 != -1 && ahead1 != sausageNo && !(plan.mask & (1 << ahead1)))
    if (!PlanSausagePush(ahead1, dir, plan, mover)) return false;
  s8 ahead2 = GetSausage(x2, y2, z);
  if (ahead2 != -1 && ahead2 != sausageNo && !(plan.mask & (1 << ahead2)))
    if (!PlanSausagePush(ahead2, dir, plan, mover)) return false;

  // A push across the sausage's long axis rolls it (flipping which side faces down);
  // a push along the axis just slides it. This is motion only -- gravity (Settle) and heat (CookMoved) run after the
  // whole move is committed, so the sausage keeps its level here even if it's now cantilevered over a drop.
  bool rolls = sausage.IsHorizontal() ? (dir == Up || dir == Down) : (dir == Left || dir == Right);
  sausage.x1 = x1;
  sausage.y1 = y1;
  sausage.x2 = x2;
  sausage.y2 = y2;
  if (rolls) sausage.flags ^= Sausage::Rolled;
  plan.sausages[sausageNo] = sausage;
  plan.mask |= (1 << sausageNo);

  // A sausage stacked directly on top is carried along: it follows by the same displacement (so it stays on top of
  // us) and rolls across its own axis. Look at our ORIGINAL footprint, since plan.sausages now holds our new spot.
  const Sausage& orig = _sausages[sausageNo];
  s8 aboveA = GetSausage(orig.x1, orig.y1, orig.z + 1);
  s8 aboveB = GetSausage(orig.x2, orig.y2, orig.z + 1);
  if (aboveA != -1 && aboveA != sausageNo && !(plan.mask & (1 << aboveA)))
    if (!PlanSausageCarry(aboveA, dx, dy, dir, plan, mover, rolls)) return false;
  if (aboveB != -1 && aboveB != sausageNo && aboveB != aboveA && !(plan.mask & (1 << aboveB)))
    if (!PlanSausageCarry(aboveB, dx, dy, dir, plan, mover, rolls)) return false;
  return true;
}

bool Level2::PlanSausageCarry(s8 sausageNo, s8 dx, s8 dy, Direction dir, MovePlan& plan, const Stephen* mover, bool baseRolled) const {
  const Sausage& orig = _sausages[sausageNo];
  const Stephen& man = mover ? *mover : _stephen;

  // Stationary support -- terrain (a ground floor or a wall-top) or a non-moving sausage. This kind of support keeps a
  // sausage from being carried at all: the base slides out from under it and it stays exactly where it is.
  auto anchoredAt = [&](s8 cellX, s8 cellY, s8 cellZ) -> bool {
    if (cellZ < 0 || IsWall(cellX, cellY, cellZ)) return false;
    if (cellZ == 0 ? CanWalkOnto(cellX, cellY, 0) : IsWall(cellX, cellY, cellZ - 1)) return true;
    s8 below = GetSausage(cellX, cellY, cellZ - 1);
    return below != -1 && below != sausageNo && !(plan.mask & (1 << below));
  };
  if (anchoredAt(orig.x1, orig.y1, orig.z) || anchoredAt(orig.x2, orig.y2, orig.z)) return true; // anchored -> stays put

  // During a TURN, Stephen's body OR fork counts as a wall in its FINAL cell (mirrors the reference IsSausageCarried
  // rotating branch). A rider end resting on his body is held (3-3 Cold Escarpment move 69). A rider end over the fork
  // is held too -- but if a sausage sits in that fork cell, the fork only takes over once that base slides/rolls OUT of
  // it (forkLeftBehind) AND the rider's OTHER end isn't itself riding a moving sausage; otherwise the rider is carried
  // along with its base instead. 3-11 Cold Terrace move 69 holds (its far end cantilevers over open space); 3-2 Cold
  // Finger move 12 carries (its far end rides the moving base).
  if (plan.rotating && man.HasFork()) {
    auto bodyUnder = [&](s8 cellX, s8 cellY, s8 cellZ) -> bool {
      return man.x == cellX && man.y == cellY && man.z == cellZ - 1;
    };
    auto forkUnder = [&](s8 cellX, s8 cellY, s8 cellZ) -> bool {
      return man.forkX == cellX && man.forkY == cellY && man.forkZ == cellZ - 1;
    };
    auto forkHolds = [&](s8 ex, s8 ey, s8 ox, s8 oy) -> bool {
      if (!forkUnder(ex, ey, orig.z)) return false;
      s8 base = GetSausage(ex, ey, orig.z - 1);            // pre-move sausage in the fork's cell
      if (base == -1) return true;                          // bare fork -> it alone holds the rider
      bool leftBehind = (plan.mask & (1 << base)) && !_sausages[base].IsAt(ex - dx, ey - dy, orig.z - 1);
      s8 otherBase = GetSausage(ox, oy, orig.z - 1);        // pre-move sausage under the rider's OTHER end
      bool otherOnMoving = otherBase != -1 && (plan.mask & (1 << otherBase));
      return leftBehind && !otherOnMoving;
    };
    if (bodyUnder(orig.x1, orig.y1, orig.z) || bodyUnder(orig.x2, orig.y2, orig.z)) return true;
    if (forkHolds(orig.x1, orig.y1, orig.x2, orig.y2) || forkHolds(orig.x2, orig.y2, orig.x1, orig.y1)) return true;
  }

  s8 aboveA = GetSausage(orig.x1, orig.y1, orig.z + 1);
  s8 aboveB = GetSausage(orig.x2, orig.y2, orig.z + 1);

  // A carried sausage aligned with the motion and resting on a PERPENDICULAR base double-moves (slides two cells) --
  // unless Stephen's body/fork is under an end, in which case he holds it to a single cell.
  bool aligned = orig.IsVertical() ? (dir == Up || dir == Down) : (dir == Left || dir == Right);
  auto onParallel = [&](s8 cellX, s8 cellY) -> bool {
    s8 below = GetSausage(cellX, cellY, orig.z - 1);
    return below != -1 && below != sausageNo && _sausages[below].IsHorizontal() == orig.IsHorizontal();
  };
  // Stephen's body under an end always holds the rider to a single cell; his fork holds it only when BARE. A fork
  // embedded in a sausage lets that sausage be the real support, so a knock-on tumble still runs (3-4 Cold Trail move
  // 75: the fork sits in the perpendicular base under the rider, which rolls, so the rider double-moves the extra cell).
  auto heldEnd = [&](s8 ex, s8 ey) -> bool {
    if (man.x == ex && man.y == ey && man.z == orig.z - 1) return true; // body under
    return man.HasFork() && man.forkX == ex && man.forkY == ey && man.forkZ == orig.z - 1
        && GetSausage(ex, ey, orig.z - 1) == -1;                        // bare fork under
  };
  bool held = heldEnd(orig.x1, orig.y1) || heldEnd(orig.x2, orig.y2);
  // A carried sausage aligned with the motion and resting on a PERPENDICULAR base that ROLLED doesn't just ride one
  // cell -- it tumbles an extra one. Per the staged model that extra cell is a separate DOUBLE-MOVE (stage 7) that runs
  // after the primary move has settled, so here we still carry it a single cell like any rider and just RECORD that it
  // owes a tumble. A base that only slides, or is dragged rigidly on the fork, imparts no roll (|baseRolled| false) and
  // so no tumble; Stephen's body/fork under an end, or a parallel base, also cancels the extra cell.
  bool doubleMove = baseRolled && aligned && !held && !onParallel(orig.x1, orig.y1) && !onParallel(orig.x2, orig.y2);

  Sausage sausage = orig;
  sausage.x1 += dx;
  sausage.y1 += dy;
  sausage.x2 += dx;
  sausage.y2 += dy;

  // Carry destination rammed into a wall -> it can't move there, so it's left exactly where it is. The later Settle
  // pass drops it if the base has slid out from under it. (Motion only here -- no fall, no cook.)
  if (IsWall(sausage.x1, sausage.y1, sausage.z) || IsWall(sausage.x2, sausage.y2, sausage.z)) return true;

  bool rolls = sausage.IsHorizontal() ? (dir == Up || dir == Down) : (dir == Left || dir == Right);
  if (rolls) sausage.flags ^= Sausage::Rolled;
  plan.sausages[sausageNo] = sausage;
  plan.mask |= (1 << sausageNo);
  if (doubleMove) {
    plan.doubleMoveMask |= (1 << sausageNo);
    plan.doubleMoveDir[sausageNo] = dir;
  }

  if (aboveA != -1 && aboveA != sausageNo && !(plan.mask & (1 << aboveA)))
    if (!PlanSausageCarry(aboveA, dx, dy, dir, plan, mover, rolls)) return false;
  if (aboveB != -1 && aboveB != sausageNo && aboveB != aboveA && !(plan.mask & (1 << aboveB)))
    if (!PlanSausageCarry(aboveB, dx, dy, dir, plan, mover, rolls)) return false;
  return true;
}

bool Level2::CookSausage(Sausage& sausage) const {
  // Each end resting on a grill browns its down-facing side: the unrolled ("A") face normally, the rolled ("B") face
  // once the sausage has been flipped.
  u8 sides = 0;
  if (IsGrill(sausage.x1, sausage.y1, sausage.z)) sides |= Sausage::Cook1A;
  if (IsGrill(sausage.x2, sausage.y2, sausage.z)) sides |= Sausage::Cook2A;
  if (sides == 0) return true;
  if (sausage.flags & Sausage::Rolled) sides *= 2; // shift A-faces to their rolled-over B-faces
  if (sausage.flags & sides) return false;         // that face is already cooked -- it would burn
  sausage.flags |= sides;
  return true;
}

bool Level2::CookMoved(MovePlan& plan, u16 movedMask, u16 preCookedMask) {
  for (int i = 0; i < _sausages.Size(); i++) {
    if (!(movedMask & (1 << i)) || (preCookedMask & (1 << i))) continue;
    if (!CookSausage(plan.sausages[i])) return false; // would burn
  }
  return true;
}

Level2::MovePlan Level2::NewPlan() const {
  // Seed the plan with a full copy of the live state, so it's a complete working tableau the move can advance without
  // ever touching the object (the planners overwrite the sausages they move; the reaction passes advance the rest).
  MovePlan plan;
  plan.stephen = _stephen;
  for (int i = 0; i < _sausages.Size(); i++) plan.sausages[i] = _sausages[i];
  return plan;
}

void Level2::Commit(const MovePlan& plan) {
  // The one and only mutation of the live state, reached after every stage has succeeded -- nothing to roll back.
  _stephen = plan.stephen;
  for (int i = 0; i < _sausages.Size(); i++) _sausages[i] = plan.sausages[i];
}

s8 Level2::GetPlannedSausage(const MovePlan& plan, s8 x, s8 y, s8 z) const {
  if (z < 0) return -1;
  for (int i = 0; i < _sausages.Size(); i++)
    if (plan.sausages[i].IsAt(x, y, z)) return (s8)i;
  return -1;
}

bool Level2::SausageSupported(const MovePlan& plan, s8 x, s8 y, s8 z) const {
  // An off-grid cell is never a footing -- mirrors the reference, whose CanWalkOnto short-circuits on IsWithinGrid.
  // (This also subsumes the z<0 guard.) Without it, a sausage rolled ENTIRELY off the top/side edge could be "supported"
  // by another sausage hanging half-off the grid below it, so L2 would accept an off-edge roll the reference refuses
  // (3-12 Cold Horizon's off-edge divergences).
  if (!IsWithinGrid(x, y, z)) return false;
  // Terrain footing: the ground floor at z==0 (CanWalkOnto with nothing below to find), or a wall-top -- a wall solid at
  // z-1 -- for z>=1. Terrain is immutable during a move, so the live predicates are exact here.
  if (z == 0 ? CanWalkOnto(x, y, 0) : IsWall(x, y, z - 1)) return true;
  // A sausage directly below in the WORKING tableau (not the live, pre-move _sausages).
  if (GetPlannedSausage(plan, x, y, z - 1) != -1) return true;
  // Stephen's body or held fork, at the plan's post-move pose, directly below also holds a sausage up.
  const Stephen& man = plan.stephen;
  if (man.x == x && man.y == y && man.z == z - 1) return true;
  if (man.HasFork() && man.forkX == x && man.forkY == y && man.forkZ == z - 1) return true;
  return false;
}

bool Level2::Settle(MovePlan& plan, u16& movedMask, const Sausage* preMove, const Stephen& prevStephen, u16 exclude) {
  // The sausage on the held fork is carried rigidly, not subject to gravity -- exempt it from the fall. The fork is at
  // the plan's post-move pose; the speared sausage is whatever sits in its cell in the working tableau.
  s8 speared = GetPlannedSausage(plan, plan.stephen.forkX, plan.stephen.forkY, plan.stephen.forkZ);

  // A sausage that rode on Stephen pre-move (resting on his body or fork) becomes subject to gravity when he steps
  // out from under it: seed it active so the fall pass below drops it if its footing is now gone.
  for (int i = 0; i < _sausages.Size(); i++) {
    if ((movedMask & (1 << i)) || (exclude & (1 << i))) continue;
    const Sausage& p = preMove[i];
    bool rode = (p.x1 == prevStephen.x && p.y1 == prevStephen.y && p.z - 1 == prevStephen.z)
             || (p.x2 == prevStephen.x && p.y2 == prevStephen.y && p.z - 1 == prevStephen.z)
             || (p.x1 == prevStephen.forkX && p.y1 == prevStephen.forkY && p.z - 1 == prevStephen.forkZ)
             || (p.x2 == prevStephen.forkX && p.y2 == prevStephen.forkY && p.z - 1 == prevStephen.forkZ);
    if (rode) movedMask |= (1 << i);
  }

  // Only sausages this move actually disturbed are subject to gravity: the ones it moved, plus any that were (in the
  // PRE-move layout) stacked on top of a disturbed one -- transitively, so a whole tower comes along when its base
  // slides out. A sausage that was already floating before this move (e.g. a parked decoy sitting over the void) was
  // never resting on anything we touched, so it's left exactly where it is. A pending double-move (|exclude|) is held
  // back too -- it hasn't come to rest, so its own DoubleMove pass owns its fall.
  u16 active = movedMask & ~exclude;
  bool grew = true;
  while (grew) {
    grew = false;
    for (int i = 0; i < _sausages.Size(); i++) {
      if ((active & (1 << i)) || (exclude & (1 << i))) continue;
      const Sausage& preMoveSausage = preMove[i];
      auto restedOnActive = [&](s8 x, s8 y, s8 z) -> bool {
        for (int k = 0; k < _sausages.Size(); k++)
          if (k != i && (active & (1 << k)) && preMove[k].IsAt(x, y, z - 1)) return true;
        return false;
      };
      if (restedOnActive(preMoveSausage.x1, preMoveSausage.y1, preMoveSausage.z) || restedOnActive(preMoveSausage.x2, preMoveSausage.y2, preMoveSausage.z)) {
        active |= (1 << i);
        grew = true;
      }
    }
  }

  // Gravity over the active set, bottom-up to a fixed point: a sausage with neither end supported drops a level;
  // repeat until the stack is stable (an upper sausage only loses its footing once the one beneath has fallen).
  bool changed = true;
  while (changed) {
    changed = false;
    for (int i = 0; i < _sausages.Size(); i++) {
      if (i == speared || !(active & (1 << i))) continue;
      Sausage& sausage = plan.sausages[i];
      if (SausageSupported(plan, sausage.x1, sausage.y1, sausage.z) || SausageSupported(plan, sausage.x2, sausage.y2, sausage.z)) continue;
      if (sausage.z <= 0) return false; // fell out of the bottom of the world -> the move is refused
      sausage.z--;
      movedMask |= (1 << i);
      changed = true;
    }
  }
  return true;
}

bool Level2::DoubleMove(MovePlan& plan) {
  if (plan.doubleMoveMask == 0) return true;

  // Snapshot the just-settled working tableau: it's the "pre-move" layout for the double-move's own settle pass.
  Sausage preMove[NUM_SAUSAGES];
  for (int i = 0; i < _sausages.Size(); i++) preMove[i] = plan.sausages[i];
  Stephen prevStephen = plan.stephen;
  u16 movedMask = 0;
  for (int i = 0; i < _sausages.Size(); i++) {
    if (!(plan.doubleMoveMask & (1 << i))) continue;
    s8 dx, dy;
    Delta(plan.doubleMoveDir[i], dx, dy);
    Sausage& sausage = plan.sausages[i];
    // Tumble one more cell in the recorded direction -- unless a wall stops it, in which case it just comes to rest
    // where it already is. Either way it now settles and cooks as its own event. (Aligned slide -> no roll.)
    if (!IsWall(sausage.x1 + dx, sausage.y1 + dy, sausage.z) && !IsWall(sausage.x2 + dx, sausage.y2 + dy, sausage.z)) {
      sausage.x1 += dx;
      sausage.y1 += dy;
      sausage.x2 += dx;
      sausage.y2 += dy;
    }
    movedMask |= (1 << i);
  }
  if (!Settle(plan, movedMask, preMove, prevStephen)) return false;
  if (!CookMoved(plan, movedMask)) return false;
  return true;
}


bool Level2::SausageBlocked(s8 sausageNo, Direction dir) const {
  Sausage sausage = _sausages[sausageNo];
  s8 dx, dy;
  Delta(dir, dx, dy);
  s8 z = sausage.z;
  s8 x1 = sausage.x1 + dx, y1 = sausage.y1 + dy;
  s8 x2 = sausage.x2 + dx, y2 = sausage.y2 + dy;

  // Driving an end straight into a wall blocks the push.
  if (IsWall(x1, y1, z) || IsWall(x2, y2, z)) return true;
  // Otherwise the block propagates down the push chain: we're blocked if a sausage we'd push into is itself blocked
  // (its blockage bottoms out against a wall, not the void -- a chain that would topple off is NOT blocked).
  s8 ahead1 = GetSausage(x1, y1, z);
  if (ahead1 != -1 && ahead1 != sausageNo && SausageBlocked(ahead1, dir)) return true;
  s8 ahead2 = GetSausage(x2, y2, z);
  if (ahead2 != -1 && ahead2 != sausageNo && ahead2 != ahead1 && SausageBlocked(ahead2, dir)) return true;
  return false;
}

void Level2::Delta(Direction dir, s8& dx, s8& dy) const {
  dx = 0;
  dy = 0;
  if (dir == Up)         dy = -1;
  else if (dir == Down)  dy = +1;
  else if (dir == Left)  dx = -1;
  else if (dir == Right) dx = +1;
}
