#include "clock_face.h"
#include "theme.h"
#include "icons.h"
#include "../app/sensors.h"

#include <TFT_eSPI.h>
// #include "DSEG7Bold15pt7b.h"
#include "DejaM10.h"
#include <time.h>

#include "../data/app_state.h"
#include "../data/nameday.h"
#include "../net/holidays.h"
#include "../util/moon.h"
#include "../util/constants.h"
#include "../net/weather_api.h"

// ---------------------------------------------------------------------------
// Externs – defined in main.cpp
// ---------------------------------------------------------------------------
extern TFT_eSPI tft;

// Clock layout
extern const int clockX;
extern const int clockY;
extern const int radius;
extern int       lastHour;
extern int       lastMin;
extern int       lastSec;
extern bool      forceClockRedraw;

// Display/format state
extern bool isDigitalClock;
extern bool is12hFormat;
extern bool showDigitalSeconds;
extern bool isWhiteTheme;
extern int  themeMode;
extern bool ShowNameday;
extern String NamedayISOCode;
extern String todayHoliday;
extern String todayNameday;

// City/region
extern String cityName;
extern String selectedCountry;

// Weather data
extern bool  initialWeatherFetched;
extern int   weatherCode;
extern float currentTemp;
extern int   currentHumidity;
extern float currentWindSpeed;
extern int   currentWindDirection;
extern int   currentPressure;
extern bool  weatherUnitF;
extern bool  weatherUnitMph;
extern bool  weatherUnitInHg;
extern String sunriseTime;
extern String sunsetTime;
extern ForecastData forecast[ 2 ];
extern String forecastDay1Name;
extern String forecastDay2Name;
extern int    moonPhaseVal;

extern float temp_sensor;
extern float humi_sensor;
extern int32_t aqi_sensor;
extern int32_t eco2_sensor;
extern int32_t tvoc_sensor;


// Bitmap icons (defined in main.cpp)
extern const unsigned char icon_sunrise[];
extern const unsigned char icon_sunset[];

// DEGTORAD
static constexpr float DEGTORAD_CF = ( float )( PI / 180.0 );

// ---------------------------------------------------------------------------
// Clock-face sprite  — 140×140 px, rendered off-screen then pushed atomically
// so no intermediate state (tick erase → ticks redraw → hand redraw) is visible.
// ---------------------------------------------------------------------------
static TFT_eSprite clockSprite( &tft );
static bool        spriteCreated = false;
static int         spriteX = 0, spriteY = 0;  // top-left of sprite on screen
static int         sCX = 0,     sCY = 0;       // clock centre in sprite coords

static void createClockSprite() {
    if ( spriteCreated ) {
        return;
    }
    int sz  = ( radius + 3 ) * 2;          // e.g. 140 for radius=67
    spriteX = clockX - ( radius + 3 );     // 160
    spriteY = clockY - ( radius + 3 );     // 15
    sCX     = radius + 3;                  // 70  (centre inside sprite)
    sCY     = radius + 3;                  // 70
    clockSprite.setColorDepth( 16 );
    clockSprite.createSprite( sz, sz );
    spriteCreated = true;
}

void drawClockStatic() {
    if ( isDigitalClock ) {
        return;    // Nothing to draw in digital mode
    }

    // Draw minute and hour tick marks
    for ( int i = 0; i < 60; i++ ) {
        float ang = ( i * 6 - 90 ) * DEGTORAD_CF;
        int r1 = ( i % 5 == 0 ) ? ( radius - 10 ) : ( radius - 5 );
        uint16_t color;
        if ( i % 5 == 0 ) {
            color = getTextColor();
        }
        else {
            color = ( themeMode == THEME_YELLOW ) ? 0x0010 : TFT_DARKGREY;
        }
        tft.drawLine( clockX + cos( ang ) * radius, clockY + sin( ang ) * radius, clockX + cos( ang ) * r1, clockY + sin( ang ) * r1, color );
    }

    // Draw numbers 1-12
    tft.setTextColor( getTextColor() );
    tft.setTextDatum( MC_DATUM );
    tft.setFreeFont( &FreeSans9pt7b );

    for ( int h = 1; h <= 12; h++ ) {
        float angle = ( h * 30 - 90 ) * DEGTORAD_CF;
        int x = clockX + cos( angle ) * ( radius - 22 );
        int y = clockY + sin( angle ) * ( radius - 22 );
        tft.drawString( String( h ), x, y );
    }
}

