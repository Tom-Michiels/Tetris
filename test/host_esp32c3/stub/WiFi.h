#pragma once
#define WIFI_STA 1
#define WL_CONNECTED 3
struct { void mode(int) {} void setHostname(const char*) {} void begin(const char* = 0, const char* = 0) {} void disconnect(bool, bool) {} int status() { return 0; } const char* localIP() { return ""; } } static WiFi;
