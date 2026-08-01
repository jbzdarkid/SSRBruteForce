#pragma once

#include "CppUnitTest.h"
#include "Level2.h"
#include "State.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <sstream>
#include <string>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

// Win32 console hooks (declared here to avoid pulling all of <windows.h> into the test header).
// The vstest host already owns a console but keeps it hidden with our std handles redirected to pipes it captures;
// BuildTest reattaches to the real console device and unhides the window to drive it live.
extern "C" __declspec(dllimport) int   __stdcall AllocConsole(void);
extern "C" __declspec(dllimport) int   __stdcall FreeConsole(void);
extern "C" __declspec(dllimport) void* __stdcall GetConsoleWindow(void);
extern "C" __declspec(dllimport) int   __stdcall ShowWindow(void* hWnd, int nCmdShow);
extern "C" __declspec(dllimport) int   __stdcall SetForegroundWindow(void* hWnd);

class TestSymmetryHelper {
  std::function<Direction(Direction)> _sym;
  Level _level;
  int _width;  // logical (pre-transform) dims -- Transform always maps in this frame
  int _height;
  Sausage _expected{}; // the test's tracked sausage, in the logical frame (set by AssertSausage)

  std::tuple<s8, s8> Transform(s8 x, s8 y) {
    if (_sym(Left) == Left  && _sym(Up) == Up)    return { x, y };
    if (_sym(Left) == Up    && _sym(Up) == Right) return { _height - 1 - y, x };
    if (_sym(Left) == Right && _sym(Up) == Down)  return { _width - 1 - x, _height - 1 - y };
    if (_sym(Left) == Down  && _sym(Up) == Left)  return { y, _width - 1 - x };
    if (_sym(Left) == Right && _sym(Up) == Up)    return { _width - 1 - x, y };
    if (_sym(Left) == Left  && _sym(Up) == Down)  return { x, _height - 1 - y };
    if (_sym(Left) == Up    && _sym(Up) == Left)  return { y, x };
    if (_sym(Left) == Down  && _sym(Up) == Right) return { _height - 1 - y, _width - 1 - x };
    assert(false);
    return { x, y };
  }

  void InlineTransform(s8& x, s8& y) {
    auto [newX, newY] = Transform(x, y);
    x = newX;
    y = newY;
  };

  // Render a sausage's flag bits as a C++ expression (e.g. "Sausage::Cook1A | Sausage::Rolled") so the recorder
  // emits readable named flags instead of a raw int.
  static std::string FlagsToString(u8 flags) {
    if (flags == 0) return "Sausage::None";
    std::string out;
    auto add = [&](u8 bit, const char* name) {
      if (flags & bit) { if (!out.empty()) out += " | "; out += name; }
    };
    add(Sausage::Cook1A, "Sausage::Cook1A");
    add(Sausage::Cook1B, "Sausage::Cook1B");
    add(Sausage::Cook2A, "Sausage::Cook2A");
    add(Sausage::Cook2B, "Sausage::Cook2B");
    add(Sausage::Rolled, "Sausage::Rolled");
    return out;
  }

public:
  TestSymmetryHelper(const std::function<Direction(Direction)>& sym, Level&& level) : _sym(sym), _level(std::move(level)) {
    _width = _level._width;
    _height = _level._height;

    // If we're rotating by 90 degrees (+/-), swap height and width.
    bool swap = (sym(Up) == Left || sym(Up) == Right);
    int newWidth = swap ? _height : _width;
    int newHeight = swap ? _width : _height;

    // Transform the grid and its derived per-cell masks in one remap.
    NArray<u16> newWalls(newWidth, newHeight);
    NArray<u16> newGrills(newWidth, newHeight);
    NArray<u16> newLadders(newWidth, newHeight);
    newLadders.Fill(0); // NArray does not default zero.
    for (int x = 0; x < _width; x++) {
      for (int y = 0; y < _height; y++) {
        auto [newX, newY] = Transform(x, y);
        newWalls(newX, newY) = _level._walls(x, y);
        newGrills(newX, newY) = _level._grills(x, y);
        u16 ladder = _level._ladders(x, y);
        if (ladder) {
          newLadders(newX, newY) = (sym((Direction)(ladder >> 8)) << 8);
          newLadders(newX, newY) |= ladder & 0xFF;
        }
      }
    }
    _level._walls = std::move(newWalls);
    _level._grills = std::move(newGrills);
    _level._ladders = std::move(newLadders);
    _level._width = (u8)newWidth;
    _level._height = (u8)newHeight;

    // Transform stephen and the exit
    {
      InlineTransform(_level._stephen.x, _level._stephen.y);
      InlineTransform(_level._stephen.forkX, _level._stephen.forkY);
      InlineTransform(_level._start.x, _level._start.y);
      InlineTransform(_level._start.forkX, _level._start.forkY);

      _level._stephen.dir = sym(_level._stephen.dir);
      _level._stephen.forkDir = sym(_level._stephen.forkDir);
      _level._start.dir = sym(_level._start.dir);
      _level._start.forkDir = sym(_level._start.forkDir);
    }

    // Transform all the sausages
    for (Sausage& sausage : _level._sausages) {
      InlineTransform(sausage.x1, sausage.y1);
      InlineTransform(sausage.x2, sausage.y2);
      // Restore the 'first point is upper-left' invariant
      if (sausage.x1 > sausage.x2 || (sausage.x1 == sausage.x2 && sausage.y1 > sausage.y2)) {
        std::swap(sausage.x1, sausage.x2);
        std::swap(sausage.y1, sausage.y2);
      }
    }

    // Ladders were rotated above as part of _ladderMask (position via the remap, facing via sym), so there is no
    // separate ladder list to transform here.

    // Initialize the engine's move scratch from the fully-transformed starting pose, exactly as the solver and
    // DiffEngines do. Notably this sets the reference's _sausageSpeared, so a level that STARTS with the fork embedded
    // in a sausage is recognised as speared (otherwise the first move would misbehave).
    State s = _level.GetState();
    _level.SetState(s);
  }

