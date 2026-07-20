#include "Level2.h"


State Level::GetState() const {
  State s{_stephen};
  assert(sizeof(s.sausages) / sizeof(Sausage) == _sausages.Size());
#if 0 // Sorting is proven to work and also greatly reduces states in complex levels.
  _sausages.CopyIntoArray(s.sausages, sizeof(s.sausages));
#else
  _sausages.SortedCopyIntoArray(s.sausages, sizeof(s.sausages), [](const Sausage& a, const Sausage& b) -> s8 {
    if (a.x1 != b.x1) return a.x1 - b.x1;
    if (a.y1 != b.y1) return a.y1 - b.y1;
    if (a.z != b.z) return a.z - b.z;
    if (a.x2 != b.x2) return a.x2 - b.x2;
    if (a.y2 != b.y2) return a.y2 - b.y2;
    if (a.flags != b.flags) return a.flags - b.flags;
    return 0;
    });
#endif
  return s;
}

void Level::SetState(const State& state) {
  _stephen = state.stephen;
  _sausages.CopyFromArray(state.sausages, sizeof(state.sausages));
}

bool Level::Move(Direction dir) {
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

bool Level::HandleBurnedStep(Direction dir, bool& handled) {
  // Stage 8 (outcomes): a body that comes to rest on a grill recoils straight back the way it came, as its own move.
  // Applied commonly so EVERY motion (step, ladder climb/descent, log-roll, spear) bounces consistently.
  if (!IsGrill(_stephen.x, _stephen.y, _stephen.z)) return true;
  handled = true;
  _feat |= F_BURNED;
  return Move(Inverse(dir));
}

bool Level::HandleSpearedMotion(Direction dir, bool& handled) {
  // If the fork is lodged in a sausage, Stephen drags it rigidly; otherwise leave |handled| false for the step/turn
  // classification. The speared sausage is whatever the fork currently occupies.
  s8 sausageNo = GetSausage(_stephen.forkX, _stephen.forkY, _stephen.forkZ);
  if (sausageNo == -1) return true;
  handled = true;
  _feat |= F_SPEARMOVE;

  s8 dx, dy;
  Delta(dir, dx, dy);
  s8 z = _stephen.z;
  s8 bodyX = _stephen.x + dx;
  s8 bodyY = _stephen.y + dy;

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

  // Everything Stephen's move carries as one rigid load -- the speared sausage and its riders, plus a head hat and its
  // stack -- translates by the SAME vector, so members must never treat each other as obstacles. Mark the whole load up
  // front (order-independent) so a carry that reaches a co-mover's cell first steps over it instead of ramming and
  // rolling it (3-2 Cold Finger: a head hat and a speared-drag rider swap cells). move-stages.md: motion is one event.
  plan.rigidLoad = RidingLoad(sausageNo) | RidingLoad(GetSausage(_stephen.x, _stephen.y, z + 1));
  bool unspear = SausageBlocked(sausageNo, dir);
  if (unspear) {
    _feat |= F_UNSPEAR;
    if (dir != Inverse(_stephen.dir)) return false;
    // Pulling free is a backward step, so the body still pushes any sausage at its destination (the speared one
    // stays put). If that sausage can't move, the move is refused.
    s8 bodySausage = GetSausage(bodyX, bodyY, z);
    if (bodySausage != -1 && bodySausage != sausageNo)
      if (!PlanSausagePush(bodySausage, dir, plan)) return false;
    // A sausage resting on the fork TIP (forkZ+1) rides out with the retreating fork. Its support end is on the fork,
    // so only its FAR end decides anchoring: if that end rests on a wall or ANY non-moving sausage (including the
    // speared base -- a square stack rather than a cantilever off the tip) the fork slides out and it stays. Otherwise
    // it rides one cell, rolling across its axis, or flicking an extra cell (a double-move) when aligned with the pull.
    // A sausage ALSO resting on Stephen's head is a head hat -- left to the head-hat carry below (4-1 Wretch's Retreat).
    s8 forkHat = GetSausage(_stephen.forkX, _stephen.forkY, _stephen.forkZ + 1);
    if (forkHat != -1 && forkHat != sausageNo && forkHat != GetSausage(_stephen.x, _stephen.y, _stephen.z + 1)
        && !(plan.mask & (1 << forkHat))) {
      Sausage hat = _sausages[forkHat];
      bool firstOnFork = (hat.x1 == _stephen.forkX && hat.y1 == _stephen.forkY);
      s8 farX = firstOnFork ? hat.x2 : hat.x1, farY = firstOnFork ? hat.y2 : hat.y1;
      s8 farBelow = GetSausage(farX, farY, hat.z - 1);
      bool farAnchored = IsWall(farX, farY, hat.z - 1) || (farBelow != -1 && !(plan.mask & (1 << farBelow)));
      bool blocked = IsWall(hat.x1 + dx, hat.y1 + dy, hat.z) || IsWall(hat.x2 + dx, hat.y2 + dy, hat.z);
      if (!farAnchored && !blocked) {
        bool aligned = hat.IsHorizontal() ? (dx != 0) : (dy != 0);
        hat.x1 += dx; hat.y1 += dy; hat.x2 += dx; hat.y2 += dy;
        if (!aligned) hat.flags ^= Sausage::Rolled;  // a perpendicular ride rolls it across its axis
        plan.sausages[forkHat] = hat;
        plan.mask |= (1 << forkHat);
        if (aligned) { plan.doubleMoveMask |= (1 << forkHat); plan.doubleMoveDir[forkHat] = dir; } // aligned -> flicks an extra cell
      }
    }
  } else {
    // The dragged sausage pushes any sausage it runs into (a chain, obeying the normal push rules); a chain member that
    // can't move refuses the drag. Mark this sausage moving first so the chain never tries to push it back.
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
    // The sausage needs no ground -- the fork holds it up, so it can ride out over the void. CookMoved browns it later.
    plan.sausages[sausageNo] = moved;

    // A sausage riding on top of the speared one is carried along the drag. It's held up by the fork-borne stack (whose
    // base is dragged rigidly, never rolling), so like the real game it inherits that zero torsion and does NOT roll --
    // rolling is reserved for a sausage sliding along the ground (3-2 Cold Finger). Use Stephen's post-move pose so the
    // body/fork test sees where he ends up.
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
        if (!PlanSausageCarry(aboveA, dx, dy, dir, plan, &mover, /*rigid=*/true)) return false;
      if (aboveB != -1 && aboveB != sausageNo && aboveB != aboveA && !(plan.mask & (1 << aboveB)))
        if (!PlanSausageCarry(aboveB, dx, dy, dir, plan, &mover, /*rigid=*/true)) return false;
    }

    // Stephen's body also shoulders aside any sausage standing where it steps, pushing it the same way.
    s8 bodySausage = GetSausage(bodyX, bodyY, z);
    if (bodySausage != -1 && bodySausage != sausageNo && !(plan.mask & (1 << bodySausage)))
      if (!PlanSausagePush(bodySausage, dir, plan)) return false;
  }

  // --- Commit --- Every lunge onto a grill (a backward unspear included) commits here as an ordinary step; the common
  // HandleBurnedStep then recoils Stephen the opposite way. That re-drags the speared sausage back while leaving
  // anything the lunge SHOVED where it landed, and a backward unspear's recoil shoves the freed sausage one cell ahead
  // (3-1 Cold Jag) -- so the grill needs no special case. Off a grill, body and fork shift and the speared sausage (plus
  // anything it pushed) rides along; a backward unspear may instead push a sausage out of the body's way.

  // A sausage on Stephen's HEAD rides along too -- it rests on his head, not the speared sausage, so the drag logic
  // never touched it. Carry it as an ordinary step would: the head hat (and anything squarely stacked on it) rides
  // rigidly while a cantilevered rider rolls across its axis (3-8 Cold Head).
  s8 headHat = GetSausage(_stephen.x, _stephen.y, z + 1);
  if (headHat != -1 && !(plan.mask & (1 << headHat))) {
    u16 rigidStack = (u16)(1 << headHat);
    for (bool grew = true; grew; ) {
      grew = false;
      for (int i = 0; i < _sausages.Size(); i++) {
        if (rigidStack & (1 << i)) continue;
        const Sausage& s = _sausages[i];
        s8 below1 = GetSausage(s.x1, s.y1, s.z - 1), below2 = GetSausage(s.x2, s.y2, s.z - 1);
        if (below1 != -1 && below2 != -1 && (rigidStack & (1 << below1)) && (rigidStack & (1 << below2))) { rigidStack |= (u16)(1 << i); grew = true; }
      }
    }
    // rigidLoad was seeded optimistically from RidingLoad(speared base), which folds in EVERY sausage stacked above it
    // -- including a cantilevered rider that the drag actually left behind (it slides off and drops). By now the drag
    // carries are all planned, so plan.mask is the true moving set; prune rigidLoad to the sausages that really moved
    // plus the head hat's own about-to-ride stack. A left-behind rider is then a genuine obstacle in the hat's path, so
    // the hat rams it (and is refused if it can't shove) instead of stepping over a cell that stays occupied during the
    // simultaneous motion (3-2 Cold Finger m61: the head hat's carry is blocked by a rider dropping onto its lane).
    plan.rigidLoad &= (plan.mask | RidingLoad(headHat));
    PlanHatCarry(headHat, dx, dy, dir, plan, rigidStack);
  }

  plan.stephen.x = bodyX;
  plan.stephen.y = bodyY;
  plan.stephen.forkX += dx;
  plan.stephen.forkY += dy;

  return ReactAndCommit(plan);
}

