/********************************************************************************
//  adapted by JN at 07/2026
//  for V4 Nager API (released in June 2026)
//  see https://date.nager.at/scalar for details 
 
*/


#include "../net/holidays.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <time.h>

#include "../util/constants.h"
#include "../net/location.h"

#define ARDUINOJSON_DECODE_UNICODE 1    // support for UTF-8 characterset (extended)

// ── Globals defined here ───────────────────────────────────────────────────

int    lastHolidayDay  = -1;
bool   holidayValid    = false;

// ── External globals owned by main.cpp ────────────────────────────────────
extern String lookupISOCode;        // ISO 3166-1 alpha-2, set by lookupCountryEmbedded/REST
extern String selectedCountry;      // Used for one-time ISO fallback when NVS has no isoCode
extern Preferences prefs;           // Shared NVS handle defined in main.cpp
extern int    lookupGmtOffset;      // UTC offset in seconds (from timezone detect)
extern bool   forceClockRedraw;

extern String NamedayISOCode;       // ISO code for Name-/Holiday lookup     
//extern String lastCheckHoliDate;    // lookup once a day/midnight

// ── Internal helpers ───────────────────────────────────────────────────────

// Build "YYYY-MM-DD" for today in local time.
// When HOLIDAY_TEST_DATE is defined (e.g. "2026-12-25") it is returned
// unconditionally — allows testing the full holiday path on any day.

String todayDateString() {
    #ifdef HOLIDAY_TEST_DATE
        return String( HOLIDAY_TEST_DATE );
    #else
        time_t     now      = time( nullptr );
        struct tm *timeinfo = localtime( &now );
        if ( !timeinfo ) {
            return "";
        }
        char buf[ 11 ];
        snprintf( buf, sizeof( buf ), "%04d-%02d-%02d",
                timeinfo->tm_year + 1900,
                timeinfo->tm_mon + 1,
                timeinfo->tm_mday );
        return String( buf );
    #endif
}

// ── Public functions ───────────────────────────────────────────────────────

String fetchTodayHoliday( const String &isoCode, int utcOffsetHours ) {
    if ( isoCode.isEmpty() ) {
        log_d( "[HOLIDAY] No ISO code — skipping" );
        return "isoCodeEmpty";
    }

    HTTPClient http;

    #ifndef HOLIDAY_TEST_DATE
    // ── Step 1: Is today a public holiday? ────────────────────────────────
    // Returns 200=yes, 204=no, 404=unsupported country.
    // Skipped when HOLIDAY_TEST_DATE is defined because the API checks the
    // server's real date, not the injected test date.

//Serial.printf("isocode : %s UTCoffset : %1d\n", isoCode, utcOffsetHours);

    http.setTimeout( HTTP_TIMEOUT_SHORT );
    String checkUrl = "https://date.nager.at/api/v4/IotHolidays/" + isoCode +
                      "/IsToday/" + String( utcOffsetHours );
    log_d( "[HOLIDAY] Checking: %s", checkUrl.c_str() );

    http.begin( checkUrl );
    int checkCode = http.GET();
    http.end();
//Serial.printf("code : %d\n", checkCode);
    log_d( "[HOLIDAY] IsTodayPublicHoliday → %d", checkCode );

    if ( checkCode == 204 ) {
        return "";   // Not a holiday — regular day
    }

    if ( checkCode != 200 ) {
        log_w( "[HOLIDAY] Unexpected status %d for country %s", checkCode, isoCode.c_str() );
        return "* Unexpected Status *";
    }    
       
    
    #else
        log_i( "[HOLIDAY] TEST MODE — date injected: %s", HOLIDAY_TEST_DATE );
    #endif

    // ── Step 2: Fetch the year list and find today's holiday name ────────────
    String today = todayDateString();
//Serial.printf( "todaystring %s (%d)\n", today, today.length());
    #ifdef HOLIDAY_TEST_DATE
        // Extract year directly from the injected date string (e.g. "2026-12-25" → 2026)
        String year = String (HOLIDAY_TEST_DATE).substring( 0, 4 );
    #else
        String year = today.substring( 0, 4 );
    #endif

    String listUrl = "https://date.nager.at/api/v4/Holidays/" + isoCode + "/" + year;
    log_d( "[HOLIDAY] Fetching year list: %s", listUrl.c_str() );

    http.setTimeout( HTTP_TIMEOUT_STANDARD );
    http.begin( listUrl );
    int listCode = http.GET();

    if ( listCode != 200 ) {
        log_w( "[HOLIDAY] Year list HTTP %d", listCode );
        http.end();
        return "";
    }

    String payload = http.getString();
    http.end();
    //Serial.printf("[HOLIDAY] years holidaylist : %s\n", payload.c_str());
    // ── Step 3: Parse + find today's entry ────────────────────────────────
    // Prefer global=true entries; accept non-global as fallback
    JsonDocument doc;
    DeserializationError err = deserializeJson( doc, payload );
    if ( err ) {
        log_e( "[HOLIDAY] JSON error: %s", err.c_str() );
        return "";
    }
    //String todayCStr = "2026-08-15"; // Hardcoded test date
    //String todayCStr = today.c_str();
    String globalMatch   = "";
    // String provincialMatch = ""; //not needed -> only national holidays gets displayed

    for (JsonObject item : doc.as<JsonArray>()) {
        String Jdate = item["date"].as<String>();
        const char* Jname = item["name"];
        bool nationalHoliday = item["nationalHoliday"].as<bool>();
    //Serial.printf( "[HOLIDAY] Checking date: %s (%d) - %s\n", Jdate, Jdate.length(), Jname );
                if ( Jdate != today || nationalHoliday != true) {
            continue;
        }
        //Jname = item["name"];
        //bool nationalHoliday = item["nationalHoliday"].as<bool>();
        globalMatch = Jname;
    //String result = Jname.substring(0,27); // Truncate to 27 characters for limited display width;    
    //Serial.printf( "[HOLIDAY] picked holiday : %s  %d\n", globalMatch, nationalHoliday );
    }

    String result = globalMatch.substring(0,32); // Truncate to 33 characters because of limited display width;
    
    if ( result.isEmpty() ) {
        // IsTodayPublicHoliday returned 200 but we couldn't find the entry —
        // shouldn't happen but handle gracefully.
        log_w( "[HOLIDAY] Holiday flagged but no matching date found in list" );
    }
    else {
        log_i( "[HOLIDAY] Today is: %s", result.c_str() );
    }

    return result;
}

