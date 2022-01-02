#include "Level.h"
#include <cstdio>

Level::Level(u8 width, u8 height, const char* name, const char* asciiGrid, const Stephen& stephen, std::initializer_list<Sausage> sausages, std::initializer_list<Ladder> ladders)
  : Level(width, height, name, asciiGrid)
{
  _stephen = stephen;
  _start = stephen;
  // Throw away sausages from the ascii grid
  u8 i = 0;
  _sausages.Resize(0);
  for (Sausage sausage : sausages) _sausages.Push(sausage);
  _ladders = Vector<Ladder>(ladders);
}

Level::Level(u8 width, u8 height, const char* name, const char* asciiGrid) {
  this->name = name;
  _width = width;
  _height = height;
  _grid = NewDoubleArray2<Tile>(width, height);

#define o(x) +1
  _movedSausages = Vector<s8>(SAUSAGES);
  _sausages.Ensure(SAUSAGES);
#undef o
  _sausages.Fill({-127, -127, -127, -127, Sausage::Flags::None});

  assert(width * height == strlen(asciiGrid));

  for (s32 i=0; i<width * height; i++) {
    s8 x = i % width;
    s8 y = i / width;
    char c = asciiGrid[i];
    if (c == ' ') _grid[x][y] = Empty;
    else if (c == '_') _grid[x][y] = Ground;
    else if (c == '#') _grid[x][y] = (Tile)(Ground | Grill);
    else if (c == '1') _grid[x][y] = Wall1;
    else if (c == '2') _grid[x][y] = Wall2;
    else if (c == '3') _grid[x][y] = Wall3;
    else if (c == '^' || c == 'v' || c == '<' || c == '>') {
      _grid[x][y] = Ground;
      Direction dir;
      if (c == '^')      dir = Up;
      else if (c == 'v') dir = Down;
      else if (c == '<') dir = Left;
      else if (c == '>') dir = Right;
      _stephen = Stephen(x, y, 0, dir);
    } else if (c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z') {
      int num;
      if (c >= 'A' && c <= 'Z') {
        _grid[x][y] = Empty;
        num = c - 'A';
      } else {
        _grid[x][y] = Ground;
        num = c - 'a';
      }

      // We need to do this here because, even if we don't *solve* levels
      // with less than the expected count, we still construct them.
      while (_sausages.Size() < num + 1) _sausages.Push({-127, -127, -127, -127, Sausage::Flags::None});
      if (_sausages[num].x1 == x-1 && _sausages[num].y1 == y) {
        _sausages[num].x2 = x;
        _sausages[num].y2 = y;
        _sausages[num].flags |= Sausage::Flags::Horizontal;
      } else if (_sausages[num].x1 == x && _sausages[num].y1 == y-1) {
        _sausages[num].x2 = x;
        _sausages[num].y2 = y;
        _sausages[num].flags &= ~Sausage::Flags::Horizontal;
      } else {
        _sausages[num].x1 = x;
        _sausages[num].y1 = y;
      }
    } else assert(false); // Not sure how to convert this ascii character into a tile
  }
  _start = _stephen;
}

Level::~Level() {
  DeleteDoubleArray2(_grid);
}

void Level::Print() const {
  putchar('+');
  for (u8 x=0; x<_width; x++) putchar('-');
  putchar('+');
  putchar('\n');

  for (u8 y=0; y<_height; y++) {
    putchar('|');
    for (u8 x=0; x<_width; x++) {
      s8 sausageNo = GetSausage(x, y, 0);
      if (sausageNo == -1) sausageNo = GetSausage(x, y, 1);
      if (sausageNo == -1) sausageNo = GetSausage(x, y, 2);
      if (sausageNo == -1) sausageNo = GetSausage(x, y, 3);

      if (x == _stephen.x && y == _stephen.y) {
        if (_stephen.dir == Up)          putchar('^');
        else if (_stephen.dir == Down)   putchar('v');
        else if (_stephen.dir == Left)   putchar('<');
        else if (_stephen.dir == Right)  putchar('>');
        else assert(false);
      }
      //else if (stephen.dir == Up && x == stephen.x && y == stephen.y - 1) row[x+1] = '|';
      //else if (stephen.dir == Down && x == stephen.x && y == stephen.y + 1) row[x+1] = '|';
      //else if (stephen.dir == Left && x == stephen.x - 1 && y == stephen.y) row[x+1] = '-';
      //else if (stephen.dir == Right && x == stephen.x + 1 && y == stephen.y) row[x+1] = '-';
      else if (sausageNo != -1 && _grid[x][y] == Empty) putchar('A' + (char)sausageNo);
      else if (sausageNo != -1)                         putchar('a' + (char)sausageNo);
      else if (_grid[x][y] == Empty)  putchar(' ');
      else if (_grid[x][y] &  Grill)  putchar('#');
      else if (_grid[x][y] == Ground) putchar('_');
      else if (_grid[x][y] == Wall1)  putchar('1');
      else if (_grid[x][y] == Wall2)  putchar('2');
      else if (_grid[x][y] == Wall3)  putchar('3');
      else assert(false);
    }
    putchar('|');
    putchar('\n');
  }

  putchar('+');
  for (u8 x=0; x<_width; x++) putchar('-');
  putchar('+');
  putchar('\n');
}

