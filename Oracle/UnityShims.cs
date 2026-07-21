using System;

namespace UnityEngine {
  public static class Mathf {
    public static int Max(int a, int b) => a > b ? a : b;
    public static int Min(int a, int b) => a < b ? a : b;
    public static float Max(float a, float b) => a > b ? a : b;
    public static float Min(float a, float b) => a < b ? a : b;
    public static int Abs(int a) => a < 0 ? -a : a;
    public static float Abs(float a) => a < 0 ? -a : a;
    public static int Clamp(int v, int lo, int hi) => v < lo ? lo : (v > hi ? hi : v);
    public static float Clamp(float v, float lo, float hi) => v < lo ? lo : (v > hi ? hi : v);
    public static int RoundToInt(float f) => (int)Math.Round(f);
    public static int FloorToInt(float f) => (int)Math.Floor(f);
    public static int CeilToInt(float f) => (int)Math.Ceiling(f);
    public static float Floor(float f) => (float)Math.Floor(f);
    public static float Ceil(float f) => (float)Math.Ceiling(f);
    public static float Round(float f) => (float)Math.Round(f);
    public static float Sqrt(float f) => (float)Math.Sqrt(f);
    public const float PI = 3.14159265f;
  }

  public static class Debug {
    public static void Log(object o) { }
    public static void LogError(object o) { Console.Error.WriteLine("[game.LogError] " + o); }
    public static void LogWarning(object o) { }
    public static void LogFormat(string f, params object[] a) { }
    public static void Assert(bool c) { }
    public static void Assert(bool c, object m) { }
  }

  public static class Application {
    public static bool isEditor = false;
    public static bool isPlaying = true;
  }

  public static class Random {
    private static System.Random _r = new System.Random(12345);
    public static int Range(int minInclusive, int maxExclusive) => _r.Next(minInclusive, maxExclusive);
    public static float Range(float min, float max) => (float)(_r.NextDouble() * (max - min) + min);
    public static float value => (float)_r.NextDouble();
  }

  public class MonoBehaviour { }
  public class ScriptableObject { }
  public class GameObject { public string name; }
  public class Transform { }
  public class Component { }

  public class TextAsset { public string text = ""; }

  public struct Vector2 {
    public float x, y;
    public Vector2(float x, float y) { this.x = x; this.y = y; }
  }
  public struct Vector3 {
    public float x, y, z;
    public Vector3(float x, float y, float z) { this.x = x; this.y = y; this.z = z; }
    public static Vector3 zero => new Vector3(0, 0, 0);
    public static Vector3 operator *(float s, Vector3 v) => new Vector3(s * v.x, s * v.y, s * v.z);
    public static Vector3 operator *(Vector3 v, float s) => new Vector3(s * v.x, s * v.y, s * v.z);
    public static Vector3 operator +(Vector3 a, Vector3 b) => new Vector3(a.x + b.x, a.y + b.y, a.z + b.z);
    public static Vector3 operator -(Vector3 a, Vector3 b) => new Vector3(a.x - b.x, a.y - b.y, a.z - b.z);
    public float magnitude => Mathf.Sqrt(x * x + y * y + z * z);
    public Vector3 normalized { get { float m = magnitude; return m == 0 ? this : new Vector3(x / m, y / m, z / m); } }
    public static float Distance(Vector3 a, Vector3 b) => (a - b).magnitude;
  }
  public struct Quaternion {
    public float x, y, z, w;
    public static Quaternion identity => new Quaternion();
    public static Quaternion Euler(float x, float y, float z) => new Quaternion();
    public static Quaternion AngleAxis(float a, Vector3 axis) => new Quaternion();
  }
  public struct Color {
    public float r, g, b, a;
    public Color(float r, float g, float b, float a = 1f) { this.r = r; this.g = g; this.b = b; this.a = a; }
  }

  [AttributeUsage(AttributeTargets.All)] public class HideInInspector : Attribute { }
  [AttributeUsage(AttributeTargets.All)] public class SerializeField : Attribute { }
  [AttributeUsage(AttributeTargets.All)] public class RangeAttribute : Attribute { public RangeAttribute(float a, float b) { } }
  [AttributeUsage(AttributeTargets.All)] public class HeaderAttribute : Attribute { public HeaderAttribute(string s) { } }
  [AttributeUsage(AttributeTargets.All)] public class TooltipAttribute : Attribute { public TooltipAttribute(string s) { } }
}
