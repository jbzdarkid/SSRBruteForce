using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

static class Oracle {
  static int Main(string[] args) {
    // THROWAWAY converter mode: emit a C++ Level(...) def for one level (arg[1] = "5-1 The Gorge").
    if (args.Length >= 1 && args[0] == "--tolevelh") {
      GameState g = LoadFromBlob(args[1]);
      Console.WriteLine(ConvertToLevelH(args[1], g));
      return 0;
    }

    // THROWAWAY: emit C++ Level(...) defs for the whole-world overworld islands. arg1 = shrine key + arg2 = display
    // name for one world, or arg1 = "all" to emit all five (worlds 1-5). Each world = its temple shrine + prereq level
    // islands merged at their overworld offsets, per-level sausages dropped, only the world sausage kept active.
    if (args.Length >= 2 && args[0] == "--world-tolevelh") {
      using var reader = new BinaryReader(File.OpenRead("Extracted/merged_binary.bin"));
      MetaGameState mg = new();
      mg.LoadBinary(reader);
      if (args[1] == "all") {
        Console.WriteLine("#pragma once");
        Console.WriteLine("#include \"Level2.h\"");
        var worlds = new[] { (1, "temple2j1"), (2, "temple1d1"), (3, "temple1x1"), (4, "temple1h1"), (5, "temple2c1") };
        foreach (var (w, key) in worlds) { Console.WriteLine(); Console.WriteLine(EmitWorldLevel(mg, key, $"{w}-final Overworld sausage")); }
      } else {
        Console.WriteLine(EmitWorldLevel(mg, args[1], args[2]));
      }
      return 0;
    }

    // THROWAWAY: dump one world's overworld geometry in absolute overworld coords -- global spawn, each level's
    // entry cell, the world-sausage cells, and every island near the world (to spot connectors between land pieces).
    if (args.Length >= 2 && args[0] == "--world-info") {
      using var reader = new BinaryReader(File.OpenRead("Extracted/merged_binary.bin"));
      MetaGameState mg = new();
      mg.LoadBinary(reader);
      string shrine = args[1];
      Coord so = mg.offsets[shrine];
      Console.WriteLine($"startpos (overworld spawn) = {mg.startpos}");
      if (mg.offsets.ContainsKey("start"))
        Console.WriteLine($"start island: offset={mg.offsets["start"]} player={mg.islands["start"].player.pos} abs={mg.islands["start"].player.pos + mg.offsets["start"]}");
      Console.WriteLine($"shrine {shrine}: offset={so} display=\"{mg.islands[shrine].displayname}\"");
      var prereqs = mg.templedat.TryGetValue(shrine, out var pr) ? pr : new List<string>();
      Console.WriteLine($"prereqs ({prereqs.Count}): {string.Join(", ", prereqs)}");

      Console.WriteLine("--- world sausage (D) cells: abs = rel + shrineOffset ---");
      foreach (var kv in mg.sausagepositions[shrine])
        Console.WriteLine($"  rel={kv.Key} abs={kv.Key + so} dir={kv.Value}");

      Console.WriteLine("--- world levels: offset, entry cell (playerpos abs), display ---");
      var worldKeys = new List<string> { shrine };
      worldKeys.AddRange(prereqs);
      foreach (string k in worldKeys) {
        string entry = mg.playerpositions.ContainsKey(k)
          ? $"entry_abs={mg.playerpositions[k].Key + mg.offsets[k]} dir={mg.playerpositions[k].Value}"
          : "entry=(none)";
        string disp = mg.islands.ContainsKey(k) ? mg.islands[k].displayname : "?";
        Console.WriteLine($"  {k} offset={mg.offsets[k]} {entry} \"{disp}\"");
      }

      // terrain abs bbox for an island (ground/barrier/ladder/bbq cells); null if it has no terrain
      (int x0, int x1, int y0, int y1)? TerrainBBox(string k) {
        if (!mg.offsets.TryGetValue(k, out Coord o) || !mg.islands.ContainsKey(k)) return null;
        GameState gi = GameState.Load(mg.islands[k].Save(false, false), null, false);
        var cells = gi.entities.Where(e => e.type == EntType.ground || e.type == EntType.barrier || e.type == EntType.bbq || e.type == EntType.ladder).ToList();
        if (cells.Count == 0) return null;
        return (cells.Min(e => e.pos.x) + o.x, cells.Max(e => e.pos.x) + o.x, cells.Min(e => e.pos.y) + o.y, cells.Max(e => e.pos.y) + o.y);
      }
      // world terrain bbox = union of all worldKeys' terrain
      int minx = int.MaxValue, maxx = int.MinValue, miny = int.MaxValue, maxy = int.MinValue;
      foreach (string k in worldKeys) { var bb = TerrainBBox(k); if (bb is (int x0, int x1, int y0, int y1)) { minx = Math.Min(minx, x0); maxx = Math.Max(maxx, x1); miny = Math.Min(miny, y0); maxy = Math.Max(maxy, y1); } }
      Console.WriteLine($"--- world TERRAIN bbox x[{minx}..{maxx}] y[{miny}..{maxy}]; islands whose terrain overlaps (margin 3) ---");
      const int margin = 3;
      foreach (string k in mg.islandnames.OrderBy(k => mg.offsets[k].y).ThenBy(k => mg.offsets[k].x)) {
        var bb = TerrainBBox(k);
        if (bb is not (int x0, int x1, int y0, int y1)) continue;
        if (x1 < minx - margin || x0 > maxx + margin || y1 < miny - margin || y0 > maxy + margin) continue; // no bbox overlap
        Coord o = mg.offsets[k];
        string tag = (k == shrine ? "[SHRINE]" : mg.IsShrine(k) ? "[shrine]" : "") + (worldKeys.Contains(k) ? "[world]" : "") + (k == "start" ? "[START]" : "");
        Console.WriteLine($"  {k,-18} offset={o} terrain=x[{x0}..{x1}] y[{y0}..{y1}] {tag} \"{mg.islands[k].displayname}\"");
      }
      return 0;
    }

    // THROWAWAY: like --world-tolevelh but also merges in |extraKeys| islands (e.g. start,controls) and spawns the
    // player at the real overworld "start" landing, so the connective land between the world's pieces is visible.
    if (args.Length >= 2 && args[0] == "--world-render") {
      using var reader = new BinaryReader(File.OpenRead("Extracted/merged_binary.bin"));
      MetaGameState mg = new();
      mg.LoadBinary(reader);
      var extras = args.Length >= 3 ? args[2].Split(',', StringSplitOptions.RemoveEmptyEntries).ToList() : new List<string>();
      Console.WriteLine(EmitWorldLevel(mg, args[1], "1-final Overworld sausage", extras, preferStartPlayer: true));
      return 0;
    }

    // NEW SCHEMA: shrines as clearable lettered sausage-walls + D as 'zz' + a levelEntrances tail. See EmitWorldSchema.
    if (args.Length >= 2 && args[0] == "--world-schema") {
      using var reader = new BinaryReader(File.OpenRead("Extracted/merged_binary.bin"));
      MetaGameState mg = new();
      mg.LoadBinary(reader);
      var extras = args.Length >= 3 ? args[2].Split(',', StringSplitOptions.RemoveEmptyEntries).ToList() : new List<string>();
      Console.WriteLine(EmitWorldSchema(mg, args[1], "1-final Overworld sausage", extras, preferStartPlayer: true));
      return 0;
    }

    // THROWAWAY AUDIT: for every world, compare each shrine's sausage resting z (its overworld terrain height) to that
    // level's entrance z. The proposed pit fix restores a cleared shrine to (entranceZ) height; this checks that the
    // sausage actually sits at that height. sausages + entrance are offset by the SAME mg.offsets[k], so z is comparable.
    if (args.Length >= 1 && args[0] == "--audit-entrance-z") {
      using var reader = new BinaryReader(File.OpenRead("Extracted/merged_binary.bin"));
      MetaGameState mg = new();
      mg.LoadBinary(reader);
      var worlds = new[] { (1, "temple2j1"), (2, "temple1d1"), (3, "temple1x1"), (4, "temple1h1"), (5, "temple2c1") };
      int shrinesChecked = 0, mismatches = 0;
      foreach (var (w, shrine) in worlds) {
        var prereqs = mg.templedat.TryGetValue(shrine, out var pr) ? pr : new List<string>();
        foreach (string k in prereqs) {
          if (!mg.sausagepositions.ContainsKey(k)) continue;
          string disp = mg.islands.ContainsKey(k) ? mg.islands[k].displayname : k;
          int? entZ = mg.playerpositions.ContainsKey(k) ? (mg.playerpositions[k].Key + mg.offsets[k]).z : (int?)null;
          var zs = mg.sausagepositions[k].Select(kv => (kv.Key + mg.offsets[k]).z).ToList();
          int baseZ = zs.Min(), topZ = zs.Max();
          bool tower = baseZ != topZ;
          bool ok = entZ.HasValue && baseZ == entZ.Value;
          shrinesChecked++;
          if (!ok) mismatches++;
          string zrange = tower ? $"[{baseZ}..{topZ}]" : $"{baseZ}";
          Console.WriteLine($"W{w} {k,-14} sausBaseZ={baseZ} ({zrange}) entZ={(entZ?.ToString() ?? "none"),-4} {(ok ? "ok" : "MISMATCH")}{(tower ? "  (tower)" : "")}  \"{disp}\"");
        }
      }
      Console.WriteLine($"--- {mismatches}/{shrinesChecked} shrines have sausage-base-z != entrance-z ---");
      return 0;
    }

    // THROWAWAY AUDIT: per shrine, collapse sausages to 2D footprints (== emitted letters) and report each footprint's
    // base terrain z. A shrine with >1 distinct base z among its letters can't be restored with a single per-shrine
    // height -- the restore terrain must ride on each letter/sausage.
    if (args.Length >= 1 && args[0] == "--audit-shrine-heights") {
      using var reader = new BinaryReader(File.OpenRead("Extracted/merged_binary.bin"));
      MetaGameState mg = new();
      mg.LoadBinary(reader);
      var worlds = new[] { (1, "temple2j1"), (2, "temple1d1"), (3, "temple1x1"), (4, "temple1h1"), (5, "temple2c1") };
      int mixed = 0, checkedShrines = 0;
      foreach (var (w, shrine) in worlds) {
        var prereqs = mg.templedat.TryGetValue(shrine, out var pr) ? pr : new List<string>();
        foreach (string k in prereqs) {
          if (!mg.sausagepositions.ContainsKey(k)) continue;
          string disp = mg.islands.ContainsKey(k) ? mg.islands[k].displayname : k;
          Coord io = mg.offsets[k];
          var footBase = new Dictionary<(int, int, int, int), int>();
          foreach (var kv in mg.sausagepositions[k]) {
            Coord a = kv.Key + io, b = a + kv.Value;
            var fp = (a.x < b.x || (a.x == b.x && a.y <= b.y)) ? (a.x, a.y, b.x, b.y) : (b.x, b.y, a.x, a.y);
            if (!footBase.TryGetValue(fp, out int cur) || a.z < cur) footBase[fp] = a.z;
          }
          var heights = footBase.Values.Distinct().OrderBy(z => z).ToList();
          bool isMixed = heights.Count > 1;
          checkedShrines++;
          if (isMixed) mixed++;
          Console.WriteLine($"W{w} {k,-14} letters={footBase.Count,-2} padZ=[{string.Join(",", heights)}] {(isMixed ? "MIXED" : "")}  \"{disp}\"");
        }
      }
      Console.WriteLine($"--- {mixed}/{checkedShrines} shrines have letters at >1 distinct pad height ---");
      return 0;
    }

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

    // World 6 levels are composite: the island matching the display name is a "hub" whose entities are all
    // `island` references, and the real terrain lives in sibling islands keyed "<hubkey>__islandN". Detect the
    // hub and pool its children (each already authored in a shared frame, shifted by its offset delta) into one
    // entity list; every other world is a single island whose own entities ARE the level.
    string dat;
    var hub = metaGame.islands.FirstOrDefault(kv => kv.Value.displayname == displayName
                                                  && kv.Value.entities.Count > 0
                                                  && kv.Value.entities.All(e => e.type == EntType.island));
    if (hub.Key != null) {
      Coord hubOff = metaGame.offsets[hub.Key];
      var children = metaGame.islands.Where(kv => kv.Key.StartsWith(hub.Key + "__")).ToList();
      if (children.Count == 0) throw new Exception($"Composite hub '{hub.Key}' for '{displayName}' has no child islands");
      // Concatenating full child saves would drop all but the first island's terrain (Save appends '*'-delimited
      // metadata and LoadDat only reads the entity run before the first '*'), so splice just the entity segment
      // from each child and give the children disjoint id ranges.
      var sb = new System.Text.StringBuilder();
      int idBase = 0;
      foreach (var kv in children) {
        Coord delta = (metaGame.offsets.ContainsKey(kv.Key) ? metaGame.offsets[kv.Key] : hubOff) - hubOff;
        GameState child = GameState.Load(kv.Value.Save(false, false), null, false);
        foreach (Entity e in child.entities) { e.pos += delta; e.id += idBase; }
        idBase = child.entities.Count > 0 ? child.entities.Max(e => e.id) + 1 : idBase;
        string save = child.Save(false, false);
        int star = save.IndexOf('*');
        sb.Append(star < 0 ? save : save[..star]);
      }
      dat = sb.ToString() + "*";
    } else {
      GameState island = metaGame.islands.Values.FirstOrDefault(g => g.displayname == displayName);
      if (island == null) throw new Exception($"No island with display name '{displayName}' in merged_binary");
      dat = island.Save(false, false);
    }

    // Fresh copy so the cached island isn't mutated; strip decoration/markers off the working copy.
    GameState work = GameState.Load(dat, null, false);
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

  // THROWAWAY: build one world's overworld island (temple shrine + prereq level islands, merged at offsets; per-level
  // sausages dropped, world sausage kept) and emit its C++ Level(...) def with a proper OverworldSausageN var name.
  static string EmitWorldLevel(MetaGameState mg, string shrine, string name, List<string> extraKeys = null, bool preferStartPlayer = false) {
    var worldKeys = new List<string> { shrine };
    if (mg.templedat.TryGetValue(shrine, out var prereqs)) worldKeys.AddRange(prereqs);
    if (extraKeys != null) worldKeys.AddRange(extraKeys);
    GameState world = GameState.Load("*", null, false);
    int id = 0;
    Entity player = null, startPlayer = null;
    foreach (string key in worldKeys) {
      if (!mg.offsets.TryGetValue(key, out Coord io) || !mg.islands.ContainsKey(key)) continue;
      GameState isl = GameState.Load(mg.islands[key].Save(false, false), null, false);
      foreach (Entity e in isl.entities) {
        if (e.Decoration() || e.type == EntType.island || e.type == EntType.spectralsausage || e.type == EntType.sausage) continue;
        if (e.type == EntType.player) {
          if (key == "start") { e.pos += io; startPlayer = e; }
          else if (key == shrine && player == null) { e.pos += io; player = e; }
          continue;
        }
        e.pos += io; e.id = id++;
        world.entities.Add(e);
        if (e.type.Dynamic()) world.dynamicentities.Add(e);
      }
    }
    if (preferStartPlayer && startPlayer != null) player = startPlayer;
    foreach (var p in mg.sausagepositions[shrine]) {
      Entity s = new Entity(world) { type = EntType.sausage, pos = p.Key + mg.offsets[shrine], direction = p.Value, rot = 0, cookdata = 0, id = id++ };
      world.entities.Add(s); world.dynamicentities.Add(s);
    }
    if (player == null) player = new Entity(world) { type = EntType.player, direction = Direction.North, pos = mg.sausagepositions[shrine][0].Key + mg.offsets[shrine] };
    player.id = id++;
    world.entities.Add(player); world.dynamicentities.Add(player);
    world.player = player;
    int minx = world.entities.Min(e => e.pos.x), miny = world.entities.Min(e => e.pos.y), minz = world.entities.Min(e => e.pos.z);
    string translated = GameState.Translate(world.Save(false, false), new Coord(-minx, -miny, -1 - minz));
    string def = ConvertToLevelH(name, GameState.Load(translated, null, true), inlineLadders: false);
    return def.Replace("Level Overworldsausage(", $"Level OverworldSausage{name.Split('-')[0]}(");
  }

  // ===== NEW-SCHEMA overworld export. Unlike EmitWorldLevel (which bakes shrines as terrain and keeps only D),
  // this emits every prereq sub-level's sausages as CLEARABLE lettered sausage-walls (uppercase A.., then a..y in
  // grid reading order, so the C++ overworld parser's num==Size() invariant holds), the world sausage D as 'zz'
  // (the engine's world-sausage marker), and a trailing levelEntrances list pairing each shrine's entrance pose
  // with the string of letters it clears (strchr-matched, so one entrance can own several sausages). =====
  static string EmitWorldSchema(MetaGameState mg, string shrine, string name, List<string> extraKeys = null, bool preferStartPlayer = false) {
    var prereqs = mg.templedat.TryGetValue(shrine, out var pr) ? pr : new List<string>();
    var worldKeys = new List<string> { shrine };
    worldKeys.AddRange(prereqs);
    if (extraKeys != null) worldKeys.AddRange(extraKeys);
    var warnings = new List<string>();

    // --- Merge terrain (drop all sausages here; we re-add them as lettered walls) and locate the spawn ---
    GameState world = GameState.Load("*", null, false);
    int id = 0;
    Entity player = null, startPlayer = null;
    foreach (string key in worldKeys) {
      if (!mg.offsets.TryGetValue(key, out Coord io) || !mg.islands.ContainsKey(key)) continue;
      GameState isl = GameState.Load(mg.islands[key].Save(false, false), null, false);
      foreach (Entity e in isl.entities) {
        if (e.Decoration() || e.type == EntType.island || e.type == EntType.spectralsausage || e.type == EntType.sausage) continue;
        if (e.type == EntType.player) {
          if (key == "start") { e.pos += io; startPlayer = e; }
          else if (key == shrine && player == null) { e.pos += io; player = e; }
          continue;
        }
        e.pos += io; e.id = id++;
        world.entities.Add(e);
        if (e.type.Dynamic()) world.dynamicentities.Add(e);
      }
    }
    if (preferStartPlayer && startPlayer != null) player = startPlayer;
    if (player == null) player = new Entity(world) { type = EntType.player, direction = Direction.North, pos = mg.sausagepositions[shrine][0].Key + mg.offsets[shrine] };

    // --- Gather shrine sausages (per prereq), the world sausage D, and entrances, in ABS coords ---
    var saus = new List<(Coord a, Coord b, string owner)>();
    foreach (string k in prereqs) {
      if (!mg.sausagepositions.ContainsKey(k)) continue;
      Coord io = mg.offsets[k];
      foreach (var kv in mg.sausagepositions[k]) { Coord a = kv.Key + io; saus.Add((a, a + kv.Value, k)); }
    }
    // Collapse stacked sausage towers: in the overworld a shrine is only a wall, so keep one letter per unique 2D
    // footprint -- several sausages sharing the same (x,y) cells (e.g. Cold Gate's 7-high tower) would otherwise collide.
    {
      var seenFoot = new HashSet<(int, int, int, int)>();
      saus = saus.Where(s => {
        var key = (s.a.x < s.b.x || (s.a.x == s.b.x && s.a.y <= s.b.y)) ? (s.a.x, s.a.y, s.b.x, s.b.y) : (s.b.x, s.b.y, s.a.x, s.a.y);
        return seenFoot.Add(key);
      }).ToList();
    }
    Coord dOff = mg.offsets[shrine];
    var dkv = mg.sausagepositions[shrine][0];
    Coord dA = dkv.Key + dOff, dB = dA + dkv.Value;
    var entrances = new List<(string owner, Coord pos, Direction dir)>();
    foreach (string k in prereqs)
      if (mg.playerpositions.ContainsKey(k))
        entrances.Add((k, mg.playerpositions[k].Key + mg.offsets[k], mg.playerpositions[k].Value));

    // --- Translation (match EmitWorldLevel: floor -> z=-1, spawn -> z=0) ---
    var everyCoord = new List<Coord>();
    foreach (var e in world.entities) everyCoord.Add(e.pos);
    everyCoord.Add(player.pos);
    foreach (var s in saus) { everyCoord.Add(s.a); everyCoord.Add(s.b); }
    everyCoord.Add(dA); everyCoord.Add(dB);
    foreach (var en in entrances) everyCoord.Add(en.pos);
    int minx = everyCoord.Min(c => c.x), miny = everyCoord.Min(c => c.y), minz = everyCoord.Min(c => c.z);
    int sx = -minx, sy = -miny, sz = -1 - minz;
    Coord T(Coord c) => new Coord(c.x + sx, c.y + sy, c.z + sz);

    // --- Grid dimensions cover terrain + every sausage/D cell ---
    var terrain = world.entities.Where(e => e.type == EntType.ground || e.type == EntType.barrier
                                         || e.type == EntType.bbq || e.type == EntType.ladder).ToList();
    int W = 0, H = 0;
    void Extend(Coord c) { Coord t = T(c); W = Math.Max(W, t.x + 1); H = Math.Max(H, t.y + 1); }
    foreach (Entity e in terrain) Extend(e.pos);
    foreach (var s in saus) { Extend(s.a); Extend(s.b); }
    Extend(dA); Extend(dB);

    // --- Terrain char maps (same rules as ConvertToLevelH) ---
    var solid = new HashSet<int>[H, W]; var grill = new HashSet<int>[H, W];
    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) { solid[y, x] = new(); grill[y, x] = new(); }
    var ladders = new List<(int x, int y, int z, Direction dir)>();
    foreach (Entity e in terrain) {
      Coord t = T(e.pos); int x = t.x, y = t.y, b = t.z + 1;
      if (x < 0 || y < 0 || x >= W || y >= H) continue;
      if (e.type == EntType.ground || e.type == EntType.barrier) solid[y, x].Add(b);
      else if (e.type == EntType.bbq) { solid[y, x].Add(b); grill[y, x].Add(b); }
      else if (e.type == EntType.ladder) { solid[y, x].Add(b); ladders.Add((x, y, t.z, e.direction)); }
    }

    // --- Overlay letters: D -> 'z', prereq sausages -> A..Z,a..y in grid reading order ---
    var overlay = new char[H, W];
    var ownerLetters = new Dictionary<string, List<char>>();
    var letterPad = new Dictionary<char, int>(); // pad terrain height (== emitted digit) under each shrine letter
    void Put(Coord c, char ch) {
      Coord t = T(c);
      if (t.x < 0 || t.y < 0 || t.x >= W || t.y >= H) { warnings.Add($"overlay cell ({t.x},{t.y}) for '{ch}' is off-grid"); return; }
      if (overlay[t.y, t.x] != '\0') warnings.Add($"overlay collision at ({t.x},{t.y}): '{overlay[t.y, t.x]}' vs '{ch}'");
      overlay[t.y, t.x] = ch;
    }
    int PadUnder(Coord cell) {
      Coord t = T(cell);
      if (t.x < 0 || t.y < 0 || t.x >= W || t.y >= H || solid[t.y, t.x].Count == 0) return 0;
      int top = solid[t.y, t.x].Max();
      if (!Enumerable.Range(0, top + 1).All(solid[t.y, t.x].Contains))
        warnings.Add($"sausage-wall pad at ({t.x},{t.y}) is non-contiguous; height {top} may need a SpecialTile");
      return Math.Clamp(top, 0, 8);
    }
    Put(dA, 'z'); Put(dB, 'z');
    // reading order by translated min-cell (row-major); guarantees the C++ num==Size() invariant on first encounter
    Coord MinCell(Coord a, Coord b) { Coord ta = T(a), tb = T(b); return (ta.y < tb.y || (ta.y == tb.y && ta.x <= tb.x)) ? a : b; }
    saus.Sort((s1, s2) => { Coord m1 = T(MinCell(s1.a, s1.b)), m2 = T(MinCell(s2.a, s2.b)); return m1.y != m2.y ? m1.y - m2.y : m1.x - m2.x; });
    if (saus.Count > 51) warnings.Add($"{saus.Count} shrine sausages exceed the 51 letter slots (A..Z,a..y)");
    for (int i = 0; i < saus.Count; i++) {
      char L = i < 26 ? (char)('A' + i) : (char)('a' + (i - 26));
      Put(saus[i].a, L); Put(saus[i].b, L);
      int pad = PadUnder(saus[i].a);
      if (PadUnder(saus[i].b) != pad) warnings.Add($"letter '{L}' spans two pad heights; both cells forced to {pad}");
      letterPad[L] = pad;
      if (!ownerLetters.TryGetValue(saus[i].owner, out var list)) ownerLetters[saus[i].owner] = list = new();
      list.Add(L);
    }

    // --- Render grid chars (overlay wins; otherwise ConvertToLevelH terrain rules) ---
    var specials = new List<string>();
    var grillCells = new List<string>(); // grills are disabled in the overworld: still detect them, but emit ground
    var grid = new char[H, W];
    for (int y = 0; y < H; y++) {
      for (int x = 0; x < W; x++) {
        if (overlay[y, x] != '\0') { grid[y, x] = overlay[y, x]; continue; }
        var S = solid[y, x]; var G = grill[y, x];
        if (S.Count == 0) { grid[y, x] = ' '; continue; }
        int top = S.Max();
        if (G.Count > 0) grillCells.Add($"({x},{y})"); // identified, but dropped from the emitted terrain
        bool contigFromMin = Enumerable.Range(S.Min(), top - S.Min() + 1).All(S.Contains);
        if (contigFromMin) {
          grid[y, x] = "_12345678"[Math.Clamp(top, 0, 8)];
        } else {
          grid[y, x] = '?';
          var wallBits = Enumerable.Range(0, S.Min()).Concat(S).OrderBy(v => v).ToList();
          specials.Add($"SpecialTile({{{string.Join(", ", wallBits)}}})");
        }
      }
    }
    if (grillCells.Count > 0) warnings.Add($"{grillCells.Count} grill cell(s) emitted as ground (grills disabled in overworld): {string.Join(",", grillCells)}");

    // --- Ladders -> explicit list (overworld mode: U/D/L/R aren't grid chars) ---
    var kept = ladders
      .Select(l => { var st = Step(l.dir); return (x: l.x + st.dx, y: l.y + st.dy, z: l.z, dir: CppDir(Opposite(l.dir))); })
      .Where(l => l.x >= 0 && l.y >= 0 && l.x < W && l.y < H && l.z >= 0)
      .Distinct().OrderBy(l => l.y).ThenBy(l => l.x).ThenBy(l => l.z).ToList();
    foreach (var l in kept.Where(l => overlay[l.y, l.x] != '\0'))
      warnings.Add($"ladder anchor ({l.x},{l.y},{l.z}) {l.dir} sits on a sausage-wall cell");

    // --- Stephen (inline arrow when he lands at z=0 on plain ground) ---
    Coord P = T(player.pos);
    char StephenChar(Direction d) => d switch { Direction.North => '^', Direction.South => 'v', Direction.West => '<', Direction.East => '>', _ => '?' };
    bool stephenInline = P.z == 0 && P.x >= 0 && P.y >= 0 && P.x < W && P.y < H
                      && overlay[P.y, P.x] == '\0' && grid[P.y, P.x] == '_';
    if (stephenInline) grid[P.y, P.x] = StephenChar(player.direction);

    // --- levelEntrances: one entry per prereq (entrance pose + its letters), sorted by letter for readability ---
    var entries = new List<(string letters, string name, string init)>();
    foreach (string k in prereqs) {
      var en = entrances.FirstOrDefault(e => e.owner == k);
      if (en.owner == null) { if (ownerLetters.ContainsKey(k)) warnings.Add($"prereq {k} has sausages but no entrance"); continue; }
      if (!ownerLetters.TryGetValue(k, out var letters)) { warnings.Add($"prereq {k} has an entrance but no sausages"); continue; }
      Coord E = T(en.pos);
      string letterStr = new string(letters.OrderBy(c => c).ToArray());
      string disp = mg.islands.ContainsKey(k) ? mg.islands[k].displayname : k;
      var pads = letterStr.Select(c => letterPad.TryGetValue(c, out int h) ? h : 0).ToList();
      string padArg = pads.All(h => h == 0) ? "" : $", {{{string.Join(", ", pads)}}}";
      entries.Add((letterStr, disp, $"{{ Stephen{{{E.x}, {E.y}, {E.z}, {CppDir(en.dir)}}}, \"{letterStr}\", nullptr{padArg} }}"));
    }
    entries.Sort((a, b) => string.CompareOrdinal(a.letters, b.letters));

    // --- Assemble ---
    specials.Reverse(); // grid parser pops specialTiles from the back per '?'
    var rows = new List<string>();
    for (int y = 0; y < H; y++) {
      var sb = new System.Text.StringBuilder();
      for (int x = 0; x < W; x++) sb.Append(grid[y, x]);
      rows.Add(sb.ToString());
    }
    string laddersArg = kept.Count == 0 ? "{}" : "{" + string.Join(", ", kept.Select(l => $"Ladder{{{l.x}, {l.y}, {l.z}, {l.dir}}}")) + "}";
    string specialsArg = specials.Count == 0 ? "{}" : "{" + string.Join(", ", specials) + "}";
    string stephenArg = stephenInline ? "{}" : $"Stephen{{{P.x}, {P.y}, {P.z}, {CppDir(player.direction)}}}";
    string entrancesArg = "{\n" + string.Join("\n", entries.Select((e, i) => $"    {e.init}{(i < entries.Count - 1 ? "," : "")} // {e.name}")) + "\n  }";

    var o = new System.Text.StringBuilder();
    if (warnings.Count > 0) o.AppendLine($"// WARNING: {string.Join("; ", warnings)}");
    o.AppendLine($"Level OverworldSausage{name.Split('-')[0]}({W}, {H}, \"{name}\",");
    for (int y = 0; y < H; y++) o.AppendLine($"  \"{rows[y]}\",");
    o.AppendLine($"  {stephenArg},");
    o.AppendLine($"  {laddersArg},");
    o.AppendLine("  {}, // sausages: all shrine-walls + D are inline grid letters");
    o.AppendLine($"  {specialsArg},");
    o.Append($"  {entrancesArg});");
    return o.ToString();
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

    // The RRT writer may embed this engine's per-move timing ("Units: u0 u1 ..."); when present, validate our own
    // tick-simulated units against it move-by-move. A |delta| <= 1 gap is the intentional sub-beat backpedal tiebreak,
    // not a model error, so only a larger gap counts. The final-state line is the line right after "Stop".
    int stopIdx = Array.IndexOf(lines, "Stop");
    long[] expectedUnits = lines.FirstOrDefault(l => l.StartsWith("Units:")) is string u
      ? u.Substring("Units:".Length).Split(' ', StringSplitOptions.RemoveEmptyEntries).Select(long.Parse).ToArray()
      : null;
    int timingMismatch = -1; // 1-based index of the first diverging move, -1 = none

    int moveNo = 0;
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

      int prevPlayerZ = debug ? gs.player.pos.z : 0;
      Direction prevPlayerDir = debug ? gs.player.direction : Direction.None;
      bool prevForkHeld = debug && gs.fork == null;
      int prevForkZ = debug && gs.fork != null ? gs.fork.pos.z : prevPlayerZ;
      var prevSaus = debug
        ? gs.dynamicentities.Where(e => e.type == EntType.sausage).ToDictionary(e => e.id, e => (z: e.pos.z, rot: e.rot, cook: e.cookdata))
        : null;
      gs.ProcessInput(dir);
      long moveUnits = Game.ResolveMove(gs);
      totalUnits += moveUnits;

      if (expectedUnits != null && moveNo < expectedUnits.Length && timingMismatch < 0
          && Math.Abs(moveUnits - expectedUnits[moveNo]) > 1)
        timingMismatch = moveNo + 1;

      if (debug) {
        string success = (gs.Lost().Length == 0) ? "SUCCEEDED" : "FAILED";
        Console.WriteLine();
        Console.WriteLine($"=== move {i + 1}: {line} {success} ===");
        Console.WriteLine($"{ToString(gs)} {moveUnits} {totalUnits}");
        int pDrop = prevPlayerZ - gs.player.pos.z, sDrop = 0, rollCount = 0, cookGained = 0;
        foreach (Entity e in gs.dynamicentities) {
          if (e.type != EntType.sausage || !prevSaus.TryGetValue(e.id, out var pv)) continue;
          if (pv.z - e.pos.z > sDrop) sDrop = pv.z - e.pos.z;
          if (pv.rot != e.rot) rollCount++;
          if (e.cookdata != pv.cook) cookGained++;
        }
        int forkDetach = (prevForkHeld && gs.fork != null) ? 1 : 0;
        int forkDrop = gs.fork != null ? prevForkZ - gs.fork.pos.z : 0;
        int turned = gs.player.direction != prevPlayerDir ? 1 : 0;
        Console.WriteLine($"DIAG playerDrop={pDrop} maxSausDrop={sDrop} rolls={rollCount} forkDetach={forkDetach} forkDrop={forkDrop} cookGained={cookGained} turned={turned}");
      }

      if (gs.Won()) return timingMismatch >= 0 ? "Timing" : ""; // Real demos have trailing moves to get to the next level, check early
      string reason = gs.Lost();
      if (reason.Length > 0) return reason;
      moveNo++;
    }