bool Level::HandleLogRolling(Direction dir, bool& handled) {
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

  // A sausage already speared on the fork rides along rigidly; capture it before any motion (3-8 Cold Head).
  s8 speared = _stephen.HasFork() ? GetSausage(_stephen.forkX, _stephen.forkY, _stephen.forkZ) : -1;

  // Roll the sausage (and anything it pushes) one cell. If it can't roll there -- a wall, or out of the world -- this
  // isn't a legal log roll; fall through so the rest of the pipeline can refuse or reinterpret the press.
  MovePlan plan = NewPlan();

  // The log, everything riding it, Stephen's head/fork hats and any speared sausage all ride the roll as ONE rigid load
  // (same translation), so they must never treat each other as obstacles (move-stages.md: motion is one event).
  plan.rigidLoad = RidingLoad(onSausage) | RidingLoad(speared)
                 | RidingLoad(GetSausage(_stephen.x, _stephen.y, _stephen.z + 1))
                 | RidingLoad(_stephen.HasFork() ? GetSausage(_stephen.forkX, _stephen.forkY, _stephen.forkZ + 1) : (s8)-1);

  if (!PlanSausagePush(onSausage, roll, plan)) return true;

  // Stephen rides the roll: his body and fork translate one cell with the sausage. Neither may ride into a wall
  // (3-12 Cold Horizon), so a blocked ride refuses the whole move even once the sausage can roll.
  s8 dx, dy;
  Delta(roll, dx, dy);
  s8 newBodyX = _stephen.x + dx, newBodyY = _stephen.y + dy;
  s8 newForkX = _stephen.forkX + dx, newForkY = _stephen.forkY + dy;
  if (IsWall(newBodyX, newBodyY, _stephen.z) || IsWall(newForkX, newForkY, _stephen.forkZ)) return false;

  // That same ride shoulders any sausage standing where his fork or body lands, pushing it the roll direction (3-8 Cold
  // Head). Use his post-ride pose so a rider is judged correctly.
  Stephen mover = _stephen;
  mover.x = newBodyX; mover.y = newBodyY;
  mover.forkX = newForkX; mover.forkY = newForkY;
  s8 forkDest = GetSausage(newForkX, newForkY, _stephen.forkZ);
  if (forkDest != -1 && forkDest != onSausage && forkDest != speared && !(plan.mask & (1 << forkDest))) {
    // The fork shoves the sausage in its path -- unless a wall blocks the shove, in which case it SPEARS it instead,
    // riding into its cell and lodging there (3-8 Cold Head). PlanSausagePush refuses (leaving the plan untouched)
    // exactly when the shove hits a wall, so a refused push simply falls through to the implicit spear.
    PlanSausagePush(forkDest, roll, plan, &mover);
  }
  s8 bodyDest = GetSausage(newBodyX, newBodyY, _stephen.z);
  if (bodyDest != -1 && bodyDest != onSausage && bodyDest != speared && !(plan.mask & (1 << bodyDest)))
    if (!PlanSausagePush(bodyDest, roll, plan, &mover)) return false;

  // --- Commit --- the sausage rolls (with anything it pushes), gravity settles it, then Stephen rides on top and drops.
  // A sausage riding on Stephen's HEAD (or balanced on his fork-tip) rides the roll with him, captured before motion.
  s8 headHat = GetSausage(_stephen.x, _stephen.y, _stephen.z + 1);
  s8 forkHat = _stephen.HasFork() ? GetSausage(_stephen.forkX, _stephen.forkY, _stephen.forkZ + 1) : (s8)-1;
  MarkDoubleMoves(plan, _sausages.begin()); // resolve any carried double-mover (the roll's push chain) before gravity
  u16 movedMask = plan.mask;
  if (!Settle(plan, movedMask, _sausages.begin(), _stephen, plan.doubleMoveMask)) return false;

  // A sausage speared on the fork rides rigidly (never rolls). A wall in its path pulls the fork free only when the roll
  // runs opposite his facing (an unspear); any other blocked drag refuses the move.
  if (speared != -1) {
    // The roll's push chain may have already grabbed the speared sausage as a RIDER and rolled it; it really rides the
    // fork rigidly, so rebuild it from the pre-move layout and apply the single rigid translate below (3-1 Cold Jag).
    Sausage& spearedSausage = plan.sausages[speared];
    spearedSausage = _sausages[speared];
    if (IsWall(spearedSausage.x1 + dx, spearedSausage.y1 + dy, spearedSausage.z) || IsWall(spearedSausage.x2 + dx, spearedSausage.y2 + dy, spearedSausage.z)) {
      if (roll != Inverse(_stephen.dir)) return false;
      movedMask &= ~(1 << speared); // unspeared: it never moved, so keep it out of the cook set
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
  // The head hat rides rigidly on Stephen's head through the whole roll (and any drop) -- it never rolls, so translate
  // it by his TOTAL displacement now that his final pose is known. A wall in its path, or a far end anchored on a
  // non-moving sausage/wall, leaves it behind (4-5 Crunchy Leaves). Settle seeded it into |movedMask| as "rode on
  // Stephen" but never relocated it, so it's still at its pre-move footprint here and we own its actual translation.
  u16 headCarried = 0;
  if (headHat != -1 && headHat != speared) {
    const Sausage& hat = _sausages[headHat]; // the pre-move footprint: the log's push chain may have already shifted
                                             // plan.sausages[headHat] (its far end rode the mid), which we now redo rigidly
    s8 heightDelta = plan.stephen.z - _stephen.z;
    bool wallBlocked = IsWall(hat.x1 + dx, hat.y1 + dy, hat.z + heightDelta) || IsWall(hat.x2 + dx, hat.y2 + dy, hat.z + heightDelta);
    bool firstOnHead = (hat.x1 == _stephen.x && hat.y1 == _stephen.y);
    s8 farX = firstOnHead ? hat.x2 : hat.x1, farY = firstOnHead ? hat.y2 : hat.y1;
    // Anchored only by a NON-MOVING support: a wall, or a sausage that isn't itself moving this turn. A far end resting
    // on a sausage that rolls/slides away (the mid, carried by the log) does NOT anchor -- the hat rides Stephen rigidly
    // (3-5 Cold Cliff: the hat bridges Stephen's head and the mid, so it moves one cell with him, no roll).
    s8 farBelow = GetSausage(farX, farY, hat.z - 1);
    bool anchored = IsWall(farX, farY, hat.z - 1) || (farBelow != -1 && !(plan.mask & (1 << farBelow)));
    if (!wallBlocked && !anchored) {
      // The head hat AND everything stacked on it rides by Stephen's total displacement. The hat and any SQUARELY
      // stacked rider ride rigidly; a CANTILEVERED rider rolls across its own axis (matching the reference's hatStack).
      u16 stack = CarriedMask(headHat);
      u16 rigid = FullySupportedStack(headHat);
      headCarried = stack;
      for (int i = 0; i < _sausages.Size(); i++) {
        if (!(stack & (1 << i))) continue;
        Sausage s = _sausages[i]; // rebuild from the pre-move footprint, discarding any push-chain carry that grabbed it
        s.x1 += dx; s.y1 += dy; s.x2 += dx; s.y2 += dy; s.z += heightDelta;
        if (!(rigid & (1 << i)) && (s.IsHorizontal() ? (dy != 0) : (dx != 0))) s.flags ^= Sausage::Rolled;
        plan.sausages[i] = s;
        movedMask |= (1 << i);
        plan.mask |= (1 << i); // also record in plan.mask so the second MarkDoubleMoves considers this carried sausage
      }
    }
  }
  // A sausage balanced on the fork-tip rides the roll with the fork -- along with everything stacked on it. Unlike a
  // head hat it is fork-borne (never rigid), so EVERY member rolls when carried across its own axis (mirrors the step
  // path's PlanHatCarry with rigidMask=0). Its far end, if anchored on a non-moving sausage or wall, leaves it behind.
  // Skip a head<->fork bridge the head-hat carry already handled (3-8 Cold Head).
  if (forkHat != -1 && forkHat != speared && forkHat != headHat && forkHat != onSausage && !(headCarried & (1 << forkHat))) {
    const Sausage& hat = plan.sausages[forkHat];
    s8 heightDelta = plan.stephen.z - _stephen.z;
    bool wallBlocked = IsWall(hat.x1 + dx, hat.y1 + dy, hat.z + heightDelta) || IsWall(hat.x2 + dx, hat.y2 + dy, hat.z + heightDelta);
    bool firstOnFork = (hat.x1 == _stephen.forkX && hat.y1 == _stephen.forkY);
    s8 farX = firstOnFork ? hat.x2 : hat.x1, farY = firstOnFork ? hat.y2 : hat.y1;
    bool anchored = IsWall(farX, farY, hat.z - 1) || GetSausage(farX, farY, hat.z - 1) != -1;
    if (!wallBlocked && !anchored) {
      u16 stack = CarriedMask(forkHat) & ~headCarried; // don't re-carry anything the head-hat stack already moved
      for (int i = 0; i < _sausages.Size(); i++) {
        if (!(stack & (1 << i))) continue;
        Sausage& s = plan.sausages[i];
        s.x1 += dx; s.y1 += dy; s.x2 += dx; s.y2 += dy; s.z += heightDelta;
        if (s.IsHorizontal() ? (dy != 0) : (dx != 0)) s.flags ^= Sausage::Rolled; // fork-borne -> rolls across its axis
        movedMask |= (1 << i);
        plan.mask |= (1 << i); // also record in plan.mask so the second MarkDoubleMoves considers this carried sausage
      }
    }
  }
  // The hat carries above ran AFTER the first MarkDoubleMoves (they need Stephen's post-drop pose), so a sausage left
  // balanced across a hat rider that just rolled hasn't been checked for its extra tumble. Re-run the detector now that
  // the hats are placed (they were recorded into plan.mask above); it skips already-flagged sausages, adding only these
  // hat-induced double-movers (e.g. an aligned rider on a rolled fork-hat).
  MarkDoubleMoves(plan, _sausages.begin());
  if (!CookMoved(plan, movedMask, plan.doubleMoveMask)) return false;
  if (!DoubleMove(plan)) return false;
  if (PlanHasOverlap(plan)) return false; // a carry produced an impossible two-in-one-cell state -> refuse
  Commit(plan);
  handled = true;
  _feat |= F_LOGROLL;
  return true;
}

bool Level::HandleLadderMotion(Direction dir, bool& handled) {
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
    // The fork carries whatever sits on it up: a speared sausage, or -- if the fork is empty -- a sausage resting on
    // top of the fork, which the rising fork lifts along (3-7 Cold Plateau).
    s8 carried = speared != -1 ? speared : GetSausage(_stephen.forkX, _stephen.forkY, _stephen.forkZ + 1);
    u16 carriedMask = CarriedMask(carried) | CarriedMask(headHat);
    // A sausage resting on the wall-top the climb ends on sits in the rising body's path: LiftStephen shoves it up rung
    // by rung (recording it in |hatMask|) so the step-off hat-carries it rather than dragging it rigidly (4-2 Toad's Folly).
    u16 hatMask = 0;
    // Only the speared sausage and the rigid head-hat stack ride without rolling; everything else carried rolls when
    // taken across its axis. The rigid stack is Stephen's head hat, PLUS the speared base when the fork is lodged in one
    // (it rides the fork rigidly), plus anything SQUARELY stacked on either (both ends resting on it); a CANTILEVERED
    // rider rolls (4-5 Crunchy Leaves). A hat that bridges his head and the speared base is squarely supported by two
    // rigid co-movers, so it translates rigidly rather than rolling (3-8 Cold Head m77). Computed from the PRE-climb
    // layout, before LiftStephen shoves wall-top hats in.
    u16 rigidStack = 0;
    if (headHat != -1) rigidStack |= (u16)(1 << headHat);
    if (speared != -1) rigidStack |= (u16)(1 << speared);
    if (rigidStack) {
      for (bool grew = true; grew; ) {
        grew = false;
        for (int i = 0; i < _sausages.Size(); i++) {
          if (rigidStack & (1 << i)) continue;
          const Sausage& s = _sausages[i];
          s8 below1 = GetSausage(s.x1, s.y1, s.z - 1), below2 = GetSausage(s.x2, s.y2, s.z - 1);
          if (below1 != -1 && below2 != -1 && (rigidStack & (1 << below1)) && (rigidStack & (1 << below2))) { rigidStack |= (u16)(1 << i); grew = true; }
        }
      }
    }
    // A BARE fork-borne sausage (balanced on the fork tip, no speared base under it) rolls as a bridge across its axis
    // when the fork carries it up (4-5 Crunchy Leaves). But a sausage resting on BOTH Stephen's head and his fork tip
    // (carried == headHat) bridges his two rigid co-moving supports, so it rides UP rigidly rather than rolling -- the
    // Oracle gives it zero torsion because its whole lower footprint is Stephen himself, moving at the sausage's speed
    // (measured on 3-5 Cold Cliff m62; corroborated by 21598 real Foul Fen rolls, none of which roll a co-mover-supported
    // sausage). This mirrors the descent case exactly.
    if (speared == -1 && carried != headHat) rigidStack &= ~CarriedMask(carried);
    u16 rollMask = (u16)(carriedMask & ~rigidStack);
    if (speared != -1) rollMask &= ~(u16)(1 << speared);
    while (IsLadder(_stephen.x, _stephen.y, _stephen.z, dir))
      if (!LiftStephen(+1, carriedMask, hatMask)) return false;
    if (!StepOffLadder(dir, carriedMask, false, hatMask, rollMask, speared)) return false;
    handled = true;
    _feat |= F_LADDER_UP;
    return true;
  }

  // Descend: the cell ahead has no footing at our level, but a back-facing ladder one level down leads to a surface.
  s8 dx, dy;
  Delta(dir, dx, dy);
  s8 ax = _stephen.x + dx, ay = _stephen.y + dy;
  if (CanWalkOnto(ax, ay, _stephen.z)) return true;                  // there's a ledge ahead -- ordinary walking handles it
  if (!IsLadder(ax, ay, _stephen.z - 1, Inverse(dir))) return true;  // nothing to climb down -- fall through
  // A sausage sitting on Stephen -- speared, on his fork, or hatted on his head -- rides DOWN the ladder only if it's
  // cantilevered (its far end hangs over open space). If that far end is anchored on a wall or another sausage, the seat
  // slides out from under it and it stays (4-4 Foul Fen off-path).
  auto cantilevered = [&](s8 no, s8 seatX, s8 seatY) -> bool {
    const Sausage& s = _sausages[no];
    bool firstOnSeat = (s.x1 == seatX && s.y1 == seatY);
    s8 farX = firstOnSeat ? s.x2 : s.x1, farY = firstOnSeat ? s.y2 : s.y1;
    return !IsWall(farX, farY, s.z - 1) && GetSausage(farX, farY, s.z - 1) == -1;
  };
  s8 carried = speared;
  if (carried == -1) {
    s8 forkRider = GetSausage(_stephen.forkX, _stephen.forkY, _stephen.forkZ + 1);
    if (forkRider != -1 && cantilevered(forkRider, _stephen.forkX, _stephen.forkY)) carried = forkRider;
  }
  s8 descentHat = (headHat != -1 && cantilevered(headHat, _stephen.x, _stephen.y)) ? headHat : (s8)-1;
  // The entire descending stack: the speared/fork-borne base, a cantilevered head hat, and everything resting on top
  // (CarriedMask grows upward). Each rung lowers the whole stack in lockstep, so a rider on the base comes down with it
  // rather than floating a level up (3-8 Cold Head).
  u16 carriedMask = CarriedMask(carried) | CarriedMask(descentHat);
  // A carried rider rides down only while its support (the fork, or Stephen's head) stays under it. The moment it comes
  // to rest on a wall-top -- or a sausage that isn't descending -- that support slides out and it stays put (4-2 Toad's
  // Folly). A head hat is always depositable, as is a FORK-BORNE base (3-5 Cold Cliff); only a SPEARED base stays lodged
  // on the descending fork.
  u16 hatMask = 0; // unused during a crouch (a descent never shoves a sausage upward); kept for LiftStephen's signature
  // Rigid units (translate without rolling): Stephen's head hat, PLUS the speared base when the fork is lodged in one,
  // plus anything squarely stacked on either. A head hat resting on the rigid speared base bridges his head and the
  // base, so it rides DOWN rigidly rather than rolling -- the exact reverse of the m77 climb (3-8 Cold Head m78). Seed
  // the full head hat (not just the cantilevered descentHat): a hat whose far end rests on the speared base isn't
  // cantilevered yet still translates rigidly. Seeding a non-descending hat is harmless -- it never enters rollMask
  // unless it's also in carriedMask.
  u16 rigidStack = 0;
  if (headHat != -1) rigidStack |= (u16)(1 << headHat);
  if (speared != -1) rigidStack |= (u16)(1 << speared);
  if (rigidStack) {
    for (bool grew = true; grew; ) {
      grew = false;
      for (int i = 0; i < _sausages.Size(); i++) {
        if (rigidStack & (1 << i)) continue;
        const Sausage& s = _sausages[i];
        s8 below1 = GetSausage(s.x1, s.y1, s.z - 1), below2 = GetSausage(s.x2, s.y2, s.z - 1);
        if (below1 != -1 && below2 != -1 && (rigidStack & (1 << below1)) && (rigidStack & (1 << below2))) { rigidStack |= (u16)(1 << i); grew = true; }
      }
    }
  }
  // A BARE fork-borne base (fork tip, no speared base) rolls as a bridge (4-5 Crunchy Leaves); drop the fork stack only
  // in that case. A SPEARED base is rigid, so it and its square riders stay in the rigid set. A head hat resting on BOTH
  // Stephen's head and the fork tip (carried == headHat) bridges his two rigid points, so it rides DOWN rigidly rather
  // than rolling -- don't strip it (3-5 Cold Cliff m59).
  if (speared == -1 && carried != headHat) rigidStack &= ~CarriedMask(carried);
  u16 rollMask = (u16)(carriedMask & ~rigidStack);
  if (speared != -1) rollMask &= ~(u16)(1 << speared);
  if (!StepOffLadder(dir, carriedMask, true, 0, rollMask, speared)) return false; // step out over the ladder (hanging, no footing yet)
  // Crouch down the ladder one rung at a time. Each rung lowers Stephen and his DIRECT cargo (a speared sausage) via
  // LiftStephen; the central gravity Settle then drops whatever he was merely CARRYING (a fork-borne base, a head hat),
  // depositing it the instant it comes to rest on a wall-top or a non-descending sausage -- no bespoke deposit
  // bookkeeping (3-5 Cold Cliff, 4-2 Toad's Folly, 4-4 Foul Fen).
  while (true) {
    if (_stephen.z <= 0) return false;
    Stephen prevStephen = _stephen;
    Sausage preMove[NUM_SAUSAGES];
    for (int i = 0; i < _sausages.Size(); i++) preMove[i] = _sausages[i];
    s8 spear = _stephen.HasFork() ? GetSausage(_stephen.forkX, _stephen.forkY, _stephen.forkZ) : (s8)-1;
    u16 directCargo = (spear != -1) ? (u16)(1 << spear) : 0;
    if (!LiftStephen(-1, directCargo, hatMask)) return false;          // lower Stephen + speared; a blocked crouch refuses
    MovePlan plan = NewPlan();
    u16 movedMask = directCargo;
    if (!Settle(plan, movedMask, preMove, prevStephen)) return false;  // drop / deposit the riders he left behind
    if (!CookMoved(plan, movedMask, 0)) return false;                  // brown anything that settled onto a grill
    Commit(plan);
    if (CanWalkOnto(_stephen.x, _stephen.y, _stephen.z)) break;        // reached a surface -- step off
    if (!IsLadder(_stephen.x, _stephen.y, _stephen.z - 1, Inverse(dir))) return false; // ladder ran out mid-air
  }
  handled = true;
  _feat |= F_LADDER_DOWN;
  return true;
}

bool Level::LiftStephen(s8 dz, u16& carried, u16& hatMask) {
  // The fork can't ram into a wall.
  s8 newForkZ = _stephen.forkZ + dz;
  if (IsWall(_stephen.forkX, _stephen.forkY, newForkZ)) return false;

  if (dz > 0) {
    // Climbing up: a sausage the rising fork or body enters is shoved up along with the climb -- it rides up as a hat.
    // If it (or its tower) can't rise -- a wall above -- the climb is refused.
    s8 forkBlock = GetSausage(_stephen.forkX, _stephen.forkY, newForkZ);
    if (forkBlock != -1 && !(carried & (1 << forkBlock)))
      if (!LiftSausageStack(forkBlock, dz, carried, hatMask)) return false;
    s8 bodyBlock = GetSausage(_stephen.x, _stephen.y, _stephen.z + dz);
    if (bodyBlock != -1 && !(carried & (1 << bodyBlock)))
      if (!LiftSausageStack(bodyBlock, dz, carried, hatMask)) return false;
  } else {
    // Descending: a sausage resting under the fork won't yield (solid ground below it), so the climb-down is blocked.
    s8 blocking = GetSausage(_stephen.forkX, _stephen.forkY, newForkZ);
    if (blocking != -1 && !(carried & (1 << blocking))) return false;
  }

  // A carried sausage (speared, or a stack riding the fork/head) that would slide into a wall at its new level blocks
  // the whole rung (4-4 Foul Fen, off-path). Checking every carried end keeps the fork and its cargo rigid.
  for (int i = 0; i < _sausages.Size(); i++) {
    if (!(carried & (1 << i))) continue;
    const Sausage& s = _sausages[i];
    if (IsWall(s.x1, s.y1, s.z + dz) || IsWall(s.x2, s.y2, s.z + dz)) return false;
    // Descending: a carried sausage lowered onto a NON-carried sausage collides -- refuse the rung rather than overlap
    // (3-8 Cold Head). Climb-up handles blockers above via LiftSausageStack, so guard to dz < 0.
    if (dz < 0) {
      s8 belowA = GetSausage(s.x1, s.y1, s.z + dz);
      s8 belowB = GetSausage(s.x2, s.y2, s.z + dz);
      if ((belowA != -1 && !(carried & (1 << belowA))) || (belowB != -1 && !(carried & (1 << belowB)))) return false;
    }
  }

  _stephen.z += dz;
  _stephen.forkZ += dz;
  for (int i = 0; i < _sausages.Size(); i++)
    if (carried & (1 << i)) _sausages[i].z += dz;
  return true;
}

bool Level::LiftSausageStack(s8 sausageNo, s8 dz, u16& carried, u16& hatMask) {
  if (carried & (1 << sausageNo)) return true;
  const Sausage& s = _sausages[sausageNo];
  // The cells it rises into must be clear of walls, or the whole climb is refused.
  if (IsWall(s.x1, s.y1, s.z + dz) || IsWall(s.x2, s.y2, s.z + dz)) return false;
  carried |= (1 << sausageNo);
  hatMask |= (1 << sausageNo);
  // Anything stacked directly on top rides up with it (a rigid tower).
  s8 aboveA = GetSausage(s.x1, s.y1, s.z + dz);
  if (aboveA != -1 && aboveA != sausageNo && !(carried & (1 << aboveA)))
    if (!LiftSausageStack(aboveA, dz, carried, hatMask)) return false;
  s8 aboveB = GetSausage(s.x2, s.y2, s.z + dz);
  if (aboveB != -1 && aboveB != sausageNo && aboveB != aboveA && !(carried & (1 << aboveB)))
    if (!LiftSausageStack(aboveB, dz, carried, hatMask)) return false;
  return true;
}

u16 Level::CarriedMask(s8 base) const {
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

u16 Level::FullySupportedStack(s8 base) const {
  if (base == -1) return 0;
  u16 stack = (u16)(1 << base);
  for (bool grew = true; grew; ) {
    grew = false;
    for (int i = 0; i < _sausages.Size(); i++) {
      if (stack & (1 << i)) continue;
      const Sausage& s = _sausages[i];
      s8 below1 = GetSausage(s.x1, s.y1, s.z - 1), below2 = GetSausage(s.x2, s.y2, s.z - 1);
      if (below1 != -1 && below2 != -1 && (stack & (1 << below1)) && (stack & (1 << below2))) { stack |= (u16)(1 << i); grew = true; }
    }
  }
  return stack;
}

bool Level::StepOffLadder(Direction dir, u16 carried, bool ladderMotion, u16 hatMask, u16 rollMask, s8 speared) {
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
  plan.rigidLoad = carried; // the whole carried stack co-moves by one vector; a carry must never ram a co-mover (move-stages.md: motion is one event)
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
    if (!(carried & (1 << i)) || (hatMask & (1 << i))) continue; // hat sausages ride as a head hat (below), not rigidly
    Sausage& carriedSausage = plan.sausages[i];
    // A carried sausage whose slide would drive an end into a wall is carry-blocked (4-2 Toad's Folly): it stays put
    // while the base slides out from under it. The SPEARED sausage is rigid on the fork, so a wall in its path refuses
    // the whole step (3-11 Cold Terrace) -- unless this is a backward unspear, when the fork pulls free. A merely-carried
    // RIDER is just left behind against the wall.
    if (IsWall(carriedSausage.x1 + dx, carriedSausage.y1 + dy, carriedSausage.z) || IsWall(carriedSausage.x2 + dx, carriedSausage.y2 + dy, carriedSausage.z)) {
      if (i == speared && dir != Inverse(_stephen.dir)) return false; // speared base can't ride into a wall
      continue;
    }
    // A carried sausage sliding into a cell another (non-carried) sausage holds must shove it the same way; if it can't
    // be pushed, refuse the whole step rather than overlap (3-8 Cold Head).
    s8 hitA = GetSausage(carriedSausage.x1 + dx, carriedSausage.y1 + dy, carriedSausage.z);
    if (hitA != -1 && !(carried & (1 << hitA)) && !(plan.mask & (1 << hitA)))
      if (!PlanSausagePush(hitA, dir, plan)) return false;
    s8 hitB = GetSausage(carriedSausage.x2 + dx, carriedSausage.y2 + dy, carriedSausage.z);
    if (hitB != -1 && hitB != hitA && !(carried & (1 << hitB)) && !(plan.mask & (1 << hitB)))
      if (!PlanSausagePush(hitB, dir, plan)) return false;
    carriedSausage.x1 += dx; carriedSausage.y1 += dy; carriedSausage.x2 += dx; carriedSausage.y2 += dy;
    plan.mask |= (1 << i); // record the carried sausage as moved so the central double-move detector can see it roll
    // A fork-borne carry across the sausage's long axis rolls it; a speared sausage or head hat rides rigidly (excluded
    // from |rollMask|).
    if ((rollMask & (1 << i)) && (carriedSausage.IsHorizontal() ? (dy != 0) : (dx != 0))) carriedSausage.flags ^= Sausage::Rolled;
  }
  // A sausage the climb shoved onto Stephen's head rides along as a hat -- a wall in its path leaves it behind (4-2
  // Toad's Folly). The head-hat stack rides rigidly, but a cantilevered rider rolls, so honor |rollMask| (4-5 Crunchy Leaves).
  for (int i = 0; i < _sausages.Size(); i++)
    if (hatMask & (1 << i)) PlanHatCarry((s8)i, dx, dy, dir, plan, (u16)~rollMask); // rigid unless flagged rolling
  // A carried rider left balanced across a base that just rolled tumbles an extra cell. The step-off carry translates
  // the stack rigidly (no per-sausage hook), so run the central double-move detector to match ordinary steps (3-8 Cold Head).
  return ReactAndCommit(plan);
}

bool Level::HandleStepMotion(Direction dir, bool& handled) {
  // A press along Stephen's facing axis is an ordinary walk (forward or backward); a perpendicular press is a turn, so
  // leave |handled| false and let HandleRotation take it.
  if (dir != _stephen.dir && dir != Inverse(_stephen.dir)) return true;
  handled = true;
  _feat |= F_STEP;

  s8 dx, dy;
  Delta(dir, dx, dy);
  s8 bodyX = _stephen.x + dx;
  s8 bodyY = _stephen.y + dy;
  s8 forkX = _stephen.forkX + dx;
  s8 forkY = _stephen.forkY + dy;
  s8 z = _stephen.z;
  bool forward = (dir == _stephen.dir);

  // The fork leads the way: it may hang out over the void, but it cannot bury itself in a wall.
  if (IsWall(forkX, forkY, z)) return false;
  // The body cannot walk into a wall, and it needs solid footing wherever it lands (the void is not footing).
  if (IsWall(bodyX, bodyY, z)) return false;
  if (!CanWalkOnto(bodyX, bodyY, z)) return false;

  // The leading cell -- the fork's when walking forward, the body's when backing up -- may hold a sausage the step
  // pushes one cell along. Plan the push now; commit it only at the end.
  s8 sausageNo = GetSausage(forward ? forkX : bodyX, forward ? forkY : bodyY, z);
  MovePlan plan = NewPlan();

  // The head hat and fork hat (and their stacks) ride Stephen's step as ONE rigid load -- they translate by the same
  // vector and must never treat each other as obstacles (move-stages.md: motion is one simultaneous event).
  plan.rigidLoad = RidingLoad(GetSausage(_stephen.x, _stephen.y, z + 1)) | RidingLoad(GetSausage(_stephen.forkX, _stephen.forkY, z + 1));

  bool spear = false;
  // Where Stephen's body and fork end up. A carried sausage checks against this final pose to see whether his body or
  // fork ends up holding it (catching a falling sausage and cancelling a double-move).
  Stephen mover = _stephen;
  mover.x = bodyX; mover.y = bodyY;
  mover.forkX = forkX; mover.forkY = forkY;
  if (sausageNo != -1 && !PlanSausagePush(sausageNo, dir, plan, &mover)) {
    if (forward) {
      // PlanSausagePush refuses only when the chain bottoms out against a wall. Walking FORWARD into such a wall-blocked
      // sausage spears it: the fork lodges in the sausage and Stephen steps onto its old cell -- unless his body bounces
      // off a grill (below), when the recoil drags the freshly-speared sausage back onto the grill.
      spear = true;
      _feat |= F_SPEAR;
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

    // The recoil drags the freshly-speared sausage one cell back onto the fork's grill; a rider on it comes along by
    // the same displacement (rigid -- no roll, no double-move). Mark the base moved and carry from its ORIGINAL
    // footprint; Stephen stays put so the carry judges support against his current pose (3-14 Cold Frustration m86).
    plan.mask |= (1 << sausageNo);
    // The recoiled base also rams any sausage in its landing footprint, shoving it opposite the press. If that neighbour
    // CAN'T be shoved, the base can't recoil onto the grill: the fork pulls free (unspear) and Stephen bounces in place.
    bool dragOk = true;
    s8 hitA = GetSausage(spearDragged.x1, spearDragged.y1, spearDragged.z);
    if (hitA != -1 && hitA != sausageNo && !(plan.mask & (1 << hitA)))
      dragOk = PlanSausagePush(hitA, Inverse(dir), plan);
    s8 hitB = GetSausage(spearDragged.x2, spearDragged.y2, spearDragged.z);
    if (dragOk && hitB != -1 && hitB != sausageNo && hitB != hitA && !(plan.mask & (1 << hitB)))
      dragOk = PlanSausagePush(hitB, Inverse(dir), plan);

    if (dragOk) {
      spearDrag = true;
      _feat |= F_SPEARDRAG;
      plan.preCookedMask |= (u16)(1 << sausageNo); // the base browned on the fork's grill above; don't re-cook it
      const Sausage& base = _sausages[sausageNo];
      s8 riderA = GetSausage(base.x1, base.y1, base.z + 1);
      s8 riderB = GetSausage(base.x2, base.y2, base.z + 1);
      if (riderA != -1 && riderA != sausageNo && !(plan.mask & (1 << riderA)))
        PlanSausageCarry(riderA, -dx, -dy, Inverse(dir), plan, nullptr);
      if (riderB != -1 && riderB != sausageNo && riderB != riderA && !(plan.mask & (1 << riderB)))
        PlanSausageCarry(riderB, -dx, -dy, Inverse(dir), plan, nullptr);
    } else {
      plan = NewPlan(); // blocked recoil: discard the partial drag; nothing moves, Stephen just bounces in place
    }
  }

  // --- Commit --- everything is planned into |plan|; the live state moves only at the final Commit.
  s8 headHat = GetSausage(_stephen.x, _stephen.y, z + 1);
  s8 forkHat = GetSausage(_stephen.forkX, _stephen.forkY, z + 1);
  if (spearDrag) plan.sausages[sausageNo] = spearDragged;
  if (!bodyOnGrill) {
    plan.stephen.x = bodyX;
    plan.stephen.y = bodyY;
    plan.stephen.forkX = forkX;
    plan.stephen.forkY = forkY;
    // A sausage held up by Stephen -- on his HEAD or balanced on his bare fork tip -- rides rigidly and never rolls;
    // rolling is reserved for a sausage sliding along the GROUND. (Reaching HandleStepMotion means the fork is un-speared,
    // so a fork hat here always rests on the bare tip, not on a rolling base -- 3-5 Cold Cliff: it translates one cell
    // without flipping.) The rigid stack is those hats plus any sausage whose BOTH ends rest on it; a rider with only one
    // end on the stack still rolls (4-2 Toad's Folly).
    u16 rigidStack = 0;
    if (headHat != -1) rigidStack |= (u16)(1 << headHat);
    if (forkHat != -1) rigidStack |= (u16)(1 << forkHat);
    if (rigidStack) {
      for (bool grew = true; grew; ) {
        grew = false;
        for (int i = 0; i < _sausages.Size(); i++) {
          if (rigidStack & (1 << i)) continue;
          const Sausage& s = _sausages[i];
          s8 below1 = GetSausage(s.x1, s.y1, s.z - 1), below2 = GetSausage(s.x2, s.y2, s.z - 1);
          if (below1 != -1 && below2 != -1 && (rigidStack & (1 << below1)) && (rigidStack & (1 << below2))) { rigidStack |= (u16)(1 << i); grew = true; }
        }
      }
    }
    PlanHatCarry(headHat, dx, dy, dir, plan, rigidStack);
    PlanHatCarry(forkHat, dx, dy, dir, plan, rigidStack); // fork-tip hat is supported by Stephen -> rides rigidly, no roll
  } else if (headHat != -1) {
    // A grill bounce is a step immediately recoiled. The step carries the head hat one cell along the press; the recoil
    // then LEAVES it there if its far end just came to rest on a wall-top or non-moving sausage (3-3 Cold Escarpment),
    // else carries it straight back (a net no-op). If a wall blocks the hat entirely, a CLEAN hat is knocked off his
    // head: it drops and the recoil shoves it one cell back, rolling it (4-6 Gator Paddock move 84).
    const Sausage& hat = _sausages[headHat];
    bool firstOnHead = (hat.x1 == _stephen.x && hat.y1 == _stephen.y);
    s8 farX = firstOnHead ? hat.x2 : hat.x1, farY = firstOnHead ? hat.y2 : hat.y1;
    bool forwardBlocked = IsWall(hat.x1 + dx, hat.y1 + dy, hat.z) || IsWall(hat.x2 + dx, hat.y2 + dy, hat.z);
    if (!forwardBlocked) {
      s8 landing = GetSausage(farX + dx, farY + dy, hat.z - 1);
      bool landsAnchored = IsWall(farX + dx, farY + dy, hat.z - 1)
                        || (landing != -1 && !(plan.mask & (1 << landing))); // a sausage that isn't itself moving away
      if (landsAnchored) {
        // The head hat (plus anything squarely stacked on it) rides rigidly; a cantilevered rider rolls across its axis.
        u16 rigidStack = (u16)(1 << headHat);
        for (bool grew = true; grew; ) {
          grew = false;
          for (int i = 0; i < _sausages.Size(); i++) {
            if (rigidStack & (1 << i)) continue;
            const Sausage& s = _sausages[i];
            s8 below1 = GetSausage(s.x1, s.y1, s.z - 1), below2 = GetSausage(s.x2, s.y2, s.z - 1);
            if (below1 != -1 && below2 != -1 && (rigidStack & (1 << below1)) && (rigidStack & (1 << below2))) { rigidStack |= (u16)(1 << i); grew = true; }
          }
        }
        PlanHatCarry(headHat, dx, dy, dir, plan, rigidStack); // ride the hat along the press; Settle deposits it on its new support
      }
    } else {
      bool clean = !IsWall(farX, farY, hat.z - 1) && GetSausage(farX, farY, hat.z - 1) == -1;
      if (clean)
        PlanSausagePush(headHat, Inverse(dir), plan); // knocked off: recoil shoves it back the way he came, rolling it
    }
  }

  return ReactAndCommit(plan);
}

bool Level::HandleRotation(Direction dir, bool& handled) {
  // A press perpendicular to Stephen's facing turns him in place; a press along his facing axis is a walk, so leave
  // |handled| false and let HandleStepMotion take it.
  if (dir == _stephen.dir || dir == Inverse(_stephen.dir)) return true;
  handled = true;
  _feat |= F_ROTATE;

  MovePlan plan = NewPlan();
  plan.rotating = true;  // The fork swings from its current cell to the perpendicular one; the corner it sweeps through is the diagonal
  // between them -- the fork's destination, offset back along the old facing.
  s8 dx, dy;
  Delta(dir, dx, dy);
  s8 forkX = _stephen.x + dx;
  s8 forkY = _stephen.y + dy;

  Delta(_stephen.dir, dx, dy);
  s8 cornerX = forkX + dx;
  s8 cornerY = forkY + dy;

  s8 z = _stephen.z;

  // A rider carried by a pushed sausage checks whether Stephen's body/fork ends up beneath it (PlanSausageCarry's
  // rotation-hold guard). During a turn the fork ends at its swung-to cell, so seed the carry checks with that POST-turn
  // pose, else a sausage the fork swings directly under is wrongly carried off its new support (3-11 Cold Terrace m69).
  Stephen mover = _stephen;
  mover.dir = dir;
  mover.forkX = forkX;
  mover.forkY = forkY;

  // A wall in the corner blocks the swing outright; a sausage there must be pushable or the turn is refused.
  if (IsWall(cornerX, cornerY, z)) return false;
  s8 cornerSausage = GetSausage(cornerX, cornerY, z);
  if (cornerSausage != -1) {
    if (!PlanSausagePush(cornerSausage, dir, plan)) return false;
  }
  u16 cornerMask = plan.mask; // the corner push lands even when Stephen bonks instead of turning

  // At the fork's destination, a sausage is pushed away from where the fork came from (the inverse of the old facing).
  // A wall-blocked sausage (or a bare wall) bonks -- the turn is abandoned but the corner push stands; a sausage pushed
  // over the void rides out and is settled later.
  bool bonk = false;
  s8 forkSausage = GetSausage(forkX, forkY, z);
  if (forkSausage != -1 && !(plan.mask & (1 << forkSausage))) {
    // PlanSausagePush refuses only when the fork-dest sausage is wall-blocked; a refused push here means the turn bonks.
    if (!PlanSausagePush(forkSausage, Inverse(_stephen.dir), plan, &mover)) {
      bonk = true;
      plan.mask = cornerMask; // keep only the corner push
    }
  } else if (forkSausage == -1 && IsWall(forkX, forkY, z)) {
    bonk = true;
  }

  // A sausage on Stephen's head pivots 90 degrees with him about the head cell -- but only a "clean hat" (far half
  // cantilevered over open space); if the far half rests on another sausage it stays put. The stack above pivots too.
  u16 hatMask = 0;
  if (!bonk) {
    PlanHatRotation(dir, plan, hatMask);
  } else {
    // Even a bonked turn still swings the head-hat as part of the attempt: the fork is what jams (its dest sausage is
    // wall-blocked), so Stephen stays put, yet the hat sweeps and can shove whatever sits in its swept corner. If that
    // shove rolls a sausage clean off the map it drowns for good -- an irreversible loss even though the turn bonks
    // (3-2 Cold Finger m33). Detect it on a scratch plan and refuse the move so the explorer prunes the branch.
    MovePlan scratch = plan;
    u16 scratchHat = 0;
    PlanHatRotation(dir, scratch, scratchHat);
    for (int i = 0; i < _sausages.Size(); i++) {
      const Sausage& s = scratch.sausages[i];
      if (!IsWithinGrid(s.x1, s.y1, s.z) && !IsWithinGrid(s.x2, s.y2, s.z)) return false;
    }
  }

  // --- Commit --- the planned pushes are in |plan|; Stephen swings to his new facing only if he didn't bonk.
  if (!bonk) {
    plan.stephen.dir = dir;
    plan.stephen.forkX = forkX;
    plan.stephen.forkY = forkY;
  }

  return ReactAndCommit(plan);
}

void Level::PlanHatRotation(Direction dir, MovePlan& plan, u16& hatMask) const {
  s8 headX = _stephen.x, headY = _stephen.y, z = _stephen.z;
  s8 hatNo = GetSausage(headX, headY, z + 1);
  if (hatNo == -1) return;

  // Only a "clean hat" pivots: the far half (the end not on the head) must be cantilevered over open space, not
  // resting on another sausage OR on wall terrain -- in either case it's anchored (held in place) and stays put.
  {
    const Sausage& hat = _sausages[hatNo];
    bool firstOnHead = (hat.x1 == headX && hat.y1 == headY);
    s8 farX = firstOnHead ? hat.x2 : hat.x1;
    s8 farY = firstOnHead ? hat.y2 : hat.y1;
    if (GetSausage(farX, farY, z) != -1 || IsWall(farX, farY, z)) return;
  }

  // Rotation taking the old facing onto the new one, applied to every end's offset about the head cell. The whole
  // stack on the head pivots together.
  s8 odx, ody, ndx, ndy;
  Delta(_stephen.dir, odx, ody);
  Delta(dir, ndx, ndy);
  bool cw = (odx * ndy - ody * ndx) > 0;
  for (s8 sausageNo = hatNo, stackZ = z + 1; sausageNo != -1 && !(plan.mask & (1 << sausageNo)); sausageNo = GetSausage(headX, headY, ++stackZ)) {
    Sausage s = _sausages[sausageNo];
    s8 ox1 = s.x1, oy1 = s.y1, ox2 = s.x2, oy2 = s.y2;
    auto rot = [&](s8& x, s8& y) { s8 ox = x - headX, oy = y - headY; if (cw) { x = headX - oy; y = headY + ox; } else { x = headX + oy; y = headY - ox; } };
    rot(s.x1, s.y1);
    rot(s.x2, s.y2);
    // A wall at a destination cell blocks the swing; so does one in the corner an end sweeps through (the cell diagonally
    // between its old and new spot = old + new - head). Either way the whole hat stays put. (3-14 Cold Frustration m101,
    // where the far half would sweep through a 2-tall Wall2's upper cell.)
    //
    // A wall in a CORNER cell (the diagonal an end sweeps through, = old + new - head) blocks the swing before it even
    // starts: the hat stays put and nothing is shoved (3-14 Cold Frustration m101). A wall only at a DESTINATION cell is
    // different -- the sweep's FIRST leg still happens (shoving whatever sits in the corner), and only the landing is
    // blocked, so the hat settles back put AFTER that corner shove, just as a fork's corner push lands even on a bonk
    // (3-2 Cold Finger m42: the corner shove rolls a stacked sausage off the edge while the wall keeps the hat home).
    if (IsWall(ox1 + s.x1 - headX, oy1 + s.y1 - headY, s.z)
     || IsWall(ox2 + s.x2 - headX, oy2 + s.y2 - headY, s.z)) return; // a corner wall blocks the sweep -> hat stays put
    bool destBlocked = IsWall(s.x1, s.y1, s.z) || IsWall(s.x2, s.y2, s.z);

    // The far end sweeps through a corner cell into its destination; a sausage sitting in either is shoved out of the
    // way (corner along the sweep's first leg, destination along its second). The corner shove lands even when the
    // destination is walled; a shove that can't happen leaves the hat put (4-2 move 81).
    if ((ox1 == headX && oy1 == headY) || (ox2 == headX && oy2 == headY)) {
      bool firstOnHead = (ox1 == headX && oy1 == headY);
      s8 oldFarX = firstOnHead ? ox2 : ox1, oldFarY = firstOnHead ? oy2 : oy1;
      s8 newFarX = firstOnHead ? s.x2 : s.x1, newFarY = firstOnHead ? s.y2 : s.y1;
      s8 cornerX = oldFarX + newFarX - headX, cornerY = oldFarY + newFarY - headY;
      auto toDir = [](s8 dx, s8 dy) -> Direction {
        if (dx > 0) return Right; if (dx < 0) return Left;
        if (dy > 0) return Down;  if (dy < 0) return Up;
        return None;
      };
      auto shove = [&](s8 cellX, s8 cellY, s8 pdx, s8 pdy) -> bool {
        s8 other = GetSausage(cellX, cellY, s.z);
        if (other == -1 || other == sausageNo || (plan.mask & (1 << other))) return true;
        return PlanSausagePush(other, toDir(pdx, pdy), plan);
      };
      MovePlan snapshot = plan;
      if (!shove(cornerX, cornerY, cornerX - oldFarX, cornerY - oldFarY)                       // corner cell, first leg -- always
       || (!destBlocked && !shove(newFarX, newFarY, newFarX - cornerX, newFarY - cornerY))) {  // destination, only if reachable
        plan = snapshot;
        return; // a swept sausage couldn't be shoved -> the hat stays put
      }
    }
    if (destBlocked) return; // the corner shove stands, but a wall blocks the landing -> the hat settles back put

    if (s.x1 > s.x2 || (s.x1 == s.x2 && s.y1 > s.y2)) { // restore upper-left; the cook bits ride with the halves
      s8 tx = s.x1; s.x1 = s.x2; s.x2 = tx;
      s8 ty = s.y1; s.y1 = s.y2; s.y2 = ty;
      s.SwapCookBits();
    }
    plan.sausages[sausageNo] = s;
    plan.mask |= (1 << sausageNo);
    hatMask |= (1 << sausageNo);
  }
}

void Level::PlanHatCarry(s8 sausageNo, s8 dx, s8 dy, Direction dir, MovePlan& plan, u16 rigidMask) const {
  // A sausage riding on Stephen's head/fork translates with him. Whether it rolls is decided per sausage by |rigidMask|:
  // the rigid hat stack rides without rolling; any other carried sausage (a fork-borne base, or a cantilevered rider)
  // rolls when carried across its long axis. A wall in its path, or a far end anchored on a non-moving sausage/wall,
  // leaves it behind.
  if (sausageNo == -1 || (plan.mask & (1 << sausageNo))) return;
  Sausage s = _sausages[sausageNo];
  s8 belowA = GetSausage(s.x1, s.y1, s.z - 1);
  s8 belowB = GetSausage(s.x2, s.y2, s.z - 1);
  // Anchored if its far end rests on a NON-MOVING support -- a stationary sausage below, or wall terrain. The support
  // end (on Stephen's head/fork) never trips the wall test (its z-1 is Stephen's own level), so checking both is safe.
  if ((belowA != -1 && belowA != sausageNo && !(plan.mask & (1 << belowA)))
   || (belowB != -1 && belowB != sausageNo && !(plan.mask & (1 << belowB)))
   || IsWall(s.x1, s.y1, s.z - 1) || IsWall(s.x2, s.y2, s.z - 1)) return; // anchored on a non-moving sausage or wall
  if (IsWall(s.x1 + dx, s.y1 + dy, s.z) || IsWall(s.x2 + dx, s.y2 + dy, s.z)) return;
  s8 aboveA = GetSausage(s.x1, s.y1, s.z + 1);
  s8 aboveB = GetSausage(s.x2, s.y2, s.z + 1);
  // Another sausage where it shifts must be shoved first; if it can't be shoved, the hat has nowhere to ride and is left
  // behind for Settle to resolve (3-2 Cold Finger).
  s8 destA = GetSausage(s.x1 + dx, s.y1 + dy, s.z);
  if (destA != -1 && destA != sausageNo && !(plan.mask & (1 << destA)) && !(plan.rigidLoad & (1 << destA)))
    if (!PlanSausagePush(destA, dir, plan)) return;
  s8 destB = GetSausage(s.x2 + dx, s.y2 + dy, s.z);
  if (destB != -1 && destB != sausageNo && destB != destA && !(plan.mask & (1 << destB)) && !(plan.rigidLoad & (1 << destB)))
    if (!PlanSausagePush(destB, dir, plan)) return;
  s.x1 += dx; s.y1 += dy; s.x2 += dx; s.y2 += dy;
  // Only sausages in the rigid hat stack ride rigidly; anyone else rolls when carried across their long axis.
  bool rolls = !(rigidMask & (1 << sausageNo));
  if (rolls && (s.IsHorizontal() ? (dy != 0) : (dx != 0))) s.flags ^= Sausage::Rolled;
  plan.sausages[sausageNo] = s;
  plan.mask |= (1 << sausageNo);
  // Riders are carried the same way; each decides its own roll from |rigidMask| (a rider is only in the stack if both
  // its ends rest on it), so a rider with a cantilevered end rolls even while the head hat beneath it stays rigid.
  PlanHatCarry(aboveA, dx, dy, dir, plan, rigidMask);
  if (aboveB != aboveA) PlanHatCarry(aboveB, dx, dy, dir, plan, rigidMask);
}

bool Level::PlanSausagePush(s8 sausageNo, Direction dir, MovePlan& plan, const Stephen* mover) const {
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

  // A push across the sausage's long axis rolls it (flipping which side faces down); a push along the axis slides it.
  // Motion only -- gravity (Settle) and heat (CookMoved) run after commit, so it keeps its level even if now cantilevered.
  bool rolls = sausage.IsHorizontal() ? (dir == Up || dir == Down) : (dir == Left || dir == Right);
  sausage.x1 = x1;
  sausage.y1 = y1;
  sausage.x2 = x2;
  sausage.y2 = y2;
  if (rolls) sausage.flags ^= Sausage::Rolled;
  plan.sausages[sausageNo] = sausage;
  plan.mask |= (1 << sausageNo);

  // This base is now committed to moving (it cleared the wall/chain checks above), so everything riding on it co-moves
  // by the same vector: none of these riders may ram another. Fold them into the co-moving set BEFORE carrying them, so
  // one rider's carry never shoves (and rolls) a sibling rider it laps over -- 3-2 Cold Finger m41: the fork-hat and the
  // mid's rider both slide, and the hat's carry must not ram the rider. (Only riders of a base that actually moves are
  // added, so a refused push leaves a stationary stack un-marked -- 3-2 Cold Finger m40 keeps its fork hat anchored.)
  plan.rigidLoad |= RidingLoad(sausageNo);

  // A sausage stacked directly on top rides along by the same displacement, but it shares THIS base's torsion: it rolls
  // only when the base itself rolled. A base that SLIDES (moves along its own axis) carries its passenger flat -- the
  // game's zero-torsion rule (GameState.CalculateTorsion): equal support+rider speed => no roll. Read our ORIGINAL
  // footprint, since plan.sausages now holds our new spot.
  const Sausage& orig = _sausages[sausageNo];
  s8 aboveA = GetSausage(orig.x1, orig.y1, orig.z + 1);
  s8 aboveB = GetSausage(orig.x2, orig.y2, orig.z + 1);
  if (aboveA != -1 && aboveA != sausageNo && !(plan.mask & (1 << aboveA)))
    if (!PlanSausageCarry(aboveA, dx, dy, dir, plan, mover, /*rigid=*/!rolls)) return false;
  if (aboveB != -1 && aboveB != sausageNo && aboveB != aboveA && !(plan.mask & (1 << aboveB)))
    if (!PlanSausageCarry(aboveB, dx, dy, dir, plan, mover, /*rigid=*/!rolls)) return false;
  return true;
}

u16 Level::RidingLoad(s8 seed) const {
  if (seed == -1) return 0;
  u16 load = (u16)(1 << seed);
  for (bool grew = true; grew; ) {
    grew = false;
    for (int i = 0; i < _sausages.Size(); i++) {
      if (load & (1 << i)) continue;
      const Sausage& s = _sausages[i];
      s8 b1 = GetSausage(s.x1, s.y1, s.z - 1), b2 = GetSausage(s.x2, s.y2, s.z - 1);
      if ((b1 != -1 && (load & (1 << b1))) || (b2 != -1 && (load & (1 << b2)))) { load |= (u16)(1 << i); grew = true; }
    }
  }
  return load;
}

bool Level::PlanSausageCarry(s8 sausageNo, s8 dx, s8 dy, Direction dir, MovePlan& plan, const Stephen* mover, bool rigid, bool baseDragRolled) const {
  const Sausage& orig = _sausages[sausageNo];
  const Stephen& actor = mover ? *mover : _stephen;

  // Stationary support -- terrain (ground floor or wall-top) or a non-moving sausage -- keeps a sausage from being
  // carried at all: the base slides out from under it and it stays put.
  auto anchoredAt = [&](s8 cellX, s8 cellY, s8 cellZ) -> bool {
    if (cellZ < 0 || IsWall(cellX, cellY, cellZ)) return false;
    if (cellZ == 0 ? CanWalkOnto(cellX, cellY, 0) : IsWall(cellX, cellY, cellZ - 1)) return true;
    s8 below = GetSausage(cellX, cellY, cellZ - 1);
    return below != -1 && below != sausageNo && !(plan.mask & (1 << below));
  };
  if (anchoredAt(orig.x1, orig.y1, orig.z) || anchoredAt(orig.x2, orig.y2, orig.z)) return true; // anchored -> stays put

  // A rider that would merely SLIDE along its own long axis (motion parallel to its length, not a roll across it) is NOT
  // carried by a base that is itself a DRAGGED roller -- a rider that only rolled because ITS base rolled (the game's
  // reverse torsion, -1). The doubly-rolled contact slips out from under the parallel rider instead of translating it,
  // so it stays put and the post-commit Settle drops it. (3-5 Cold Cliff move 63: sausage A lies E-W atop the rolling
  // C-on-B stack, its west end cantilevered over the void; the game leaves it behind and it falls in.)
  bool wouldRoll = orig.IsHorizontal() ? (dir == Up || dir == Down) : (dir == Left || dir == Right);
  if (baseDragRolled && !wouldRoll) return true; // parallel slide on a dragged roller -> not carried, falls

  // During a TURN, Stephen's body OR fork counts as a wall in its FINAL cell. A rider end on his body is held (3-3 Cold
  // Escarpment m69). A rider end over the fork is held too -- but if a sausage sits in that fork cell, the fork only
  // takes over once that base slides OUT of it (leftBehind) AND the rider's OTHER end isn't itself on a moving sausage;
  // otherwise the rider is carried with its base (3-11 Cold Terrace m69 holds; 3-2 Cold Finger m12 carries).
  if (plan.rotating && actor.HasFork()) {
    auto bodyUnder = [&](s8 cellX, s8 cellY, s8 cellZ) -> bool {
      return actor.x == cellX && actor.y == cellY && actor.z == cellZ - 1;
    };
    auto forkUnder = [&](s8 cellX, s8 cellY, s8 cellZ) -> bool {
      return actor.forkX == cellX && actor.forkY == cellY && actor.forkZ == cellZ - 1;
    };
    auto forkHolds = [&](s8 ex, s8 ey, s8 ox, s8 oy) -> bool {
      if (!forkUnder(ex, ey, orig.z)) return false;
      s8 base = GetSausage(ex, ey, orig.z - 1);            // pre-move sausage in the fork's cell
      if (base == -1) return true;                          // bare fork -> it alone holds the rider
      bool leftBehind = (plan.mask & (1 << base)) && !_sausages[base].IsAt(ex - dx, ey - dy, orig.z - 1);
      // The fork catches the rider only when it sits STRICTLY under the rider's LEADING end in the roll direction. A
      // rider whose free end points the SAME way as the base's roll tumbles AWAY from the fork -- the fork is behind it,
      // so it double-rolls off instead of being held (3-5 Cold Cliff move 27). A rider lying PERPENDICULAR to the slide
      // (both ends projecting equally) likewise rides along on its sliding base rather than being caught, so only a
      // fork strictly under the leading end holds it (3-2 Cold Finger m77: the horizontal rider is carried north on the
      // vertical base sliding out from under it).
      if (leftBehind && (s8)(ex * dx + ey * dy) <= (s8)(ox * dx + oy * dy)) return false;
      s8 otherBase = GetSausage(ox, oy, orig.z - 1);        // pre-move sausage under the rider's OTHER end
      bool otherOnMoving = otherBase != -1 && (plan.mask & (1 << otherBase));
      return leftBehind && !otherOnMoving;
    };
    if (bodyUnder(orig.x1, orig.y1, orig.z) || bodyUnder(orig.x2, orig.y2, orig.z)) return true;
    if (forkHolds(orig.x1, orig.y1, orig.x2, orig.y2) || forkHolds(orig.x2, orig.y2, orig.x1, orig.y1)) return true;
  }

  s8 aboveA = GetSausage(orig.x1, orig.y1, orig.z + 1);
  s8 aboveB = GetSausage(orig.x2, orig.y2, orig.z + 1);

  // Double-move is resolved post-hoc by MarkDoubleMoves, so a plain carry just slides the rider one cell.
  Sausage sausage = orig;
  sausage.x1 += dx;
  sausage.y1 += dy;
  sausage.x2 += dx;
  sausage.y2 += dy;

  // Carry destination rammed into a wall -> it stays put; the later Settle drops it if the base slid out from under it.
  if (IsWall(sausage.x1, sausage.y1, sausage.z) || IsWall(sausage.x2, sausage.y2, sausage.z)) return true;

  // A non-carried sausage where this carried sausage lands is shoved the same way first (the carry propagates as a
  // push); if it can't be shoved, this sausage is left behind like the wall case above (3-2 Cold Finger).
  s8 destA = GetSausage(sausage.x1, sausage.y1, sausage.z);
  if (destA != -1 && destA != sausageNo && !(plan.mask & (1 << destA)) && !(plan.rigidLoad & (1 << destA)))
    if (!PlanSausagePush(destA, dir, plan)) return true;
  s8 destB = GetSausage(sausage.x2, sausage.y2, sausage.z);
  if (destB != -1 && destB != sausageNo && destB != destA && !(plan.mask & (1 << destB)) && !(plan.rigidLoad & (1 << destB)))
    if (!PlanSausagePush(destB, dir, plan)) return true;

  bool rolls = sausage.IsHorizontal() ? (dir == Up || dir == Down) : (dir == Left || dir == Right);
  bool rolled = rolls && !rigid; // a rider on a rigidly-dragged (fork-held) or sliding base inherits its zero torsion -> no roll
  if (rolled) sausage.flags ^= Sausage::Rolled;
  plan.sausages[sausageNo] = sausage;
  plan.mask |= (1 << sausageNo);

  // Anything stacked on THIS rider shares its torsion in turn: it rolls only if this rider actually rolled. And if this
  // rider rolled, it becomes a DRAGGED roller for whatever sits on it -- a parallel passenger there is not carried.
  if (aboveA != -1 && aboveA != sausageNo && !(plan.mask & (1 << aboveA)))
    if (!PlanSausageCarry(aboveA, dx, dy, dir, plan, mover, /*rigid=*/!rolled, /*baseDragRolled=*/rolled)) return false;
  if (aboveB != -1 && aboveB != sausageNo && aboveB != aboveA && !(plan.mask & (1 << aboveB)))
    if (!PlanSausageCarry(aboveB, dx, dy, dir, plan, mover, /*rigid=*/!rolled, /*baseDragRolled=*/rolled)) return false;
  return true;
}

bool Level::CookSausage(Sausage& sausage) const {
  // Each end resting on a grill browns its down-facing side: the unrolled ("A") face normally, the rolled ("B") face
  // once the sausage has been flipped.
  u8 sides = 0;
  if (IsGrill(sausage.x1, sausage.y1, sausage.z)) sides |= Sausage::Cook1A;
  if (IsGrill(sausage.x2, sausage.y2, sausage.z)) sides |= Sausage::Cook2A;
  if (sides == 0) return true;
  if (sausage.flags & Sausage::Rolled) sides *= 2; // shift A-faces to their rolled-over B-faces
  if (sausage.flags & sides) return false;         // that face is already cooked -- it would burn
  sausage.flags |= sides;
  _feat |= F_COOK;
  return true;
}

bool Level::CookMoved(MovePlan& plan, u16 movedMask, u16 preCookedMask) {
  for (int i = 0; i < _sausages.Size(); i++) {
    if (!(movedMask & (1 << i)) || (preCookedMask & (1 << i))) continue;
    if (!CookSausage(plan.sausages[i])) return false; // would burn
  }
  return true;
}

Level::MovePlan Level::NewPlan() const {
  // Seed the plan with a full copy of the live state, so it's a complete working tableau the move can advance without
  // touching the object.
  MovePlan plan;
  plan.stephen = _stephen;
  for (int i = 0; i < _sausages.Size(); i++) plan.sausages[i] = _sausages[i];
  return plan;
}

void Level::Commit(const MovePlan& plan) {
  // The one and only mutation of the live state, reached after every stage has succeeded -- nothing to roll back.
  _stephen = plan.stephen;
  for (int i = 0; i < _sausages.Size(); i++) _sausages[i] = plan.sausages[i];
}

bool Level::PlanHasOverlap(const MovePlan& plan) const {
  // Two sausages sharing a cell is impossible in the real game: it means a carry lapped onto a co-mover that didn't
  // actually vacate its cell (an anchored or wall-blocked rider wrongly folded into rigidLoad). The game instead can't
  // place the carried sausage, so it drops (and drowns off the map). Level has no "lost" state, so we refuse the move
  // -- the explorer then never walks this path, matching the game's dead-end (3-2 Cold Finger: a speared drag laps a
  // wall-pinned rider onto a co-mover).
  for (int i = 0; i < _sausages.Size(); i++) {
    const Sausage& a = plan.sausages[i];
    for (int j = i + 1; j < _sausages.Size(); j++) {
      const Sausage& b = plan.sausages[j];
      if (b.IsAt(a.x1, a.y1, a.z) || b.IsAt(a.x2, a.y2, a.z)) return true;
    }
  }
  return false;
}

bool Level::ReactAndCommit(MovePlan& plan) {
  // Stages 5-7 (gravity -> heat -> knock-on) + the single commit. The live _sausages/_stephen are still the pre-move
  // layout the gravity pass needs, so pass them straight in (no snapshot). |preCookedMask| excludes sausages already
  // browned inline (a spear-drag) from CookMoved; double-movers are held back from Settle and cooked by DoubleMove.
  MarkDoubleMoves(plan, _sausages.begin()); // resolve any carried double-mover before gravity settles it
  u16 movedMask = plan.mask;
  if (!Settle(plan, movedMask, _sausages.begin(), _stephen, plan.doubleMoveMask)) return false;
  if (!CookMoved(plan, movedMask, plan.doubleMoveMask | plan.preCookedMask)) return false;
  if (!DoubleMove(plan)) return false;
  if (PlanHasOverlap(plan)) return false; // a carry produced an impossible two-in-one-cell state -> refuse
  Commit(plan);
  return true;
}

s8 Level::GetPlannedSausage(const MovePlan& plan, s8 x, s8 y, s8 z) const {
  if (z < 0) return -1;
  for (int i = 0; i < _sausages.Size(); i++)
    if (plan.sausages[i].IsAt(x, y, z)) return (s8)i;
  return -1;
}

bool Level::SausageSupported(const MovePlan& plan, s8 x, s8 y, s8 z) const {
  // Stephen's body or held fork, at the plan's post-move pose, directly beneath holds a sausage up -- even out over the
  // void at the grid's edge (4-4 Foul Fen off-path). Checked FIRST so an off-grid cell isn't dismissed before we notice
  // the fork under it.
  const Stephen& actor = plan.stephen;
  if (actor.x == x && actor.y == y && actor.z == z - 1) return true;
  if (actor.HasFork() && actor.forkX == x && actor.forkY == y && actor.forkZ == z - 1) return true;
  // An off-grid cell is otherwise never a footing (this also subsumes the z<0 guard). Without it, a sausage rolled
  // ENTIRELY off an edge could be "supported" by another hanging half-off the grid below it (3-12 Cold Horizon).
  if (!IsWithinGrid(x, y, z)) return false;
  // Terrain footing: the ground floor at z==0, or a wall-top (a wall solid at z-1) for z>=1.
  if (z == 0 ? CanWalkOnto(x, y, 0) : IsWall(x, y, z - 1)) return true;
  // A sausage directly below in the WORKING tableau (not the live, pre-move _sausages).
  if (GetPlannedSausage(plan, x, y, z - 1) != -1) return true;
  return false;
}

bool Level::Settle(MovePlan& plan, u16& movedMask, const Sausage* preMove, const Stephen& prevStephen, u16 exclude) {
  // The sausage on the held fork is carried rigidly, not subject to gravity -- exempt it from the fall. The fork is at
  // the plan's post-move pose; the speared sausage is whatever sits in its cell in the working tableau.
  s8 speared = GetPlannedSausage(plan, plan.stephen.forkX, plan.stephen.forkY, plan.stephen.forkZ);

  // The fork holds up the whole stack that rests (post-move) on the speared sausage, not just the sausage itself -- the
  // entire load rides the fork and stays put, even out over the void where SausageSupported would otherwise bail on the
  // off-grid cell (3-2 Cold Finger: a rider dragged off the grid's edge on the speared base must not drop).
  u16 forkHeld = 0;
  if (speared != -1) {
    forkHeld = (u16)(1 << speared);
    for (bool grew = true; grew; ) {
      grew = false;
      for (int i = 0; i < _sausages.Size(); i++) {
        if (forkHeld & (1 << i)) continue;
        const Sausage& s = plan.sausages[i];
        s8 b1 = GetPlannedSausage(plan, s.x1, s.y1, s.z - 1);
        s8 b2 = GetPlannedSausage(plan, s.x2, s.y2, s.z - 1);
        if ((b1 != -1 && (forkHeld & (1 << b1))) || (b2 != -1 && (forkHeld & (1 << b2)))) { forkHeld |= (u16)(1 << i); grew = true; }
      }
    }
  }

  // A sausage that rode on Stephen pre-move (resting on his body or fork) becomes subject to gravity when he steps
  // out from under it: seed it active so the fall pass below drops it if its footing is now gone.
  for (int i = 0; i < _sausages.Size(); i++) {
    if ((movedMask & (1 << i)) || (exclude & (1 << i))) continue;
    const Sausage& pre = preMove[i];
    bool rode = (pre.x1 == prevStephen.x && pre.y1 == prevStephen.y && pre.z - 1 == prevStephen.z)
             || (pre.x2 == prevStephen.x && pre.y2 == prevStephen.y && pre.z - 1 == prevStephen.z)
             || (pre.x1 == prevStephen.forkX && pre.y1 == prevStephen.forkY && pre.z - 1 == prevStephen.forkZ)
             || (pre.x2 == prevStephen.forkX && pre.y2 == prevStephen.forkY && pre.z - 1 == prevStephen.forkZ)
             // ...or was SPEARED on the fork (same z, sharing the fork's cell). When a backward step pulls the fork free
             // it leaves the sausage put but no longer held up, so it must fall if nothing else supports it (3-11 Cold
             // Terrace: backing a speared sausage off the top edge drops it into the void). A still-speared/dragged
             // sausage is already in |movedMask|, so this only fires once the fork has actually let go.
             || (prevStephen.HasFork() && pre.IsAt(prevStephen.forkX, prevStephen.forkY, prevStephen.forkZ));
    if (rode) movedMask |= (1 << i);
  }

  // Only sausages this move disturbed are subject to gravity: the ones it moved, plus any that were (in the PRE-move
  // layout) stacked on a disturbed one -- transitively, so a whole tower comes along when its base slides out. A sausage
  // already floating before this move is left alone. A pending double-move (|exclude|) is held back -- its own
  // DoubleMove pass owns its fall.
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
      if ((forkHeld & (1 << i)) || !(active & (1 << i))) continue;
      Sausage& sausage = plan.sausages[i];
      if (SausageSupported(plan, sausage.x1, sausage.y1, sausage.z) || SausageSupported(plan, sausage.x2, sausage.y2, sausage.z)) continue;
      if (sausage.z <= 0) return false; // fell out of the bottom of the world -> the move is refused
      sausage.z--;
      _feat |= F_DROP;
      movedMask |= (1 << i);
      changed = true;
    }
  }
  return true;
}

