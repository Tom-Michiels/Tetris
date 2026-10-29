#pragma once
extern int g_portal;
struct WiFiManager { void setConfigPortalBlocking(bool) {} void startConfigPortal(const char*) { g_portal++; } void process() {} void stopConfigPortal() {} };
