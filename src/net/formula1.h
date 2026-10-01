#pragma once
#include <Arduino.h>


// ── State variables (defined in holidays.cpp) ─────────────────────────────
extern String todayNameday;    // used for racename display
extern String todayHoliday;    // used for circuitname and race country

// ── Functions ──────────────────────────────────────────────────────────────
// Performs F1 api requests; call from a WiFi-connected context only.
String fetchTodayF1race();

// Updates once per day; sets forceClockRedraw = true when value changes.
void handleRacedayUpdate();
