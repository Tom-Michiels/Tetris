#pragma once
#define ESP_GPIO_WAKEUP_GPIO_LOW 0
inline void esp_deep_sleep_enable_gpio_wakeup(unsigned long long, int) {}
inline void esp_deep_sleep_start() { g_sleeps++; }