    string expectedFinalState = stopIdx >= 0 && stopIdx + 1 < lines.Length ? lines[stopIdx + 1] : lines[^1];
    string actualFinalState = ToString(gs);
    if (expectedFinalState != actualFinalState) return "Final state";
    return timingMismatch >= 0 ? "Timing" : "";
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

  // ===== THROWAWAY: game GameState -> C++ Level(...) def (see Levels.h). Delete after generating worlds 5 & 6. =====
  static string CppDir(Direction d) => d switch {
    Direction.North => "Up", Direction.South => "Down", Direction.West => "Left", Direction.East => "Right", _ => "None"
  };
  static (int dx, int dy) Step(Direction d) => d switch {
    Direction.North => (0, -1), Direction.South => (0, 1), Direction.East => (1, 0), Direction.West => (-1, 0), _ => (0, 0)
  };
  static Direction Opposite(Direction d) => d switch {
    Direction.North => Direction.South, Direction.South => Direction.North,
    Direction.East => Direction.West, Direction.West => Direction.East, _ => d
  };

  static string ConvertToLevelH(string name, GameState gs, bool inlineLadders = true) {
    var terrain = gs.entities.Where(e => e.type == EntType.ground || e.type == EntType.barrier
                                      || e.type == EntType.bbq    || e.type == EntType.ladder).ToList();
    int W = terrain.Max(e => e.pos.x) + 1;
    int H = terrain.Max(e => e.pos.y) + 1;

    var solid = new HashSet<int>[H, W];   // C++ wall-bit indices present (bit = game_z + 1)
    var grill = new HashSet<int>[H, W];   // grill bits (from bbq)
    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) { solid[y, x] = new(); grill[y, x] = new(); }

