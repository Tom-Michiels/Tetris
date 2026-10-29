#include <cassert>
#include <cstdio>
#include <map>
#include "stub/Arduino.h"
std::map<std::string, unsigned long> g_nvs; String g_arg, g_body; int g_code; int g_sleeps, g_portal; unsigned g_freq;
unsigned long g_ms = 1; int g_pin[40];
#include "../../src/xiao_esp32c3/main.cpp"
int main() {
  for (int &p : g_pin) p = 1;  // all buttons released (pullup)
  setup(); startGame(0);
  // line clear: fill bottom row + a marker above, expect shift down and row 0 cleared
  clearGrid();
  for (int x = 0; x < WIDTH; x++) grid[HEIGHT-1][x] = 3;
  grid[HEIGHT-2][0] = 5; grid[0][1] = 4; grid[1][2] = 6;
  unsigned before = level;
  block_y = 4; block_x = 2; block_shape = 1; block_orientation = 0;  // O-piece away from rows
  loop();                                   // highlight
  assert(isHighlightedLine(HEIGHT-1)); assert(level == before - 2);
  g_ms += 60; loop();                       // delete after 50ms
  assert(grid[HEIGHT-1][0] == 5 && grid[HEIGHT-1][1] == 0);
  assert(grid[1][1] == 4 && grid[2][2] == 6 && grid[0][1] == 0 && grid[1][2] == 0);  // top rows shifted, row 0 cleared
  // level clamp: no unsigned wrap
  level = 52; for (int i = 0; i < 10; i++) { for (int x = 0; x < WIDTH; x++) grid[HEIGHT-1][x] = 3; loop(); g_ms += 60; loop(); }
  assert(level >= 50 && level < 1000);
  // new block orientation reset
  block_orientation = 3; initBlock(); assert(block_orientation == 0 && block_y == 0);
  // button: press left for 4 loops -> exactly one move
  clearGrid(); initBlock(); block_shape = 1; int x0 = block_x;
  g_pin[BUTTON_LEFT] = 0; for (int i = 0; i < 10; i++) loop(); g_pin[BUTTON_LEFT] = 1; loop();
  assert(block_x == x0 - 1);
  // stuck button (>3s held) ignored
  int x1 = block_x; g_pin[BUTTON_LEFT] = 0; for (int i = 0; i < 2000; i++) { g_ms += 2; loop(); }
  assert(block_x >= x1 - 1);
  // play 20k loops random input: never out of bounds, game keeps going
  for (int i = 0; i < 20000; i++) { for (int p = 4; p <= 7; p++) g_pin[p] = rand() % 3 != 0; g_ms += 5; loop();
    assert(block_x >= -1 && block_x < WIDTH && block_y >= 0 && block_y < HEIGHT); }
  puts("all host tests passed");
}
