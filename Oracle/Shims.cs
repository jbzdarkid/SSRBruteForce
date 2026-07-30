using System;
using System.Collections.Generic;

// Custom collection types the game uses for speed; here backed by System Dictionary.
// A `new` indexer returns default(TValue) on a missing key (matching the game's forgiving lookup), rather than throwing.
public class IntDictionary<TValue> : Dictionary<int, TValue> {
  public IntDictionary() { }
  public IntDictionary(int capacity) : base(capacity) { }
  public new TValue this[int key] {
    get { TValue v; return TryGetValue(key, out v) ? v : default(TValue); }
    set { base[key] = value; }
  }
}

public class StringDictionary<TValue> : Dictionary<string, TValue> {
  public StringDictionary() { }
  public StringDictionary(int capacity) : base(capacity) { }
  public new TValue this[string key] {
    get { TValue v; return key != null && TryGetValue(key, out v) ? v : default(TValue); }
    set { base[key] = value; }
  }
}

// Minimal MetaGameState: only the members GameState references. Puzzle levels are loaded with metagame == null,
// so these are never exercised at runtime; the type just needs to exist and expose the referenced surface.
public class MetaGameState {
  public GameState gamestate;
  public StringDictionary<GameState> islands = new StringDictionary<GameState>();
  public StringDictionary<IntDictionary<List<Coord>>> coastdat = new StringDictionary<IntDictionary<List<Coord>>>();
  public StringDictionary<IntDictionary<List<Coord>>> splashdat = new StringDictionary<IntDictionary<List<Coord>>>();
  public StringDictionary<IslandMask> islandmasks = new StringDictionary<IslandMask>();
  public StringDictionary<KeyValuePair<Coord, Direction>> playerpositions = new StringDictionary<KeyValuePair<Coord, Direction>>();
  public StringDictionary<List<KeyValuePair<Coord, Direction>>> sausagepositions = new StringDictionary<List<KeyValuePair<Coord, Direction>>>();
  public StringDictionary<List<string>> templedat = new StringDictionary<List<string>>();
  public StringDictionary<StringDictionary<bool[,]>> projectioncompatibilities = new StringDictionary<StringDictionary<bool[,]>>();
  public bool IsShrine(string shrinename) => false;
  public void RegenIslands() { }
  public static MetaGameState Blank() => new MetaGameState();
  public static MetaGameState Load(string dat, bool precalc = true) => new MetaGameState();
}

// Game-shell statics referenced on save/sfx/overworld paths (no-ops headlessly).
public static partial class Game {
  public static bool forktwang;
  public static string loadedLevelName = "";
}
public static class EnvironmentFader {
  public static int global_target_tileset = 0;
}
public static class LoaderSaver {
  public static string _loadedLevelName = "";
}
public static class SaveGame {
  public static void SaveToSlot(string dat, int sausagescooked, string lastpushed) { }
}
public static class Resources {
  public static object Load(string path) => null;
  public static object Load(string path, Type t) => null;
}