bool Level::InteractiveSolver() {
  _interactive = true;
  Print();
  printf("ULDR: ");
  Vector<State> undoHistory({GetState()});
  while (!Won()) {
    int ch = getchar();
    if (ch == '\n') {
      Print();
      printf("ULDR: ");
    } else if (ch == 'z' || ch == 'Z') {
      if (undoHistory.Size() > 1) { // Cannot pop the initial state
        undoHistory.Pop();
        SetState(&undoHistory[undoHistory.Size()-1]); // The state at the (new) end of the list
      }
    } else if (ch == 'q' || ch == 'Q') {
      printf("Reset level\n");
      SetState(&undoHistory[0]);
      undoHistory.Resize(1);
    } else {
      Direction dir = None;
      if (ch == 'u' || ch == 'U') dir = Up;
      else if (ch == 'd' || ch == 'D') dir = Down;
      else if (ch == 'l' || ch == 'L') dir = Left;
      else if (ch == 'r' || ch == 'R') dir = Right;
      if (dir != None) {
        if (Move(dir)) { // Valid move, add to the history
          undoHistory.Push(GetState());
        } else { // Invalid move, restore to fix side-effects
          SetState(&undoHistory[undoHistory.Size()-1]);
        }
      }
    }
  }

  printf("Level completed in %d moves\n", undoHistory.Size() - 1);
  return true;
}

bool Level::Won() const {
  if (_stephen != _start) return false;
  for (const Sausage& sausage : _sausages) {
    if ((sausage.flags & Sausage::Flags::FullyCooked) != Sausage::Flags::FullyCooked) return false;
  }
  return true;
}

State Level::GetState() const {
  State s{_stephen};
  /*if (_sausages[0].x1 < _sausages[1].x1
   || (_sausages[0].x1 == _sausages[1].x1 && _sausages[0].y1 < _sausages[1].y1)) {
    s.sausages[0] = _sausages[1];
    s.sausages[1] = _sausages[0];
    if (_sausageSpeared != -1) _sausageSpeared = 1 - _sausageSpeared;
  } else */{
    _sausages.CopyIntoArray(s.sausages, sizeof(s.sausages));
  }
  return s;
}

void Level::SetState(const State* s) {
  _stephen = s->stephen;
  _sausages.CopyFromArray(s->sausages, sizeof(s->sausages));

  // Speared state is not saved, because it's recoverable. Memory > speed tradeoff.
  if (_stephen.HasFork()) _sausageSpeared = GetSausage(_stephen.forkX, _stephen.forkY, _stephen.z);
}

const char* DIRS[] = {
  nullptr,
  "North",
  "South",
  nullptr,
  "West",
  nullptr,
  nullptr,
  nullptr,
  "East",
};

