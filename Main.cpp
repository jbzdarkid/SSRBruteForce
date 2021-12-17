#include "Level.h"
#include <cstdio>


int main() {
  Level MaidensWalk(4, 4,
  "#0_#"
  "#0__"
  " _^_"
  "__  ");

  MaidensWalk.Print();
  printf("ULDR :");
  while (true) {
    int ch = getchar();
    if (ch == '\n') continue;
    if (ch == 'u')      MaidensWalk.Move(Up);
    else if (ch == 'd') MaidensWalk.Move(Down);
    else if (ch == 'l') MaidensWalk.Move(Left);
    else if (ch == 'r') MaidensWalk.Move(Right);
    MaidensWalk.Print();
  printf("ULDR :");
  }
}