    var ladders = new List<(int x, int y, int z, Direction dir)>(); // raw game (x,y,game_z,dir); transformed at emit
    foreach (Entity e in terrain) {
      int x = e.pos.x, y = e.pos.y, b = e.pos.z + 1;
      if (x < 0 || y < 0 || x >= W || y >= H) continue;
      if (e.type == EntType.ground || e.type == EntType.barrier) solid[y, x].Add(b);
      else if (e.type == EntType.bbq) { solid[y, x].Add(b); grill[y, x].Add(b); }
      else if (e.type == EntType.ladder) { solid[y, x].Add(b); ladders.Add((x, y, e.pos.z, e.direction)); }
    }

    var warnings = new List<string>();
    var specials = new List<string>(); // SpecialTile tokens, in grid order (matches '?' consumption... see note)
    var grid = new char[H, W];
    var effWalls = new HashSet<int>[H, W]; // wall bit indices the emitted char actually produces (for IsWall sim)
    var cellGrill = new bool[H, W];
    for (int y = 0; y < H; y++) {
      for (int x = 0; x < W; x++) {
        var S = solid[y, x]; var G = grill[y, x];
        char c; HashSet<int> eff;
        if (S.Count == 0) { c = ' '; eff = new(); }
        else {
          int top = S.Max();
          bool grillTop = G.Contains(top) && top <= 2; // grill occupies the standable top surface
          // A plain char is exact only for a run contiguous from the lowest block (implicitly-solid interior) whose
          // grill, if any, sits on a standable (<=2) top; anything else -> an explicit z-level SpecialTile.
          bool contigFromMin = Enumerable.Range(S.Min(), top - S.Min() + 1).All(S.Contains);
          bool grillOk = G.Count == 0 || (grillTop && G.Count == 1);
          if (contigFromMin && grillOk) {
            c = grillTop ? "#$%"[top] : "_12345678"[Math.Clamp(top, 0, 8)];
            eff = new(Enumerable.Range(0, Math.Clamp(top, 0, 8) + 1));
          } else {
            c = '?';
            var wallBits = Enumerable.Range(0, S.Min()).Concat(S).OrderBy(v => v).ToList(); // implied-solid below the lowest block
            eff = new(wallBits);
            specials.Add(G.Count == 0
              ? $"SpecialTile({{{string.Join(", ", wallBits)}}})"
              : $"SpecialTile({{{string.Join(", ", wallBits)}}}, {{{string.Join(", ", G.OrderBy(v => v))}}})");
          }
        }
        grid[y, x] = c; effWalls[y, x] = eff; cellGrill[y, x] = G.Count > 0;
      }
    }

