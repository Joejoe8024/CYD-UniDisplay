/********************************************************************************
//  new Formula-1 schedule by JN at 07/2026
//  
//  see https://f1api.dev/docs for details 
*/


#include "../net/formula1.h"
#include "../net/holidays.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#define ARDUINOJSON_DECODE_UNICODE 1
#include <time.h>

#include "../util/constants.h"

extern bool namedayValid;
extern String todayNameday;
extern String todayHoliday;

// ── Public functions ───────────────────────────────────────────────────────

String fetchTodayF1race() {
    todayNameday = "";
    todayHoliday = "";
    String result = "";
    String Racename = "* No F1 race today *";
    String Racecircuitname = ". . .";
    String Racecountryname = "***";
    String Racedate = "***";
    bool found = false;
    HTTPClient http;
    String today = todayDateString();
    //String today = "2026-09-26"; // Hardcoded test date
    //Serial.printf( "[FORMULA1] Today date string: %s\n", today.c_str() );

    time_t     now      = time( nullptr );
    struct tm *timeinfo = localtime( &now );
    if ( !timeinfo ) {
        return "";
    }
    int year = timeinfo->tm_year + 1900;
    String listUrl = "https://f1api.dev/api/" + String( year );

    http.setTimeout( HTTP_TIMEOUT_STANDARD );
    http.begin( listUrl );
    int listCode = http.GET();

    if ( listCode != 200 ) {
        http.end();
        return "* bad request *";
    }

    String payload = http.getString();
    http.end();
    //Serial.printf( "[FORMULA1] Year F1 schedule: %s\n", payload.c_str() );
    
    JsonDocument doc;
    DeserializationError err = deserializeJson( doc, payload );
    if ( err ) {
        return "";
    }

// the outcomented structure is for complete JSON document with all fields
/*
const char* api = doc["api"]; // "https://f1api.dev"
const char* url = doc["url"]; // "https://f1api.dev/api/2026"
int limit = doc["limit"]; // 30
int offset = doc["offset"]; // 0
int total = doc["total"]; // 22
int season = doc["season"]; // 2026

JsonObject championship = doc["championship"];
const char* championship_championshipId = championship["championshipId"]; // "f1_2026"
const char* championship_championshipName = championship["championshipName"]; // "2026 Formula 1 World ...
const char* championship_url = championship["url"];
int championship_year = championship["year"]; // 2026

*/

for (JsonObject race : doc["races"].as<JsonArray>()) {

//  const char* race_raceId = race["raceId"]; // "australian_2026", "chinese_2026", "japanese_2026", ...
//  const char* race_championshipId = race["championshipId"]; // "f1_2026", "f1_2026", "f1_2026", "f1_2026", ...
  String race_raceName = race["raceName"]; // "Formula 1 Qatar Airways Australian Grand Prix", ...

  for (JsonPair race_schedule_item : race["schedule"].as<JsonObject>()) {
    String race_schedule_item_key = race_schedule_item.key().c_str(); // "race", "qualy", "fp1", "fp2", ...
    String race_schedule_item_value_date = race_schedule_item.value()["date"]; // "2026-03-08", ...
    const char* race_schedule_item_value_time = race_schedule_item.value()["time"]; // "04:00:00Z", ...

    if (race_schedule_item_key == "race" && today == race_schedule_item_value_date) {
        found = true;
        Racedate = race_schedule_item_value_date;
        Serial.printf(" %s %s\n", race_schedule_item_key, Racedate);
        break;  // escape first for-loop if date match found
        }
  }


/*
  int race_laps = race["laps"]; // 58, 56, 53, 57, 70, 78, 66, 71, 52, 44, 70, 72, 53, 0, 51, 62, 56, 71, ...
  int race_round = race["round"]; // 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, ...
  const char* race_url = race["url"]; // "https://en.wikipedia.org/wiki/2026_Australian_Grand_Prix", ...
  
  JsonObject race_fast_lap = race["fast_lap"];
  const char* race_fast_lap_fast_lap = race_fast_lap["fast_lap"]; // "1:22.091", "1:35.275", "1:32.432", ...
  const char* race_fast_lap_fast_lap_driver_id = race_fast_lap["fast_lap_driver_id"]; // "max_verstappen", ...
  const char* race_fast_lap_fast_lap_team_id = race_fast_lap["fast_lap_team_id"]; // "red_bull", ...
*/


  JsonObject race_circuit = race["circuit"];
  //const char* race_circuit_circuitId = race_circuit["circuitId"]; // "albert_park", "shanghai", "suzuka", ...
  String race_circuit_circuitName = race_circuit["circuitName"]; // "Albert Park Circuit", "Shanghai ...
  String race_circuit_country = race_circuit["country"]; // "Australia", "China", "Japan", "United ...
  
/*
  const char* race_circuit_city = race_circuit["city"]; // "Melbourne", "Shanghai", "Suzuka", "Miami", ...
  const char* race_circuit_circuitLength = race_circuit["circuitLength"]; // "5278km", "5451km", "5807km", ...
  const char* race_circuit_lapRecord = race_circuit["lapRecord"]; // "1:19:813", "1:32:238", "1:30:983", ...
  int race_circuit_firstParticipationYear = race_circuit["firstParticipationYear"]; // 1996, 2004, 1987, ...
  int race_circuit_corners = race_circuit["corners"]; // 14, 16, 18, 19, 14, 19, 14, 10, 18, 20, 14, 14, ...
  const char* race_circuit_fastestLapDriverId = race_circuit["fastestLapDriverId"]; // "leclerc", ...
  const char* race_circuit_fastestLapTeamId = race_circuit["fastestLapTeamId"]; // "ferrari", "ferrari", ...
  int race_circuit_fastestLapYear = race_circuit["fastestLapYear"]; // 2024, 2004, 2019, 2023, 2019, 2021, ...
  const char* race_circuit_url = race_circuit["url"];

  JsonObject race_winner = race["winner"];
  const char* race_winner_driverId = race_winner["driverId"]; // "russell", "antonelli", "antonelli", ...
  const char* race_winner_name = race_winner["name"]; // "George", "Andrea", "Andrea", "Andrea", "Andrea", ...
  const char* race_winner_surname = race_winner["surname"]; // "Russell", "Kimi Antonelli", "Kimi ...
  const char* race_winner_country = race_winner["country"]; // "Great Britain", "Italy", "Italy", "Italy", ...
  const char* race_winner_birthday = race_winner["birthday"]; // "15/02/1998", "2006-08-25", "2006-08-25", ...
  int race_winner_number = race_winner["number"]; // 63, 12, 12, 12, 12, 12, 44, 63, 16, 12, 0, 0, 0, 0, ...
  const char* race_winner_shortName = race_winner["shortName"]; // "RUS", "ANT", "ANT", "ANT", "ANT", ...
  const char* race_winner_url = race_winner["url"];

  JsonObject race_teamWinner = race["teamWinner"];
  const char* race_teamWinner_teamId = race_teamWinner["teamId"]; // "mercedes", "mercedes", "mercedes", ...
  const char* race_teamWinner_teamName = race_teamWinner["teamName"]; // "Mercedes Formula 1 Team", ...
  const char* race_teamWinner_country = race_teamWinner["country"]; // "Germany", "Germany", "Germany", ...
  int race_teamWinner_firstAppearance = race_teamWinner["firstAppearance"]; // 1954, 1954, 1954, 1954, ...
  int race_teamWinner_constructorsChampionships = race_teamWinner["constructorsChampionships"]; // 8, 8, ...
  int race_teamWinner_driversChampionships = race_teamWinner["driversChampionships"]; // 9, 9, 9, 9, 9, 9, ...
  const char* race_teamWinner_url = race_teamWinner["url"];

*/

if (found) {
    Racename = race_raceName.substring(10,42);      // skip 'Formula 1' in race name and limit to 33 characters
    Racecircuitname = race_circuit_circuitName + "-" + race_circuit_country;
    //Serial.printf( "%s # %s\n", Racedate, Racename.c_str());
    //Serial.printf( "%s # %s\n", Racecircuitname.c_str(), Racecountryname.c_str());
    todayNameday = Racecircuitname.substring(0,32);  // Truncate to 33 characters because of limited display width
    result = Racename.c_str();
    //Serial.printf( "todayNameday: %s\n", todayNameday.c_str());
    break;  // escape second for-loop if match found
    }
}
    return result;
}

void handleRacedayUpdate() {

    // Gate on WiFi and valid time
    if ( WiFi.status() != WL_CONNECTED ) {
        return;
        }
    time_t     now      = time( nullptr );
    struct tm *timeinfo = localtime( &now );
    if ( !timeinfo ) {
        return;
        }
    if ( timeinfo->tm_year < 125 ) {
        return;    // time not synced (<2025)
        }

    String name = fetchTodayF1race();
    todayHoliday = name;
    //Serial.printf( "+todayNameday: %s\n", todayNameday.c_str());
    //Serial.printf( "+todayHoliday: %s\n", todayHoliday.c_str());
    namedayValid = true;    // enables display of F1 data after reboot/change of country selection
    /*
    if (!found) {
        todayNameday = "";  // clear circuit name; clear both lines (nothing is displayed)
        todayHoliday = "";  // clear ´No F1 race today´ message
    }
    */
}