  void AssertMoveSucceeds(Direction dir) {
    Assert::IsTrue(_level.Move(_sym(dir)));
  }
    
  void AssertMoveFails(Direction dir) {
    State before = _level.GetState();
    Assert::IsFalse(_level.Move(_sym(dir)));
    _level.SetState(before);
  }

  void AssertPosition(s8 x, s8 y, Direction dir) {
    InlineTransform(x, y);
    dir = _sym(dir);
    State s = _level.GetState();
    Assert::AreEqual(x, s.stephen.x);
    Assert::AreEqual(y, s.stephen.y);
    Assert::AreEqual(dir, s.stephen.dir);

    s8 forkX = x;
    s8 forkY = y;
    if (dir == Up) forkY--;
    else if (dir == Down) forkY++;
    else if (dir == Left) forkX--;
    else if (dir == Right) forkX++;
    Assert::AreEqual(forkX, s.stephen.forkX);
    Assert::AreEqual(forkY, s.stephen.forkY);
  }

  void AssertSausage(Sausage expected) {
    _expected = expected; // remember the logical baseline so AssertSausageMoved can step from it
    InlineTransform(expected.x1, expected.y1);
    InlineTransform(expected.x2, expected.y2);

    // Restore the upper-left invariant the sim keeps. If the transform reversed the two halves, the
    // cook bits move with them (Cook1*<->Cook2*); planar D4 never flips the A/B faces or Rolled.
    if (expected.x1 > expected.x2 || (expected.x1 == expected.x2 && expected.y1 > expected.y2)) {
      std::swap(expected.x1, expected.x2);
      std::swap(expected.y1, expected.y2);
      expected.SwapCookBits();
    }

    for (const Sausage& s : _level._sausages) {
      if (s == expected) return;
    }

    wchar_t want[96];
    swprintf(want, 96, L"want (%d,%d)-(%d,%d) f=%d | have:",
      (int)expected.x1, (int)expected.y1, (int)expected.x2, (int)expected.y2, (int)expected.flags);
    std::wstring detail = want;
    for (const Sausage& s : _level._sausages) {
      wchar_t have[64];
      swprintf(have, 64, L" (%d,%d)-(%d,%d) f=%d", (int)s.x1, (int)s.y1, (int)s.x2, (int)s.y2, (int)s.flags);
      detail += have;
    }
    Assert::Fail(detail.c_str());
  }

