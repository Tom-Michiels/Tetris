#pragma once
#include <cstdint>
#include <cstdlib>
#include <string>
#include <algorithm>
#include <cassert>
using std::max; using std::min;
#define D1 3
#define D2 4
#define D3 5
#define D4 6
#define D5 7
#define D10 10
#define INPUT_PULLUP 5
#define OUTPUT 3
#define NEO_GRB 0
#define NEO_KHZ800 0
extern unsigned long g_ms; extern int g_pin[40];
inline unsigned long millis() { return g_ms; }
inline void delay(unsigned long d) { g_ms += d; }
inline int digitalRead(int p) { return g_pin[p]; }
inline void pinMode(int, int) {}
inline void tone(int, int, int) {}
inline long random(long n) { return rand() % n; }
struct String : std::string { String(const char* s = "") : std::string(s) {} String(unsigned long v) : std::string(std::to_string(v)) {} String(int v) : std::string(std::to_string(v)) {}
  String(const std::string& s) : std::string(s) {} };
struct { void begin(int) {} template<class T> void print(T) {} template<class T> void println(T) {} } static Serial;
#define A0 2
extern int g_sleeps;
inline uint32_t analogReadMilliVolts(int) { return 1900; }
#define constrain(v, lo, hi) ((v) < (lo) ? (lo) : (v) > (hi) ? (hi) : (v))
inline long map(long x, long a, long b, long c, long d) { return (x - a) * (d - c) / (b - a) + c; }
extern unsigned g_freq;
inline void ledcSetup(int, int, int) {} inline void ledcAttachPin(int, int) {} inline void ledcWriteTone(int, unsigned f) { g_freq = f; }