    // Stephen
    Entity p = gs.player;
    string stephen = $"Stephen{{{p.pos.x}, {p.pos.y}, {p.pos.z}, {CppDir(p.direction)}}}";
    bool forkPlaced = gs.fork != null;

    // Sausages (reuse the validated ToString decode, but emit C++ initializers)
    bool Cooked(int q) => q == 1 || q == 2;
    var saus = new List<(Coord a, Coord b, string flags, int z)>();
    foreach (Entity e in gs.dynamicentities) {
      if (e.type != EntType.sausage) continue;
      int cd = e.cookdata;
      var pos = (cell: e.pos, r0: Cooked((cd >> 6) & 3), r1: Cooked((cd >> 4) & 3));
      var coord = (cell: e.pos + e.direction, r0: Cooked(cd & 3), r1: Cooked((cd >> 2) & 3));
      var (lo, hi) = Compare(pos.cell, coord.cell) <= 0 ? (pos, coord) : (coord, pos);
      var fl = new List<string>();
      if (lo.r0) fl.Add("Sausage::Cook1A");
      if (lo.r1) fl.Add("Sausage::Cook1B");
      if (hi.r0) fl.Add("Sausage::Cook2A");
      if (hi.r1) fl.Add("Sausage::Cook2B");
      if (e.rot != 0) fl.Add("Sausage::Rolled");
      saus.Add((lo.cell, hi.cell, fl.Count == 0 ? "Sausage::None" : string.Join(" | ", fl), lo.cell.z));
    }
    // Row-major by the upper-left half, so inline letters (a,b,c…) land in the C++ grid's parse order (top-to-bottom, left-to-right).
    saus.Sort((s1, s2) => s1.a.y != s2.a.y ? s1.a.y - s2.a.y : (s1.a.x != s2.a.x ? s1.a.x - s2.a.x : s1.a.z - s2.a.z));

