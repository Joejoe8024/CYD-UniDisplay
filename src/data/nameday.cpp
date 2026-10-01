#include "../data/nameday.h"
#include "../net/holidays.h"
#include <WiFi.h>
#include <time.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include "../util/constants.h"

#define ARDUINOJSON_DECODE_UNICODE 1    // support for UTF-8 characterset (extended)

// ── Globals defined here ───────────────────────────────────────────────────────

int    lastNamedayDay  = -1;
int    lastNamedayHour = -1;
bool   namedayValid    = false;

// ── External globals owned by main.cpp ────────────────────────────────────────
extern TFT_eSPI tft;
extern String selectedCountry;
extern String namedays;
extern String NamedayISOCode;
extern String lastNamedayISO;
extern bool   forceClockRedraw;
//extern String lastCheckNameDate;    // lookup once a day/midnight
extern int lookupGmtOffset;         // UTC offset in seconds (from timezone detect)

// ── Public functions ───────────────────────────────────────────────────────────
// Get nameday for today using the Abalin API
String getNamedayForDate( int day, int month ) {
    if ( month < 1 || month > 12 || day < 1 || day > 31 ) {
        return "--";
    }
    // UTC offset in whole hours
    //int offsetHours = lookupGmtOffset / 3600;
    HTTPClient http;
    http.setTimeout( HTTP_TIMEOUT_STANDARD );
    String listUrl = "https://nameday.abalin.net/api/V2/date?day="+String(day)+"&month="+String(month);
    log_d( "[NAMEDAY] Checking: %s", listUrl.c_str() );
    http.begin( listUrl );
    int listCode = http.GET();
    if ( listCode != 200 ) {
        log_w( "[NAMEDAY] Fetched nameday-list HTTP %d", listCode );
        http.end();
        return "* bad return code *";
    }
    String payload = http.getString();
    http.end();
    //Serial.printf("[NAMEDAY] today list : %s\n", payload.c_str());
    // ── next step : Parse + pick nameday for country's entry ────────────────────────────────
    JsonDocument doc;
    DeserializationError err = deserializeJson( doc, payload );
    if ( err ) {
        log_e( "[NAMEDAY] JSON error: %s", err.c_str() );
        return "??";
    }
    JsonObject data = doc["data"];
    const char* data_vari = data[NamedayISOCode.c_str()];
    namedays = data_vari ? String(data_vari) : "--";
    namedays = namedays.substring(0,32); // Truncate to 33 characters because of limited display width
    return namedays;
}

void handleNamedayUpdate() {
    //namedayValid = false;
    //todayNameday = "--";
    time_t     now      = time( nullptr );
    struct tm *timeinfo = localtime( &now );
    if ( !timeinfo ) {
        return;
    }
    // Gate on WiFi and valid time
    if ( WiFi.status() != WL_CONNECTED || ( timeinfo->tm_year < 125 )) {
        return;
    }
    
    //if ( lastCheckNameDate.isEmpty() || lastCheckNameDate != todayDateString() ) {
    int today = timeinfo->tm_mday;
    int month = timeinfo->tm_mon + 1;
    log_d( "[NAMEDAY] Getting nameday for day %d.%d", today, month );
    //tft.setTextColor( TFT_BLACK, TFT_RED );
    //tft.drawString( "Fetch country namedays", 40, 120, 3 );
    todayNameday = getNamedayForDate( today, month );
    //lastCheckNameDate = todayDateString();
    namedayValid = ( todayNameday != "--" );
    //namedayValid = true;
    if ( namedayValid ) {
        log_d( "[NAMEDAY] SUCCESS: %s", todayNameday.c_str() );
        forceClockRedraw = true;
    }
    else {
        log_d( "[NAMEDAY] No nameday for this date" );
        }
}

