#pragma once
#include <cstring>
struct Adafruit_NeoPixel { uint32_t px[256]; Adafruit_NeoPixel(int, int, int) { clear(); } void begin() {} void clear() { memset(px, 0, sizeof px); }
  void show() { delay(4); } void setPixelColor(int i, int r, int g, int b) { assert(i >= 0 && i < 128); px[i] = (r << 16) | (g << 8) | b; }
  uint32_t getPixelColor(int i) { return px[i]; } };