void Level::MarkDoubleMoves(MovePlan& plan, const Sausage* preMove) const {
  for (int rider = 0; rider < _sausages.Size(); rider++) {
    if (!(plan.mask & (1 << rider)) || (plan.doubleMoveMask & (1 << rider))) continue; // only just-moved sausages, not already flagged
    const Sausage& orig = preMove[rider];
    const Sausage& now = plan.sausages[rider];
    // Infer this sausage's own move direction from its one-cell displacement, so a single detector serves every path
    // (step, spear, log-roll, rotation, ladder). Only a rigid unit cardinal shift can double-move.
    s8 mdx = now.x1 - orig.x1, mdy = now.y1 - orig.y1;
    if (now.x2 - orig.x2 != mdx || now.y2 - orig.y2 != mdy) continue; // not a rigid translation
    if (mdx * mdx + mdy * mdy != 1) continue;                         // not a unit cardinal step
    Direction dir = mdx > 0 ? Right : mdx < 0 ? Left : mdy > 0 ? Down : Up;
    // Only a sausage aligned WITH its motion slides the extra cell; one shifting across its axis rolled instead.
    bool aligned = orig.IsVertical() ? (dir == Up || dir == Down) : (dir == Left || dir == Right);
    if (!aligned) continue;
    // Stephen's body, or a BARE fork, under an end holds the rider to a single cell -- judged at his PRE-move pose.
    auto heldEnd = [&](s8 ex, s8 ey) -> bool {
      if (_stephen.x == ex && _stephen.y == ey && _stephen.z == orig.z - 1) return true;
      return _stephen.HasFork() && _stephen.forkX == ex && _stephen.forkY == ey && _stephen.forkZ == orig.z - 1
          && GetSausage(ex, ey, orig.z - 1) == -1;
    };
    if (heldEnd(orig.x1, orig.y1) || heldEnd(orig.x2, orig.y2)) continue;
    // Locate the rider's LEADING end (the one ahead in the motion, largest projection on the step) up front: both the
    // pin overhead and the shelf ahead are judged against it.
    int dot1 = orig.x1 * mdx + orig.y1 * mdy, dot2 = orig.x2 * mdx + orig.y2 * mdy;
    s8 leadX = dot1 >= dot2 ? orig.x1 : orig.x2;
    s8 leadY = dot1 >= dot2 ? orig.y1 : orig.y2;
    // A wall or sausage directly above the LEADING end pins it: the extra tumble needs clear space overhead to roll up
    // and over, so a tile there cancels the double-move and it shifts only the one cell (3-5 Cold Cliff: a head hat sits
    // on the mid sausage's leading end as the log rolls out from under it, so the mid moves once, not twice). A blocker
    // above only the TRAILING end does NOT pin it -- the rider slides out from under that as it advances (3-2 Cold
    // Finger m40: an Over3 overhang caps the mid's west end, yet it slides east away from it and still double-moves).
    if (IsWall(leadX, leadY, orig.z + 1)) continue;
    s8 above = GetSausage(leadX, leadY, orig.z + 1);
    if (above != -1 && above != rider) {
      // A sausage overhead pins the leading end only if it stays perched there through the slide -- i.e. it travels in
      // lockstep with the rider (a head hat riding Stephen keeps its far end on the mid: LogRollHeadHatOverMovingMid).
      // A STATIONARY sausage above does NOT follow: the rider slides out from under it after the first cell, its leading
      // end clears, and it still double-moves (3-5 Cold Cliff m55: an anchored vertical rests on the mid's leading end,
      // but the mid slides east out from beneath it and drops onto the grill).
      const Sausage& ao = preMove[above], & an = plan.sausages[above];
      bool followsRider = (plan.mask & (1 << above))
          && an.x1 - ao.x1 == mdx && an.y1 - ao.y1 == mdy && an.x2 - ao.x2 == mdx && an.y2 - ao.y2 == mdy;
      if (followsRider) continue;
    }
    // The extra tumble is imparted only by a PERPENDICULAR base that actually ROLLED under it; a parallel base, or one
    // that merely slid, imparts none. Read the base and its roll (Rolled flag flipped) from the pre-move layout.
    bool baseRolled = false, onParallel = false;
    auto examine = [&](s8 ex, s8 ey) {
      s8 b = GetSausage(ex, ey, orig.z - 1);
      if (b == -1 || b == rider) return;
      if (_sausages[b].IsHorizontal() == orig.IsHorizontal()) onParallel = true;
      else if (((preMove[b].flags ^ plan.sausages[b].flags) & Sausage::Rolled) != 0) baseRolled = true;
    };
    examine(orig.x1, orig.y1);
    examine(orig.x2, orig.y2);
    // A base under the TRAILING end (opposite the leading one) that merely SLID -- a co-mover translating without
    // rolling -- plants that end and pins the rider to a single cell: the trailing end can't lift up and over for the
    // extra tumble (3-2 Cold Finger m77: the mid's west base is fork-dragged east beneath it, so the mid rides one cell
    // even though its east base rolled). A rolling base under the trailing end instead flicks it forward (m40), and a
    // cantilevered trailing end is free -- both still double-move.
    s8 trailX = dot1 >= dot2 ? orig.x2 : orig.x1;
    s8 trailY = dot1 >= dot2 ? orig.y2 : orig.y1;
    s8 trailBase = GetSausage(trailX, trailY, orig.z - 1);
    bool trailPlanted = trailBase != -1 && trailBase != rider && (plan.mask & (1 << trailBase))
        && ((preMove[trailBase].flags ^ plan.sausages[trailBase].flags) & Sausage::Rolled) == 0;
    // The extra flick only lands if the rider keeps sliding after the first cell. What halts it is its LEADING end
    // settling onto a raised terrain shelf whose top sits at the rider's own level: the shelf catches the front and the
    // game rolls the push back to a single cell (GameState.ApplyPassiveForce undoes the speed+1 push when the post-push
    // footprint rests on non-moving ground). A leading end that clears the shelf -- over open void or lower ground after
    // one cell -- carries the rider the full two cells. Judge the leading end at its SINGLE-cell destination, against
    // the ORIGINAL layout. (3-8 Cold Head m48 seed beef: the leading end lands on a height-1 shelf -> single; m61 seed
    // a: the leading end clears into void -> double.)
    bool caughtByShelf = IsWall(leadX + mdx, leadY + mdy, orig.z - 1);
    if (baseRolled && !onParallel && !caughtByShelf && !trailPlanted) {
      plan.doubleMoveMask |= (1 << rider);
      plan.doubleMoveDir[rider] = dir;
    }
  }
}

bool Level::DoubleMove(MovePlan& plan) {
  if (plan.doubleMoveMask == 0) return true;
  _feat |= F_DOUBLEMOVE;

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


bool Level::SausageBlocked(s8 sausageNo, Direction dir) const {
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

void Level::Delta(Direction dir, s8& dx, s8& dy) const {
  dx = 0;
  dy = 0;
  if (dir == Up)         dy = -1;
  else if (dir == Down)  dy = +1;
  else if (dir == Left)  dx = -1;
  else if (dir == Right) dx = +1;
}