    // Transform each game ladder to its C++ anchor cell + opposite facing. The C++ _ladders array is indexed by
    // (x,y,dir), so a cell CAN carry ladders on several faces: the inline pass below promotes at most one face per
    // cell to a U/D/L/R grid char and leaves any other faces in the explicit Ladder{...} list.
    var xl = ladders.Select(l => { var st = Step(l.dir); return (x: l.x + st.dx, y: l.y + st.dy, z: l.z, dir: CppDir(Opposite(l.dir))); }).ToList();
    // A ladder whose anchor cell is off the grid (a boundary face) or below the floor can't be indexed by the C++
    // _ladders array -- in the frozen island snapshot it backs onto void anyway, so drop it and flag for review.
    foreach (var l in xl.Where(l => l.x < 0 || l.y < 0 || l.x >= W || l.y >= H || l.z < 0))
      warnings.Add($"dropped off-grid ladder anchor ({l.x},{l.y},{l.z}) {l.dir}");
    var kept = xl.Where(l => l.x >= 0 && l.y >= 0 && l.x < W && l.y < H && l.z >= 0)
                 .Select(l => (l.x, l.y, l.z, l.dir)).Distinct()
                 .OrderBy(l => l.y).ThenBy(l => l.x).ThenBy(l => l.z).ToList();

