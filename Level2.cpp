#include "Level2.h"


State Level::GetState(bool sort) const {
  State s{_stephen};
  assert(sizeof(s.sausages) / sizeof(Sausage) == _sausages.Size());
  if (sort) {
    // Sorting sausages reduces the number of redundant states (especially in large-sausage levels).
    _sausages.SortedCopyIntoArray(s.sausages, sizeof(s.sausages), [](const Sausage& a, const Sausage& b) -> s8 {
      if (a.x1 != b.x1) return a.x1 - b.x1;
      if (a.y1 != b.y1) return a.y1 - b.y1;
      if (a.z != b.z) return a.z - b.z;
      if (a.x2 != b.x2) return a.x2 - b.x2;
      if (a.y2 != b.y2) return a.y2 - b.y2;
      if (a.flags != b.flags) return a.flags - b.flags;
      return 0;
    });
  } else {
    // Disabled for the 'cost' computation, which needs to compare sausage positions between consecutive states
    _sausages.CopyIntoArray(s.sausages, sizeof(s.sausages));
  }

#if OVERWORLD_HACK // In the overworld, sausages are walls until collected.
  for (int i = 0; i < _levelEntrances.Size(); i++) {
    const Sausage& sausage = _overworldSausages[i];
    // The sausage is completed if we've already dropped the sausage-walls,
    // or if stephen is currently on the level entrance (and thus completed the level).
    if (!IsWall(sausage.x1, sausage.y1, sausage.z) || _stephen == _levelEntrances[i]) {
      s.overworldSausages |= 1ull << i;
    }
  }

  // If all other levels are cleared, mark the final sausage as cleared
  u64 allLevelsCleared = (1ull << _levelEntrances.Size()) - 1;
  if (s.overworldSausages == allLevelsCleared) {
    s.overworldSausages |= (allLevelsCleared + 1);
  }
#endif

  return s;
}

void Level::SetState(const State& state) {
  _stephen = state.stephen;
  _sausages.CopyFromArray(state.sausages, sizeof(state.sausages));

#if OVERWORLD_HACK // In the overworld, sausages are walls until collected.
  for (int i = 0; i < _overworldSausages.Size(); i++) {
    const Sausage& sausage = _overworldSausages[i];
    if ((state.overworldSausages >> i) & 1) {
      _walls(sausage.x1, sausage.y1) &= ~(2 << sausage.z);
      _walls(sausage.x2, sausage.y2) &= ~(2 << sausage.z);
    } else {
      _walls(sausage.x1, sausage.y1) |= (2 << sausage.z);
      _walls(sausage.x2, sausage.y2) |= (2 << sausage.z);
    }
  }
#endif
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

  // Reconnect a thrown fork: the instant Stephen's body ends a move directly behind it -- the fork lying one cell ahead
  // in his facing at his own level AND pointing the way he faces -- he takes it back into his hand (TryReattachFork:
  // fork.pos == pos + dir && fork.direction == dir). A fork facing crosswise is not picked up.
  if (!_stephen.HasFork()) {
    auto [dx, dy] = Delta(_stephen.dir);
    if (_stephen.forkX == _stephen.x + dx && _stephen.forkY == _stephen.y + dy && _stephen.forkZ == _stephen.z
        && _stephen.forkDir == _stephen.dir)
      _stephen.forkDir = None;
  }

  return true;
}

bool Level::HandleBurnedStep(Direction dir, bool& handled) {
  // Stage 8 (outcomes): a body that comes to rest on a grill recoils straight back the way it came, as its own move.
  // Applied commonly so EVERY motion (step, ladder climb/descent, log-roll, spear) bounces consistently.
  if (!IsGrill(_stephen.x, _stephen.y, _stephen.z)) return true;
  handled = true;
  return Move(Inverse(dir));
}

