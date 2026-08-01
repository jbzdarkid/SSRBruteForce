# Correctness oracle
To verify my engine is correct, I have pulled out a few key files from the game's code.

Direct copies:
- `BoundingBox.cs`
- `Coord.cs`
- `Direction.cs`
- `DirectionUtil.cs`
- `EntitySkeleton.cs`
- `EntType.cs`
- `Fraction.cs`
- `GameState.cs`
- `IslandMask.cs`
- `MetaGameState.cs`
- `Movement.cs`
- `Occupancy.cs`
- `Pair.cs`
- `ParseUtils.cs`
Minor edits:
- `Utility.cs`: Removed `IsVisibleFrom(Renderer, Camera)`
- `Entity.cs`: Added some explicit casts.
New files:
- `Main.cs`: Orchestrator
- `Shims.cs`: Shims for game code
- `UnityShims.cs`: Shims for unity code