    // Inline ladders into the grid where the hand-authored style allows it: a ladder can become a U/D/L/R char iff its
    // anchor cell is a plain '_', its rungs start at z=0, and the parser's auto-extension (climb while a wall backs each
    // level) reproduces exactly its z-set. Anything else stays in the {Ladder{...}} list.
    bool IsWallEff(int x, int y, int z) => x >= 0 && y >= 0 && x < W && y < H && z >= 0 && effWalls[y, x].Contains(z + 1);
    (int dx, int dy) DirStep(string d) => d switch { "Up" => (0, -1), "Down" => (0, 1), "Left" => (-1, 0), "Right" => (1, 0), _ => (0, 0) };
    char DirChar(string d) => d switch { "Up" => 'U', "Down" => 'D', "Left" => 'L', "Right" => 'R', _ => '?' };
    var stillList = new List<(int x, int y, int z, string dir)>();
    foreach (var g in kept.GroupBy(l => (l.x, l.y, l.dir))) {
      int ax = g.Key.x, ay = g.Key.y; string dir = g.Key.dir;
      var zset = g.Select(l => l.z).ToHashSet();
      bool inlinable = ax >= 0 && ay >= 0 && ax < W && ay < H && grid[ay, ax] == '_' && !cellGrill[ay, ax];
      if (inlinable) {
        var (sx, sy) = DirStep(dir);
        var produced = new HashSet<int>();
        for (int z = 0; z < 9; z++) { produced.Add(z); if (!IsWallEff(ax + sx, ay + sy, z + 1)) break; }
        inlinable = produced.SetEquals(zset);
      }
      if (inlineLadders && inlinable) grid[ay, ax] = DirChar(dir);
      else stillList.AddRange(g);
    }
    kept = stillList.OrderBy(l => l.y).ThenBy(l => l.x).ThenBy(l => l.z).ToList();

