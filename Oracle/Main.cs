using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

static class Oracle {
  static int Main(string[] args) {
    string levelName = args[0];
    string demos = args[1];
    if (Directory.Exists(args[1])) BulkReplay(levelName, demos);
    else Console.WriteLine($"Reason: {ReplayDemo(levelName, demos, debug: true)}"); // was actually a file

    return 0;
  }

  static Dictionary<string, string> loadedLevels = new();
  static GameState LoadFromBlob(string levelName) {
    if (loadedLevels.TryGetValue(levelName, out string levelData)) {
      return GameState.Load(levelData, null, true);
    }

    using var reader = new BinaryReader(File.OpenRead("Extracted/merged_binary.bin"));
    MetaGameState metaGame = new();
    metaGame.LoadBinary(reader);

    // Strip the "1-1 " level prefix; the rest is the game's display name verbatim
    string displayName = levelName[(levelName.IndexOf(' ') + 1)..];
    GameState island = metaGame.islands.Values.FirstOrDefault(g => g.displayname == displayName);
    if (island == null) throw new Exception($"No island with display name '{displayName}' in merged_binary");

    // Fresh copy so the cached island isn't mutated; strip decoration/markers off the working copy.
    GameState work = GameState.Load(island.Save(false, false), null, false);
    work.entities.RemoveAll(e => e.Decoration() || e.type == EntType.island || e.type == EntType.spectralsausage);
    work.dynamicentities.RemoveAll(e => e.Decoration() || e.type == EntType.island || e.type == EntType.spectralsausage);

    // Some source islands store a sausage pre-rolled; flatten rot to match how the C++ authors it (rot 0).
    foreach (Entity e in work.entities) {
      if (e.type == EntType.sausage) e.rot = 0;
    }

    // An isolated level is never the overworld (impacts some internal checks)
    work.overworld = false;

    int minx = work.entities.Min(e => e.pos.x);
    int miny = work.entities.Min(e => e.pos.y);
    int minz = work.entities.Min(e => e.pos.z);
    string translated = GameState.Translate(work.Save(false, false), new Coord(-minx, -miny, -1 - minz));

    loadedLevels[levelName] = translated;
    return GameState.Load(translated, null, true);
  }

  static int Compare(Coord a, Coord b) {
    if (a.x != b.x) return a.x - b.x;
    if (a.y != b.y) return a.y - b.y;
    return a.z - b.z;
  }

  static void BulkReplay(string levelName, string folderPath) {
    string[] demoPaths = Directory.GetFiles(folderPath, "*.dem");
    Dictionary<string, List<string>> failureReasons = [];
    foreach (string demoPath in demoPaths) {
      string reason = ReplayDemo(levelName, demoPath);
      if (!failureReasons.ContainsKey(reason)) failureReasons[reason] = [];
      failureReasons[reason].Add(demoPath);
    }

    foreach ((string reason, List<string> demos) in failureReasons) {
      Console.WriteLine($"Reason: {reason}; Count: {demos.Count}; Sample: {demos[0]}");
    }
  }
  
  static string ReplayDemo(string levelName, string demoPath, bool debug=false) {
    GameState gs = LoadFromBlob(levelName);

    long totalUnits = 0;
    string[] lines = File.ReadAllLines(demoPath);
    for (int i = 0; i < lines.Length; i++) {
      if (lines[i] == "Undo") i += 2; // Real demos have a second-move Undo, then immediately replay move 1 as move 3. Jump to move 4.
      string line = lines[i];
      if (line == "Stop") break;

      Direction dir = line switch {
        "North" => Direction.North,
        "South" => Direction.South,
        "East"  => Direction.East,
        "West"  => Direction.West,
        _     => Direction.None,
      };

      gs.ProcessInput(dir);
      long moveUnits = Game.ResolveMove(gs);

      if (debug) {
        string success = (gs.Lost().Length == 0) ? "SUCCEEDED" : "FAILED";
        Console.WriteLine();
        Console.WriteLine($"=== move {i + 1}: {line} {success} ===");
        Console.WriteLine($"{ToString(gs)} {moveUnits} {totalUnits}");
      }

      if (gs.Won()) return ""; // Real demos have trailing moves to get to the next level, check early
      string reason = gs.Lost();
      if (reason.Length > 0) return reason;
    }

    string expectedFinalState = lines[^1];
    string actualFinalState = ToString(gs);
    return expectedFinalState == actualFinalState ? "" : "Final state";
  }

  static string ToString(GameState gs) {
    Entity p = gs.player;
    Coord forkPos = gs.fork != null ? gs.fork.pos : p.pos + p.direction;
    string forkDir = gs.fork != null ? gs.fork.direction.ToString() : "None";
    string line = $"{p.pos.x} {p.pos.y} {p.pos.z} {p.direction} {forkPos.x} {forkPos.y} {forkPos.z} {forkDir}";

    // A quarter is "cooked" at level 1 or 2 (0 = raw; 3 = burnt can't reach here -- Lost() returns "Burned" first).
    bool Cooked(int q) => q == 1 || q == 2;
    var sausages = new List<(Coord a, Coord b, string flags)>();
    foreach (Entity e in gs.dynamicentities) {
      if (e.type != EntType.sausage) continue;
      int cd = e.cookdata; // DoCook faces: pos-end = q3/q2 (rot0/rot1), coord-end = q0/q1 (rot0/rot1)
      var pos   = (cell: e.pos,               r0: Cooked((cd >> 6) & 3), r1: Cooked((cd >> 4) & 3));
      var coord = (cell: e.pos + e.direction, r0: Cooked(cd & 3),        r1: Cooked((cd >> 2) & 3));
      var (lo, hi) = Compare(pos.cell, coord.cell) <= 0 ? (pos, coord) : (coord, pos); // smaller cell = end1 (x1,y1)
      // Match the C++ Sausage operator<< tokens exactly (A = down when not rolled, B = down when rolled; rot != 0 == Rolled).
      string flags = "";
      if (lo.r0)      flags += " Cook1A";
      if (lo.r1)      flags += " Cook1B";
      if (hi.r0)      flags += " Cook2A";
      if (hi.r1)      flags += " Cook2B";
      if (e.rot != 0) flags += " Rolled";
      if (flags.Length == 0) flags = " NoFlags";
      sausages.Add((lo.cell, hi.cell, flags));
    }
    sausages.Sort((s1, s2) => {
      int c = Compare(s1.a, s2.a);
      if (c != 0) return c;
      return Compare(s1.b, s2.b);
    });
    foreach (var s in sausages) line += $" | {s.a.x} {s.a.y} {s.b.x} {s.b.y} {s.a.z}{s.flags}";
    return line;
  }
}
