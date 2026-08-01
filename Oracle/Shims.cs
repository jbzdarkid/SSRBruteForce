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
  public static void DeleteAll() { }
}
public static class Resources {
  public static object Load(string path) => null;
  public static object Load(string path, Type t) => null;
}