    // Inline sausages into the grid using the hand-authored letter convention: each sausage's two halves share a
    // letter = its 0-based index, lowercase over ground ('_'), uppercase over void (' '). The parser fixes inline
    // sausages at z=0, uncooked and unrolled, so only such a sausage whose BOTH halves sit on a plain '_'/' ' cell (not
    // a raised/grill/special/ladder tile) can be inlined; anything else stays in the {Sausage{...}} list. Halves are
    // adjacent and |saus| keeps the upper-left half in |a|, matching the parser's left-to-right/top-to-bottom pairing.
    bool InlineCell(Coord c) => c.x >= 0 && c.y >= 0 && c.x < W && c.y < H && (grid[c.y, c.x] == '_' || grid[c.y, c.x] == ' ');
    var sausList = new List<(Coord a, Coord b, string flags, int z)>();
    int inlined = 0;
    foreach (var s in saus) {
      bool adjacent = Math.Abs(s.a.x - s.b.x) + Math.Abs(s.a.y - s.b.y) == 1;
      if (s.z == 0 && s.flags == "Sausage::None" && adjacent && inlined < 26 && InlineCell(s.a) && InlineCell(s.b)) {
        foreach (Coord c in new[] { s.a, s.b })
          grid[c.y, c.x] = (char)((grid[c.y, c.x] == ' ' ? 'A' : 'a') + inlined); // uppercase over void, lowercase over ground
        inlined++;
      } else {
        sausList.Add(s);
      }
    }