#define FAIL(reason, ...) \
  { \
    if (_interactive) { \
      printf("[%d] Move was illegal: " reason "\n", __LINE__, ##__VA_ARGS__); \
    } \
    return false; \
  }

bool Level::WouldStephenStepOnGrill(const Stephen& stephen, Direction dir) {
  if (dir == Up)         return IsGrill(stephen.x, stephen.y-1, stephen.z);
  else if (dir == Down)  return IsGrill(stephen.x, stephen.y+1, stephen.z);
  else if (dir == Left)  return IsGrill(stephen.x-1, stephen.y, stephen.z);
  else if (dir == Right) return IsGrill(stephen.x+1, stephen.y, stephen.z);
  else {
    assert(false);
    FAIL("Stephen is moving in an invalid direction %d", dir);
  }
}

// Where possible, I prefer to check to see if we should call a function instead of using &handled.
bool Level::Move(Direction dir) {
  s8 standingOnSausage = GetSausage(_stephen.x, _stephen.y, _stephen.z - 1);
  if (standingOnSausage != -1) {
    Sausage sausage = _sausages[standingOnSausage];
    bool handled = false;
    if (!HandleLogRolling(sausage, dir, handled)) return false;
    if (handled) return true;
  }

  if (_sausageSpeared != -1) {
    if (!HandleSpearedMotion(dir)) return false;
    if (IsGrill(_stephen.x, _stephen.y, _stephen.z)) {
      if (!HandleBurnedStep(dir)) return false;
    }
    return true;
  }

  bool handled = false;
  if (!HandleLadderMotion(dir, handled)) return false;
  if (handled) return true;

  if (_stephen.HasFork()) {
    if (!HandleDefaultMotion(dir)) return false;
  } else {
    if (!HandleForklessMotion(dir)) return false;
  }

  if (IsGrill(_stephen.x, _stephen.y, _stephen.z)) {
    if (!HandleBurnedStep(dir)) return false;
  }

  return true;
}

bool Level::HandleLogRolling(const Sausage& sausage, Direction dir, bool& handled) {
  if (dir == Up && sausage.IsHorizontal() && (_stephen.dir == Up || _stephen.dir == Down)) {
    if (CanPhysicallyMove(sausage.x1, sausage.y1, sausage.z, Down)
      && CanPhysicallyMove(sausage.x2, sausage.y2, sausage.z, Down)) {
      if (!MoveThroughSpace(sausage.x1, sausage.y1, sausage.z, Down)) return false; // This *should* move the entire sausage.
      if (!MoveStephenThroughSpace(Down)) return false;
      handled = true;
    }
  } else if (dir == Down && sausage.IsHorizontal() && (_stephen.dir == Up || _stephen.dir == Down)) {
    if (CanPhysicallyMove(sausage.x1, sausage.y1, sausage.z, Up)
      && CanPhysicallyMove(sausage.x2, sausage.y2, sausage.z, Up)) {
      if (!MoveThroughSpace(sausage.x1, sausage.y1, sausage.z, Up)) return false; // This *should* move the entire sausage.
      if (!MoveStephenThroughSpace(Up)) return false;
      handled = true;
    }
  } else if (dir == Left && sausage.IsVertical() && (_stephen.dir == Left || _stephen.dir == Right)) {
    if (CanPhysicallyMove(sausage.x1, sausage.y1, sausage.z, Right)
      && CanPhysicallyMove(sausage.x2, sausage.y2, sausage.z, Right)) {
      if (!MoveThroughSpace(sausage.x1, sausage.y1, sausage.z, Right)) return false; // This *should* move the entire sausage.
      if (!MoveStephenThroughSpace(Right)) return false;
      handled = true;
    }
  } else if (dir == Right && sausage.IsVertical() && (_stephen.dir == Left || _stephen.dir == Right)) {
    if (CanPhysicallyMove(sausage.x1, sausage.y1, sausage.z, Left)
      && CanPhysicallyMove(sausage.x2, sausage.y2, sausage.z, Left)) {
      if (!MoveThroughSpace(sausage.x1, sausage.y1, sausage.z, Left)) return false; // This *should* move the entire sausage.
      if (!MoveStephenThroughSpace(Left)) return false;
      handled = true;
    }
  }
  if (handled) {
    // TODO: Nix this once I actually handle fork positions.
    _stephen.forkX = _stephen.x;
    _stephen.forkY = _stephen.y;
    if (_stephen.dir == Up)         _stephen.forkY = _stephen.y - 1;
    else if (_stephen.dir == Down)  _stephen.forkY = _stephen.y + 1;
    else if (_stephen.dir == Left)  _stephen.forkX = _stephen.x - 1;
    else if (_stephen.dir == Right) _stephen.forkX = _stephen.x + 1;

    while (true) {
      bool stephenSupported = CanWalkOnto(_stephen.x, _stephen.y, _stephen.z);
      if (stephenSupported) break;
      // Log rolls are allowed to make stephen fall off of cliffs, but we've already checked that sausages cannot be lost.
      if (_stephen.z <= 0) FAIL("Stephen would fall below the world");

      if (_stephen.HasFork()) {
        bool forkSupported = CanWalkOnto(_stephen.forkX, _stephen.forkY, _stephen.z);
        if (forkSupported) { // Stephen becomes forkless!
          _stephen.forkZ = _stephen.z;
          _stephen.forkDir = _stephen.dir;
        }
      }
      _stephen.z--;
    }

    if (!_stephen.HasFork()) FAIL("Cannot currently handle fork disconnects");
  }

  return true;
}

bool Level::HandleSpearedMotion(Direction dir) {
  // First, see if stephen can move
  if (dir == Up) {
    if (!CanPhysicallyMove(_stephen.x, _stephen.y - 1, _stephen.z, dir)) FAIL("Stephen cannot physically move up");
  } else if (dir == Down) {
    if (!CanPhysicallyMove(_stephen.x, _stephen.y + 1, _stephen.z, dir)) FAIL("Stephen cannot physically move down");
  } else if (dir == Left) {
    if (!CanPhysicallyMove(_stephen.x - 1, _stephen.y, _stephen.z, dir)) FAIL("Stephen cannot physically move left");
  } else if (dir == Right) {
    if (!CanPhysicallyMove(_stephen.x + 1, _stephen.y, _stephen.z, dir)) FAIL("Stephen cannot physically move right");
  } else {
    assert(false);
    FAIL("Stephen cannot move in direction %d", dir);
  }
  // Then, check to see if our sausage gets unspeared
  if (dir == Up && _stephen.dir == Down) {
    if (!CanPhysicallyMove(_stephen.x, _stephen.y + 1, _stephen.z, dir)) _sausageSpeared = -1;
  } else if (dir == Down && _stephen.dir == Up) {
    if (!CanPhysicallyMove(_stephen.x, _stephen.y - 1, _stephen.z, dir)) _sausageSpeared = -1;
  } else if (dir == Left && _stephen.dir == Right) {
    if (!CanPhysicallyMove(_stephen.x + 1, _stephen.y, _stephen.z, dir)) _sausageSpeared = -1;
  } else if (dir == Right && _stephen.dir == Left) {
    if (!CanPhysicallyMove(_stephen.x - 1, _stephen.y, _stephen.z, dir)) _sausageSpeared = -1;
  }
  // If the sausage is still speared, try to move it, and fail the movement is invalid
  if (_sausageSpeared != -1) {
    if (_stephen.dir == Up) {
      if (!MoveThroughSpace(_stephen.x, _stephen.y - 1, _stephen.z, dir)) return false;
    } else if (_stephen.dir == Down) {
      if (!MoveThroughSpace(_stephen.x, _stephen.y + 1, _stephen.z, dir)) return false;
    } else if (_stephen.dir == Left) {
      if (!MoveThroughSpace(_stephen.x - 1, _stephen.y, _stephen.z, dir)) return false;
    } else if (_stephen.dir == Right) {
      if (!MoveThroughSpace(_stephen.x + 1, _stephen.y, _stephen.z, dir)) return false;
    }
  }
  // Finally, move stephen, now that our sausage has moved out of the way
  if (dir == Up) {
    if (!CanWalkOnto(_stephen.x, _stephen.y - 1, _stephen.z)) FAIL("Stephen is unsupported if he moves up");
    if (!MoveThroughSpace(_stephen.x, _stephen.y - 1, _stephen.z, Up)) return false;
    _stephen.y--;
  } else if (dir == Down) {
    if (!CanWalkOnto(_stephen.x, _stephen.y + 1, _stephen.z)) FAIL("Stephen is unsupported if he moves down");
    if (!MoveThroughSpace(_stephen.x, _stephen.y + 1, _stephen.z, Down)) return false;
    _stephen.y++;
  } else if (dir == Left) {
    if (!CanWalkOnto(_stephen.x - 1, _stephen.y, _stephen.z)) FAIL("Stephen is unsupported if he moves left");
    if (!MoveThroughSpace(_stephen.x - 1, _stephen.y, _stephen.z, Left)) return false;
    _stephen.x--;
  } else if (dir == Right) {
    if (!CanWalkOnto(_stephen.x + 1, _stephen.y, _stephen.z)) FAIL("Stephen is unsupported if he moves right");
    if (!MoveThroughSpace(_stephen.x + 1, _stephen.y, _stephen.z, Right)) return false;
    _stephen.x++;
  }
  return true;
}

bool Level::HandleLadderMotion(Direction dir, bool& handled) {
  if (dir == Up && (_stephen.dir == Left || _stephen.dir == Right)) {
    if (IsLadder(_stephen.x, _stephen.y - 1, _stephen.z, Down)) { // Climbing a ladder
      if (!MoveStephenThroughSpace(Jump)) return false;
      if (!MoveStephenThroughSpace(Up)) return false;
      handled = true;
    } else if (IsLadder(_stephen.x, _stephen.y, _stephen.z - 1, Up)) { // Descending a ladder
      if (CanPhysicallyMove(_stephen.x, _stephen.y - 1, _stephen.z - 1, Crouch)) {
        if (!MoveStephenThroughSpace(Up)) return false;
        if (!MoveStephenThroughSpace(Crouch)) return false;
        if (!CanWalkOnto(_stephen.x, _stephen.y, _stephen.z)) FAIL("Stephen would walk off of a cliff");
        handled = true;
      }
    }
  } else if (dir == Down && (_stephen.dir == Left || _stephen.dir == Right)) {
    if (IsLadder(_stephen.x, _stephen.y + 1, _stephen.z, Up)) { // Climbing a ladder
      if (!MoveStephenThroughSpace(Jump)) return false;
      if (!MoveStephenThroughSpace(Down)) return false;
      handled = true;
    } else if (IsLadder(_stephen.x, _stephen.y, _stephen.z - 1, Down)) { // Descending a ladder
      if (CanPhysicallyMove(_stephen.x, _stephen.y + 1, _stephen.z - 1, Crouch)) {
        if (!MoveStephenThroughSpace(Down)) return false;
        if (!MoveStephenThroughSpace(Crouch)) return false;
        if (!CanWalkOnto(_stephen.x, _stephen.y, _stephen.z)) FAIL("Stephen would walk off of a cliff");
        handled = true;
      }
    }
  } else if (dir == Left && (_stephen.dir == Up || _stephen.dir == Down)) {
    if (IsLadder(_stephen.x - 1, _stephen.y, _stephen.z, Right)) { // Climbing a ladder
      if (!MoveStephenThroughSpace(Jump)) return false;
      if (!MoveStephenThroughSpace(Left)) return false;
      handled = true;
    } else if (IsLadder(_stephen.x, _stephen.y, _stephen.z - 1, Left)) { // Descending a ladder
      if (CanPhysicallyMove(_stephen.x - 1, _stephen.y, _stephen.z - 1, Crouch)) {
        if (!MoveStephenThroughSpace(Left)) return false;
        if (!MoveStephenThroughSpace(Crouch)) return false;
        if (!CanWalkOnto(_stephen.x, _stephen.y, _stephen.z)) FAIL("Stephen would walk off of a cliff");
        handled = true;
      }
    }
  } else if (dir == Right && (_stephen.dir == Up || _stephen.dir == Down)) {
    if (IsLadder(_stephen.x + 1, _stephen.y, _stephen.z, Left)) { // Climbing a ladder
      if (!MoveStephenThroughSpace(Jump)) return false;
      if (!MoveStephenThroughSpace(Right)) return false;
      handled = true;
    } else if (IsLadder(_stephen.x, _stephen.y, _stephen.z - 1, Right)) { // Descending a ladder
      if (CanPhysicallyMove(_stephen.x + 1, _stephen.y, _stephen.z - 1, Crouch)) {
        if (!MoveStephenThroughSpace(Right)) return false;
        if (!MoveStephenThroughSpace(Crouch)) return false;
        if (!CanWalkOnto(_stephen.x, _stephen.y, _stephen.z)) FAIL("Stephen would walk off of a cliff");
        handled = true;
      }
    }
  }

  return true;
}

bool Level::HandleBurnedStep(Direction dir) {
  if (dir == Up)         return Move(Down);
  else if (dir == Down)  return Move(Up);
  else if (dir == Left)  return Move(Right);
  else if (dir == Right) return Move(Left);
  else {
    assert(false);
    FAIL("Stephen cannot move in direction %d", dir);
  }
}

bool Level::HandleForklessMotion(Direction dir) {
  if (dir == Up && (_stephen.dir == Up || _stephen.dir == Down)) {
    if (!MoveStephenThroughSpace(dir)) return false;
  } else if (dir == Down && (_stephen.dir == Up || _stephen.dir == Down)) {
    if (!MoveStephenThroughSpace(dir)) return false;
  } else if (dir == Left && (_stephen.dir == Left || _stephen.dir == Right)) {
    if (!MoveStephenThroughSpace(dir)) return false;
  } else if (dir == Right && (_stephen.dir == Left || _stephen.dir == Right)) {
    if (!MoveStephenThroughSpace(dir)) return false;
  } else {
    _stephen.dir = dir;
  }

  if (_stephen.z == _stephen.forkZ && _stephen.dir == _stephen.forkDir) {
    bool reconnectFork = false;
    if (_stephen.dir == Up) {
      reconnectFork = (_stephen.x == _stephen.forkX && _stephen.y - 1 == _stephen.forkY);
    } else if (_stephen.dir == Down) {
      reconnectFork = (_stephen.x == _stephen.forkX && _stephen.y + 1 == _stephen.forkY);
    } else if (_stephen.dir == Left) {
      reconnectFork = (_stephen.x == _stephen.forkX - 1 && _stephen.y == _stephen.forkY);
    } else if (_stephen.dir == Right) {
      reconnectFork = (_stephen.x == _stephen.forkX + 1 && _stephen.y == _stephen.forkY);
    }
    if (reconnectFork) {
      _stephen.forkZ = -1;
      _stephen.forkDir = None;
    }
  }
  return true;
}

bool Level::HandleDefaultMotion(Direction dir) {
  if (dir == Up) {
    if (_stephen.dir == Left) {
      if (!MoveThroughSpace( _stephen.x - 1, _stephen.y - 1, _stephen.z, Up)) return false;
      if (!CanPhysicallyMove(_stephen.x,     _stephen.y - 1, _stephen.z, Right)) return true; // Bonk
      if (!MoveThroughSpace( _stephen.x,     _stephen.y - 1, _stephen.z, Right)) return false;
      _stephen.dir = dir;
      _stephen.forkY--;
      _stephen.forkX++;
    } else if (_stephen.dir == Right) {
      if (!MoveThroughSpace( _stephen.x + 1, _stephen.y - 1, _stephen.z, Up)) return false;
      if (!CanPhysicallyMove(_stephen.x,     _stephen.y - 1, _stephen.z, Left)) return true; // Bonk
      if (!MoveThroughSpace( _stephen.x,     _stephen.y - 1, _stephen.z, Left)) return false;
      _stephen.dir = dir;
      _stephen.forkY--;
      _stephen.forkX--;
    } else {
      if (!MoveStephenThroughSpace(dir)) return false;
      if (!CanWalkOnto(_stephen.x, _stephen.y, _stephen.z)) FAIL("Stephen would walk off of a cliff");
    }
  } else if (dir == Down) {
    if (_stephen.dir == Left) {
      if (!MoveThroughSpace( _stephen.x - 1, _stephen.y + 1, _stephen.z, Down)) return false;
      if (!CanPhysicallyMove(_stephen.x,     _stephen.y + 1, _stephen.z, Right)) return true; // Bonk
      if (!MoveThroughSpace( _stephen.x,     _stephen.y + 1, _stephen.z, Right)) return false;
      _stephen.dir = dir;
      _stephen.forkY++;
      _stephen.forkX--;
    } else if (_stephen.dir == Right) {
      if (!MoveThroughSpace( _stephen.x + 1, _stephen.y + 1, _stephen.z, Down)) return false;
      if (!CanPhysicallyMove(_stephen.x,     _stephen.y + 1, _stephen.z, Left)) return true; // Bonk
      if (!MoveThroughSpace( _stephen.x,     _stephen.y + 1, _stephen.z, Left)) return false;
      _stephen.dir = dir;
      _stephen.forkY++;
      _stephen.forkX++;
    } else {
      if (!MoveStephenThroughSpace(dir)) return false;
      if (!CanWalkOnto(_stephen.x, _stephen.y, _stephen.z)) FAIL("Stephen would walk off of a cliff");
    }
  } else if (dir == Left) {
    if (_stephen.dir == Up) {
      if (!MoveThroughSpace( _stephen.x - 1, _stephen.y - 1, _stephen.z, Left)) return false;
      if (!CanPhysicallyMove(_stephen.x - 1, _stephen.y,     _stephen.z, Down)) return true; // Bonk
      if (!MoveThroughSpace( _stephen.x - 1, _stephen.y,     _stephen.z, Down)) return false;
      _stephen.dir = dir;
      _stephen.forkY++;
      _stephen.forkX--;
    } else if (_stephen.dir == Down) {
      if (!MoveThroughSpace( _stephen.x - 1, _stephen.y + 1, _stephen.z, Left)) return false;
      if (!CanPhysicallyMove(_stephen.x - 1, _stephen.y,     _stephen.z, Up)) return true; // Bonk
      if (!MoveThroughSpace( _stephen.x - 1, _stephen.y,     _stephen.z, Up)) return false;
      _stephen.dir = dir;
      _stephen.forkY++;
      _stephen.forkX--;
    } else {
      if (!MoveStephenThroughSpace(dir)) return false;
      if (!CanWalkOnto(_stephen.x, _stephen.y, _stephen.z)) FAIL("Stephen would walk off of a cliff");
    }
  } else if (dir == Right) {
    if (_stephen.dir == Up) {
      if (!MoveThroughSpace( _stephen.x + 1, _stephen.y - 1, _stephen.z, Right)) return false;
      if (!CanPhysicallyMove(_stephen.x + 1, _stephen.y,     _stephen.z, Down)) return true; // Bonk
      if (!MoveThroughSpace( _stephen.x + 1, _stephen.y,     _stephen.z, Down)) return false;
      _stephen.dir = dir;
      _stephen.forkY--;
      _stephen.forkX++;
    } else if (_stephen.dir == Down) {
      if (!MoveThroughSpace( _stephen.x + 1, _stephen.y + 1, _stephen.z, Right)) return false;
      if (!CanPhysicallyMove(_stephen.x + 1, _stephen.y,     _stephen.z, Up)) return true; // Bonk
      if (!MoveThroughSpace( _stephen.x + 1, _stephen.y,     _stephen.z, Up)) return false;
      _stephen.dir = dir;
      _stephen.forkY++;
      _stephen.forkX++;
    } else {
      if (!MoveStephenThroughSpace(dir)) return false;
      if (!CanWalkOnto(_stephen.x, _stephen.y, _stephen.z)) FAIL("Stephen would walk off of a cliff");
    }
  }
  return true;
}

bool Level::CanPhysicallyMove(s8 x, s8 y, s8 z, Direction dir, Vector<s8>* movedSausages, s8* sausageWithFork) {
  if (IsWall(x, y, z)) return false; // No, walls cannot move.

  s8 sausageNo = GetSausage(x, y, z);

  if (!_stephen.HasFork()) {
    if (x == _stephen.forkX && y == _stephen.forkY && z == _stephen.forkZ) {
      if (sausageWithFork) {
        assert(*sausageWithFork == -1);
        *sausageWithFork = sausageNo;
      }
      if (dir == Up) {
        return CanPhysicallyMove(x, y - 1, z, dir, movedSausages, sausageWithFork);
      } else if (dir == Down) {
        return CanPhysicallyMove(x, y + 1, z, dir, movedSausages, sausageWithFork);
      } else if (dir == Left) {
        return CanPhysicallyMove(x - 1, y, z, dir, movedSausages, sausageWithFork);
      } else if (dir == Right) {
        return CanPhysicallyMove(x + 1, y, z, dir, movedSausages, sausageWithFork);
      } else if (dir == Jump) {
        return CanPhysicallyMove(x, y, z + 1, dir, movedSausages, sausageWithFork);
      } else if (dir == Crouch) {
        return false;
        // return CanPhysicallyMove(x, y, z - 1, dir, movedSausages, sausageWithFork);
      } else {
        assert(false); // Unknown direction
        FAIL("Attempted to physically move in an unknown direction");
      }
    }
  }

  if (sausageNo == -1) return true; // Empty spaces can move.

  if (movedSausages) {
    for (s8 i : *movedSausages) {
      // We have already evaluated this sausage; it would've failed then.
      if (i == sausageNo) return true;
    }
    movedSausages->UnsafePush(sausageNo);
  }
  Sausage sausage = _sausages[sausageNo];

  // Sausages can move if the tiles beyond them can move (either other sausages or empty space)
  if (dir == Up) {
    if (sausage.IsVertical()) {
      return CanPhysicallyMove(sausage.x1, sausage.y1 - 1, sausage.z, dir, movedSausages, sausageWithFork);
    } else {
      return CanPhysicallyMove(sausage.x1, sausage.y1 - 1, sausage.z, dir, movedSausages, sausageWithFork)
          && CanPhysicallyMove(sausage.x2, sausage.y2 - 1, sausage.z, dir, movedSausages, sausageWithFork);
    }
  } else if (dir == Down) {
    if (sausage.IsVertical()) {
      return CanPhysicallyMove(sausage.x2, sausage.y2 + 1, sausage.z, dir, movedSausages, sausageWithFork);
    } else {
      return CanPhysicallyMove(sausage.x1, sausage.y1 + 1, sausage.z, dir, movedSausages, sausageWithFork)
          && CanPhysicallyMove(sausage.x2, sausage.y2 + 1, sausage.z, dir, movedSausages, sausageWithFork);
    }
  } else if (dir == Left) {
    if (sausage.IsHorizontal()) {
      return CanPhysicallyMove(sausage.x1 - 1, sausage.y1, sausage.z, dir, movedSausages, sausageWithFork);
    } else {
      return CanPhysicallyMove(sausage.x1 - 1, sausage.y1, sausage.z, dir, movedSausages, sausageWithFork)
          && CanPhysicallyMove(sausage.x2 - 1, sausage.y2, sausage.z, dir, movedSausages, sausageWithFork);
    }
  } else if (dir == Right) {
    if (sausage.IsHorizontal()) {
      return CanPhysicallyMove(sausage.x1 + 1, sausage.y1, sausage.z, dir, movedSausages, sausageWithFork);
    } else {
      return CanPhysicallyMove(sausage.x1 + 1, sausage.y1, sausage.z, dir, movedSausages, sausageWithFork)
          && CanPhysicallyMove(sausage.x2 + 1, sausage.y2, sausage.z, dir, movedSausages, sausageWithFork);
    }
  } else if (dir == Jump) {
    return CanPhysicallyMove(sausage.x1, sausage.y1, sausage.z + 1, dir, movedSausages, sausageWithFork)
        && CanPhysicallyMove(sausage.x2, sausage.y2, sausage.z + 1, dir, movedSausages, sausageWithFork);
  } else if (dir == Crouch) {
    return false; // It's never possible for sausages to move down, so don't even bother.
    // return                          CanPhysicallyMove(sausage.x1, sausage.y1, sausage.z - 1, dir, movedSausages, sausageWithFork)
    //   &&                            CanPhysicallyMove(sausage.x2, sausage.y2, sausage.z - 1, dir, movedSausages, sausageWithFork);
  } else {
    assert(false); // Unknown direction
    FAIL("Attempted to physically move in an unknown direction");
  }
}

bool Level::MoveThroughSpace(s8 x, s8 y, s8 z, Direction dir, bool spear) {
  _movedSausages.Resize(0);
  s8 sausageWithFork = -1;
  bool canPhysicallyMove = CanPhysicallyMove(x, y, z, dir, &_movedSausages, &sausageWithFork);

  if (!canPhysicallyMove) {
    if (spear && _movedSausages.Size() > 0) {
      _sausageSpeared = _movedSausages[0];
      // This location cannot move, but we can stil move into it (by spearing).
      return true;
    }

    if (!_interactive) return false;

    // This branch can be hit from a bunch of places. Let's try to give a clean error message.
    if (spear) FAIL("Stephen's fork cannot move in direction %s", DIRS[dir]);
    if (_movedSausages.Size() > 0) FAIL("Cannot move sausage %c in direction %s", 'a' + _movedSausages[0], DIRS[dir]);
    FAIL("Wall at [%d, %d, %d] cannot move %s (probably you pushed a fork into it)", x, y, z, DIRS[dir]);
  }

  // Currently not worrying about side-effects. Wouldn't be too hard to prevent, though:
  // Just save the sausages into a temp array and then move them all at once.
  for (s8 sausageNo : _movedSausages) {
    Sausage sausage = _sausages[sausageNo];

    // Move the sausage
    if (dir == Up) {
      sausage.y1--;
      sausage.y2--;
    } else if (dir == Down) {
      sausage.y1++;
      sausage.y2++;
    } else if (dir == Left) {
      sausage.x1--;
      sausage.x2--;
    } else if (dir == Right) {
      sausage.x1++;
      sausage.x2++;
    } else if (dir == Jump) {
      sausage.z++;
    } else if (dir == Crouch) {
      assert(false); // I don't think this is possible.
      sausage.z--;
    } else {
      assert(false);
      FAIL("Cannot move sausage %c in direction %d", 'a' + sausageNo, dir);
    }

    if (_stephen.HasFork() && sausageNo == _sausageSpeared) {
      // Speared sausages do not roll nor fall
    } else {
      if ((sausage.IsHorizontal() && (dir == Up || dir == Down))
        || (sausage.IsVertical() && (dir == Left || dir == Right))) {
        sausage.flags ^= Sausage::Flags::Rolled;

        if (sausageWithFork == sausageNo) {
          if (_stephen.forkDir == Up)         _stephen.forkDir = Down;
          else if (_stephen.forkDir == Down)  _stephen.forkDir = Up;
          else if (_stephen.forkDir == Left)  _stephen.forkDir = Right;
          else if (_stephen.forkDir == Right) _stephen.forkDir = Left;
        }
      }

      while (true) {
        bool supported = CanWalkOnto(sausage.x1, sausage.y1, sausage.z)
                      || CanWalkOnto(sausage.x2, sausage.y2, sausage.z);
        if (supported) break;
        if (sausage.z <= 0) FAIL("Sausage %c would fall below the world", 'a' + sausageNo);
        sausage.z--;
      }
    }

    // Cook the sausage
    u8 sidesToCook = 0;
    if (IsGrill(sausage.x1, sausage.y1, sausage.z)) sidesToCook |= Sausage::Flags::Cook1A;
    if (IsGrill(sausage.x2, sausage.y2, sausage.z)) sidesToCook |= Sausage::Flags::Cook2A;

    if (sidesToCook != 0) {
      if (sausage.flags & Sausage::Flags::Rolled) sidesToCook *= 2; // Shift cooking flags to the rolled side
      if (sausage.flags & sidesToCook) FAIL("Sausage %c would burn", 'a' + sausageNo);
      sausage.flags |= sidesToCook;
    }

    // If nothing has gone wrong, save the updated sausage
    _sausages[sausageNo] = sausage;
  }

  if (false /*movedFork*/) { // implies !_stephen.HasFork
    if (dir == Up) {
      _stephen.forkY--;
    } else if (dir == Down) {
      _stephen.forkY++;
    } else if (dir == Left) {
      _stephen.forkX--;
    } else if (dir == Right) {
      _stephen.forkX++;
    } else if (dir == Jump) {
      _stephen.forkZ++;
    } else if (dir == Crouch) {
      assert(false); // I don't think this is possible.
      _stephen.forkZ--;
    } else {
      assert(false);
      FAIL("Cannot move disconnected fork in direction %d", dir);
    }

    if (sausageWithFork == -1) { // Forks do not fall when they are inside a sausage
      while (true) {
        bool supported = CanWalkOnto(_stephen.forkX, _stephen.forkY, _stephen.forkZ);
        if (supported) break;
        if (_stephen.forkZ <= 0) FAIL("Disconnected fork would fall below the world");
        _stephen.forkZ--;
      }
    }
  }

  return true;
}

bool Level::MoveStephenThroughSpace(Direction dir) {
  // This function is allowed side effects, so we can move stephen's body before calling MoveThroughSpace
  s8 dx = 0;
  s8 dy = 0;
  s8 dz = 0;
  if (dir == Up)          dy = -1;
  else if (dir == Down)   dy = +1;
  else if (dir == Left)   dx = -1;
  else if (dir == Right)  dx = +1;
  else if (dir == Crouch) dz = -1;
  else if (dir == Jump)   dz = +1;

  _stephen.x += dx;
  _stephen.y += dy;
  _stephen.z += dz;

#if STAY_NEAR_THE_SAUSAGES // only if the sausages aren't all cooked?
  u16 distanceToSausage0 =
    (_sausages[0].x1 - _stephen.x) * (_sausages[0].x1 - _stephen.x) +
    (_sausages[0].y1 - _stephen.y) * (_sausages[0].y1 - _stephen.y);
  u16 distanceToSausage1 =
    (_sausages[1].x1 - _stephen.x) * (_sausages[1].x1 - _stephen.x) +
    (_sausages[1].y1 - _stephen.y) * (_sausages[1].y1 - _stephen.y);
  u16 distanceToSausage2 =
    (_sausages[0].x2 - _stephen.x) * (_sausages[0].x2 - _stephen.x) +
    (_sausages[0].y2 - _stephen.y) * (_sausages[0].y2 - _stephen.y);
  u16 distanceToSausage3 =
    (_sausages[1].x2 - _stephen.x) * (_sausages[1].x2 - _stephen.x) +
    (_sausages[1].y2 - _stephen.y) * (_sausages[1].y2 - _stephen.y);
  if (distanceToSausage0 > 9 && distanceToSausage1 > 9 && distanceToSausage2 > 9 && distanceToSausage3 > 9) {
    FAIL("Stephen would move too far away from the sausages");
  }
#endif

  if (_stephen.HasFork()) {
    _stephen.forkX += dx;
    _stephen.forkY += dy;

    bool spear = (dir == _stephen.dir);
    if (!MoveThroughSpace(_stephen.forkX, _stephen.forkY, _stephen.z, dir, spear)) return false;
  }
  if (!MoveThroughSpace(_stephen.x, _stephen.y, _stephen.z, dir)) return false;
  return true;
}

s8 Level::GetSausage(s8 x, s8 y, s8 z) const {
  if (z < 0) return -1;
  for (u8 i=0; i<_sausages.Size(); i++) {
    const Sausage& sausage = _sausages[i];
    if (z == sausage.z) {
      if (x == sausage.x1 && y == sausage.y1) return i;
      if (x == sausage.x2 && y == sausage.y2) return i;
    }
  }

  return -1;
}

bool Level::IsWithinGrid(s8 x, s8 y, s8 z) const {
  return x >= 0 && x <= _width - 1 && y >= 0 && y <= _height - 1 && z >= 0;
}

bool Level::IsWall(s8 x, s8 y, s8 z) const {
  if (!IsWithinGrid(x, y, z)) return false;
  u8 cell = _grid[x][y];
  // At z=0, we are blocked if the cell has the second bit set, i.e. (cell & 0b10).
  return cell & (Ground << 1 << z);
}

//   4 = Wall2
// > 2 = Wall1
//   1 = Ground
bool Level::CanWalkOnto(s8 x, s8 y, s8 z) const {
  if (!IsWithinGrid(x, y, z)) return false;
  u8 cell = _grid[x][y];
  // If you are at z=0, you can walk onto anything at ground level. If you are at z=1, you can walk onto Wall1
  if ((cell & (Ground << z)) != 0) return true; // Ground at our current level
  if (GetSausage(x, y, z-1) != -1) return true; // Sausage at our current level
  return false;
}

bool Level::IsGrill(s8 x, s8 y, s8 z) const {
  if (!IsWithinGrid(x, y, z)) return false;
  u8 cell = _grid[x][y];
  if ((cell & Grill) == 0) return false;
  return (cell & (Ground << z)) != 0; // There is ground at this level
}

bool Level::IsLadder(s8 x, s8 y, s8 z, Direction dir) const {
  for (const Ladder& ladder : _ladders) {
    if (ladder.x == x && ladder.y == y && ladder.z == z && ladder.dir == dir) return true;
  }
  return false;
}

bool State::operator==(const State& other) const {
  if (stephen.x != other.stephen.x) return false;
  if (stephen.y != other.stephen.y) return false;
  if (stephen.dir != other.stephen.dir) return false;
#define o(x) if (sausages[x] != other.sausages[x]) return false;
    SAUSAGES;
#undef o
  return true;
}

u32 triple32_hash(u32 x) {
    x ^= x >> 16;
    x *= 0x45d9f3bU;
    x ^= x >> 11;
    x *= 0xac4c1b51U;
    x ^= x >> 15;
    x *= 0x31848babU;
    x ^= x >> 14;
    return x;
}

void combine_hash(u32& a, u32 b) {
  b = triple32_hash(b);
  a += (b << 6) + (b >> 2);
}

u32 triple32_hash(u64 x) {
  u32 a = (u32)x;
  combine_hash(a, (u32)(x >> 32));
  return a;
}

void combine_hash(u32& a, u64 b) {
  combine_hash(a, (u32)b);
  combine_hash(a, (u32)(b >> 32));
}

u32 State::Hash() const {
  static_assert(sizeof(Stephen) == 8);
  static_assert(sizeof(Sausage) == 8);
  u32 hash = triple32_hash(*(u64*)&stephen);
#define o(x) combine_hash(hash, *(u64*)&sausages[x]);
  SAUSAGES
#undef o

  return hash;
}