void handleHolidayUpdate() {
    time_t     now      = time( nullptr );
    struct tm *timeinfo = localtime( &now );
    if ( !timeinfo ) {
        return;
    }
    // Gate on WiFi and valid time
    if ( WiFi.status() != WL_CONNECTED || ( timeinfo->tm_year < 125 )) {
        return;
    }

//    if ( lastCheckHoliDate.isEmpty() || lastCheckHoliDate != todayDateString() ) {
/*
    // JN : not relevant anymore - holiday and nameday lookup depends on NamedayISOCode only (independent from lookupISOCode)
    
    // ISO country code set by lookupCountryEmbedded() or lookupCountryRESTAPI().
    // On first boot after firmware update the NVS key may be absent — try once
    // via REST while WiFi is already connected rather than silently skipping.
    if ( lookupISOCode.isEmpty() && !selectedCountry.isEmpty() ) {
        log_i( "[HOLIDAY] lookupISOCode empty — REST fallback for '%s'", selectedCountry.c_str() );
        
        lookupCountryGeonames( selectedCountry );
        // Persist so subsequent boots don't repeat the REST call
        if ( !lookupISOCode.isEmpty() ) {
            prefs.begin( "sys", false );
            prefs.putString( "isoCode", lookupISOCode );
            prefs.end();
            log_i( "[HOLIDAY] Persistend isoCode '%s' to NVS", lookupISOCode.c_str() );
        }
    }
    String isoCode = lookupISOCode;
*/
    // UTC offset in whole hours
    int offsetHours = lookupGmtOffset / 3600;
    String Holiday = fetchTodayHoliday( NamedayISOCode, offsetHours );
    todayHoliday = Holiday.c_str();
    //Serial.printf( "[handleHolidayUpdateReturn] holidayname : %s  %s\n", todayHoliday, NamedayISOCode);
    //lastCheckHoliDate = todayDateString();
    holidayValid = true;
    forceClockRedraw = true;
    log_d( "[HOLIDAY] Updated: '%s' (valid=%d)", todayHoliday, holidayValid );
    /*
    if ( name != todayHoliday ) {
        todayHoliday = name;
        holidayValid = !name.isEmpty();
        forceClockRedraw = true;
        log_d( "[HOLIDAY] Updated: '%s' (valid=%d)", todayHoliday.c_str(), holidayValid );
    }
    */
}
//}
