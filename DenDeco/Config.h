#pragma once
#include <stdint.h>
namespace Config {
constexpr uint8_t POWER_PIN=0, MINUS_PIN=1, PLUS_PIN=3, MODE_PIN=4, LED_PIN=5;
constexpr uint16_t LED_COUNT=20, HEIGHT_COUNT=10;
constexpr uint32_t DEBOUNCE_MS=30, TOUCH_STARTUP_MS=1000, REPEAT_START_MS=600;
constexpr uint32_t REPEAT_MS=50, LONG_HOLD_MS=2000, FADE_MS=800;
constexpr uint32_t SAVE_IDLE_MS=10000, SAVE_RETRY_MS=5000, WIFI_FALLBACK_MS=30000;
constexpr uint32_t WIFI_RESET_HOLD_MS=5000;
constexpr float GAMMA=2.2f;
// Low-level starting point: sleeping orange remains approximately RGB(6,2,1).
// Calibrate on the physical lamp. Raising the cap requires power-path measurements.
constexpr float MIN_OUTPUT=6.0f/255.0f;
constexpr uint8_t MIN_CAP_PERCENT=3, MAX_CAP_PERCENT=30;
constexpr uint8_t SLEEP_R=255, SLEEP_G=96, SLEEP_B=24;
static_assert(LED_COUNT==2*HEIGHT_COUNT,"Update the height mapping when changing LED count");
}