bool Level::HandleSpearedMotion(Direction dir, bool& handled) {
  // If the fork is lodged in a sausage, Stephen drags it rigidly; otherwise leave |handled| false for the step/turn
  // classification. The speared sausage is whatever the fork currently occupies.
  // A DETACHED fork merely lying inside a sausage (thrown, then the sausage settled around it) is inert -- only a HELD
  // fork spears. Without this guard a forkless Stephen would drag the sausage his loose fork happens to sit in.
  if (!_stephen.HasFork()) return true;
  s8 sausageNo = GetSausage(_stephen.forkX, _stephen.forkY, _stephen.forkZ);
  if (sausageNo == -1) return true;
  handled = true;

  auto [dx, dy] = Delta(dir);
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
  // rolling it. move-stages.md: motion is one event.
  plan.rigidLoad = CarriedMask(sausageNo) | CarriedMask(GetSausage(_stephen.x, _stephen.y, z + 1));
  bool unspear = SausageBlocked(sausageNo, dir);
  if (unspear) {
    if (dir != Inverse(_stephen.dir)) return false;
    // Pulling free is a backward step, so the body still pushes any sausage at its destination (the speared one
    // stays put). If that sausage can't move, the move is refused.
    s8 bodySausage = GetSausage(bodyX, bodyY, z);
    if (bodySausage != -1 && bodySausage != sausageNo)
      if (!PlanSausagePush(bodySausage, dir, plan)) return false;
    // A sausage resting on the fork TIP (forkZ+1) rides out with the retreating fork ONLY if BOTH its ends are
    // cantilevered -- neither resting on a wall nor a non-moving sausage. Its NEAR end sits over the fork cell, i.e.
    // directly on the SPEARED base, which stays put through an unspear, so that end is virtually always anchored and
    // the hat simply stays. When it does ride, it goes one cell -- rolling across its axis, or
    // flicking an extra cell (a double-move) when aligned with the pull.
    // A sausage ALSO resting on Stephen's head is a head hat -- left to the head-hat carry below.
    s8 forkHat = GetSausage(_stephen.forkX, _stephen.forkY, _stephen.forkZ + 1);
    if (forkHat != -1 && forkHat != sausageNo && forkHat != GetSausage(_stephen.x, _stephen.y, _stephen.z + 1)
        && !(plan.mask & (1ull << forkHat))) {
      Sausage hat = _sausages[forkHat];
      auto [farX, farY] = hat.OtherEnd(_stephen.forkX, _stephen.forkY);
      s8 farBelow = GetSausage(farX, farY, hat.z - 1);
      bool farAnchored = IsWall(farX, farY, hat.z - 1) || (farBelow != -1 && !(plan.mask & (1ull << farBelow)));
      // The hat's NEAR (fork-tip) end sits directly over the fork cell -- i.e. on the SPEARED base, which stays put
      // during an unspear -- so that end is grounded through a non-moving sausage and the hat stays even when its far
      // end cantilevers off the tip.
      s8 nearBelow = GetSausage(_stephen.forkX, _stephen.forkY, hat.z - 1);
      bool nearAnchored = IsWall(_stephen.forkX, _stephen.forkY, hat.z - 1) || (nearBelow != -1 && !(plan.mask & (1ull << nearBelow)));
      bool blocked = IsWall(hat.x1 + dx, hat.y1 + dy, hat.z) || IsWall(hat.x2 + dx, hat.y2 + dy, hat.z);
      if (!farAnchored && !nearAnchored && !blocked) {
        bool aligned = hat.IsHorizontal() ? (dx != 0) : (dy != 0);
        hat.x1 += dx; hat.y1 += dy; hat.x2 += dx; hat.y2 += dy;
        if (!aligned) hat.flags ^= Sausage::Rolled;  // a perpendicular ride rolls it across its axis
        plan.sausages[forkHat] = hat;
        plan.mask |= (1ull << forkHat);
        if (aligned) { plan.doubleMoveMask |= (1ull << forkHat); plan.doubleMoveDir[forkHat] = dir; } // aligned -> flicks an extra cell
      }
    }
  } else {
    // The dragged sausage pushes any sausage it runs into (a chain, obeying the normal push rules); a chain member that
    // can't move refuses the drag. Mark this sausage moving first so the chain never tries to push it back.
    plan.mask |= (1ull << sausageNo);
    s8 ahead1 = GetSausage(x1, y1, z);
    if (ahead1 != -1 && ahead1 != sausageNo && !(plan.mask & (1ull << ahead1)))
      if (!PlanSausagePush(ahead1, dir, plan)) return false;
    s8 ahead2 = GetSausage(x2, y2, z);
    if (ahead2 != -1 && ahead2 != sausageNo && ahead2 != ahead1 && !(plan.mask & (1ull << ahead2)))
      if (!PlanSausagePush(ahead2, dir, plan)) return false;

    moved.x1 = x1;
    moved.y1 = y1;
    moved.x2 = x2;
    moved.y2 = y2;
    // The sausage needs no ground -- the fork holds it up, so it can ride out over the void. CookMoved browns it later.
    plan.sausages[sausageNo] = moved;

    // A sausage riding on top of the speared one is carried along the drag. It's held up by the fork-borne stack (whose
    // base is dragged rigidly, never rolling), so like the real game it inherits that zero torsion and does NOT roll --
    // rolling is reserved for a sausage sliding along the ground. Use Stephen's post-move pose so the
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
      if (aboveA != -1 && aboveA != sausageNo && !(plan.mask & (1ull << aboveA)))
        if (!PlanSausageCarry(aboveA, dx, dy, dir, plan, &mover, /*rigid=*/true)) return false;
      if (aboveB != -1 && aboveB != sausageNo && aboveB != aboveA && !(plan.mask & (1ull << aboveB)))
        if (!PlanSausageCarry(aboveB, dx, dy, dir, plan, &mover, /*rigid=*/true)) return false;
    }

    // Stephen's body also shoulders aside any sausage standing where it steps, pushing it the same way.
    s8 bodySausage = GetSausage(bodyX, bodyY, z);
    if (bodySausage != -1 && bodySausage != sausageNo && !(plan.mask & (1ull << bodySausage)))
      if (!PlanSausagePush(bodySausage, dir, plan)) return false;
  }

  // --- Commit --- Every lunge onto a grill (a backward unspear included) commits here as an ordinary step; the common
  // HandleBurnedStep then recoils Stephen the opposite way. That re-drags the speared sausage back while leaving
  // anything the lunge SHOVED where it landed, and a backward unspear's recoil shoves the freed sausage one cell ahead
  // -- so the grill needs no special case. Off a grill, body and fork shift and the speared sausage (plus
  // anything it pushed) rides along; a backward unspear may instead push a sausage out of the body's way.

  // A sausage on Stephen's HEAD rides along too -- it rests on his head, not the speared sausage, so the drag logic
  // never touched it. Carry it as an ordinary step would: the head hat (and anything squarely stacked on it) rides
  // rigidly while a cantilevered rider rolls across its axis.
  s8 headHat = GetSausage(_stephen.x, _stephen.y, z + 1);
  if (headHat != -1 && !(plan.mask & (1ull << headHat))) {
    u64 rigidStack = FullySupportedStack(headHat);
    // rigidLoad was seeded optimistically from CarriedMask(speared base), which folds in EVERY sausage stacked above it
    // -- including a cantilevered rider that the drag actually left behind (it slides off and drops). By now the drag
    // carries are all planned, so plan.mask is the true moving set; prune rigidLoad to the sausages that really moved
    // plus the head hat's own about-to-ride stack. A left-behind rider is then a genuine obstacle in the hat's path, so
    // the hat rams it (and is refused if it can't shove) instead of stepping over a cell that stays occupied during the
    // simultaneous motion.
    plan.rigidLoad &= (plan.mask | CarriedMask(headHat));
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

  // A forkless press toward a ladder climbs it: the reference tests the ladder (TryClimbUp/Down) BEFORE the
  // sausage-underfoot roll inside TryMovePlayer, so a ladder in the press direction preempts the log roll. Defer to
  // HandleLadderMotion (leave |handled| false) rather than rolling the sausage he stands on.
  if (!_stephen.HasFork()) {
    auto [adx, ady] = Delta(dir);
    s8 ax = _stephen.x + adx, ay = _stephen.y + ady;
    bool climbUp   = (dir == _stephen.dir)          && IsLadder(_stephen.x, _stephen.y, _stephen.z, dir);
    bool climbDown = (dir == Inverse(_stephen.dir)) && !CanWalkOnto(ax, ay, _stephen.z)
                  && IsLadder(ax, ay, _stephen.z - 1, Inverse(dir));
    if (climbUp || climbDown) return true;
  }

  // It fires when he presses ACROSS the sausage's long axis. Normally he must also be FACING along that press axis --
  // a press perpendicular to his facing is otherwise a free turn in place, which leaves the log unrolled (LogRoll test:
  // facing Right on a horizontal log, a press Up just turns him to face Up). But a fork speared into a sausage locks
  // his rotation, so a perpendicular press can no longer turn -- it rolls the log instead, whatever way he faces. The
  // sausage spins backward underfoot, so it -- and Stephen with it -- rolls one cell the OPPOSITE way to the press.
  // Whether he holds the fork or threw it, a press ALONG his facing axis reaches the reference's walk path
  // (TryMovePlayer), which rolls the log when that press runs across the sausage's axis -- so a forkless press along his
  // facing rolls too (the log spins backward underfoot, carrying it and Stephen one cell the OPPOSITE way to the press).
  // A press PERPENDICULAR to his facing is normally a free turn that leaves the log unrolled; only a fork speared into a
  // sausage locks his rotation so that a perpendicular press rolls the log instead, whatever way he faces.
  Sausage sausage = _sausages[onSausage];
  bool hasFork = _stephen.HasFork();
  s8 speared = hasFork ? GetSausage(_stephen.forkX, _stephen.forkY, _stephen.forkZ) : -1;
  bool facingAlongPress = (dir == Up || dir == Down) ? (_stephen.dir == Up || _stephen.dir == Down)
                                                     : (_stephen.dir == Left || _stephen.dir == Right);
  bool across = ((sausage.IsHorizontal() && (dir == Up || dir == Down))
              || (sausage.IsVertical()   && (dir == Left || dir == Right)))
             && (facingAlongPress || speared != -1);
  if (!across) return true; // a press along the axis is just an ordinary step -- let the pipeline handle it
  Direction roll = Inverse(dir);

  // Roll the sausage (and anything it pushes) one cell. If it can't roll there -- a wall, or out of the world -- this
  // isn't a legal log roll; fall through so the rest of the pipeline can refuse or reinterpret the press.
  MovePlan plan = NewPlan();

  // The log, everything riding it, Stephen's head/fork hats and any speared sausage all ride the roll as ONE rigid load
  // (same translation), so they must never treat each other as obstacles (move-stages.md: motion is one event).
  plan.rigidLoad = CarriedMask(onSausage) | CarriedMask(speared)
                 | CarriedMask(GetSausage(_stephen.x, _stephen.y, _stephen.z + 1))
                 | CarriedMask(_stephen.HasFork() ? GetSausage(_stephen.forkX, _stephen.forkY, _stephen.forkZ + 1) : (s8)-1);

  if (!PlanSausagePush(onSausage, roll, plan)) return true;

  // Stephen rides the roll: his body and fork translate one cell with the sausage. Neither may ride into a wall,
  // so a blocked ride refuses the whole move even once the sausage can roll.
  auto [dx, dy] = Delta(roll);
  s8 newBodyX = _stephen.x + dx, newBodyY = _stephen.y + dy;
  // A HELD fork rides the roll with the body; a DETACHED fork lies in the world and stays put (leave its pose alone).
  s8 newForkX = hasFork ? _stephen.forkX + dx : _stephen.forkX;
  s8 newForkY = hasFork ? _stephen.forkY + dy : _stephen.forkY;
  if (IsWall(newBodyX, newBodyY, _stephen.z) || (hasFork && IsWall(newForkX, newForkY, _stephen.forkZ))) return false;

  // That same ride shoulders any sausage standing where his fork or body lands, pushing it the roll direction. Use his
  // post-ride pose so a rider is judged correctly.
  Stephen mover = _stephen;
  mover.x = newBodyX; mover.y = newBodyY;
  mover.forkX = newForkX; mover.forkY = newForkY;
  if (hasFork) {
    s8 forkDest = GetSausage(newForkX, newForkY, _stephen.forkZ);
    if (forkDest != -1 && forkDest != onSausage && forkDest != speared && !(plan.mask & (1ull << forkDest))) {
      // The fork shoves the sausage in its path -- unless a wall blocks the shove, in which case it SPEARS it instead,
      // riding into its cell and lodging there. PlanSausagePush refuses (leaving the plan untouched)
      // exactly when the shove hits a wall, so a refused push simply falls through to the implicit spear.
      PlanSausagePush(forkDest, roll, plan, &mover);
    }
  }
  s8 bodyDest = GetSausage(newBodyX, newBodyY, _stephen.z);
  if (bodyDest != -1 && bodyDest != onSausage && bodyDest != speared && !(plan.mask & (1ull << bodyDest)))
    if (!PlanSausagePush(bodyDest, roll, plan, &mover)) return false;

  // --- Commit --- the sausage rolls (with anything it pushes), gravity settles it, then Stephen rides on top and drops.
  // A sausage riding on Stephen's HEAD (or balanced on his fork-tip) rides the roll with him, captured before motion.
  s8 headHat = GetSausage(_stephen.x, _stephen.y, _stephen.z + 1);
  s8 forkHat = _stephen.HasFork() ? GetSausage(_stephen.forkX, _stephen.forkY, _stephen.forkZ + 1) : (s8)-1;
  MarkDoubleMoves(plan, _sausages.begin()); // resolve any carried double-mover (the roll's push chain) before gravity
  u64 movedMask = plan.mask;
  if (!Settle(plan, movedMask, _sausages.begin(), _stephen, plan.doubleMoveMask)) return false;

  // A sausage speared on the fork rides rigidly (never rolls). A wall in its path pulls the fork free only when the roll
  // runs opposite his facing (an unspear); any other blocked drag refuses the move.
  if (speared != -1) {
    // The roll's push chain may have already grabbed the speared sausage as a RIDER and rolled it; it really rides the
    // fork rigidly, so rebuild it from the pre-move layout and apply the single rigid translate below.
    Sausage& spearedSausage = plan.sausages[speared];
    spearedSausage = _sausages[speared];
    if (IsWall(spearedSausage.x1 + dx, spearedSausage.y1 + dy, spearedSausage.z) || IsWall(spearedSausage.x2 + dx, spearedSausage.y2 + dy, spearedSausage.z)) {
      if (roll != Inverse(_stephen.dir)) return false;
      movedMask &= ~(1ull << speared); // unspeared: it never moved, so keep it out of the cook set
      speared = -1; // unspear: the fork pulls free and the sausage stays put
    } else {
      spearedSausage.x1 += dx; spearedSausage.y1 += dy; spearedSausage.x2 += dx; spearedSausage.y2 += dy;
      movedMask |= (1ull << speared);
    }
  }
  plan.stephen.x += dx;
  plan.stephen.y += dy;
  if (hasFork) { plan.stephen.forkX += dx; plan.stephen.forkY += dy; } // a detached fork stays where it was thrown
  while (!SausageSupported(plan, plan.stephen.x, plan.stephen.y, plan.stephen.z)) {
    if (plan.stephen.z <= 0) return false;
    // A speared sausage rides the fork down with the body -- but only while it, too, is still falling. The instant its
    // own far end catches a wall or sausage, Stephen falls PAST it: the reference detaches the fork mid-fall
    // (TryDetatchFork), leaving it lodged in the now-independent sausage at that height while the body drops on alone.
    if (speared != -1) {
      const Sausage& sp = plan.sausages[speared];
      if (SausageSupported(plan, sp.x1, sp.y1, sp.z) || SausageSupported(plan, sp.x2, sp.y2, sp.z)) {
        plan.stephen.forkDir = _stephen.dir; // detach: the fork stays lodged in the caught sausage
        speared = -1;
      }
    }
    plan.stephen.z--;
    if (speared != -1) { plan.stephen.forkZ--; plan.sausages[speared].z--; } // speared: fork + its sausage ride down with the body
  }
  // During a DESCENDING roll a held (un-speared) fork doesn't ride Stephen's hand down -- it settles on its OWN support.
  // The body-drop loop above left forkZ up at the rolled height (it only lowers the fork when speared), so let the fork
  // now fall to whatever lies beneath it. If that support holds it ABOVE the body's final level it can't stay in his
  // hand, so it detaches there (facing his direction), matching the reference leaving the fork behind on a descending
  // log-roll; otherwise it falls back level with the body and stays held. This is the held fork itself, NOT a sausage
  // resting on the fork tip (that's |forkHat|). A non-descending roll leaves forkZ == z, so nothing detaches.
  if (speared == -1 && plan.stephen.HasFork()) {
    while (plan.stephen.forkZ > plan.stephen.z
           && !IsWall(plan.stephen.forkX, plan.stephen.forkY, plan.stephen.forkZ - 1)
           && GetPlannedSausage(plan, plan.stephen.forkX, plan.stephen.forkY, plan.stephen.forkZ - 1) == -1) {
      plan.stephen.forkZ--;
    }
    if (plan.stephen.forkZ != plan.stephen.z) plan.stephen.forkDir = _stephen.dir; // left behind, held aloft above the body
  }
  // The head hat rides rigidly on Stephen's head through the whole roll (and any drop) -- it never rolls, so translate
  // it by his TOTAL displacement now that his final pose is known. A wall in its path, or a far end anchored on a
  // non-moving sausage/wall, leaves it behind. Settle seeded it into |movedMask| as "rode on
  // Stephen" but never relocated it, so it's still at its pre-move footprint here and we own its actual translation.
  u64 headCarried = 0;
  if (headHat != -1 && headHat != speared) {
    const Sausage& hat = _sausages[headHat]; // the pre-move footprint: the log's push chain may have already shifted
                                             // plan.sausages[headHat] (its far end rode the mid), which we now redo rigidly
    s8 heightDelta = plan.stephen.z - _stephen.z;
    bool wallBlocked = IsWall(hat.x1 + dx, hat.y1 + dy, hat.z + heightDelta) || IsWall(hat.x2 + dx, hat.y2 + dy, hat.z + heightDelta);
    auto [farX, farY] = hat.OtherEnd(_stephen.x, _stephen.y);
    // Anchored only by a NON-MOVING support: a wall, or a sausage that isn't itself moving this turn. A far end resting
    // on a sausage that rolls/slides away (the mid, carried by the log) does NOT anchor -- the hat rides Stephen rigidly.
    // The speared sausage is dragged rigidly by the fork (tracked in movedMask, not plan.mask), so a far end resting on
    // it likewise rides along, not anchors.
    s8 farBelow = GetSausage(farX, farY, hat.z - 1);
    bool anchored = IsWall(farX, farY, hat.z - 1)
        || (farBelow != -1 && farBelow != speared && !(plan.mask & (1ull << farBelow)));
    if (!wallBlocked && !anchored) {
      // The head hat AND everything stacked on it rides by Stephen's total displacement. The hat and any SQUARELY
      // stacked rider ride rigidly; a CANTILEVERED rider rolls across its own axis (matching the reference's hatStack).
      u64 stack = CarriedMask(headHat);
      u64 rigid = FullySupportedStack(headHat);
      headCarried = stack;
      for (int i = 0; i < _sausages.Size(); i++) {
        if (!(stack & (1ull << i))) continue;
        Sausage s = _sausages[i]; // rebuild from the pre-move footprint, discarding any push-chain carry that grabbed it
        s.x1 += dx; s.y1 += dy; s.x2 += dx; s.y2 += dy; s.z += heightDelta;
        if (!(rigid & (1ull << i)) && (s.IsHorizontal() ? (dy != 0) : (dx != 0))) s.flags ^= Sausage::Rolled;
        plan.sausages[i] = s;
        movedMask |= (1ull << i);
        plan.mask |= (1ull << i); // also record in plan.mask so the second MarkDoubleMoves considers this carried sausage
      }
    }
  }
  // A sausage balanced on the fork-tip rides the roll with the fork -- along with everything stacked on it. Unlike a
  // head hat it is fork-borne (never rigid), so EVERY member rolls when carried across its own axis (mirrors the step
  // path's PlanHatCarry with rigidMask=0). Its far end, if anchored on a non-moving sausage or wall, leaves it behind.
  // Skip a head<->fork bridge the head-hat carry already handled.
  if (forkHat != -1 && forkHat != speared && forkHat != headHat && forkHat != onSausage && !(headCarried & (1ull << forkHat))) {
    const Sausage& hat = plan.sausages[forkHat];
    s8 heightDelta = plan.stephen.z - _stephen.z;
    bool wallBlocked = IsWall(hat.x1 + dx, hat.y1 + dy, hat.z + heightDelta) || IsWall(hat.x2 + dx, hat.y2 + dy, hat.z + heightDelta);
    auto [farX, farY] = hat.OtherEnd(_stephen.forkX, _stephen.forkY);
    bool anchored = IsWall(farX, farY, hat.z - 1) || GetSausage(farX, farY, hat.z - 1) != -1;
    if (!wallBlocked && !anchored) {
      u64 stack = CarriedMask(forkHat) & ~headCarried; // don't re-carry anything the head-hat stack already moved
      for (int i = 0; i < _sausages.Size(); i++) {
        if (!(stack & (1ull << i))) continue;
        Sausage& s = plan.sausages[i];
        s.x1 += dx; s.y1 += dy; s.x2 += dx; s.y2 += dy; s.z += heightDelta;
        if (s.IsHorizontal() ? (dy != 0) : (dx != 0)) s.flags ^= Sausage::Rolled; // fork-borne -> rolls across its axis
        movedMask |= (1ull << i);
        plan.mask |= (1ull << i); // also record in plan.mask so the second MarkDoubleMoves considers this carried sausage
      }
    }
  }
  // The hat carries above ran AFTER the first MarkDoubleMoves (they need Stephen's post-drop pose), so a sausage left
  // balanced across a hat rider that just rolled hasn't been checked for its extra tumble. Re-run the detector now that
  // the hats are placed (they were recorded into plan.mask above); it skips already-flagged sausages, adding only these
  // hat-induced double-movers (e.g. an aligned rider on a rolled fork-hat).
  MarkDoubleMoves(plan, _sausages.begin());
  // The speared sausage rides the fork rigidly -- exactly one cell with Stephen -- so it can never double-move. The
  // roll's push chain may have grabbed it as a rider and a MarkDoubleMoves flagged it before the speared block rebuilt
  // it as a rigid single translate; clear that stale flag so DoubleMove doesn't tumble it an extra cell.
  if (speared != -1) plan.doubleMoveMask &= ~(1ull << speared);
  if (!CookMoved(plan, movedMask, plan.doubleMoveMask)) return false;
  if (!DoubleMove(plan)) return false;
  if (PlanHasOverlap(plan)) return false; // a carry produced an impossible two-in-one-cell state -> refuse
  Commit(plan);
  handled = true;
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

  // Forkless (the fork is thrown and lying in the world): only the body climbs the ladder; the detached fork is left
  // exactly where it is. A sausage riding the head during a forkless climb is rare and unmodelled -- refuse it rather
  // than mis-place it (a refusal is safe: it can never produce a false divergence, it just isn't explored).
  if (!_stephen.HasFork()) {
    if (GetSausage(_stephen.x, _stephen.y, _stephen.z + 1) != -1) return false;
    auto [fdx, fdy] = Delta(dir);
    s8 ax = _stephen.x + fdx, ay = _stephen.y + fdy;
    if (dir == _stephen.dir && IsLadder(_stephen.x, _stephen.y, _stephen.z, dir)) {
      // Rise rung by rung while the ladder continues and the body isn't blocked above, then step off its top in |dir|.
      s8 nz = _stephen.z;
      while (IsLadder(_stephen.x, _stephen.y, nz, dir)) {
        if (IsWall(_stephen.x, _stephen.y, nz + 1) || GetSausage(_stephen.x, _stephen.y, nz + 1) != -1) return false;
        nz++;
      }
      if (IsWall(ax, ay, nz) || !CanWalkOnto(ax, ay, nz)) return false;
      // A sausage occupying the step-off cell itself (not merely under it) would be shoved along by the body arriving --
      // the game pushes it; we don't model that push during a forkless climb, so refuse rather than overlap it
      // (a refusal never causes a false divergence). Covers the fork-lodged-in-a-wall-borne-sausage case (5-6 Crater m195).
      if (GetSausage(ax, ay, nz) != -1) return false;
      MovePlan plan = NewPlan();
      plan.stephen.x = ax; plan.stephen.y = ay; plan.stephen.z = nz;
      // If the thrown fork lies on the cell we step off onto, the body shoves it one cell further along |dir| (a wall
      // directly behind it blocks the whole climb), then it falls to its support -- the common reconnect at the end of
      // Move() takes it back into hand when it lands one cell ahead at Stephen's level.
      if (_stephen.forkX == ax && _stephen.forkY == ay && _stephen.forkZ == nz) {
        s8 pfx = ax + fdx, pfy = ay + fdy;
        if (IsWall(pfx, pfy, nz)) return false;
        plan.stephen.forkX = pfx; plan.stephen.forkY = pfy;
        while (plan.stephen.forkZ > 0 && !IsWall(pfx, pfy, plan.stephen.forkZ - 1)
               && GetSausage(pfx, pfy, plan.stephen.forkZ - 1) == -1)
          plan.stephen.forkZ--;
      }
      handled = true;
      return ReactAndCommit(plan);
    }
    if (dir == Inverse(_stephen.dir) && !CanWalkOnto(ax, ay, _stephen.z)
        && IsLadder(ax, ay, _stephen.z - 1, Inverse(dir))) {
      // Back onto a descending ladder and crouch down to the first surface below.
      MovePlan plan = NewPlan();
      plan.stephen.x = ax; plan.stephen.y = ay; plan.stephen.z = _stephen.z;
      while (!CanWalkOnto(plan.stephen.x, plan.stephen.y, plan.stephen.z)) {
        if (plan.stephen.z <= 0) return false;
        if (!IsLadder(plan.stephen.x, plan.stephen.y, plan.stephen.z - 1, Inverse(dir))) return false;
        plan.stephen.z--;
      }
      handled = true;
      return ReactAndCommit(plan);
    }
    return true; // no ladder to use this way -- let the forkless walk take an ordinary step
  }

  s8 speared = GetSausage(_stephen.forkX, _stephen.forkY, _stephen.forkZ);
  // A sausage riding on Stephen's HEAD rides with him up or down the ladder.
  s8 headHat = GetSausage(_stephen.x, _stephen.y, _stephen.z + 1);

  // Climb up: while a ladder facing the press direction is in our cell, rise a rung; then step off its top in |dir|.
  if (IsLadder(_stephen.x, _stephen.y, _stephen.z, dir)) {
    // The fork carries whatever sits on it up: a speared sausage, or -- if the fork is empty -- a sausage resting on
    // top of the fork, which the rising fork lifts along.
    s8 carried = speared != -1 ? speared : GetSausage(_stephen.forkX, _stephen.forkY, _stephen.forkZ + 1);
    u64 carriedMask = CarriedMask(carried) | CarriedMask(headHat);
    // A sausage resting on the wall-top the climb ends on sits in the rising body's path: LiftStephen shoves it up rung
    // by rung (recording it in |hatMask|) so the step-off hat-carries it rather than dragging it rigidly.
    u64 hatMask = 0;
    // Only the speared sausage and the rigid head-hat stack ride without rolling; everything else carried rolls when
    // taken across its axis. The rigid stack is Stephen's head hat, PLUS the speared base when the fork is lodged in one
    // (it rides the fork rigidly), plus anything SQUARELY stacked on either (both ends resting on it); a CANTILEVERED
    // rider rolls. A hat that bridges his head and the speared base is squarely supported by two
    // rigid co-movers, so it translates rigidly rather than rolling. Computed from the PRE-climb
    // layout, before LiftStephen shoves wall-top hats in.
    u64 rigidStack = 0;
    if (headHat != -1) rigidStack |= (1ull << headHat);
    if (speared != -1) rigidStack |= (1ull << speared);
    // A cantilevered rider (one end over open space) rolls UNLESS a SPEARED base is under it: the
    // fork locks it flat over the void so it keeps its face. Pass Cantilever::Air only then.
    rigidStack = GrowRigidStack(rigidStack, speared != -1 ? Cantilever::Air : Cantilever::None);
    // A BARE fork-borne sausage (balanced on the fork tip, no speared base under it) rolls as a bridge across its axis
    // when the fork carries it up. But a sausage resting on BOTH Stephen's head and his fork tip
    // (carried == headHat) bridges his two rigid co-moving supports, so it rides UP rigidly rather than rolling -- the
    // Oracle gives it zero torsion because its whole lower footprint is Stephen himself, moving at the sausage's speed.
    // This mirrors the descent case exactly.
    if (speared == -1 && carried != headHat) rigidStack &= ~CarriedMask(carried);
    u64 rollMask = (carriedMask & ~rigidStack);
    if (speared != -1) rollMask &= ~(1ull << speared);
    while (IsLadder(_stephen.x, _stephen.y, _stephen.z, dir))
      if (!LiftStephen(+1, carriedMask, hatMask)) return false;
    if (!StepOffLadder(dir, carriedMask, false, hatMask, rollMask, speared)) return false;
    handled = true;
    return true;
  }

  // Descend: the cell ahead has no footing at our level, but a back-facing ladder one level down leads to a surface.
  auto [dx, dy] = Delta(dir);
  s8 ax = _stephen.x + dx, ay = _stephen.y + dy;
  if (CanWalkOnto(ax, ay, _stephen.z)) return true;                  // there's a ledge ahead -- ordinary walking handles it
  if (!IsLadder(ax, ay, _stephen.z - 1, Inverse(dir))) return true;  // nothing to climb down -- fall through
  // A sausage sitting on Stephen -- speared, on his fork, or hatted on his head -- rides DOWN the ladder only if it's
  // cantilevered (its far end hangs over open space). If that far end is anchored on a wall or another sausage, the seat
  // slides out from under it and it stays.
  auto cantilevered = [&](s8 no, s8 seatX, s8 seatY) -> bool {
    const Sausage& s = _sausages[no];
    auto [farX, farY] = s.OtherEnd(seatX, seatY);
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
  // rather than floating a level up.
  u64 carriedMask = CarriedMask(carried) | CarriedMask(descentHat);
  // A carried rider rides down only while its support (the fork, or Stephen's head) stays under it. The moment it comes
  // to rest on a wall-top -- or a sausage that isn't descending -- that support slides out and it stays put. A head hat
  // is always depositable, as is a FORK-BORNE base; only a SPEARED base stays lodged
  // on the descending fork.
  u64 hatMask = 0; // unused during a crouch (a descent never shoves a sausage upward); kept for LiftStephen's signature
  // Rigid units (translate without rolling): Stephen's head hat, PLUS the speared base when the fork is lodged in one,
  // plus anything squarely stacked on either. A head hat resting on the rigid speared base bridges his head and the
  // base, so it rides DOWN rigidly rather than rolling -- the exact reverse of the climb. Seed
  // the full head hat (not just the cantilevered descentHat): a hat whose far end rests on the speared base isn't
  // cantilevered yet still translates rigidly. Seeding a non-descending hat is harmless -- it never enters rollMask
  // unless it's also in carriedMask.
  u64 rigidStack = 0;
  if (headHat != -1) rigidStack |= (1ull << headHat);
  if (speared != -1) rigidStack |= (1ull << speared);
  // As on the climb, a fork-locked base carries a rider rigidly even when its other end cantilevers
  // over open space; without a speared base a cantilevered rider rolls. Pass Cantilever::Air only when the fork is lodged.
  rigidStack = GrowRigidStack(rigidStack, speared != -1 ? Cantilever::Air : Cantilever::None);
  // A BARE fork-borne base (fork tip, no speared base) rolls as a bridge; drop the fork stack only
  // in that case. A SPEARED base is rigid, so it and its square riders stay in the rigid set. A head hat resting on BOTH
  // Stephen's head and the fork tip (carried == headHat) bridges his two rigid points, so it rides DOWN rigidly rather
  // than rolling -- don't strip it.
  if (speared == -1 && carried != headHat) rigidStack &= ~CarriedMask(carried);
  u64 rollMask = (carriedMask & ~rigidStack);
  if (speared != -1) rollMask &= ~(1ull << speared);
  if (!StepOffLadder(dir, carriedMask, true, 0, rollMask, speared)) return false; // step out over the ladder (hanging, no footing yet)
  // Crouch down the ladder one rung at a time. Each rung lowers Stephen and his DIRECT cargo (a speared sausage) via
  // LiftStephen; the central gravity Settle then drops whatever he was merely CARRYING (a fork-borne base, a head hat),
  // depositing it the instant it comes to rest on a wall-top or a non-descending sausage -- no bespoke deposit
  // bookkeeping.
  while (true) {
    if (_stephen.z <= 0) return false;
    Stephen prevStephen = _stephen;
    Sausage preMove[NUM_SAUSAGES];
    for (int i = 0; i < _sausages.Size(); i++) preMove[i] = _sausages[i];
    s8 spear = _stephen.HasFork() ? GetSausage(_stephen.forkX, _stephen.forkY, _stephen.forkZ) : (s8)-1;
    u64 directCargo = (spear != -1) ? (1ull << spear) : 0;
    if (!LiftStephen(-1, directCargo, hatMask)) return false;          // lower Stephen + speared; a blocked crouch refuses
    MovePlan plan = NewPlan();
    u64 movedMask = directCargo;
    if (!Settle(plan, movedMask, preMove, prevStephen)) return false;  // drop / deposit the riders he left behind
    if (!CookMoved(plan, movedMask, 0)) return false;                  // brown anything that settled onto a grill
    Commit(plan);
    if (CanWalkOnto(_stephen.x, _stephen.y, _stephen.z)) break;        // reached a surface -- step off
    if (!IsLadder(_stephen.x, _stephen.y, _stephen.z - 1, Inverse(dir))) return false; // ladder ran out mid-air
  }
  handled = true;
  return true;
}

bool Level::LiftStephen(s8 dz, u64& carried, u64& hatMask) {
  // The fork can't ram into a wall.
  s8 newForkZ = _stephen.forkZ + dz;
  if (IsWall(_stephen.forkX, _stephen.forkY, newForkZ)) return false;

  if (dz > 0) {
    // Climbing up: a sausage the rising fork or body enters is shoved up along with the climb -- it rides up as a hat.
    // If it (or its tower) can't rise -- a wall above -- the climb is refused.
    s8 forkBlock = GetSausage(_stephen.forkX, _stephen.forkY, newForkZ);
    if (forkBlock != -1 && !(carried & (1ull << forkBlock)))
      if (!LiftSausageStack(forkBlock, dz, carried, hatMask)) return false;
    s8 bodyBlock = GetSausage(_stephen.x, _stephen.y, _stephen.z + dz);
    if (bodyBlock != -1 && !(carried & (1ull << bodyBlock)))
      if (!LiftSausageStack(bodyBlock, dz, carried, hatMask)) return false;
  } else {
    // Descending: a sausage resting under the fork won't yield (solid ground below it), so the climb-down is blocked.
    s8 blocking = GetSausage(_stephen.forkX, _stephen.forkY, newForkZ);
    if (blocking != -1 && !(carried & (1ull << blocking))) return false;
  }

  // A carried sausage (speared, or a stack riding the fork/head) that would slide into a wall at its new level blocks
  // the whole rung. Checking every carried end keeps the fork and its cargo rigid.
  for (int i = 0; i < _sausages.Size(); i++) {
    if (!(carried & (1ull << i))) continue;
    const Sausage& s = _sausages[i];
    if (IsWall(s.x1, s.y1, s.z + dz) || IsWall(s.x2, s.y2, s.z + dz)) return false;
    // Descending: a carried sausage lowered onto a NON-carried sausage collides -- refuse the rung rather than overlap.
    // Climb-up handles blockers above via LiftSausageStack, so guard to dz < 0.
    if (dz < 0) {
      s8 belowA = GetSausage(s.x1, s.y1, s.z + dz);
      s8 belowB = GetSausage(s.x2, s.y2, s.z + dz);
      if ((belowA != -1 && !(carried & (1ull << belowA))) || (belowB != -1 && !(carried & (1ull << belowB)))) return false;
    }
  }

  _stephen.z += dz;
  _stephen.forkZ += dz;
  for (int i = 0; i < _sausages.Size(); i++)
    if (carried & (1ull << i)) _sausages[i].z += dz;
  return true;
}

bool Level::LiftSausageStack(s8 sausageNo, s8 dz, u64& carried, u64& hatMask) {
  if (carried & (1ull << sausageNo)) return true;
  const Sausage& s = _sausages[sausageNo];
  // The cells it rises into must be clear of walls, or the whole climb is refused.
  if (IsWall(s.x1, s.y1, s.z + dz) || IsWall(s.x2, s.y2, s.z + dz)) return false;
  carried |= (1ull << sausageNo);
  hatMask |= (1ull << sausageNo);
  // Anything stacked directly on top rides up with it (a rigid tower).
  s8 aboveA = GetSausage(s.x1, s.y1, s.z + dz);
  if (aboveA != -1 && aboveA != sausageNo && !(carried & (1ull << aboveA)))
    if (!LiftSausageStack(aboveA, dz, carried, hatMask)) return false;
  s8 aboveB = GetSausage(s.x2, s.y2, s.z + dz);
  if (aboveB != -1 && aboveB != sausageNo && aboveB != aboveA && !(carried & (1ull << aboveB)))
    if (!LiftSausageStack(aboveB, dz, carried, hatMask)) return false;
  return true;
}

u64 Level::CarriedMask(s8 base) const {
  if (base == -1) return 0;
  u64 mask = (1ull << base);
  bool grew = true;
  while (grew) {
    grew = false;
    for (int i = 0; i < _sausages.Size(); i++) {
      if (mask & (1ull << i)) continue;
      const Sausage& s = _sausages[i];
      s8 belowA = GetSausage(s.x1, s.y1, s.z - 1), belowB = GetSausage(s.x2, s.y2, s.z - 1);
      if ((belowA != -1 && (mask & (1ull << belowA))) || (belowB != -1 && (mask & (1ull << belowB)))) { mask |= (1ull << i); grew = true; }
    }
  }
  return mask;
}

u64 Level::GrowRigidStack(u64 seed, Cantilever cant) const {
  // Grow the rigid (non-rolling) set from |seed| to a fixed point: a sausage joins when EACH of its two ends is either
  // resting on a stack member (rigid) or an allowed cantilever, and at least one end is rigid (so it's actually attached
  // to the moving tower). |cant| controls whether an end over open space (no stack sausage beneath) still counts:
  //   None -- never; the end must rest squarely on a stack member (a pure both-ends stack, e.g. a head hat). A
  //           cantilevered rider therefore rolls when carried across its axis (an unspeared climb).
  //   Air  -- an end cantilevered over GENUINE open space (below == -1 && !IsWall) rides rigidly too, taking zero
  //           torsion: a pure horizontal translation (a step), or a fork-locked speared base that holds a rider flat
  //           over the void. A wall-top end never qualifies -- it's anchored, and PlanHatCarry/StepOffLadder leave such
  //           a hat behind regardless.
  u64 stack = seed;
  for (bool grew = true; grew; ) {
    grew = false;
    for (int i = 0; i < _sausages.Size(); i++) {
      if (stack & (1ull << i)) continue;
      const Sausage& s = _sausages[i];
      s8 below1 = GetSausage(s.x1, s.y1, s.z - 1), below2 = GetSausage(s.x2, s.y2, s.z - 1);
      bool rigid1 = below1 != -1 && (stack & (1ull << below1));
      bool rigid2 = below2 != -1 && (stack & (1ull << below2));
      bool void1 = cant == Cantilever::Air && below1 == -1 && !IsWall(s.x1, s.y1, s.z - 1);
      bool void2 = cant == Cantilever::Air && below2 == -1 && !IsWall(s.x2, s.y2, s.z - 1);
      if ((rigid1 || void1) && (rigid2 || void2) && (rigid1 || rigid2)) { stack |= (1ull << i); grew = true; }
    }
  }
  return stack;
}

u64 Level::FullySupportedStack(s8 base) const {
  return base == -1 ? 0 : GrowRigidStack((1ull << base), Cantilever::None);
}

bool Level::StepOffLadder(Direction dir, u64 carried, bool ladderMotion, u64 hatMask, u64 rollMask, s8 speared) {
  auto [dx, dy] = Delta(dir);
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
  if (forkSausage != -1 && !(carried & (1ull << forkSausage)))
    if (!PlanSausagePush(forkSausage, dir, plan, &mover)) return false;
  s8 bodySausage = GetSausage(bodyX, bodyY, bodyZ);
  if (bodySausage != -1 && !(carried & (1ull << bodySausage)) && !(plan.mask & (1ull << bodySausage)))
    if (!PlanSausagePush(bodySausage, dir, plan, &mover)) return false;

  // --- Commit --- pushed sausages land in |plan|, Stephen (and the stack he carries) translates rigidly, then settle + cook.
  plan.stephen.x = bodyX;
  plan.stephen.y = bodyY;
  plan.stephen.forkX = forkX;
  plan.stephen.forkY = forkY;
  for (int i = 0; i < _sausages.Size(); i++) {
    if (!(carried & (1ull << i)) || (hatMask & (1ull << i))) continue; // hat sausages ride as a head hat (below), not rigidly
    Sausage& carriedSausage = plan.sausages[i];
    // A carried sausage whose slide would drive an end into a wall is carry-blocked: it stays put
    // while the base slides out from under it. The SPEARED sausage is rigid on the fork, so a wall in its path refuses
    // the whole step -- unless this is a backward unspear, when the fork pulls free. A merely-carried
    // RIDER is just left behind against the wall.
    if (IsWall(carriedSausage.x1 + dx, carriedSausage.y1 + dy, carriedSausage.z) || IsWall(carriedSausage.x2 + dx, carriedSausage.y2 + dy, carriedSausage.z)) {
      if (i == speared && dir != Inverse(_stephen.dir)) return false; // speared base can't ride into a wall
      continue;
    }
    // A carried sausage sliding into a cell another (non-carried) sausage holds must shove it the same way; if it can't
    // be pushed, refuse the whole step rather than overlap.
    s8 hitA = GetSausage(carriedSausage.x1 + dx, carriedSausage.y1 + dy, carriedSausage.z);
    if (hitA != -1 && !(carried & (1ull << hitA)) && !(plan.mask & (1ull << hitA)))
      if (!PlanSausagePush(hitA, dir, plan)) return false;
    s8 hitB = GetSausage(carriedSausage.x2 + dx, carriedSausage.y2 + dy, carriedSausage.z);
    if (hitB != -1 && hitB != hitA && !(carried & (1ull << hitB)) && !(plan.mask & (1ull << hitB)))
      if (!PlanSausagePush(hitB, dir, plan)) return false;
    carriedSausage.x1 += dx; carriedSausage.y1 += dy; carriedSausage.x2 += dx; carriedSausage.y2 += dy;
    plan.mask |= (1ull << i); // record the carried sausage as moved so the central double-move detector can see it roll
    // A fork-borne carry across the sausage's long axis rolls it; a speared sausage or head hat rides rigidly (excluded
    // from |rollMask|).
    if ((rollMask & (1ull << i)) && (carriedSausage.IsHorizontal() ? (dy != 0) : (dx != 0))) carriedSausage.flags ^= Sausage::Rolled;
  }
  // A sausage the climb shoved onto Stephen's head rides along as a hat -- a wall in its path leaves it behind. The
  // head-hat stack rides rigidly, but a cantilevered rider rolls, so honor |rollMask|.
  for (int i = 0; i < _sausages.Size(); i++)
    if (hatMask & (1ull << i)) PlanHatCarry((s8)i, dx, dy, dir, plan, ~rollMask); // rigid unless flagged rolling
  // A carried rider left balanced across a base that just rolled tumbles an extra cell. The step-off carry translates
  // the stack rigidly (no per-sausage hook), so run the central double-move detector to match ordinary steps.
  return ReactAndCommit(plan);
}

bool Level::HandleStepMotion(Direction dir, bool& handled) {
  // A press along Stephen's facing axis is an ordinary walk (forward or backward); a perpendicular press is a turn, so
  // leave |handled| false and let HandleRotation take it.
  if (dir != _stephen.dir && dir != Inverse(_stephen.dir)) return true;
  handled = true;

  // A detached fork stays in the world: forkless, only Stephen's body walks. It steps one cell along the press (keeping
  // his facing), pushing a sausage directly in its path, then settles; the thrown fork is untouched.
  if (!_stephen.HasFork()) {
    auto [fdx, fdy] = Delta(dir);
    s8 nbx = _stephen.x + fdx, nby = _stephen.y + fdy, nz = _stephen.z;
    if (IsWall(nbx, nby, nz) || !CanWalkOnto(nbx, nby, nz)) return false;
    MovePlan plan = NewPlan();
    // The thrown fork is a solid entity: when the body steps into its cell it shoves the fork one cell along (a wall
    // directly behind the fork blocks the whole step).
    if (_stephen.forkX == nbx && _stephen.forkY == nby && _stephen.forkZ == nz) {
      s8 pfx = nbx + fdx, pfy = nby + fdy;
      if (IsWall(pfx, pfy, nz)) return false;
      plan.stephen.forkX = pfx; plan.stephen.forkY = pfy;
      // The shoved fork falls until it rests on a wall-top or a sausage.
      while (plan.stephen.forkZ > 0 && !IsWall(pfx, pfy, plan.stephen.forkZ - 1)
             && GetSausage(pfx, pfy, plan.stephen.forkZ - 1) == -1)
        plan.stephen.forkZ--;
      // Shoved over a column with no footing at all, the fork falls off the map (Fork Lost) -- refuse rather than
      // resting it on the void floor.
      if (!CanWalkOnto(pfx, pfy, plan.stephen.forkZ)) return false;
    }
    s8 bumped = GetSausage(nbx, nby, nz);
    Stephen mover = _stephen; mover.x = nbx; mover.y = nby;
    if (bumped != -1 && !PlanSausagePush(bumped, dir, plan, &mover)) return false;
    // A thrown fork perched on top of a sausage this step just rolled/slid away would be flung along by that motion and
    // dropped -- a rider-fling we don't model. Refuse rather than leave the fork mis-placed (a refusal never yields a
    // false divergence; the path just isn't explored). The fork's support is the sausage directly beneath it.
    s8 forkBase = GetSausage(_stephen.forkX, _stephen.forkY, _stephen.forkZ - 1);
    if (forkBase != -1 && (plan.mask & (1ull << forkBase))) return false;
    plan.stephen.x = nbx; plan.stephen.y = nby;
    return ReactAndCommit(plan);
  }

  auto [dx, dy] = Delta(dir);
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
  plan.rigidLoad = CarriedMask(GetSausage(_stephen.x, _stephen.y, z + 1)) | CarriedMask(GetSausage(_stephen.forkX, _stephen.forkY, z + 1));

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

    // The recoil can't drag the speared sausage into terrain: if either end would land inside a wall, the base can't
    // recoil onto the grill at all, so the fork pulls free (unspear) and Stephen bounces in place with nothing moved.
    bool dragOk = !(IsWall(spearDragged.x1, spearDragged.y1, spearDragged.z)
                 || IsWall(spearDragged.x2, spearDragged.y2, spearDragged.z));
    if (dragOk && !CookSausage(spearDragged)) return false;

    // The recoil drags the freshly-speared sausage one cell back onto the fork's grill; a rider on it comes along by
    // the same displacement (rigid -- no roll, no double-move). Mark the base moved and carry from its ORIGINAL
    // footprint; Stephen stays put so the carry judges support against his current pose.
    plan.mask |= (1ull << sausageNo);
    // The recoiled base also rams any sausage in its landing footprint, shoving it opposite the press. If that neighbour
    // CAN'T be shoved, the base can't recoil onto the grill: the fork pulls free (unspear) and Stephen bounces in place.
    s8 hitA = GetSausage(spearDragged.x1, spearDragged.y1, spearDragged.z);
    if (dragOk && hitA != -1 && hitA != sausageNo && !(plan.mask & (1ull << hitA)))
      dragOk = PlanSausagePush(hitA, Inverse(dir), plan);
    s8 hitB = GetSausage(spearDragged.x2, spearDragged.y2, spearDragged.z);
    if (dragOk && hitB != -1 && hitB != sausageNo && hitB != hitA && !(plan.mask & (1ull << hitB)))
      dragOk = PlanSausagePush(hitB, Inverse(dir), plan);

    if (dragOk) {
      spearDrag = true;
      plan.preCookedMask |= (1ull << sausageNo); // the base browned on the fork's grill above; don't re-cook it
      const Sausage& base = _sausages[sausageNo];
      s8 riderA = GetSausage(base.x1, base.y1, base.z + 1);
      s8 riderB = GetSausage(base.x2, base.y2, base.z + 1);
      if (riderA != -1 && riderA != sausageNo && !(plan.mask & (1ull << riderA)))
        PlanSausageCarry(riderA, -dx, -dy, Inverse(dir), plan, nullptr);
      if (riderB != -1 && riderB != sausageNo && riderB != riderA && !(plan.mask & (1ull << riderB)))
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
    // so a fork hat here always rests on the bare tip, not on a rolling base -- it translates one cell without flipping.)
    // A step is a PURE TRANSLATION, so the entire tower riding Stephen takes zero torsion: the rigid stack is those hats
    // plus any sausage whose support rests ENTIRELY on rigid co-movers -- both ends on the stack, or one end on the stack
    // with the other merely CANTILEVERED over open space. A cantilevered rider therefore rides flat and does NOT roll.
    // Only an end propped on a SEPARATE base (a ground roller) keeps a rider out of the rigid set so it can roll.
    u64 rigidStack = 0;
    if (headHat != -1) rigidStack |= (1ull << headHat);
    if (forkHat != -1) rigidStack |= (1ull << forkHat);
    // A step is a pure translation, so the whole tower riding Stephen -- squarely-stacked riders AND ones cantilevered
    // over open space -- takes zero torsion and rides flat. Only an end propped on a SEPARATE
    // ground base (a roller) keeps a rider out of the rigid set. (A wall-top end is anchored, not a cantilever -- it
    // never joins, but PlanHatCarry leaves such a hat behind anyway, so it's moot.)
    rigidStack = GrowRigidStack(rigidStack, Cantilever::Air);
    PlanHatCarry(headHat, dx, dy, dir, plan, rigidStack);
    PlanHatCarry(forkHat, dx, dy, dir, plan, rigidStack); // fork-tip hat is supported by Stephen -> rides rigidly, no roll
  } else if (headHat != -1) {
    // A grill bounce is a step immediately recoiled. The step carries the head hat one cell along the press; the recoil
    // then LEAVES it there if its far end just came to rest on a wall-top or non-moving sausage,
    // else carries it straight back (a net no-op). If a wall blocks the hat entirely, a CLEAN hat is knocked off his
    // head: it drops and the recoil shoves it one cell back, rolling it.
    const Sausage& hat = _sausages[headHat];
    auto [farX, farY] = hat.OtherEnd(_stephen.x, _stephen.y);
    bool forwardBlocked = IsWall(hat.x1 + dx, hat.y1 + dy, hat.z) || IsWall(hat.x2 + dx, hat.y2 + dy, hat.z);
    if (!forwardBlocked) {
      s8 landing = GetSausage(farX + dx, farY + dy, hat.z - 1);
      bool landsAnchored = IsWall(farX + dx, farY + dy, hat.z - 1)
                        || (landing != -1 && !(plan.mask & (1ull << landing))); // a sausage that isn't itself moving away
      if (landsAnchored) {
        // The head hat (plus anything squarely stacked on it) rides rigidly; a cantilevered rider rolls across its axis.
        u64 rigidStack = FullySupportedStack(headHat);
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

  // A detached fork lies in the world and no longer swings from Stephen's hand. A perpendicular press just turns his
  // body in place to the new facing; the fork stays exactly where it was thrown.
  if (!_stephen.HasFork()) {
    MovePlan plan = NewPlan();
    plan.stephen.dir = dir;
    return ReactAndCommit(plan);
  }

  MovePlan plan = NewPlan();
  plan.rotating = true;  // The fork swings from its current cell to the perpendicular one; the corner it sweeps through is the diagonal
  // between them -- the fork's destination, offset back along the old facing.
  auto [dx, dy] = Delta(dir);
  s8 forkX = _stephen.x + dx;
  s8 forkY = _stephen.y + dy;

  auto [odx, ody] = Delta(_stephen.dir);
  s8 cornerX = forkX + odx;
  s8 cornerY = forkY + ody;

  s8 z = _stephen.z;

  // A rider carried by a pushed sausage checks whether Stephen's body/fork ends up beneath it (PlanSausageCarry's
  // rotation-hold guard). During a turn the fork ends at its swung-to cell, so seed the carry checks with that POST-turn
  // pose, else a sausage the fork swings directly under is wrongly carried off its new support.
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
  u64 cornerMask = plan.mask; // the corner push lands even when Stephen bonks instead of turning

  // At the fork's destination, a sausage is pushed away from where the fork came from (the inverse of the old facing).
  // A wall-blocked sausage (or a bare wall) bonks -- the turn is abandoned but the corner push stands; a sausage pushed
  // over the void rides out and is settled later.
  bool bonk = false;
  s8 forkSausage = GetSausage(forkX, forkY, z);
  if (forkSausage != -1 && !(plan.mask & (1ull << forkSausage))) {
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
  u64 hatMask = 0;
  if (!bonk) {
    PlanHatRotation(dir, plan, hatMask);
  } else {
    // Even a bonked turn still swings the head-hat as part of the attempt: the fork is what jams (its dest sausage is
    // wall-blocked), so Stephen stays put, yet the hat sweeps and can shove whatever sits in its swept corner. If that
    // shove rolls a sausage clean off the map it drowns for good -- an irreversible loss even though the turn bonks.
    // Detect it on a scratch plan and refuse the move so the explorer prunes the branch.
    MovePlan scratch = plan;
    u64 scratchHat = 0;
    PlanHatRotation(dir, scratch, scratchHat);
    // If that sweep shoves a sausage clean off the map -- fully out of the grid, or onto an unsupported VOID cell within
    // the grid's bounds where it drowns -- the bonk is an irreversible loss, so refuse it. Settle the scratch first (it
    // returns false when a sausage falls out of the world), then still reject a fully-off-grid one.
    u64 sweptMask = scratch.mask;
    if (!Settle(scratch, sweptMask, _sausages.begin(), _stephen)) return false;
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

void Level::PlanHatRotation(Direction dir, MovePlan& plan, u64& hatMask) const {
  s8 headX = _stephen.x, headY = _stephen.y, z = _stephen.z;
  s8 hatNo = GetSausage(headX, headY, z + 1);
  if (hatNo == -1) return;

  // Only a "clean hat" pivots: the far half (the end not on the head) must be cantilevered over open space, not
  // resting on another sausage OR on wall terrain -- in either case it's anchored (held in place) and stays put.
  {
    const Sausage& hat = _sausages[hatNo];
    auto [farX, farY] = hat.OtherEnd(headX, headY);
    if (GetSausage(farX, farY, z) != -1 || IsWall(farX, farY, z)) return;
  }

  // Rotation taking the old facing onto the new one, applied to every end's offset about the head cell. The direct head
  // hat pivots; a squarely-stacked rider above it pivots with it, but a rider resting only on the stationary pivot cell
  // stays put (guarded per-level below).
  auto [odx, ody] = Delta(_stephen.dir);
  auto [ndx, ndy] = Delta(dir);
  bool cw = (odx * ndy - ody * ndx) > 0;
  for (s8 sausageNo = hatNo, stackZ = z + 1; sausageNo != -1 && !(plan.mask & (1ull << sausageNo)); sausageNo = GetSausage(headX, headY, ++stackZ)) {
    // Only the DIRECT head hat is spun by Stephen's head. A sausage stacked ABOVE it spins only if it is a rigid part of
    // the tower -- its NON-head end must rest on a lower sausage that is itself pivoting (a squarely-stacked rider whose
    // far end sits on the hat's swinging end). A rider supported only at the head cell -- the pivot, which stays put --
    // does NOT spin: its support never moves. Once an upper level fails this, nothing above it spins.
    if (stackZ > z + 1) {
      const Sausage& u = _sausages[sausageNo];
      bool firstAtHead = (u.x1 == headX && u.y1 == headY);
      s8 ufx = firstAtHead ? u.x2 : u.x1, ufy = firstAtHead ? u.y2 : u.y1;
      s8 support = GetSausage(ufx, ufy, u.z - 1);
      if (support == -1 || !(hatMask & (1ull << support))) break;
    }
    Sausage s = _sausages[sausageNo];
    s8 ox1 = s.x1, oy1 = s.y1, ox2 = s.x2, oy2 = s.y2;
    auto rot = [&](s8& x, s8& y) { s8 ox = x - headX, oy = y - headY; if (cw) { x = headX - oy; y = headY + ox; } else { x = headX + oy; y = headY - ox; } };
    rot(s.x1, s.y1);
    rot(s.x2, s.y2);
    // A wall at a destination cell blocks the swing; so does one in the corner an end sweeps through (the cell diagonally
    // between its old and new spot = old + new - head). Either way the whole hat stays put.
    //
    // A wall in a CORNER cell (the diagonal an end sweeps through, = old + new - head) blocks the swing before it even
    // starts: the hat stays put and nothing is shoved. A wall only at a DESTINATION cell is
    // different -- the sweep's FIRST leg still happens (shoving whatever sits in the corner), and only the landing is
    // blocked, so the hat settles back put AFTER that corner shove, just as a fork's corner push lands even on a bonk.
    if (IsWall(ox1 + s.x1 - headX, oy1 + s.y1 - headY, s.z)
     || IsWall(ox2 + s.x2 - headX, oy2 + s.y2 - headY, s.z)) return; // a corner wall blocks the sweep -> hat stays put
    bool destBlocked = IsWall(s.x1, s.y1, s.z) || IsWall(s.x2, s.y2, s.z);

    // The far end sweeps through a corner cell into its destination; a sausage sitting in either is shoved out of the
    // way (corner along the sweep's first leg, destination along its second). The corner shove lands even when the
    // destination is walled; a shove that can't happen leaves the hat put.
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
        if (other == -1 || other == sausageNo || (plan.mask & (1ull << other))) return true;
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
    plan.mask |= (1ull << sausageNo);
    hatMask |= (1ull << sausageNo);
  }
}

void Level::PlanHatCarry(s8 sausageNo, s8 dx, s8 dy, Direction dir, MovePlan& plan, u64 rigidMask) const {
  // A sausage riding on Stephen's head/fork translates with him. Whether it rolls is decided per sausage by |rigidMask|:
  // the rigid hat stack rides without rolling; any other carried sausage (a fork-borne base, or a cantilevered rider)
  // rolls when carried across its long axis. A wall in its path, or a far end anchored on a non-moving sausage/wall,
  // leaves it behind.
  if (sausageNo == -1 || (plan.mask & (1ull << sausageNo))) return;
  Sausage s = _sausages[sausageNo];
  s8 belowA = GetSausage(s.x1, s.y1, s.z - 1);
  s8 belowB = GetSausage(s.x2, s.y2, s.z - 1);
  // Anchored if its far end rests on a NON-MOVING support -- a stationary sausage below, or wall terrain. The support
  // end (on Stephen's head/fork) never trips the wall test (its z-1 is Stephen's own level), so checking both is safe.
  if ((belowA != -1 && belowA != sausageNo && !(plan.mask & (1ull << belowA)))
   || (belowB != -1 && belowB != sausageNo && !(plan.mask & (1ull << belowB)))
   || IsWall(s.x1, s.y1, s.z - 1) || IsWall(s.x2, s.y2, s.z - 1)) return; // anchored on a non-moving sausage or wall
  if (IsWall(s.x1 + dx, s.y1 + dy, s.z) || IsWall(s.x2 + dx, s.y2 + dy, s.z)) return;
  s8 aboveA = GetSausage(s.x1, s.y1, s.z + 1);
  s8 aboveB = GetSausage(s.x2, s.y2, s.z + 1);
  // Another sausage where it shifts must be shoved first; if it can't be shoved, the hat has nowhere to ride and is left
  // behind for Settle to resolve.
  s8 destA = GetSausage(s.x1 + dx, s.y1 + dy, s.z);
  if (destA != -1 && destA != sausageNo && !(plan.mask & (1ull << destA)) && !(plan.rigidLoad & (1ull << destA)))
    if (!PlanSausagePush(destA, dir, plan)) return;
  s8 destB = GetSausage(s.x2 + dx, s.y2 + dy, s.z);
  if (destB != -1 && destB != sausageNo && destB != destA && !(plan.mask & (1ull << destB)) && !(plan.rigidLoad & (1ull << destB)))
    if (!PlanSausagePush(destB, dir, plan)) return;
  s.x1 += dx; s.y1 += dy; s.x2 += dx; s.y2 += dy;
  // Only sausages in the rigid hat stack ride rigidly; anyone else rolls when carried across their long axis.
  bool rolls = !(rigidMask & (1ull << sausageNo));
  if (rolls && (s.IsHorizontal() ? (dy != 0) : (dx != 0))) s.flags ^= Sausage::Rolled;
  plan.sausages[sausageNo] = s;
  plan.mask |= (1ull << sausageNo);
  // Riders are carried the same way; each decides its own roll from |rigidMask| (a rider is only in the stack if both
  // its ends rest on it), so a rider with a cantilevered end rolls even while the head hat beneath it stays rigid.
  PlanHatCarry(aboveA, dx, dy, dir, plan, rigidMask);
  if (aboveB != aboveA) PlanHatCarry(aboveB, dx, dy, dir, plan, rigidMask);
}

bool Level::PlanSausagePush(s8 sausageNo, Direction dir, MovePlan& plan, const Stephen* mover) const {
  Sausage sausage = _sausages[sausageNo];
#if OVERWORLD_HACK // Sausage may not be pushed in the overworld if there's a wall in the way.
  if (IsWall(sausage.x1, sausage.y1, sausage.z) || IsWall(sausage.x2, sausage.y2, sausage.z)) return false;
#endif

  auto [dx, dy] = Delta(dir);
  s8 z = sausage.z;
  s8 x1 = sausage.x1 + dx;
  s8 y1 = sausage.y1 + dy;
  s8 x2 = sausage.x2 + dx;
  s8 y2 = sausage.y2 + dy;

  // A push either fully commits or leaves the plan exactly as it found it: a chain that bottoms out against a wall (or
  // a rider that can't be carried) must not strand the sausages already pushed ahead of the failure. Snapshot up front
  // and roll back on every refusal, so a caller that reinterprets a false return -- a forward push becoming a spear --
  // sees no partial motion. (5-3 Skeleton m273: a fork-push chain shoved one sausage before a second, wall-blocked one
  // refused the whole push; without rollback the first stayed shoved even though the move speared instead.)
  MovePlan snapshot = plan;

  // A sausage cannot be pushed into a wall.
  if (IsWall(x1, y1, z) || IsWall(x2, y2, z)) { plan = snapshot; return false; }

  // Push chain: another sausage standing where an end is headed gets pushed the same way first; if it can't move, the
  // whole chain is refused. (Skip our own other end -- a slide along the axis -- and anything already pushed.)
  s8 ahead1 = GetSausage(x1, y1, z);
  if (ahead1 != -1 && ahead1 != sausageNo && !(plan.mask & (1ull << ahead1)))
    if (!PlanSausagePush(ahead1, dir, plan, mover)) { plan = snapshot; return false; }
  s8 ahead2 = GetSausage(x2, y2, z);
  if (ahead2 != -1 && ahead2 != sausageNo && !(plan.mask & (1ull << ahead2)))
    if (!PlanSausagePush(ahead2, dir, plan, mover)) { plan = snapshot; return false; }

  // A push across the sausage's long axis rolls it (flipping which side faces down); a push along the axis slides it.
  // Motion only -- gravity (Settle) and heat (CookMoved) run after commit, so it keeps its level even if now cantilevered.
  bool rolls = sausage.IsHorizontal() ? (dir == Up || dir == Down) : (dir == Left || dir == Right);
  if (rolls) {
    const Sausage& o = _sausages[sausageNo];
    // Classify the support directly beneath one of the rider's ends: MOVING (a sausage folded into this plan that
    // actually shifts), ANCHORED (a solid terrain top, or a sausage that stays put), or void. Also report how a moving
    // base shifts and whether it rolled, so a co-moving slide can be recognised.
    auto classify = [&](s8 ex, s8 ey, bool& moving, bool& anchored, s8& mdx, s8& mdy, bool& rolled) {
      moving = false; anchored = false; mdx = 0; mdy = 0; rolled = false;
      if (IsWall(ex, ey, o.z - 1)) { anchored = true; return; }   // terrain top -> fixed support
      s8 b = GetSausage(ex, ey, o.z - 1);
      if (b == -1 || b == sausageNo) return;                       // void
      if (plan.mask & (1ull << b)) {
        mdx = plan.sausages[b].x1 - _sausages[b].x1;
        mdy = plan.sausages[b].y1 - _sausages[b].y1;
        rolled = ((_sausages[b].flags ^ plan.sausages[b].flags) & Sausage::Rolled) != 0;
      }
      if (mdx != 0 || mdy != 0) moving = true; else anchored = true; // a stationary sausage anchors too
    };
    bool mA, aA, mB, aB, aRolled, bRolled; s8 adx, ady, bdx, bdy;
    classify(o.x1, o.y1, mA, aA, adx, ady, aRolled);
    classify(o.x2, o.y2, mB, aB, bdx, bdy, bRolled);
    // The game's CalculateTorsion carries a rider rigidly (torsion 0) in two situations, so a cross-axis shove leaves it
    // flat instead of rolling:
    //   1. it spans a MOVING base and a FIXED support -- the anchored end can't rotate, so it only translates;
    //   2. a base beneath it SLIDES the same way at the rider's own speed without rolling -- it inherits that zero
    //      torsion (a rider riding one or more co-moving carriers).
    // Otherwise (a rolling base under an unanchored end, or a void cantilever) it tumbles with its roller: default roll.
    bool anchoredCarry = (mA || mB) && (aA || aB);
    bool coMovingSlide = (mA && !aRolled && adx == dx && ady == dy) ||
                         (mB && !bRolled && bdx == dx && bdy == dy);
    if (anchoredCarry || coMovingSlide) rolls = false;
  }
  sausage.x1 = x1;
  sausage.y1 = y1;
  sausage.x2 = x2;
  sausage.y2 = y2;
  if (rolls) sausage.flags ^= Sausage::Rolled;
  plan.sausages[sausageNo] = sausage;
  plan.mask |= (1ull << sausageNo);

  // This base is now committed to moving (it cleared the wall/chain checks above), so everything riding on it co-moves
  // by the same vector: none of these riders may ram another. Fold them into the co-moving set BEFORE carrying them, so
  // one rider's carry never shoves (and rolls) a sibling rider it laps over. (Only riders of a base that actually moves
  // are added, so a refused push leaves a stationary stack un-marked.)
  plan.rigidLoad |= CarriedMask(sausageNo);

  // A sausage stacked directly on top rides along by the same displacement, but it shares THIS base's torsion: it rolls
  // only when the base itself rolled. A base that SLIDES (moves along its own axis) carries its passenger flat -- the
  // game's zero-torsion rule (GameState.CalculateTorsion): equal support+rider speed => no roll. Read our ORIGINAL
  // footprint, since plan.sausages now holds our new spot.
  const Sausage& orig = _sausages[sausageNo];
  s8 aboveA = GetSausage(orig.x1, orig.y1, orig.z + 1);
  s8 aboveB = GetSausage(orig.x2, orig.y2, orig.z + 1);
  if (aboveA != -1 && aboveA != sausageNo && !(plan.mask & (1ull << aboveA)))
    if (!PlanSausageCarry(aboveA, dx, dy, dir, plan, mover, /*rigid=*/!rolls)) { plan = snapshot; return false; }
  if (aboveB != -1 && aboveB != sausageNo && aboveB != aboveA && !(plan.mask & (1ull << aboveB)))
    if (!PlanSausageCarry(aboveB, dx, dy, dir, plan, mover, /*rigid=*/!rolls)) { plan = snapshot; return false; }
  return true;
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
    return below != -1 && below != sausageNo && !(plan.mask & (1ull << below));
  };
  if (anchoredAt(orig.x1, orig.y1, orig.z) || anchoredAt(orig.x2, orig.y2, orig.z)) return true; // anchored -> stays put

  // A rider that would merely SLIDE along its own long axis (motion parallel to its length, not a roll across it) is NOT
  // carried by a base that is itself a DRAGGED roller -- a rider that only rolled because ITS base rolled (the game's
  // reverse torsion, -1). The doubly-rolled contact slips out from under the parallel rider instead of translating it,
  // so it stays put and the post-commit Settle drops it.
  bool wouldRoll = orig.IsHorizontal() ? (dir == Up || dir == Down) : (dir == Left || dir == Right);
  if (baseDragRolled && !wouldRoll) return true; // parallel slide on a dragged roller -> not carried, falls

  // During a TURN, Stephen's body OR fork counts as a wall in its FINAL cell. A rider end on his body is held. A rider
  // end over the fork is held too -- but if a sausage sits in that fork cell, the fork only
  // takes over once that base slides OUT of it (leftBehind) AND the rider's OTHER end isn't itself on a moving sausage;
  // otherwise the rider is carried with its base.
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
      bool leftBehind = (plan.mask & (1ull << base)) && !_sausages[base].IsAt(ex - dx, ey - dy, orig.z - 1);
      // The fork catches the rider only when it sits STRICTLY under the rider's LEADING end in the roll direction. A
      // rider whose free end points the SAME way as the base's roll tumbles AWAY from the fork -- the fork is behind it,
      // so it double-rolls off instead of being held. A rider lying PERPENDICULAR to the slide
      // (both ends projecting equally) likewise rides along on its sliding base rather than being caught, so only a
      // fork strictly under the leading end holds it.
      if (leftBehind && (s8)(ex * dx + ey * dy) <= (s8)(ox * dx + oy * dy)) return false;
      // The rider is carried off the fork only if its OTHER end is dragged the SAME way as this carry -- then the whole
      // rider translates with both supports. An other-end base that moves a DIFFERENT way (e.g. the
      // turn's corner-sweep, perpendicular to the fork-dest push) just slides out, leaving that end to cantilever while
      // the fork catches this one.
      s8 otherBase = GetSausage(ox, oy, orig.z - 1);        // pre-move sausage under the rider's OTHER end
      bool otherCarries = false;
      if (otherBase != -1 && (plan.mask & (1ull << otherBase))) {
        s8 obdx = plan.sausages[otherBase].x1 - _sausages[otherBase].x1;
        s8 obdy = plan.sausages[otherBase].y1 - _sausages[otherBase].y1;
        otherCarries = (obdx == dx && obdy == dy);          // moves the same way as the carry -> the rider rides along
      }
      return leftBehind && !otherCarries;
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
  // push); if it can't be shoved, this sausage is left behind like the wall case above.
  s8 destA = GetSausage(sausage.x1, sausage.y1, sausage.z);
  if (destA != -1 && destA != sausageNo && !(plan.mask & (1ull << destA)) && !(plan.rigidLoad & (1ull << destA)))
    if (!PlanSausagePush(destA, dir, plan)) return true;
  s8 destB = GetSausage(sausage.x2, sausage.y2, sausage.z);
  if (destB != -1 && destB != sausageNo && destB != destA && !(plan.mask & (1ull << destB)) && !(plan.rigidLoad & (1ull << destB)))
    if (!PlanSausagePush(destB, dir, plan)) return true;

  bool rolls = sausage.IsHorizontal() ? (dir == Up || dir == Down) : (dir == Left || dir == Right);
  // A hat resting on Stephen's head rides on him -- a rigid co-mover -- so it inherits his zero torsion and does NOT
  // roll, even when a push chain carried it by its OTHER end sitting on a rolling base.
  bool onStephenHead = _stephen.z == orig.z - 1
      && ((_stephen.x == orig.x1 && _stephen.y == orig.y1) || (_stephen.x == orig.x2 && _stephen.y == orig.y2));
  bool rolled = rolls && !rigid && !onStephenHead; // a rider on a rigidly-dragged/sliding base, or on Stephen, takes zero torsion -> no roll
  if (rolled) sausage.flags ^= Sausage::Rolled;
  plan.sausages[sausageNo] = sausage;
  plan.mask |= (1ull << sausageNo);

  // Anything stacked on THIS rider shares its torsion in turn: it rolls only if this rider actually rolled. And if this
  // rider rolled, it becomes a DRAGGED roller for whatever sits on it -- a parallel passenger there is not carried.
  if (aboveA != -1 && aboveA != sausageNo && !(plan.mask & (1ull << aboveA)))
    if (!PlanSausageCarry(aboveA, dx, dy, dir, plan, mover, /*rigid=*/!rolled, /*baseDragRolled=*/rolled)) return false;
  if (aboveB != -1 && aboveB != sausageNo && aboveB != aboveA && !(plan.mask & (1ull << aboveB)))
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
  return true;
}

bool Level::CookMoved(MovePlan& plan, u64 movedMask, u64 preCookedMask) {
  for (int i = 0; i < _sausages.Size(); i++) {
    if (!(movedMask & (1ull << i)) || (preCookedMask & (1ull << i))) continue;
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
  // -- the explorer then never walks this path, matching the game's dead-end.
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
  u64 movedMask = plan.mask;
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
  // void at the grid's edge. Checked FIRST so an off-grid cell isn't dismissed before we notice
  // the fork under it.
  const Stephen& actor = plan.stephen;
  if (actor.x == x && actor.y == y && actor.z == z - 1) return true;
  if (actor.HasFork() && actor.forkX == x && actor.forkY == y && actor.forkZ == z - 1) return true;
  // An off-grid cell is otherwise never a footing (this also subsumes the z<0 guard). Without it, a sausage rolled
  // ENTIRELY off an edge could be "supported" by another hanging half-off the grid below it.
  if (!IsWithinGrid(x, y, z)) return false;
  // Terrain footing: the ground floor at z==0, or a wall-top (a wall solid at z-1) for z>=1.
  if (z == 0 ? CanWalkOnto(x, y, 0) : IsWall(x, y, z - 1)) return true;
  // A sausage directly below in the WORKING tableau (not the live, pre-move _sausages).
  if (GetPlannedSausage(plan, x, y, z - 1) != -1) return true;
  return false;
}

bool Level::Settle(MovePlan& plan, u64& movedMask, const Sausage* preMove, const Stephen& prevStephen, u64 exclude) {
  // The sausage on the held fork is carried rigidly, not subject to gravity -- exempt it from the fall. The fork is at
  // the plan's post-move pose; the speared sausage is whatever sits in its cell in the working tableau.
  s8 speared = GetPlannedSausage(plan, plan.stephen.forkX, plan.stephen.forkY, plan.stephen.forkZ);

  // The fork holds up the whole stack that rests (post-move) on the speared sausage, not just the sausage itself -- the
  // entire load rides the fork and stays put, even out over the void where SausageSupported would otherwise bail on the
  // off-grid cell.
  u64 forkHeld = 0;
  if (speared != -1) {
    forkHeld = (1ull << speared);
    for (bool grew = true; grew; ) {
      grew = false;
      for (int i = 0; i < _sausages.Size(); i++) {
        if (forkHeld & (1ull << i)) continue;
        const Sausage& s = plan.sausages[i];
        s8 b1 = GetPlannedSausage(plan, s.x1, s.y1, s.z - 1);
        s8 b2 = GetPlannedSausage(plan, s.x2, s.y2, s.z - 1);
        if ((b1 != -1 && (forkHeld & (1ull << b1))) || (b2 != -1 && (forkHeld & (1ull << b2)))) { forkHeld |= (1ull << i); grew = true; }
      }
    }
  }

  // A sausage that rode on Stephen pre-move (resting on his body or fork) becomes subject to gravity when he steps
  // out from under it: seed it active so the fall pass below drops it if its footing is now gone.
  for (int i = 0; i < _sausages.Size(); i++) {
    if ((movedMask & (1ull << i)) || (exclude & (1ull << i))) continue;
    const Sausage& pre = preMove[i];
    bool rode = (pre.x1 == prevStephen.x && pre.y1 == prevStephen.y && pre.z - 1 == prevStephen.z)
             || (pre.x2 == prevStephen.x && pre.y2 == prevStephen.y && pre.z - 1 == prevStephen.z)
             || (pre.x1 == prevStephen.forkX && pre.y1 == prevStephen.forkY && pre.z - 1 == prevStephen.forkZ)
             || (pre.x2 == prevStephen.forkX && pre.y2 == prevStephen.forkY && pre.z - 1 == prevStephen.forkZ)
             // ...or was SPEARED on the fork (same z, sharing the fork's cell). When a backward step pulls the fork free
             // it leaves the sausage put but no longer held up, so it must fall if nothing else supports it. A
             // still-speared/dragged sausage is already in |movedMask|, so this only fires once the fork has actually let go.
             || (prevStephen.HasFork() && pre.IsAt(prevStephen.forkX, prevStephen.forkY, prevStephen.forkZ));
    if (rode) movedMask |= (1ull << i);
  }

  // Only sausages this move disturbed are subject to gravity: the ones it moved, plus any that were (in the PRE-move
  // layout) stacked on a disturbed one -- transitively, so a whole tower comes along when its base slides out. A sausage
  // already floating before this move is left alone. A pending double-move (|exclude|) is held back -- its own
  // DoubleMove pass owns its fall.
  u64 active = movedMask & ~exclude;
  bool grew = true;
  while (grew) {
    grew = false;
    for (int i = 0; i < _sausages.Size(); i++) {
      if ((active & (1ull << i)) || (exclude & (1ull << i))) continue;
      const Sausage& preMoveSausage = preMove[i];
      auto restedOnActive = [&](s8 x, s8 y, s8 z) -> bool {
        for (int k = 0; k < _sausages.Size(); k++)
          if (k != i && (active & (1ull << k)) && preMove[k].IsAt(x, y, z - 1)) return true;
        return false;
      };
      if (restedOnActive(preMoveSausage.x1, preMoveSausage.y1, preMoveSausage.z) || restedOnActive(preMoveSausage.x2, preMoveSausage.y2, preMoveSausage.z)) {
        active |= (1ull << i);
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
      if ((forkHeld & (1ull << i)) || !(active & (1ull << i))) continue;
      Sausage& sausage = plan.sausages[i];
      if (SausageSupported(plan, sausage.x1, sausage.y1, sausage.z) || SausageSupported(plan, sausage.x2, sausage.y2, sausage.z)) continue;
      if (sausage.z <= 0) return false; // fell out of the bottom of the world -> the move is refused
      sausage.z--;
      movedMask |= (1ull << i);
      changed = true;
    }
  }
  return true;
}

void Level::MarkDoubleMoves(MovePlan& plan, const Sausage* preMove) const {
  for (int rider = 0; rider < _sausages.Size(); rider++) {
    if (!(plan.mask & (1ull << rider)) || (plan.doubleMoveMask & (1ull << rider))) continue; // only just-moved sausages, not already flagged
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
    // and over, so a tile there cancels the double-move and it shifts only the one cell. A blocker
    // above only the TRAILING end does NOT pin it -- the rider slides out from under that as it advances.
    if (IsWall(leadX, leadY, orig.z + 1)) continue;
    s8 above = GetSausage(leadX, leadY, orig.z + 1);
    if (above != -1 && above != rider) {
      // A sausage overhead pins the leading end only if it stays perched there through the slide -- i.e. it travels in
      // lockstep with the rider (a head hat riding Stephen keeps its far end on the mid: LogRollHeadHatOverMovingMid).
      // A STATIONARY sausage above does NOT follow: the rider slides out from under it after the first cell, its leading
      // end clears, and it still double-moves.
      const Sausage& ao = preMove[above], & an = plan.sausages[above];
      bool followsRider = (plan.mask & (1ull << above))
          && an.x1 - ao.x1 == mdx && an.y1 - ao.y1 == mdy && an.x2 - ao.x2 == mdx && an.y2 - ao.y2 == mdy;
      // ...but a follower PINS the leading end only if it is anchored to STEPHEN (his head/fork), which advances
      // exactly one cell. A follower that is itself a rider stacked on the moving base is no anchor -- it tumbles WITH
      // the double-move rather than capping it, and DoubleMove drags it along. The head-hat
      // (LogRollHeadHatOverMovingMid) rests on Stephen,
      // so it still pins.
      bool aboveOnStephen = (_stephen.z == ao.z - 1 && ((_stephen.x == ao.x1 && _stephen.y == ao.y1) || (_stephen.x == ao.x2 && _stephen.y == ao.y2)))
                         || (_stephen.HasFork() && _stephen.forkZ == ao.z - 1 && ((_stephen.forkX == ao.x1 && _stephen.forkY == ao.y1) || (_stephen.forkX == ao.x2 && _stephen.forkY == ao.y2)));
      if (followsRider && aboveOnStephen) continue;
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
    // extra tumble. A rolling base under the trailing end instead flicks it forward, and a
    // cantilevered trailing end is free -- both still double-move.
    s8 trailX = dot1 >= dot2 ? orig.x2 : orig.x1;
    s8 trailY = dot1 >= dot2 ? orig.y2 : orig.y1;
    s8 trailBase = GetSausage(trailX, trailY, orig.z - 1);
    bool trailPlanted = trailBase != -1 && trailBase != rider && (plan.mask & (1ull << trailBase))
        && ((preMove[trailBase].flags ^ plan.sausages[trailBase].flags) & Sausage::Rolled) == 0;
    // The extra flick only lands if the rider keeps sliding after the first cell. What halts it is its LEADING end
    // settling onto a raised terrain shelf whose top sits at the rider's own level: the shelf catches the front and the
    // game rolls the push back to a single cell (GameState.ApplyPassiveForce undoes the speed+1 push when the post-push
    // footprint rests on non-moving ground). A leading end that clears the shelf -- over open void or lower ground after
    // one cell -- carries the rider the full two cells. Judge the leading end at its SINGLE-cell destination, against
    // the ORIGINAL layout.
    bool caughtByShelf = IsWall(leadX + mdx, leadY + mdy, orig.z - 1);
    // A non-moving sausage at that same leading-end destination (one level down) is a shelf too: its top sits at the
    // rider's own level and catches the front, halting the extra tumble. A sausage that is itself moving away this turn
    // doesn't catch.
    s8 shelfSausage = GetSausage(leadX + mdx, leadY + mdy, orig.z - 1);
    if (shelfSausage != -1 && shelfSausage != rider && !(plan.mask & (1ull << shelfSausage))) caughtByShelf = true;
    if (baseRolled && !onParallel && !caughtByShelf && !trailPlanted) {
      plan.doubleMoveMask |= (1ull << rider);
      plan.doubleMoveDir[rider] = dir;
    }
  }
}

bool Level::DoubleMove(MovePlan& plan) {
  if (plan.doubleMoveMask == 0) return true;

  // Each double-mover tumbles one more cell in its recorded direction; anything RIDING a double-mover is thrown along
  // with it by the same vector, so a stack tipped off a rolling base double-moves as a unit. Grow the moving set up the
  // ride chain, tagging
  // each rider with its base's direction. A rider that is also anchored on a NON-moving support is held, not thrown, so
  // it is left out of the group.
  Direction ddir[NUM_SAUSAGES];
  u64 group = 0;
  for (int i = 0; i < _sausages.Size(); i++)
    if (plan.doubleMoveMask & (1ull << i)) { group |= (1ull << i); ddir[i] = plan.doubleMoveDir[i]; }
  for (bool grew = true; grew; ) {
    grew = false;
    for (int i = 0; i < _sausages.Size(); i++) {
      if (group & (1ull << i)) continue;
      const Sausage& s = plan.sausages[i];
      s8 b1 = GetPlannedSausage(plan, s.x1, s.y1, s.z - 1);
      s8 b2 = GetPlannedSausage(plan, s.x2, s.y2, s.z - 1);
      bool anchored = IsWall(s.x1, s.y1, s.z - 1) || IsWall(s.x2, s.y2, s.z - 1)
          || (b1 != -1 && !(group & (1ull << b1))) || (b2 != -1 && !(group & (1ull << b2)));
      if (anchored) continue;
      s8 base = (b1 != -1 && (group & (1ull << b1))) ? b1 : (b2 != -1 && (group & (1ull << b2))) ? b2 : (s8)-1;
      if (base != -1) { group |= (1ull << i); ddir[i] = ddir[base]; grew = true; }
    }
  }

  // Snapshot the just-settled working tableau: it's the "pre-move" layout for the double-move's own settle pass.
  Sausage preMove[NUM_SAUSAGES];
  for (int i = 0; i < _sausages.Size(); i++) preMove[i] = plan.sausages[i];
  Stephen prevStephen = plan.stephen;
  u64 movedMask = 0;
  for (int i = 0; i < _sausages.Size(); i++) {
    if (!(group & (1ull << i))) continue;
    auto [dx, dy] = Delta(ddir[i]);
    Sausage& sausage = plan.sausages[i];
    // Tumble one more cell in the recorded direction -- unless a wall stops it, in which case it just comes to rest
    // where it already is. Either way it now settles and cooks as its own event. (Aligned slide / rigid carry -> no
    // roll.)
    s8 nx1 = sausage.x1 + dx, ny1 = sausage.y1 + dy, nx2 = sausage.x2 + dx, ny2 = sausage.y2 + dy;
    bool blocked = IsWall(nx1, ny1, sausage.z) || IsWall(nx2, ny2, sausage.z);
    // A non-group sausage sitting where the tumble lands is shoved the same way FIRST -- the game resolves this
    // horizontal collision as a push, before gravity gets to drop the tumbler under it. A shove that wall-bottoms stops the
    // tumble; a shove off the world is left for Settle below to drown (refusing the move).
    if (!blocked) {
      u64 pushed = 0;
      s8 h1 = GetPlannedSausage(plan, nx1, ny1, sausage.z);
      if (h1 != -1 && h1 != i && !(group & (1ull << h1)) && !PushPlanned(plan, h1, dx, dy, group, pushed)) blocked = true;
      s8 h2 = GetPlannedSausage(plan, nx2, ny2, sausage.z);
      if (!blocked && h2 != -1 && h2 != i && h2 != h1 && !(group & (1ull << h2)) && !PushPlanned(plan, h2, dx, dy, group, pushed)) blocked = true;
      movedMask |= pushed; // shoved sausages settle & cook as part of this event
    }
    if (!blocked) {
      sausage.x1 = nx1;
      sausage.y1 = ny1;
      sausage.x2 = nx2;
      sausage.y2 = ny2;
    }
    movedMask |= (1ull << i);
  }
  if (!Settle(plan, movedMask, preMove, prevStephen)) return false;
  if (!CookMoved(plan, movedMask)) return false;
  return true;
}

bool Level::PushPlanned(MovePlan& plan, s8 sausageNo, s8 dx, s8 dy, u64 protect, u64& pushed) const {
  Sausage& s = plan.sausages[sausageNo];
  s8 nx1 = s.x1 + dx, ny1 = s.y1 + dy, nx2 = s.x2 + dx, ny2 = s.y2 + dy;
  // Driving an end straight into a wall bottoms the chain out -- the whole push is refused.
  if (IsWall(nx1, ny1, s.z) || IsWall(nx2, ny2, s.z)) return false;
  // Propagate down the chain: another sausage standing where an end is headed gets pushed the same way first (skip our
  // own other end, protected group members, and anything already pushed).
  s8 a1 = GetPlannedSausage(plan, nx1, ny1, s.z);
  if (a1 != -1 && a1 != sausageNo && !(protect & (1ull << a1)) && !(pushed & (1ull << a1))
      && !PushPlanned(plan, a1, dx, dy, protect, pushed)) return false;
  s8 a2 = GetPlannedSausage(plan, nx2, ny2, s.z);
  if (a2 != -1 && a2 != sausageNo && a2 != a1 && !(protect & (1ull << a2)) && !(pushed & (1ull << a2))
      && !PushPlanned(plan, a2, dx, dy, protect, pushed)) return false;
  // A push across the long axis rolls it; along the axis it slides. Motion only -- Settle/CookMoved run afterward.
  bool rolls = s.IsHorizontal() ? (dy != 0) : (dx != 0);
  s.x1 = nx1;
  s.y1 = ny1;
  s.x2 = nx2;
  s.y2 = ny2;
  if (rolls) s.flags ^= Sausage::Rolled;
  pushed |= (1ull << sausageNo);
  return true;
}


bool Level::SausageBlocked(s8 sausageNo, Direction dir) const {
  Sausage sausage = _sausages[sausageNo];
  auto [dx, dy] = Delta(dir);
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
