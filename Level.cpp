#include "Level.h"
#include <cstdio>

Level::Level(u8 width, u8 height, const char* name, const char* asciiGrid, const Stephen& stephen, std::initializer_list<Sausage> sausages, std::initializer_list<Ladder> ladders)
  : Level(width, height, name, asciiGrid)
{
  _stephen = stephen;
  _start = stephen;
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
#undef o

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
      _stephen.x = x;
      _stephen.y = y;
      if (c == '^')      _stephen.dir = Up;
      else if (c == 'v') _stephen.dir = Down;
      else if (c == '<') _stephen.dir = Left;
      else if (c == '>') _stephen.dir = Right;
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
  _explain = true;
  Print();
  printf("ULDR: ");
  Vector<State> undoHistory({GetState()});
  while (!Won()) {
    int ch = getchar();
    if (ch == '\n') {
      Print();
      printf("ULDR: ");
      continue;
    }
    else if (ch == 'z' || ch == 'Z') {
      if (undoHistory.Size() > 1) { // Cannot pop the initial state
        undoHistory.Pop();
        SetState(&undoHistory[undoHistory.Size()-1]); // The state at the (new) end of the list
      }
    }
    else if (ch == 'q' || ch == 'Q') return false;

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

  printf("Level completed in %d moves\n", undoHistory.Size() - 1);
  return true;
}

bool Level::Won() const {
  if (_stephen.sausageSpeared != -1) return false;
  //if (_stephen.x != _start.x) return false;
  //if (_stephen.y != _start.y) return false;
  //if (_stephen.dir != _start.dir) return false;
#define o(x) if ((_sausages[x].flags & Sausage::Flags::FullyCooked) != Sausage::Flags::FullyCooked) return false;
  SAUSAGES
#undef o
  return true;
}

State Level::GetState() const {
  State s;
  s.stephen = _stephen;
  /*if (_sausages[0].x1 < _sausages[1].x1
   || (_sausages[0].x1 == _sausages[1].x1 && _sausages[0].y1 < _sausages[1].y1)) {
    s.sausages[0] = _sausages[1];
    s.sausages[1] = _sausages[0];
    if (_stephen.sausageSpeared != -1) s.stephen.sausageSpeared = 1 - _stephen.sausageSpeared;
  } else */{
    _sausages.CopyIntoArray(s.sausages, sizeof(s.sausages));
  }
  return s;
}

void Level::SetState(const State* s) {
  _stephen = s->stephen;
  _sausages.CopyFromArray(s->sausages, sizeof(s->sausages));
}

bool Level::WouldStephenStepOnGrill(const Stephen& stephen, Direction dir) {
  if (dir == Up)         return IsGrill(stephen.x, stephen.y-1, stephen.z);
  else if (dir == Down)  return IsGrill(stephen.x, stephen.y+1, stephen.z);
  else if (dir == Left)  return IsGrill(stephen.x-1, stephen.y, stephen.z);
  else if (dir == Right) return IsGrill(stephen.x+1, stephen.y, stephen.z);
  else {
    assert(false);
    return false;
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

  if (_stephen.sausageSpeared != -1) {
    if (!HandleSpearedMotion(dir)) return false;
    if (IsGrill(_stephen.x, _stephen.y, _stephen.z)) {
      if (!HandleBurnedStep(dir)) return false;
    }
    return true;
  }

  bool handled = false;
  if (!HandleLadderMotion(dir, handled)) return false;
  if (handled) return true;

  if (!HandleDefaultMotion(dir)) return false;
  if (IsGrill(_stephen.x, _stephen.y, _stephen.z)) {
    if (!HandleBurnedStep(dir)) return false;
  }

  return true;
}

bool Level::HandleLogRolling(const Sausage& sausage, Direction dir, bool& handled) {
  if (dir == Up && sausage.IsHorizontal() && (_stephen.dir == Up || _stephen.dir == Down)) {
    s8 fork = (_stephen.dir == Down ? 1 : 0);
    if (!CanPhysicallyMove(_stephen.x, _stephen.y + fork, _stephen.z, Down)) return false;
    if (CanPhysicallyMove(sausage.x1, sausage.y1, sausage.z, Down)
      && CanPhysicallyMove(sausage.x2, sausage.y2, sausage.z, Down)) {
      if (!MoveThroughSpace(sausage.x1, sausage.y1, sausage.z, Down)) return false; // This *should* move the entire sausage.
      if (!MoveStephenThroughSpace(Down)) return false;
      handled = true;
    }
  } else if (dir == Down && sausage.IsHorizontal() && (_stephen.dir == Up || _stephen.dir == Down)) {
    s8 fork = (_stephen.dir == Up ? 1 : 0);
    if (!CanPhysicallyMove(_stephen.x, _stephen.y - fork, _stephen.z, Up)) return false;
    if (CanPhysicallyMove(sausage.x1, sausage.y1, sausage.z, Up)
      && CanPhysicallyMove(sausage.x2, sausage.y2, sausage.z, Up)) {
      if (!MoveThroughSpace(sausage.x1, sausage.y1, sausage.z, Up)) return false; // This *should* move the entire sausage.
      if (!MoveStephenThroughSpace(Up)) return false;
      handled = true;
    }
  } else if (dir == Left && sausage.IsVertical() && (_stephen.dir == Left || _stephen.dir == Right)) {
    s8 fork = (_stephen.dir == Right ? 1 : 0);
    if (!CanPhysicallyMove(_stephen.x + fork, _stephen.y, _stephen.z, Right)) return false;
    if (CanPhysicallyMove(sausage.x1, sausage.y1, sausage.z, Right)
      && CanPhysicallyMove(sausage.x2, sausage.y2, sausage.z, Right)) {
      if (!MoveThroughSpace(sausage.x1, sausage.y1, sausage.z, Right)) return false; // This *should* move the entire sausage.
      if (!MoveStephenThroughSpace(Right)) return false;
      handled = true;
    }
  } else if (dir == Right && sausage.IsVertical() && (_stephen.dir == Left || _stephen.dir == Right)) {
    s8 fork = (_stephen.dir == Left ? 1 : 0);
    if (!CanPhysicallyMove(_stephen.x - fork, _stephen.y, _stephen.z, Left)) return false;
    if (CanPhysicallyMove(sausage.x1, sausage.y1, sausage.z, Left)
      && CanPhysicallyMove(sausage.x2, sausage.y2, sausage.z, Left)) {
      if (!MoveThroughSpace(sausage.x1, sausage.y1, sausage.z, Left)) return false; // This *should* move the entire sausage.
      if (!MoveStephenThroughSpace(Left)) return false;
      handled = true;
    }
  }
  return true;
}

bool Level::HandleSpearedMotion(Direction dir) {
  // First, see if stephen can move
  if (dir == Up) {
    if (!CanPhysicallyMove(_stephen.x, _stephen.y - 1, _stephen.z, dir)) return false;
  } else if (dir == Down) {
    if (!CanPhysicallyMove(_stephen.x, _stephen.y + 1, _stephen.z, dir)) return false;
  } else if (dir == Left) {
    if (!CanPhysicallyMove(_stephen.x - 1, _stephen.y, _stephen.z, dir)) return false;
  } else if (dir == Right) {
    if (!CanPhysicallyMove(_stephen.x + 1, _stephen.y, _stephen.z, dir)) return false;
  }
  // Then, check to see if our sausage gets unspeared
  if (dir == Up && _stephen.dir == Down) {
    if (!CanPhysicallyMove(_stephen.x, _stephen.y + 1, _stephen.z, dir)) _stephen.sausageSpeared = -1;
  } else if (dir == Down && _stephen.dir == Up) {
    if (!CanPhysicallyMove(_stephen.x, _stephen.y - 1, _stephen.z, dir)) _stephen.sausageSpeared = -1;
  } else if (dir == Left && _stephen.dir == Right) {
    if (!CanPhysicallyMove(_stephen.x + 1, _stephen.y, _stephen.z, dir)) _stephen.sausageSpeared = -1;
  } else if (dir == Right && _stephen.dir == Left) {
    if (!CanPhysicallyMove(_stephen.x - 1, _stephen.y, _stephen.z, dir)) _stephen.sausageSpeared = -1;
  }
  // If the sausage is still speared, try to move it, and fail the movement is invalid
  if (_stephen.sausageSpeared != -1) {
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
    if (!CanWalkOnto(_stephen.x, _stephen.y - 1, _stephen.z)) return false;
    if (!MoveThroughSpace(_stephen.x, _stephen.y - 1, _stephen.z, Up)) return false;
    _stephen.y--;
  } else if (dir == Down) {
    if (!CanWalkOnto(_stephen.x, _stephen.y + 1, _stephen.z)) return false;
    if (!MoveThroughSpace(_stephen.x, _stephen.y + 1, _stephen.z, Down)) return false;
    _stephen.y++;
  } else if (dir == Left) {
    if (!CanWalkOnto(_stephen.x - 1, _stephen.y, _stephen.z)) return false;
    if (!MoveThroughSpace(_stephen.x - 1, _stephen.y, _stephen.z, Left)) return false;
    _stephen.x--;
  } else if (dir == Right) {
    if (!CanWalkOnto(_stephen.x + 1, _stephen.y, _stephen.z)) return false;
    if (!MoveThroughSpace(_stephen.x + 1, _stephen.y, _stephen.z, Right)) return false;
    _stephen.x++;
  }
  return true;
}

bool Level::HandleLadderMotion(Direction dir, bool& handled) {
  // TODO: call CanPhysicallyMove to see if there's a sausage in the way of the ladder.
  // Why not just... ask? "If you would descend a ladder, but there's a sausage in the way (*at the same elevation*), you step onto it instead.
  if (dir == Up && (_stephen.dir == Left || _stephen.dir == Right)) {
    if (IsLadder(_stephen.x, _stephen.y - 1, _stephen.z, Down)) { // Climbing a ladder
      if (!MoveStephenThroughSpace(Jump)) return false;
      if (!MoveStephenThroughSpace(Up)) return false;
      handled = true;
    } else if (IsLadder(_stephen.x, _stephen.y, _stephen.z - 1, Up)) { // Descending a ladder
      if (!MoveStephenThroughSpace(Up)) return false;
      if (!MoveStephenThroughSpace(Crouch)) return false;
      handled = true;
    }
  } else if (dir == Down && (_stephen.dir == Left || _stephen.dir == Right)) {
    if (IsLadder(_stephen.x, _stephen.y + 1, _stephen.z, Up)) { // Climbing a ladder
      if (!MoveStephenThroughSpace(Jump)) return false;
      if (!MoveStephenThroughSpace(Down)) return false;
      handled = true;
    } else if (IsLadder(_stephen.x, _stephen.y, _stephen.z - 1, Down)) { // Descending a ladder
      if (!MoveStephenThroughSpace(Down)) return false;
      if (!MoveStephenThroughSpace(Crouch)) return false;
      handled = true;
    }
  } else if (dir == Left && (_stephen.dir == Up || _stephen.dir == Down)) {
    if (IsLadder(_stephen.x - 1, _stephen.y, _stephen.z, Right)) { // Climbing a ladder
      if (!MoveStephenThroughSpace(Jump)) return false;
      if (!MoveStephenThroughSpace(Left)) return false;
      handled = true;
    } else if (IsLadder(_stephen.x, _stephen.y, _stephen.z - 1, Left)) { // Descending a ladder
      if (!MoveStephenThroughSpace(Left)) return false;
      if (!MoveStephenThroughSpace(Crouch)) return false;
      handled = true;
    }
  } else if (dir == Right && (_stephen.dir == Up || _stephen.dir == Down)) {
    if (IsLadder(_stephen.x + 1, _stephen.y, _stephen.z, Left)) { // Climbing a ladder
      if (!MoveStephenThroughSpace(Jump)) return false;
      if (!MoveStephenThroughSpace(Right)) return false;
      handled = true;
    } else if (IsLadder(_stephen.x, _stephen.y, _stephen.z - 1, Right)) { // Descending a ladder
      if (!MoveStephenThroughSpace(Right)) return false;
      if (!MoveStephenThroughSpace(Crouch)) return false;
      handled = true;
    }
  }

  return true;
}

bool Level::HandleBurnedStep(Direction dir) {
  if (dir == Up)    return Move(Down);
  if (dir == Down)  return Move(Up);
  if (dir == Left)  return Move(Right);
  if (dir == Right) return Move(Left);
  assert(false);
  return false;
}

bool Level::HandleDefaultMotion(Direction dir) {
  if (dir == Up) {
    if (_stephen.dir == Left) {
      if (!MoveThroughSpace( _stephen.x - 1, _stephen.y - 1, _stephen.z, Up)) return false;
      if (!CanPhysicallyMove(_stephen.x,     _stephen.y - 1, _stephen.z, Right)) return true; // Bonk
      if (!MoveThroughSpace( _stephen.x,     _stephen.y - 1, _stephen.z, Right)) return false;
      _stephen.dir = Up;
    } else if (_stephen.dir == Right) {
      if (!MoveThroughSpace( _stephen.x + 1, _stephen.y - 1, _stephen.z, Up)) return false;
      if (!CanPhysicallyMove(_stephen.x,     _stephen.y - 1, _stephen.z, Left)) return true; // Bonk
      if (!MoveThroughSpace( _stephen.x,     _stephen.y - 1, _stephen.z, Left)) return false;
      _stephen.dir = Up;
    } else {
      if (!MoveStephenThroughSpace(Up)) return false;
    }
  } else if (dir == Down) {
    if (_stephen.dir == Left) {
      if (!MoveThroughSpace( _stephen.x - 1, _stephen.y + 1, _stephen.z, Down)) return false;
      if (!CanPhysicallyMove(_stephen.x,     _stephen.y + 1, _stephen.z, Right)) return true; // Bonk
      if (!MoveThroughSpace( _stephen.x,     _stephen.y + 1, _stephen.z, Right)) return false;
      _stephen.dir = Down;
    } else if (_stephen.dir == Right) {
      if (!MoveThroughSpace( _stephen.x + 1, _stephen.y + 1, _stephen.z, Down)) return false;
      if (!CanPhysicallyMove(_stephen.x,     _stephen.y + 1, _stephen.z, Left)) return true; // Bonk
      if (!MoveThroughSpace( _stephen.x,     _stephen.y + 1, _stephen.z, Left)) return false;
      _stephen.dir = Down;
    } else {
      if (!MoveStephenThroughSpace(Down)) return false;
    }
  } else if (dir == Left) {
    if (_stephen.dir == Up) {
      if (!MoveThroughSpace( _stephen.x - 1, _stephen.y - 1, _stephen.z, Left)) return false;
      if (!CanPhysicallyMove(_stephen.x - 1, _stephen.y,     _stephen.z, Down)) return true; // Bonk
      if (!MoveThroughSpace( _stephen.x - 1, _stephen.y,     _stephen.z, Down)) return false;
      _stephen.dir = Left;
    } else if (_stephen.dir == Down) {
      if (!MoveThroughSpace( _stephen.x - 1, _stephen.y + 1, _stephen.z, Left)) return false;
      if (!CanPhysicallyMove(_stephen.x - 1, _stephen.y,     _stephen.z, Up)) return true; // Bonk
      if (!MoveThroughSpace( _stephen.x - 1, _stephen.y,     _stephen.z, Up)) return false;
      _stephen.dir = Left;
    } else {
      if (!MoveStephenThroughSpace(Left)) return false;
    }
  } else if (dir == Right) {
    if (_stephen.dir == Up) {
      if (!MoveThroughSpace( _stephen.x + 1, _stephen.y - 1, _stephen.z, Right)) return false;
      if (!CanPhysicallyMove(_stephen.x + 1, _stephen.y,     _stephen.z, Down)) return true; // Bonk
      if (!MoveThroughSpace( _stephen.x + 1, _stephen.y,     _stephen.z, Down)) return false;
      _stephen.dir = Right;
    } else if (_stephen.dir == Down) {
      if (!MoveThroughSpace( _stephen.x + 1, _stephen.y + 1, _stephen.z, Right)) return false;
      if (!CanPhysicallyMove(_stephen.x + 1, _stephen.y,     _stephen.z, Up)) return true; // Bonk
      if (!MoveThroughSpace( _stephen.x + 1, _stephen.y,     _stephen.z, Up)) return false;
      _stephen.dir = Right;
    } else {
      if (!MoveStephenThroughSpace(Right)) return false;
    }
  }
  return true;
}

bool Level::CanPhysicallyMove(s8 x, s8 y, s8 z, Direction dir, Vector<s8>* movedSausages) {
  if (IsWall(x, y, z)) return false; // No, walls cannot move.

  s8 sausageNo = GetSausage(x, y, z);
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
    return                        CanPhysicallyMove(sausage.x1, sausage.y1 - 1, sausage.z, dir, movedSausages)
      && (sausage.IsVertical() || CanPhysicallyMove(sausage.x2, sausage.y2 - 1, sausage.z, dir, movedSausages));
  } else if (dir == Down) {
    return                        CanPhysicallyMove(sausage.x2, sausage.y2 + 1, sausage.z, dir, movedSausages)
      && (sausage.IsVertical() || CanPhysicallyMove(sausage.x1, sausage.y1 + 1, sausage.z, dir, movedSausages));
  } else if (dir == Left) {
    return                          CanPhysicallyMove(sausage.x1 - 1, sausage.y1, sausage.z, dir, movedSausages)
      && (sausage.IsHorizontal() || CanPhysicallyMove(sausage.x2 - 1, sausage.y2, sausage.z, dir, movedSausages));
  } else if (dir == Right) {
    return                          CanPhysicallyMove(sausage.x2 + 1, sausage.y2, sausage.z, dir, movedSausages)
      && (sausage.IsHorizontal() || CanPhysicallyMove(sausage.x1 + 1, sausage.y1, sausage.z, dir, movedSausages));
  } else if (dir == Jump) {
    return                          CanPhysicallyMove(sausage.x1, sausage.y1, sausage.z + 1, dir, movedSausages)
      &&                            CanPhysicallyMove(sausage.x2, sausage.y2, sausage.z + 1, dir, movedSausages);
  } else if (dir == Crouch) {
    return false;
    // return                          CanPhysicallyMove(sausage.x1, sausage.y1, sausage.z - 1, dir, movedSausages)
    //   &&                            CanPhysicallyMove(sausage.x2, sausage.y2, sausage.z - 1, dir, movedSausages);
  } else {
    assert(false); // Unknown direction
    return false;
  }
}

bool Level::MoveThroughSpace(s8 x, s8 y, s8 z, Direction dir, bool spear) {
  _movedSausages.Resize(0);
  bool canPhysicallyMove = CanPhysicallyMove(x, y, z, dir, &_movedSausages);

  if (!canPhysicallyMove) {
    if (spear && _movedSausages.Size() > 0) {
      _stephen.sausageSpeared = _movedSausages[0];
      // This location cannot move, but we can stil move into it (by spearing).
      return true;
    }
    // This location cannot move (because some part of it pushes into a wall).
    return false;
  }

  // Currently not worrying about side-effects. Wouldn't be too hard to prevent, though.
  for (s8 sausageNo : _movedSausages) {
    Sausage sausage = _sausages[sausageNo];

    // Move the sausage
    if (dir == Up) {
      sausage.y1--;
      sausage.y2--;
    } else if (dir == Down) {
      if (sausage.y2 == 5 && (sausage.flags & Sausage::Flags::Cook2) != Sausage::Flags::Cook2) return false;

      sausage.y1++;
      sausage.y2++;
    } else if (dir == Left) {
      sausage.x1--;
      sausage.x2--;
    } else if (dir == Right) {
      sausage.x1++;
      sausage.x2++;
    } else assert(false);


    // if (sausage.x1 != 0) return false; // hack, of course

    // Speared sausages do not roll nor fall off cliffs
    if (sausageNo != _stephen.sausageSpeared) {
      // Check if the sausage would fall off a cliff
      if (!CanWalkOnto(sausage.x1, sausage.y1, sausage.z)
       && !CanWalkOnto(sausage.x2, sausage.y2, sausage.z)) {
        return false;
      }

      // Roll the sausage
      if ((sausage.IsHorizontal() && (dir == Up || dir == Down))
        || (sausage.IsVertical() && (dir == Left || dir == Right))) {
        sausage.flags ^= Sausage::Flags::Rolled;
      }
    }

    // Cook the sausage
    u8 sidesToCook = 0;
    if (IsGrill(sausage.x1, sausage.y1, sausage.z)) sidesToCook |= Sausage::Flags::Cook1A;
    if (IsGrill(sausage.x2, sausage.y2, sausage.z)) sidesToCook |= Sausage::Flags::Cook2A;

    if (sidesToCook > 0) {
      if (sausage.flags & Sausage::Flags::Rolled) sidesToCook *= 2; // Shift cooking flags to the rolled side
      if (sausage.flags & sidesToCook) return false; // If any of the sides are already cooked
      sausage.flags |= sidesToCook;
    }

    // If nothing has gone wrong, save the updated sausage
    _sausages[sausageNo] = sausage;
  }

  return true;
}

bool Level::MoveStephenThroughSpace(Direction dir) {
  // This function is allowed side effects, so we can move stephen's body before calling MoveThroughSpace
  if (dir == Up)          _stephen.y--;
  else if (dir == Down)   _stephen.y++;
  else if (dir == Left)   _stephen.x--;
  else if (dir == Right)  _stephen.x++;
  else if (dir == Crouch) _stephen.z--;
  else if (dir == Jump)   _stephen.z++;
  
  s8 forkX = 0;
  s8 forkY = 0;
  if (_stephen.dir == Up)         forkY = -1;
  else if (_stephen.dir == Down)  forkY = +1;
  else if (_stephen.dir == Left)  forkX = -1;
  else if (_stephen.dir == Right) forkX = +1;
  // Move stephen's fork, then stephen.
  if (!CanWalkOnto(_stephen.x, _stephen.y, _stephen.z)) return false;
  // TODO: Pull spearing out of MoveThroughSpace and into this? Would make the code much cleaner.
  bool spear = (dir == _stephen.dir);
  if (!MoveThroughSpace(_stephen.x + forkX, _stephen.y + forkY, _stephen.z, dir, spear)) return false;
  if (!MoveThroughSpace(_stephen.x, _stephen.y, _stephen.z, dir)) return false;
  return true;
}

s8 Level::GetSausage(s8 x, s8 y, s8 z) const {
  if (z < 0) return -1;
  Sausage sausage;
#define o(num) \
  sausage = _sausages[num]; \
  if (z == sausage.z) { \
    if (x == sausage.x1 && y == sausage.y1) return num; \
    if (x == sausage.x2 && y == sausage.y2) return num; \
  }
  SAUSAGES
#undef o

  return -1;
}

bool Level::IsWithinGrid(s8 x, s8 y, s8 z) const {
  return x >= 0 && x <= _width - 1 && y >= 0 && y <= _height - 1 && z >= 0;
}

bool Level::IsWall(s8 x, s8 y, s8 z) const {
  if (!IsWithinGrid(x, y, z)) return false;
  u8 cell = _grid[x][y];
  return cell & (Wall1 << z); // If you are at z=0, a wall at z=1 collides with you.
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
  return stephen.x == other.stephen.x
      && stephen.y == other.stephen.y
      && stephen.dir == other.stephen.dir
#define o(x) && sausages[x] == other.sausages[x]
    SAUSAGES;
#undef o
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