    // Inline Stephen with a facing arrow (^ v < >) when he starts at z=0 on a plain ground cell -- the only start the
    // grid char can express (it fixes z=0 and sets the cell to '_'); otherwise keep the explicit Stephen{...}. (A
    // detached start fork can't be expressed either way; the fork warning above still flags it.)
    char StephenChar(Direction d) => d switch {
      Direction.North => '^', Direction.South => 'v', Direction.West => '<', Direction.East => '>', _ => '?'
    };
    bool stephenInline = p.pos.z == 0 && p.pos.x >= 0 && p.pos.y >= 0 && p.pos.x < W && p.pos.y < H
                      && grid[p.pos.y, p.pos.x] == '_';
    if (stephenInline) grid[p.pos.y, p.pos.x] = StephenChar(p.direction);

    var rows = new List<string>();
    for (int y = 0; y < H; y++) {
      var sb = new System.Text.StringBuilder();
      for (int x = 0; x < W; x++) sb.Append(grid[y, x]);
      rows.Add(sb.ToString());
    }

    // The grid parser pops specialTiles from the BACK per '?' in row-major order, so emit them reversed.
    specials.Reverse();

    // Positional args after the grid: an inline Stephen becomes a default "{}" placeholder (the grid arrow supplies
    // him); trailing "{}" are dropped so an all-inline level emits just the grid, matching the hand-authored style.
    var parts = new List<string>();
    parts.Add(stephenInline ? "{}" : stephen);
    parts.Add(kept.Count == 0 ? "{}" : "{" + string.Join(", ", kept.Select(l => $"Ladder{{{l.x}, {l.y}, {l.z}, {l.dir}}}")) + "}");
    parts.Add(sausList.Count == 0 ? "{}" : "{" + string.Join(", ", sausList.Select(s =>
      s.flags == "Sausage::None"
        ? $"Sausage{{{s.a.x}, {s.a.y}, {s.b.x}, {s.b.y}, {s.z}}}"
        : $"Sausage{{{s.a.x}, {s.a.y}, {s.b.x}, {s.b.y}, {s.z}, {s.flags}}}")) + "}");
    if (specials.Count > 0) parts.Add("{" + string.Join(", ", specials) + "}");
    while (parts.Count > 0 && parts[^1] == "{}") parts.RemoveAt(parts.Count - 1);

    // Assemble
    var o = new System.Text.StringBuilder();
    string disp = name.Substring(name.IndexOf(' ') + 1);
    string varName = new string(disp.Where(char.IsLetterOrDigit).ToArray()); // C++ ident: no numeric prefix
    if (warnings.Count > 0) o.AppendLine($"// WARNING: {string.Join("; ", warnings)}");
    if (forkPlaced) o.AppendLine($"// WARNING fork placed at start ({gs.fork.pos.x},{gs.fork.pos.y},{gs.fork.pos.z}) dir {gs.fork.direction} -- Stephen ctor can't express this");
    o.AppendLine($"Level {varName}({W}, {H}, \"{name}\",");
    for (int y = 0; y < H; y++) o.AppendLine($"  \"{rows[y]}\"{(y == H - 1 && parts.Count > 0 ? "," : "")}");
    if (parts.Count > 0) o.Append("  " + string.Join(",\n  ", parts));
    o.Append(");");
    return o.ToString();
  }
}