void drawClockFace() {
    tft.fillScreen( getBgColor() );
    if ( !isDigitalClock ) {
        createClockSprite();   // allocate sprite (no-op if already done)
        tft.drawCircle( clockX, clockY, radius + 2, getTextColor() );
        drawClockStatic();
    }
    forceClockRedraw = true;
}

void drawSensorData( float temp_sensor, float humi_sensor, int32_t aqi_sensor, int32_t eco2_sensor, int32_t tvoc_sensor ) {
    uint16_t bg = getBgColor();
    uint16_t txt = getTextColor();
    uint16_t colors[]  = {TFT_GREEN, TFT_YELLOW, TFT_ORANGE, TFT_MAGENTA, TFT_RED}; // colors for AQI
    tft.setFreeFont( NULL );
    tft.setTextFont( 2 );
    tft.setTextColor( txt, bg ); 
    tft.setCursor( 160, 168 );
    GetSensorData();  // Get latest sensor readings
    float dispTemp_sensor = weatherUnitF ? ( temp_sensor * 9.0 / 5.0 + 32 ) : temp_sensor;
    if ( weatherUnitF ) {
        tft.printf( "Temp %.1f F  RH %.0f %%", dispTemp_sensor, humi_sensor );
      }
      else {
        tft.printf( "Temp %.1f C  RH %.0f %%", dispTemp_sensor, humi_sensor );
      }
    drawDegreeCircle( 227, 171, 1, TFT_WHITE );  
    tft.setCursor( 160, 188 );
    tft.printf( "eCO2 %4d ppm  ", eco2_sensor );
     tft.setCursor( 270, 188 );
    tft.setTextColor( colors [ aqi_sensor-1 ], bg );
    tft.printf( "AQI %1d", aqi_sensor );
    tft.setTextColor( txt, bg ); 
    tft.setCursor( 160, 208 );
    tft.printf( "TVOC %6d ppb  ", tvoc_sensor ); // Clear any leftover characters from previous value
}

