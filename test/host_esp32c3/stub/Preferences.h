#pragma once
#include <map>
extern std::map<std::string, unsigned long> g_nvs;
struct Preferences { void begin(const char*) {} unsigned long getULong(const char* k, unsigned long d) { return g_nvs.count(k) ? g_nvs[k] : d; }
  void putULong(const char* k, unsigned long v) { g_nvs[k] = v; } };
