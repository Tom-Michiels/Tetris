#pragma once
#include <functional>
struct { void setHostname(const char*) {} void setPassword(const char*) {} void begin() {} void handle() {}
  std::function<void()> start; std::function<void(unsigned, unsigned)> prog;
  void onStart(std::function<void()> f) { start = f; } void onProgress(std::function<void(unsigned, unsigned)> f) { prog = f; } } static ArduinoOTA;
