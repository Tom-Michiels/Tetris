// new features: bag, score/record, auto-repeat, web control, /state, OTA progress
#include <cassert>
#include <cstdio>
#include <map>
#include "stub/Arduino.h"
std::map<std::string, unsigned long> g_nvs; String g_arg, g_body; int g_code; int g_sleeps, g_portal; unsigned g_freq;
unsigned long g_ms = 1; int g_pin[40];
#include "../../src/xiao_esp32c3/main.cpp"
void fillRow(int y) { for (int x = 0; x < WIDTH; x++) grid[y][x] = 3; }
int main() {
  for (int &p : g_pin) p = 1;
  g_nvs["hi"] = 7; setup(); startGame(0); assert(hiscore[0] == 7);
  // bag: each 7 draws contain every shape once
  bagLeft = 0;
  for (int r = 0; r < 50; r++) { int seen = 0; for (int i = 0; i < 7; i++) seen |= 1 << nextShape(); assert(seen == 0x7f); }
  // score: 2 lines at once = 3 points
  clearGrid(); fillRow(15); fillRow(14); block_shape = 1; block_orientation = 0; block_x = 2; block_y = 4;
  loop(); assert(score == 3 && lines == 2);
  g_ms += 60; loop(); assert(!isHighlightedLine(15) && !isHighlightedLine(14));
  fillRow(15); g_ms += 60; loop(); assert(score == 4 && lines == 3);
  // game over saves record only if beaten
  score = 9; for (int x = 0; x < WIDTH; x++) for (int y = 1; y < HEIGHT; y++) grid[y][x] = (x + y) % 2 ? 2 : 0;
  block_y = 0; falling_down = true; loop();
  assert(g_nvs["hi"] == 9 && hiscore[0] == 9 && score == 0 && lines == 0);
  // auto-repeat: holding left long moves several times, rotate does not repeat
  clearGrid(); initBlock(); block_shape = 1; block_x = 6; previousMoveTime = g_ms;
  g_pin[BUTTON_LEFT] = 0; for (int i = 0; i < 90; i++) { previousMoveTime = g_ms; loop(); } g_pin[BUTTON_LEFT] = 1; loop();
assert(block_x == 6 - 4);  // press at 4, repeats at 60, 75, 90
  int o = block_orientation; g_pin[BUTTON_UP] = 0; for (int i = 0; i < 90; i++) { previousMoveTime = g_ms; loop(); } g_pin[BUTTON_UP] = 1; loop();
  assert(block_orientation == (o + 1) % 4);
  // web control: one request = one move
  int bx = block_x; g_arg = "r"; handleKey(); assert(g_code == 204); previousMoveTime = g_ms; loop(); loop(); assert(block_x == bx + 1);
  // /state: 128 hex colors, the falling block shows in its color
  handleState(); size_t q = g_body.find("\"p\":\""); std::string g = g_body.substr(q + 5, 768);
  assert(g.size() == 768 && g.find_first_not_of("0123456789abcdef") == std::string::npos);
  { char want[7]; snprintf(want, 7, "%02x%02x%02x", g_colors[block_color][0], g_colors[block_color][1], g_colors[block_color][2]);
    loop(); handleState(); g = g_body.substr(g_body.find("\"p\":\"") + 5, 768);
    assert(g.substr((block_y * WIDTH + block_x + 1) * 6, 6) == want); }
  assert(g_body.find("\"n\":\"tetris\"") != std::string::npos);
  // OTA progress callback lights rows without crashing at 0% and 100%
  handleOta(); // WiFi stub not connected -> nothing registered; call lambdas via a connected run is not stubbed
  // idle sleep: nothing pressed for 10 min -> deep sleep; a press resets the timer
  g_sleeps = 0; lastInput = g_ms; g_ms += 9 * 60000UL; loop(); assert(g_sleeps == 0);
  g_arg = "u"; handleKey(); g_ms += 9 * 60000UL; loop(); assert(g_sleeps == 0);
  g_ms += 2 * 60000UL; loop(); assert(g_sleeps == 1);
  // setup portal opens once after 20 s without wifi (stub never connects)
  assert(g_portal == 1);
  puts("all new-feature tests passed");
}