  // Steps the tracked sausage (last passed to AssertSausage) one cell in the logical |dir| and
  // toggles |flags| (e.g. Sausage::Rolled for a roll), then asserts the real sausage matches.
  void AssertSausageMoved(Direction dir, u8 flags = 0) {
    switch (dir) {
      case Up:    _expected.y1--; _expected.y2--; break;
      case Down:  _expected.y1++; _expected.y2++; break;
      case Left:  _expected.x1--; _expected.x2--; break;
      case Right: _expected.x1++; _expected.x2++; break;
      default: break;
    }
    _expected.flags ^= flags;
    AssertSausage(_expected);
  }

  // Interactive recorder: drive the wrapped level from stdin (u/d/l/r move, z undo, q finish), build
  // a symmetry-test body, then OVERWRITE the level.BuildTest() call site in the source file with it.
  // Build the helper with the identity symmetry so the emitted coordinates are in the logical frame.
  // Invoke it as level.BuildTest(): the macro at the bottom of this header forwards the caller's
  // __FILE__/__LINE__ here (a method's own __FILE__/__LINE__ would point into this header instead).
  //
  // The vstest host owns a hidden, output-redirected console, so plain getchar()/printf() never reach
  // a window. We grab that console window (creating one only if there genuinely isn't any), reattach
  // the CRT streams to the real console device (CONIN$/CONOUT$) -- bypassing VS's pipe redirection --
  // then unhide and foreground the window so it can be driven live.
  void BuildTestAt(const char* file, int callLine) {
    static const char* const DIRS[] = { "None", "Up", "Left", "Jump", "Crouch", "Right", "Down" };

    void* hwnd = GetConsoleWindow();
    if (!hwnd) {
      // The host attaches a windowless pseudo-console (ConPTY) to capture output, so AllocConsole
      // fails with ERROR_ACCESS_DENIED (a console is already attached) and GetConsoleWindow() is
      // null. Detach from it first, then allocate a fresh real console that actually has a window.
      FreeConsole();
      AllocConsole();
      hwnd = GetConsoleWindow();
    }
    FILE* f = nullptr;
    freopen_s(&f, "CONIN$", "r", stdin);
    freopen_s(&f, "CONOUT$", "w", stdout);
    if (hwnd) { ShowWindow(hwnd, 5 /*SW_SHOW*/); SetForegroundWindow(hwnd); }

    _level.Print();
    printf("Build-test mode. Keys: u/d/l/r = move, z = undo, q = finish & emit.\n");

    std::vector<State> undoHistory{ _level.GetState() };
    Vector<Sausage> prevSausages = _level._sausages.Copy();
    int tracked = -1; // sausage index AssertSausageMoved's single _expected currently follows (-1 = none)

    // One record per accepted move: its emitted chunk plus the tracked index as of after it. 'z' drops the last record
    // and restores tracked, so a later re-recorded move re-anchors correctly.
    struct Recorded { std::string code; int tracked; };
    std::vector<Recorded> moves;

    // Build one generated line from its pieces; s8 fields are cast to int so they stream as numbers.
    auto emit = [](auto const&... parts) {
      std::ostringstream o;
      (o << ... << parts);
      return o.str();
    };

    const std::string head = emit("    level.AssertPosition(", (int)_level._stephen.x, ", ",
                                  (int)_level._stephen.y, ", ", DIRS[_level._stephen.dir], ");\n");

    while (true) {
      int ch = getchar();
      if (ch == EOF) break; // No interactive input (e.g. run as a test) -- emit what we have and stop.
      if (ch == 'q' || ch == 'Q') break;
      if (ch == '\n') continue;
      if (ch == 'z' || ch == 'Z') {
        if (undoHistory.size() > 1) {
          undoHistory.pop_back();
          _level.SetState(undoHistory[undoHistory.size() - 1]);
          prevSausages = _level._sausages.Copy();
          if (!moves.empty()) {
            moves.pop_back();
            tracked = moves.empty() ? -1 : moves.back().tracked;
          }
          _level.Print();
        }
        continue;
      }
      Direction dir = None;
      if (ch == 'u' || ch == 'U') dir = Up;
      else if (ch == 'd' || ch == 'D') dir = Down;
      else if (ch == 'l' || ch == 'L') dir = Left;
      else if (ch == 'r' || ch == 'R') dir = Right;
      if (dir == None) continue;

      // Each accepted move builds one chunk; undo just pops it, so no per-move length bookkeeping.
      std::string chunk;
      if (_level.Move(dir)) {
        chunk += emit("    level.AssertMoveSucceeds(", DIRS[dir], ");\n");
        chunk += emit("    level.AssertPosition(", (int)_level._stephen.x, ", ", (int)_level._stephen.y,
                      ", ", DIRS[_level._stephen.dir], ");\n");

        // Gather which sausages this move shifted. AssertSausageMoved tracks a SINGLE _expected, so its compact
        // relative form is only valid for the lone sausage the tracker currently follows; a move that shifts several
        // (a push chain) emits an absolute AssertSausage for the others so nothing compounds off one tracker.
        std::vector<int> moved;
        for (int i = 0; i < _level._sausages.Size(); i++)
          if (!(_level._sausages[i] == prevSausages[i])) moved.push_back(i);

        for (int i : moved) {
          const Sausage& cur = _level._sausages[i];
          const Sausage& prev = prevSausages[i];
          s8 dx = cur.x1 - prev.x1, dy = cur.y1 - prev.y1;
          int adx = dx < 0 ? -dx : dx, ady = dy < 0 ? -dy : dy;
          u8 flagDelta = cur.flags ^ prev.flags;
          bool cleanTranslation = dx == cur.x2 - prev.x2 && dy == cur.y2 - prev.y2 &&
                                  cur.z == prev.z && adx + ady == 1 &&
                                  (flagDelta == 0 || flagDelta == Sausage::Rolled);
          if (cleanTranslation && moved.size() == 1) {
            // Lone clean step: if the tracker is on another sausage, re-anchor by prepending this one's pre-move
            // baseline (it must run before the move), then emit the compact relative form.
            if (tracked != i)
              chunk.insert(0, emit("    level.AssertSausage({", (int)prev.x1, ", ", (int)prev.y1, ", ",
                                   (int)prev.x2, ", ", (int)prev.y2, ", ", (int)prev.z, ", ", FlagsToString(prev.flags), "});\n"));
            Direction moveDir = dx > 0 ? Right : dx < 0 ? Left : dy > 0 ? Down : Up;
            if (flagDelta & Sausage::Rolled)
              chunk += emit("    level.AssertSausageMoved(", DIRS[moveDir], ", Sausage::Rolled);\n");
            else
              chunk += emit("    level.AssertSausageMoved(", DIRS[moveDir], ");\n");
          } else {
            // A chain (or a non-unit jump): assert the absolute pose -- self-contained, no compounding.
            chunk += emit("    level.AssertSausage({", (int)cur.x1, ", ", (int)cur.y1, ", ", (int)cur.x2,
                          ", ", (int)cur.y2, ", ", (int)cur.z, ", ", FlagsToString(cur.flags), "});\n");
          }
          tracked = i;
        }
        prevSausages = _level._sausages.Copy();
        undoHistory.push_back(_level.GetState());
      } else {
        chunk += emit("    level.AssertMoveFails(", DIRS[dir], ");\n");
        _level.SetState(undoHistory[undoHistory.size() - 1]); // undo the rejected move's side-effects
        // Keep undoHistory in lockstep with |moves| (one entry per attempt) so a later 'z' rewinds
        // exactly one move. Without this, undoing a rejected move over-pops undoHistory and silently
        // rewinds _level an extra real step, corrupting every later assert.
        undoHistory.push_back(_level.GetState());
      }
      moves.push_back({ chunk, tracked });
      _level.Print();
      Stephen stephen = _level._stephen;
      printf("Stephen is at %d %d %d, facing %s\n", stephen.x, stephen.y, stephen.z, DIRS[stephen.dir]);
    }

    // Overwrite the call site with the generated body, using the line from the macro.
    std::string out, line, code;
    {
      std::ifstream in(file);
      for (int lineNo = 1; std::getline(in, line); lineNo++) {
        if (lineNo == callLine) {
          out += head;
          for (const Recorded& m : moves) code += m.code;

          out += code;
        } else {
          out += line + "\n";
        }
      }
    }

    std::ofstream(file) << out;
  }
};

// level.BuildTest() routes through this macro so the recorder learns the *caller's* source location
// (file + line of the call), then overwrites that exact line with the generated test body. (A macro
// is required: __FILE__/__LINE__ used inside BuildTestAt would resolve to this header, not the test.)
#define BUILD_LEVEL() level.BuildTestAt(__FILE__, __LINE__)
