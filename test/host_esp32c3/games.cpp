// menu + the four new games
#include <cassert>
#include <cstdio>
#include <map>
#include "stub/Arduino.h"
std::map<std::string, unsigned long> g_nvs; String g_arg, g_body; int g_code; int g_sleeps, g_portal; unsigned g_freq;
unsigned long g_ms = 1; int g_pin[40];
#include "../../src/xiao_esp32c3/main.cpp"
void press(int pin) { g_pin[pin] = 0; for (int i = 0; i < 5; i++) loop(); g_pin[pin] = 1; loop(); }
void key(char b) { g_arg = String(std::string(1, b).c_str()); handleKey(); loop(); }
void run(unsigned long ms) { unsigned long end = g_ms + ms; while (g_ms < end) { lastInput = g_ms; loop(); } }
int main() {
  for (int &p : g_pin) p = 1;
  setup();
  // boots into the menu; right/left cycle, up starts
  loop(); assert(game == -1 && menuPick == 0);
  press(BUTTON_RIGHT); assert(menuPick == 1);
  press(BUTTON_LEFT); press(BUTTON_LEFT); assert(menuPick == NR_GAMES - 1);
  press(BUTTON_RIGHT); press(BUTTON_UP); assert(game == 0);
  // hold up + down -> menu; web menu key too
  g_pin[BUTTON_UP] = g_pin[BUTTON_DOWN] = 0; for (int i = 0; i < 160; i++) loop(); g_pin[BUTTON_UP] = g_pin[BUTTON_DOWN] = 1; loop();
  assert(game == -1);
  startGame(1); key('m'); assert(game == -1);

  // snake: eats food, grows, scores; wall = game over + record saved
  startGame(1); assert(snakeLen == 3);
  foodX = snakeX[0]; foodY = snakeY[0] - 1;   // food straight ahead
  run(300); assert(snakeLen == 4 && score == 1);
  assert(!onSnake(foodX, foodY, snakeLen));
  key('d'); run(300); assert(snakeDy == -1);  // can't reverse into itself
  key('l'); key('d'); run(300); assert(!(snakeLastDx == 0 && snakeLastDy == 1) || snakeLen == 3);  // quick l+d: no instant reverse
  startGame(1); foodX = 0; foodY = 0; run(5000); assert(snakeLen == 3 && score == 0);  // ran into the top wall, restarted
  // snake into own body
  startGame(1); snakeLen = 5; int bx[] = {3, 3, 4, 4, 3}, by[] = {5, 6, 6, 5, 4};
  for (int i = 0; i < 5; i++) { snakeX[i] = bx[i]; snakeY[i] = by[i]; } snakeDx = 1; snakeDy = 0; snakeLastDx = 1; snakeLastDy = 0;
  foodX = 7; foodY = 15; snakeMoved = 0; loop(); assert(snakeLen == 3);  // (4,5) is body -> restarted
  g_nvs.clear(); startGame(1); score = 5; snakeX[0] = 0; snakeDx = -1; snakeDy = 0; snakeLastDx = -1; snakeLastDy = 0; snakeMoved = 0; loop();
  assert(hiscore[1] == 5 && g_nvs["hi1"] == 5 && g_nvs.count("hi") == 0);

  // dodge: walls come with a 3 wide gap, sitting in the gap survives and scores
  startGame(2); int crashes = 0;
  for (int t = 0; t < 4000; t++) {
    // autopilot: head for the gap of the next wall below the car's rows
    int target = -1;
    for (int y = HEIGHT - 1; y >= 0 && target < 0; y--) for (int x = 0; x < WIDTH; x++) if (grid[y][x]) { for (int g = 0; g < WIDTH; g++) if (!grid[y][g]) { target = g + 1; break; } break; }
    if (target >= 0 && carX < target) webRight = true;
    if (target >= 0 && carX > target) webLeft = true;
    unsigned long sc = score; lastInput = g_ms; loop(); if (score < sc) crashes++;
  }
  printf("dodge crashes %d score %lu\n", crashes, score); assert(crashes == 0 && score >= 10);
  printf("dodge autopilot score %lu\n", score);
  startGame(2); carX = 0; for (int x = 0; x < WIDTH; x++) grid[HEIGHT - 1][x] = 4; loop(); assert(score == 0 && grid[HEIGHT - 1][0] == 0);  // crash restarts

  // bricks: ball waits on paddle, launch, paddle bounces, bricks break
  startGame(3); loop(); assert(!ballLaunched && ballX == padX + 1);
  key('u'); assert(ballLaunched);
  // autopilot paddle under the ball: never lose, keeps breaking bricks
  unsigned long best = 0; int aim = 0, rounds = 0;
  for (int t = 0; t < 60000; t++) {
    if (ballDy > 0 && ballY < 8) aim = (int)random(3) - 1;  // like a player: hit with different parts of the paddle
    int want = constrain(ballX - 1 + aim, 0, WIDTH - 3);
    if (padX < want) webRight = true;
    if (padX > want) webLeft = true;
    if (ballSpeed < 160 && !rounds) rounds = 1;
    lastInput = g_ms; loop(); assert(ballX >= 0 && ballX < WIDTH && ballY >= 0 && ballY < HEIGHT);
    if (!ballLaunched) webUp = true;
    best = max(best, score);
  }
  printf("bricks autopilot best %lu\n", best);
  assert(best >= 15);
  // center hit goes straight up, so x + y parity changes and every brick can be reached
  startGame(3); key('u'); padX = 2; ballX = 3; ballY = HEIGHT - 2; ballDx = 1; ballDy = 1;
  ballX = 2; ballMoved = 0; loop(); assert(ballDx == 0 && ballDy == -1);  // hit = 2 + 1 - 2 = 1: middle
  int parity = (ballX + ballY) % 2; ballMoved = 0; loop(); assert((ballX + ballY) % 2 != parity);
  startGame(3); key('u'); padX = 0; ballX = 7; ballY = HEIGHT - 2; ballDx = 1; ballDy = 1; ballMoved = 0; loop();
  assert(!ballLaunched && score == 0);  // missed -> restart

  // memory: wait for the show, repeat it right -> next round; wrong -> restart
  startGame(4); assert(seqLen == 1);
  const char K[] = {'u', 'l', 'r', 'd'};
  for (int round = 1; round <= 5; round++) {
    run(1000 + 550 * seqLen + 50); assert(!memShowing);
    for (int i = 0; i < seqLen; i++) key(K[seq[i]]);
    assert(score == (unsigned long)round && seqLen == round + 1);
  }
  run(1000 + 550 * seqLen + 50); assert(!memShowing);
  key(K[(seq[0] + 1) % 4]); assert(seqLen == 1 && score == 0 && hiscore[4] == 5);
  // presses during the show are ignored
  startGame(4); key(K[(seq[0] + 1) % 4]); assert(seqLen == 1 && memShowing);

  // menu + games never draw outside 8x16 (setPixelColor stub asserts), /state names the game
  startGame(2); handleState(); assert(g_body.find("\"n\":\"dodge\"") != std::string::npos);
  game = -1; for (int i = 0; i < NR_GAMES; i++) { menuPick = i; loop(); }
  // beeps don't queue: a new one sounds at once, and each ends on time
  beep(440, 400); assert(g_freq == 440);
  g_ms += 50; beep(880, 150); assert(g_freq == 880);
  g_ms += 100; beepUpdate(); assert(g_freq == 880);
  g_ms += 60; beepUpdate(); assert(g_freq == 0);
  puts("all game tests passed");
}