void drawDateAndWeek( const struct tm *ti ) {
    uint16_t dateColor = getTextColor();
    uint16_t bgColor = getBgColor();
    if ( themeMode == THEME_YELLOW ) {
        dateColor = TFT_BLACK;
    }

    tft.setFreeFont( NULL );
    tft.setTextColor( dateColor, bgColor );
    tft.setTextDatum( MC_DATUM );

    tft.fillRect( 142, 66, 177, 102, bgColor );

    char dateBuf[ 30 ];
    strftime( dateBuf, sizeof( dateBuf ), "%B %d  %Y", ti );
    tft.drawString( String( dateBuf ), clockX, 80, 2 );

    int weekNum = 0;
    char weekBuf[ 20 ];
    strftime( weekBuf, sizeof( weekBuf ), "%V", ti );
    weekNum = atoi( weekBuf );

    const char *dayNames[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    String dayStr = String( dayNames[ ti->tm_wday ] );
    String weekStr = "Week " + String( weekNum ) + "  " + dayStr;
    tft.drawString( weekStr, clockX, 98, 2 );

    if ( cityName != "" ) {
        if ( themeMode == THEME_YELLOW ) {
            tft.setTextColor( 0x0220, bgColor ); // Dark green
        }
        else {
            tft.setTextColor( TFT_SKYBLUE, bgColor );
        }
        tft.drawString( cityName, clockX, 116, 2 );
    }
}

void drawNamedayAndHoliday() {
    if ( namedayValid && todayNameday != "--" && ShowNameday && isDigitalClock) {
    //Serial.printf("Nameday:%s valid:%d Shownameday:%d\n", todayNameday.c_str(), namedayValid, ShowNameday);
    //if ( namedayValid && ShowNameday ) {
        uint16_t namedayColor;
        if ( themeMode == THEME_YELLOW ) {
            namedayColor = 0x0220;
        }
        else {
            namedayColor = isWhiteTheme ? TFT_DARKGREEN : TFT_YELLOW;
        }
        tft.setTextDatum( MC_DATUM );
        tft.fillRoundRect( 305, 115, 14, 12, 1, TFT_NAVY );     // display selected country indicator
        tft.setTextColor( TFT_GREEN, TFT_NAVY );
        tft.setTextFont( 1 );
        tft.drawString( NamedayISOCode, 312, 122, 1 );
        
        // tft.setFreeFont( &TomThumb );
        //tft.setTextFont( 1 );
        tft.loadFont( DejaM10 );
        tft.setTextColor( namedayColor, getBgColor() );                  // display nameday
        tft.drawString( todayNameday.c_str(), clockX+1, 138, 1 );
        // tft.drawString( "AbcdefghijxAbcdefghijxAbcdefghijx", clockX-9, 138, 1 );
        //tft.fillRect( 132, 130, 187, 14, TFT_BLUE );
        //tft.fillRect( 132, 148, 187, 14, TFT_RED );
        //tft.drawString( "María más Ésto Černo Ñunez Güel", clockX+1, 138, 1 );
        //tft.drawString ( "Asunci\u00f3n, Estrella, Mar\u00eda, Paloma", clockX+1, 138, 1 );
        // tft.fillRoundRect( 122, 148, 197, 14, 1, TFT_BLUE ); 

        if (!todayHoliday.isEmpty()) {
            //tft.setTextColor( TFT_GREEN, bgColor );               
            // tft.setFreeFont( &TomThumb );
            // tft.setTextFont( 1 );
            
            tft.fillRoundRect( 137, 148, 182, 14, 1, TFT_BLUE ); 
            tft.setTextDatum( MC_DATUM );
            tft.setTextColor( TFT_YELLOW, TFT_BLUE );                
            tft.drawString( todayHoliday.c_str(), clockX+1, 156, 1 );   // display holiday (if any..)
            //tft.drawString( "AbcdefghijxAbcdefghijxAbcdefghijx", clockX+6, 155, 1 );
            }
        else {
           tft.fillRoundRect( 132, 148, 187, 14, 1, getBgColor() );      // clear holiday line if no holiday
            }
        tft.unloadFont();
        }
    else {
        // Clear the area nameday/holiday and country indicator if neither applies (e.g. change analog/digital display)
        tft.fillRect( 132, 130, 187, 32, getBgColor() );         // clear Name-/Holiday area
        tft.fillRoundRect( 305, 115, 14, 12, 1, getBgColor() );  // clear country indicator
    }
}

void drawDigitalClock( int h, int m, int s ) {
    uint16_t clockColor = getTextColor();
    uint16_t bgColor = getBgColor();

    if ( themeMode == THEME_BLUE ) {
        clockColor = TFT_WHITE;
    }
    if ( themeMode == THEME_YELLOW ) {
        clockColor = TFT_BLACK;
    }

    int displayH = h;
    bool isPM = false;

    if ( is12hFormat ) {  // Convert 24h to 12h format
        if ( displayH >= 12 ) {
            isPM = true;
            if ( displayH > 12 ) {
                displayH -= 12;
            }
        }
        else {
            if ( displayH == 0 ) {
                displayH = 12;
            }
        }
    }

    char timeStr[ 6 ];
    if ( is12hFormat ) {
        sprintf( timeStr, "%2d:%02d", displayH, m );   // no leading zero in 12h, but pad single-digit hours with space to keep the colon position consistent
    }
    else {
        sprintf( timeStr, "%02d:%02d", displayH, m ); // leading zero in 24h
    }

    // Compute font 7 metrics once — used for layout of all elements below.
    int fh7      = tft.fontHeight( 7 );
    int maxTimeW = tft.textWidth( "12:59", 7 );

    tft.setTextDatum( MR_DATUM ); // right justified digital clock face numbers
    tft.setTextColor( clockColor, bgColor );

    // Redraw HH:MM only when the string changes (or a full redraw is forced).
    static char prevTimeStr[ 6 ] = "";
    if ( forceClockRedraw || strcmp( timeStr, prevTimeStr ) != 0 ) {
        tft.fillRect( clockX -12 - maxTimeW / 2, clockY -45 - fh7 / 2, maxTimeW, fh7, bgColor );
        tft.drawString( timeStr, clockX+60, clockY-45, 7 );
        strncpy( prevTimeStr, timeStr, sizeof( prevTimeStr ) );
    }

    // Seconds in DSEG7 Bold 15pt (GFX free font) — only when showDigitalSeconds is true.
    // TFT_eSPI renders GFX font glyphs pixel-by-pixel, so setTextColor(fg, bg) fills
    // the background per-glyph — no separate fillRect needed, no blank-frame flicker.
    //tft.setFreeFont( &DSEG7Bold15pt7b );
    int secFontH = tft.fontHeight();
    // Centre seconds exactly between the bottom of HH:MM and the top of the date line
    // (date drawn at y=175, font 2, 16px → top of date = 175 - fontHeight(2)/2).
    int bottomHHMM = clockY + fh7 / 2;
    int topDate    = 175 - tft.fontHeight( 2 ) / 2;
    int secY       = ( bottomHHMM + topDate ) / 2;
    uint16_t bg = getBgColor();
    uint16_t txt = getTextColor();

    if ( showDigitalSeconds ) {
        tft.setTextFont( 4 );
        char secStr[ 3 ];
        sprintf( secStr, "%02d", s );
        tft.setTextColor( getSecHandColor(), bgColor );
        // tft.drawString( secStr, 308, 58 );
        tft.drawString( secStr, 320, 56 );
        // Serial.printf( "clockX= %4d clockY= %4d", clockX, clockY );
    }
    
    else if ( forceClockRedraw ) {
        // Clear the seconds area when hidden (only needed on forced redraws —
        // per-glyph bg fill keeps it clean during normal second ticks).
        // int secW = tft.textWidth( "00" );
        // tft.fillRect( clockX - secW / 2, secY - secFontH / 2, secW, secFontH, bgColor );
        tft.setTextFont( 4 );
        tft.setTextColor( getSecHandColor(), bgColor ); // set fg=bg to "erase" by overdrawing with bg color
        tft.drawString( "  ", 320, 56 );
    }
        tft.setTextFont( 0 );  // clear free font, restore default

    // AM/PM indicator — only in 12h mode digital clock face.
    // AM/PM indicator allways for analog clock face.
    // Indicator is now drawn as text "AM"/"PM" instead of a filled circle.
    // 
    // Fixed X: 4 px to the right of the widest time string ("12:59"), so the dot never
    // shifts when the display is only 3 digits wide.
    // AM circle: top of circle = top of main digits (clockY - fh7/2).
    // PM circle: bottom of circle = bottom of main digits (clockY + fh7/2).
    // Circles are only redrawn when isPM changes (or a full redraw is forced) to avoid
    // the every-second erase/redraw touching the HH:MM glyph area and causing flicker.
    const int circR = 5;
    // Clamp so the circle never clips the right screen edge when the font is wide.
    const int circX = min( clockX-10 + maxTimeW / 2 + circR + 4, ( int )tft.width() - circR - 1 );
    const int amY   = clockY -30 - fh7 / 2 + circR;
    const int pmY   = clockY -45 + fh7 / 2 - circR;
    static bool prevIsPM = !isPM;  // initialise to opposite so first call always draws
    if ( forceClockRedraw || isPM != prevIsPM ) {
        // tft.fillCircle( circX-10, amY, circR, bgColor );
        // tft.fillCircle( circX-10, pmY, circR, bgColor );
        tft.setTextFont( 2 );
            tft.setTextColor( TFT_ORANGE, bgColor );
            tft.drawString( "   ", circX+16, pmY-25 ); // Clear previous AM/PM indicator
        if ( is12hFormat ) {
            tft.setTextFont( 2 );
            tft.setTextColor( TFT_ORANGE, bgColor );
            tft.drawString( isPM ? "PM" : "AM", circX+16, pmY-25 );
            }
        }
        prevIsPM = isPM;

    forceClockRedraw = false;   // consumed — reset so guards fire only on real changes
}

void updateHands( int h, int m, int s ) {
    if ( isDigitalClock ) {
        drawDigitalClock( h, m, s );
        return;
    }

    createClockSprite();   // no-op after first call; safe even if drawClockFace() skipped

    uint16_t bgColor       = getBgColor();
    uint16_t mainHandColor = getTextColor();
    uint16_t secColor      = getSecHandColor();

    //tft.fillRect( 142, 130, 177, 38, getBgColor() );         // clear Name-/Holiday area
    //tft.fillRoundRect( 305, 132, 14, 12, 1, getBgColor() );  // clear country indicator

    // ── Render full clock face into sprite then push in one write (no flicker) ──
    clockSprite.fillSprite( bgColor );

    // Outer border circle
    clockSprite.drawCircle( sCX, sCY, radius + 2, mainHandColor );

    // Tick marks
    for ( int i = 0; i < 60; i++ ) {
        float    ang   = ( i * 6 - 90 ) * DEGTORAD_CF;
        int      r1    = ( i % 5 == 0 ) ? ( radius - 10 ) : ( radius - 5 );
        uint16_t tkcol = ( i % 5 == 0 )
                         ? mainHandColor
                         : ( ( themeMode == THEME_YELLOW ) ? ( uint16_t )0x0010 : ( uint16_t )TFT_DARKGREY );
        clockSprite.drawLine(
            sCX + ( int )( cos( ang ) * radius ), sCY + ( int )( sin( ang ) * radius ),
            sCX + ( int )( cos( ang ) * r1     ), sCY + ( int )( sin( ang ) * r1     ),
            tkcol );
    }

    // Hour numerals
    clockSprite.setFreeFont( &FreeSans9pt7b );
    clockSprite.setTextDatum( MC_DATUM );
    clockSprite.setTextColor( mainHandColor );
    for ( int n = 1; n <= 12; n++ ) {
        float ang = ( n * 30 - 90 ) * DEGTORAD_CF;
        clockSprite.drawString( String( n ),
                                sCX + ( int )( cos( ang ) * ( radius - 22 ) ),
                                sCY + ( int )( sin( ang ) * ( radius - 22 ) ) );
    }
    clockSprite.setFreeFont( NULL );

    // Hands
    float hA = ( ( h % 12 ) + ( m / 60.0f ) ) * 30.0f - 90.0f;
    float mA = m * 6.0f  - 90.0f;
    float sA = s * 6.0f  - 90.0f;

    clockSprite.drawLine( sCX, sCY,
                          sCX + ( int )( cos( hA * DEGTORAD_CF ) * ( radius - 35 ) ),
                          sCY + ( int )( sin( hA * DEGTORAD_CF ) * ( radius - 35 ) ),
                          mainHandColor );
    clockSprite.drawLine( sCX, sCY,
                          sCX + ( int )( cos( mA * DEGTORAD_CF ) * ( radius - 20 ) ),
                          sCY + ( int )( sin( mA * DEGTORAD_CF ) * ( radius - 20 ) ),
                          mainHandColor );
    clockSprite.drawLine( sCX, sCY,
                          sCX + ( int )( cos( sA * DEGTORAD_CF ) * ( radius - 14 ) ),
                          sCY + ( int )( sin( sA * DEGTORAD_CF ) * ( radius - 14 ) ),
                          secColor );
    clockSprite.fillCircle( sCX, sCY, 3, TFT_LIGHTGREY );

    // add AM/PM indicator for analog clock
    clockSprite.setTextFont( 2 );
    clockSprite.setTextColor( TFT_ORANGE, bgColor );

        if (h >= 12) {
        clockSprite.drawString( "PM", sCX + 60, sCY + 63 );
    }
    else {
        clockSprite.drawString( "AM", sCX + 60, sCY + 63 );
    }

    // Push sprite — single SPI burst, no intermediate state on screen
    // and clear the nameday/holiday line before drawing sprite to avoid rest of text being visible behind the sprite.
    //tft.fillRect( 148, 130, 170, 35, getBgColor() );        // clear nameday/holiday line
    //tft.fillRoundRect( 305, 132, 14, 12, 1, getBgColor() ); // clear nameday country indicator
    clockSprite.pushSprite( spriteX, spriteY );
}

void drawWeatherSection() {
    uint16_t bg = getBgColor();
    uint16_t txt = getTextColor();
    uint16_t txtContrast = TFT_SKYBLUE;

    if ( themeMode == THEME_BLUE ) {
        txtContrast = TFT_YELLOW;
    }
    else if ( themeMode == THEME_YELLOW ) {
        txtContrast = TFT_BLACK;
    }

    //tft.fillRect( 0, 0, 155, 95, TFT_RED );
    //tft.fillRect( 0, 105, 155, 100, TFT_RED );
    //tft.fillRect( 0, 206, 155, 34, TFT_RED );
    tft.fillRect( 0, 0, 142, 239, bg );
    // ****** clear weather canvas only once on midnight to avoid overwrite long nameday/holiday text
    // moved to main.cpp ==> NO, clear canvas is needed as weathersection is called every 30 min;
    // so repaint Name-/Holiday line every 30 min as well.

    if ( !initialWeatherFetched ) {
        tft.setTextColor( txt, bg );
        tft.setTextDatum( MC_DATUM );
        tft.drawString( "Loading...", 75, 120 );
        return;
    }

    // --- 1. Current temperature with icon ---
    drawWeatherIconVector( weatherCode, 5, 15 );
    tft.setTextDatum( TL_DATUM );
    tft.setTextColor( txtContrast, bg );
    tft.setFreeFont( &FreeSansBold18pt7b );

    float dispTemp = weatherUnitF ? ( currentTemp * 9.0 / 5.0 + 32 ) : currentTemp;
    String unit = weatherUnitF ? "F" : "C";
    String tempStr = String( ( int )dispTemp );
    tft.drawString( tempStr, 45, 15 );

    int tempWidth = tft.textWidth( tempStr );
    drawDegreeCircle( 45 + tempWidth + 5, 20, 3, txtContrast );
    tft.drawString( unit, 45 + tempWidth + 12, 15 );

    tft.setFreeFont( NULL );
    tft.setTextColor( txt, bg );
    tft.setCursor( 5, 60 );
    tft.print( "Today" );

    tft.setFreeFont( &FreeSans9pt7b );
    tft.setTextColor( txt, bg );
    tft.drawString( getWeatherDesc( weatherCode ), 52, 48 );

    tft.setFreeFont( NULL );
    tft.setTextColor( txt, bg );
    tft.setCursor( 5, 75 );
    if ( weatherUnitInHg ) {
        float pressInHg = currentPressure * 0.02953f;
        tft.printf( "P %.2f inHg  RH %d%%", pressInHg, currentHumidity );
    }
    else {
        tft.printf( "P %d hPa  RH %d%%", currentPressure, currentHumidity );
    }

    tft.setCursor( 5, 88 );
    if ( weatherUnitMph ) {
    //    float windMph = currentWindSpeed * 0.621371;
    //    tft.printf( "Wind: %.1f mph %s", windMph, getWindDir( currentWindDirection ).c_str() );
    // ! Remember : weatherUnitMph toggles between km/h and m/s, not mph. So we need to convert km/h to m/s when weatherUnitMph is true.
    float windms = currentWindSpeed * 0.277778; // convert km/h to m/s
        tft.printf( "Wind %.1f m/s    %s", windms, getWindDir( currentWindDirection ).c_str() );
    }
    else {
        tft.printf( "Wind %.1f km/h   %s", currentWindSpeed, getWindDir( currentWindDirection ).c_str() );
    }

    // --- Sunrise/Sunset ---
    tft.drawBitmap( 2, 100, icon_sunrise, 16, 16, TFT_YELLOW );
    tft.setCursor( 28, 104 );
    tft.setTextColor( TFT_YELLOW, bg );
    tft.print( sunriseTime );
    tft.setTextColor( TFT_WHITE, bg );
    tft.print( " SUN" );

    tft.drawBitmap( 89, 100, icon_sunset, 16, 16, TFT_ORANGE );
    tft.setCursor( 112, 104 );
    tft.setTextColor( TFT_ORANGE, bg );
    tft.print( sunsetTime );

    tft.setTextColor( txt, bg );
    tft.drawFastHLine( 2, 120, 142, TFT_DARKGREY );

    // --- 2. Forecast ---
    tft.setTextColor( txt, bg );
    tft.setFreeFont( NULL );
    tft.drawString( "Forecast", 5, 127 );

    drawWeatherIconVectorSmall( forecast[ 0 ].code, 8, 138 );
    tft.setTextDatum( ML_DATUM );
    tft.setFreeFont( NULL );
    tft.setTextColor( txt, bg );
    int day1x = 70;
    int day1y = 138;
    tft.drawString( forecastDay1Name, day1x, day1y );

    tft.setTextColor( txtContrast, bg );
    float fMin1 = weatherUnitF ? ( forecast[ 0 ].tempMin * 9.0 / 5.0 + 32 ) : forecast[ 0 ].tempMin;
    float fMax1 = weatherUnitF ? ( forecast[ 0 ].tempMax * 9.0 / 5.0 + 32 ) : forecast[ 0 ].tempMax;
    String tempMin1 = String( ( int )fMin1 );
    String tempMax1 = String( ( int )fMax1 );
    String tempRangeOnly1 = tempMin1 + "/" + tempMax1;
    tft.drawString( tempRangeOnly1, day1x+4, day1y + 13 );
    int tempWidth1 = tft.textWidth( tempRangeOnly1 );
    int degreeX1 = day1x + tempWidth1 + 3;
    int degreeY1 = day1y + 8;
    drawDegreeCircle( degreeX1+4, degreeY1, 1, txtContrast );
    tft.drawString( unit, degreeX1 + 8, day1y + 13 );

    drawWeatherIconVectorSmall( forecast[ 1 ].code, 8, 170 );
    tft.setTextColor( txt, bg );
    int day2x = 70;
    int day2y = 170;
    tft.drawString( forecastDay2Name, day2x, day2y );

    tft.setTextColor( txtContrast, bg );
    float fMin2 = weatherUnitF ? ( forecast[ 1 ].tempMin * 9.0 / 5.0 + 32 ) : forecast[ 1 ].tempMin;
    float fMax2 = weatherUnitF ? ( forecast[ 1 ].tempMax * 9.0 / 5.0 + 32 ) : forecast[ 1 ].tempMax;
    String tempMin2 = String( ( int )fMin2 );
    String tempMax2 = String( ( int )fMax2 );
    String tempRangeOnly2 = tempMin2 + "/" + tempMax2;
    tft.drawString( tempRangeOnly2, day2x+4, day2y + 13 );
    int tempWidth2 = tft.textWidth( tempRangeOnly2 );
    int degreeX2 = day2x + tempWidth2 + 3;
    int degreeY2 = day2y + 8;
    drawDegreeCircle( degreeX2+4, degreeY2, 1, txtContrast );
    tft.drawString( unit, degreeX2 + 8, day2y + 13 );
    // ---------------------------------------------------------------------------------



    tft.setTextColor( txt, bg );
    tft.drawFastHLine( 2, 200, 142, TFT_DARKGREY );

    // --- 3. Moon phase ---
    struct tm ti;
    if ( getLocalTime( &ti ) ) {
        int phase = getMoonPhase( ti.tm_year + 1900, ti.tm_mon + 1, ti.tm_mday );
        moonPhaseVal = phase;

        tft.setTextColor( txt, bg );
        tft.setFreeFont( NULL );
        tft.drawString( "Moon Phase", 5, 211 );

        String phaseNames[] = {"New Moon", "Waxing Crescent", "First Quarter", "Waxing Gibbous", "Full Moon", "Waning Gibbous", "Last Quarter", "Waning Crescent"};
        if ( phase >= 0 && phase <= 7 ) {
            tft.drawString( phaseNames[ phase ], 5, 225 );
        }

        int mx = 120;
        int my = 222;
        int r = 13;
        drawMoonPhaseIcon( mx, my, r, phase, txt, bg );

        log_d( "[MOON] Phase: %d | Date: %d-%d-%d", phase, ti.tm_year + 1900, ti.tm_mon + 1, ti.tm_mday );
    }
}
