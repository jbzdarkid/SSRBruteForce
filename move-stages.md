# How a move resolves (game-world model)

A game-world model of what one key press *is*, independent of how the code is laid out.

## The stages

0. **Classify the input** — Decide what the press *means* given Stephen's situation: standing **on** a sausage → log-roll; facing/touching a **ladder** → climb; the fork **lodged** in a sausage → a rigid **drag**; otherwise the press axis decides — along Stephen's facing → plain **walk**, across it → **turn**.
1. **Project the motion** — Work out where Stephen's **body** and **fork** intend to go.
2. **Trace contacts & test feasibility** — Follow the fork (leading edge) and body into the world: which sausages are **speared / pushed / lifted / carried**, and does the whole chain have somewhere to go? If any link is blocked, the **entire move is refused**.
3. **Enact the motion (all at once)** — Slide Stephen, slide/roll pushed sausages, rotate carried ones, lift the speared one — as one simultaneous shove.
4. **Commit Stephen** — Body and held fork now stand at the new cell.
5. **Settle (gravity)** — Anything no longer resting on ground/wall/another sausage, or **Stephen's body/fork at their new position**, falls until it lands. Resolved **bottom-up**.
6. **Apply heat** — Each face that comes to rest on a **grill** cooks; an already-cooked face touching a grill again **burns** (move fails).
7. **Knock-on motion** — A sausage balanced across a perpendicular one keeps tumbling an extra cell; runs as its own little move → settle → cook.
8. **Resolve outcomes** — Register hot-ground steps (which chain a recoil move), check win (all cooked + home) / loss (burn).

## Why this order

- **Classify before simulating (0→1).** The same key is ambiguous — "up" can mean climb, walk, turn, roll the log underfoot, or drag a speared sausage. The terrain around Stephen dictates which, so that decision must come first. Dispatch priority (log-roll → ladder → drag → walk/turn) runs from most context-hijacking to default. (Walk/turn is not a priority decision, just based on the movement key).
- **Feasibility before motion (2→3).** A move is **atomic**: either the whole tableau shifts or nothing does. You never see a sausage shoved halfway into a wall, so the full push-chain is validated before anything actually moves.
- **The fork leads.** It's the manipulator out front — it pokes, spears, and lifts before the body's feet arrive.
- **Motion is one simultaneous event (3, and the crux of 4→5).** Stephen and what he pushes/carries are rigidly linked during the shove, so "is this supported?" has no stable answer mid-motion. It's only meaningful against the **final** tableau: a sausage lifted on the fork is held up by the fork *at its new height*. Judging support from Stephen's *old* position would wrongly drop it. So support is always checked against where Stephen and his fork **end up**.
- **Settle after motion, bottom-up (5).** Gravity is a reaction to the new arrangement — you can't know what's floating until every slide/roll/turn is done, and pieces fall from the ground up so each lands on already-settled support.
- **Cook only at the resting place (5→6).** A sausage cooks the face that's *down where it stops*, not every grill cell it skims over rolling — so heat (and burn-detection) waits until the piece settles.
- **Knock-ons are consequences (6→7).** The extra tumble is *caused by* the primary push completing. Running it as its own fresh move-settle-cook keeps each event's gravity/heat bookkeeping separate, so the secondary roll never retroactively re-cooks or re-drops pieces that already came to rest.
- **Outcomes read the settled world (8).** Win and loss are judgments about the resulting state, so they come last.


The thread through all of it: **decide the meaning, prove the whole motion is legal, move everything at once, then let the world react — gravity, then heat, then knock-ons, then outcome.** Each reaction needs the previous stage's result to even be well-defined, which is what pins down the order.

## Design rule — plan aside, then commit once

The stages describe *what* a move is; this is the one rule for *how* it's carried out.

**A move reasons on a private plan, then commits once.** It works against a scratch copy of the board — Stephen's pose and every sausage, seeded from the current state — and every stage writes into that copy: the motion (1–3) *and* the reactions (gravity, heat, knock-ons). The live world is left untouched until the plan is complete and legal, then applied in a single step.

This is what makes a move **atomic**. Because even the reactions play out on the plan, an illegal outcome — a sausage lost off the map, a burn — is simply a refusal, with nothing to undo. The move never changes the world and then rolls it back; it settles everything first and applies the result once.
