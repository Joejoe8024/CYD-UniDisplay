#pragma once
#include <Arduino.h>

// ================= NAMEDAY MODULE =================

// ── State variables (defined in nameday.cpp) ──────────────────────────────────
extern String todayNameday;   // Name to display, e.g. "Josef"
extern int    lastNamedayDay;  // tm_mday of the last update  (-1 = never)
extern int    lastNamedayHour; // tm_hour of the last update  (-1 = never)
extern bool   namedayValid;    // true when todayNameday is meaningful

// ── Functions ─────────────────────────────────────────────────────────────────

// Return nameday string for the given day/month via REST-api lookup
// Returns "--" for invalid dates or no nameday found
String getNamedayForDate( int day, int month );

// Call once per loop iteration (or on demand) to refresh todayNameday.
// Sets forceClockRedraw = true when the name changes.
void handleNamedayUpdate();
