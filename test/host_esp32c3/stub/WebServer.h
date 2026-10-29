#pragma once
#include <functional>
extern String g_arg, g_body; extern int g_code;
struct WebServer { WebServer(int) {} void on(const char*, std::function<void()>) {} void begin() {} void handleClient() {}
  String arg(const char*) { return g_arg; } void send(int c, const char* = "", const String& b = "") { g_code = c; g_body = b; } };
