#include "icons.h"
#include "theme.h"
#include "../util/constants.h"

#include <WiFi.h>
#include <TFT_eSPI.h>

// ---------------------------------------------------------------------------
// Externs – defined in main.cpp
// ---------------------------------------------------------------------------
extern TFT_eSPI  tft;
extern bool      isWhiteTheme;
extern int       themeMode;
extern uint16_t  blueDark;
extern uint16_t  yellowDark;
extern bool      updateAvailable;

static constexpr float DEGTORAD_ICONS = ( float )( PI / 180.0 );

// WiFi RSSI icons
const unsigned char icon_wifi_1[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x00, 0xe0, 0x00, 0xe0, 0x00
};
const unsigned char icon_wifi_2[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0e, 0x00, 0x0e, 0x00, 0x0e, 0x00, 0xee, 0x00, 0xee, 0x00, 0xee, 0x00
};
const unsigned char icon_wifi_3[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x00, 0xe0, 0x00, 0xe0,
    0x0e, 0xe0, 0x0e, 0xe0, 0x0e, 0xe0, 0xee, 0xe0, 0xee, 0xe0, 0xee, 0xe0
};
const unsigned char icon_wifi_4[] = {
    0x00, 0x0e, 0x00, 0x0e, 0x00, 0x0e, 0x00, 0xee, 0x00, 0xee, 0x00, 0xee,
    0x0e, 0xee, 0x0e, 0xee, 0x0e, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee
};

// Sun icons
extern const unsigned char icon_sunrise[] = {
    0x00, 0x80, 0x00, 0x80, 0x20, 0x82, 0x10, 0x84, 0x09, 0xc8, 0x07, 0xf0, 0x4f, 0xf9, 0x3f, 0xfe,
    0x1f, 0xfc, 0x1f, 0xfc, 0x7f, 0xff, 0x1f, 0xfc, 0x0f, 0xf8, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00
};
extern const unsigned char icon_sunset[] = {
    0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x3f, 0xf8, 0x7f, 0xfc, 0x7f, 0xfc, 0x7f, 0xfc, 0x3f, 0xf8,
    0x7f, 0xfc, 0x9f, 0xf2, 0x0f, 0xe0, 0x13, 0x90, 0x21, 0x08, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00
};

// ---------------------------------------------------------------------------

void drawCloudVector( int x, int y, uint32_t color ) {
    tft.fillCircle( x + 10, y + 15, 8, color );
    tft.fillCircle( x + 18, y + 10, 10, color );
    tft.fillCircle( x + 28, y + 15, 8, color );
    tft.fillRoundRect( x + 10, y + 15, 20, 8, 4, color );
}

void drawWeatherIconVector( int code, int x, int y ) {

    // Icon colors adapt to the theme
    // removed draw shadows as these are incorrect and nearly no visible
    
    uint16_t cloudCol = TFT_SILVER;
    uint16_t shadowCol = isWhiteTheme ? 0x8410 : 0x4208; // Shadow in blue/yellow theme

    switch ( code ) {
        case 0: // Clear
            // Sun with shadow
            tft.fillCircle( x + 16, y + 16, 10, TFT_YELLOW );
            tft.drawCircle( x + 16, y + 16, 11, shadowCol ); // Shadow
            for ( int i = 0; i < 360; i += 45 ) {
                float rad = i * 0.01745;
                tft.drawLine( x + 16 + cos( rad ) * 11, y + 16 + sin( rad ) * 11, x + 16 + cos( rad ) * 16, y + 16 + sin( rad ) * 16, TFT_YELLOW );
            }
            break;

        case 1:
        case 2:
        case 3: // Partly cloudy
            tft.fillCircle( x + 22, y + 10, 8, TFT_YELLOW );
            tft.drawCircle( x + 22, y + 10, 9, shadowCol ); // Shadow
            drawCloudVector( x, y + 5, cloudCol );
            break;

        case 45:
        case 48: // Fog
            for ( int i = 0; i < 3; i++ ) {
                tft.fillRoundRect( x + 4, y + 9 + ( i * 6 ), 24, 3, 2, TFT_SILVER );
                //tft.drawRoundRect( x + 4, y + 9 + ( i * 6 ), 24, 3, 2, shadowCol ); // Shadow
            }
            break;

        case 51:
        case 53:
        case 55:
        case 56:
        case 57:
        case 61:
        case 63:
        case 65:
        case 66:
        case 67: // Rain / freezing drizzle / freezing rain
            drawCloudVector( x, y + 2, TFT_SILVER );
            for ( int i = 0; i < 3; i++ ) {
                tft.fillRoundRect( x + 10 + ( i * 6 ), y + 27, 2, 6, 1, TFT_SKYBLUE );
                //tft.drawRoundRect( x + 10 + ( i * 6 ), y + 22, 2, 6, 1, shadowCol ); // Drop shadow
            }
            break;

        case 71:
        case 73:
        case 75:
        case 77: // Snow
            drawCloudVector( x, y + 2, cloudCol );
            tft.setTextColor( TFT_SKYBLUE );
            tft.drawString( "*", x + 12, y + 22 );
            tft.drawString( "*", x + 22, y + 22 );
            break;

        case 80:
        case 81:
        case 82: // Showers
            tft.fillCircle( x + 22, y + 10, 7, TFT_YELLOW );
            //tft.drawCircle( x + 22, y + 10, 8, shadowCol ); // Shadow
            drawCloudVector( x, y + 2, TFT_SILVER );
            tft.fillRoundRect( x + 16, y + 27, 2, 6, 1, TFT_SKYBLUE );
            break;

        case 85:
        case 86: // Snow showers
            tft.fillCircle( x + 22, y + 10, 7, TFT_YELLOW );
            tft.drawCircle( x + 22, y + 10, 8, shadowCol );
            drawCloudVector( x, y + 2, TFT_SILVER );
            tft.setTextColor( TFT_SKYBLUE );
            tft.drawString( "*", x + 12, y + 22 );
            tft.drawString( "*", x + 22, y + 22 );
            break;

        case 95:
        case 96:
        case 99: // Storm
            drawCloudVector( x, y + 2, TFT_BLUE ); // Dark cloud
            tft.drawLine( x + 18, y + 15, x + 14, y + 23, TFT_YELLOW );
            tft.drawLine( x + 14, y + 23, x + 20, y + 23, TFT_YELLOW );
            tft.drawLine( x + 20, y + 23, x + 16, y + 31, TFT_YELLOW );
            break;

        default:
            drawCloudVector( x, y + 5, TFT_SILVER );
            break;
    }
}

// ============================================
// NEW FEATURE: Reduced icons for forecast
// ============================================
void drawWeatherIconVectorSmall( int code, int x, int y ) {
    // A scaled-down version for forecast, but with better proportions
    uint16_t cloudCol = TFT_SILVER;
    uint16_t shadowCol = isWhiteTheme ? 0x8410 : 0x4208;

    switch ( code ) {
        case 0: // Clear
            tft.fillCircle( x + 16, y + 16, 9, TFT_YELLOW );
            tft.drawCircle( x + 16, y + 16, 10, shadowCol );
            for ( int i = 0; i < 360; i += 45 ) {
                float rad = i * 0.01745;
                tft.drawLine( x + 16 + cos( rad ) * 10, y + 16 + sin( rad ) * 10, x + 16 + cos( rad ) * 14, y + 16 + sin( rad ) * 14, TFT_YELLOW );
            }
            break;

        case 1:
        case 2:
        case 3: // Partly cloudy
            tft.fillCircle( x + 20, y + 10, 7, TFT_YELLOW );
            tft.drawCircle( x + 20, y + 10, 8, shadowCol );
            tft.fillCircle( x + 8, y + 14, 6, cloudCol );
            tft.fillCircle( x + 14, y + 11, 7, cloudCol );
            tft.fillCircle( x + 20, y + 14, 5, cloudCol );
            tft.fillRoundRect( x + 8, y + 14, 15, 5, 2, cloudCol );
            break;

        case 45:
        case 48: // Fog
            for ( int i = 0; i < 3; i++ ) {
                tft.fillRoundRect( x + 4, y + 10 + ( i * 5 ), 20, 2, 1, TFT_SILVER );
                //tft.drawRoundRect( x + 4, y + 10 + ( i * 5 ), 20, 2, 1, shadowCol );
            }
            break;

        case 51:
        case 53:
        case 55:
        case 56:
        case 57:
        case 61:
        case 63:
        case 65:
        case 66:
        case 67: // Rain / freezing drizzle / freezing rain
            tft.fillCircle( x + 9, y + 13, 6, cloudCol );
            tft.fillCircle( x + 15, y + 10, 8, cloudCol );
            tft.fillCircle( x + 22, y + 13, 6, cloudCol );
            tft.fillRoundRect( x + 9, y + 13, 16, 6, 3, cloudCol );
            for ( int i = 0; i < 3; i++ ) {
                tft.fillRoundRect( x + 10 + ( i * 5 ), y + 21, 2, 5, 1, TFT_SKYBLUE );
                //tft.drawRoundRect( x + 10 + ( i * 5 ), y + 21, 2, 5, 1, shadowCol );
            }
            break;

        case 71:
        case 73:
        case 75:
        case 77: // Snow
            tft.fillCircle( x + 9, y + 13, 6, cloudCol );
            tft.fillCircle( x + 15, y + 10, 8, cloudCol );
            tft.fillCircle( x + 22, y + 13, 6, cloudCol );
            tft.fillRoundRect( x + 9, y + 13, 16, 6, 3, cloudCol );
            tft.setTextColor( TFT_SKYBLUE );
            tft.drawString( "*", x + 11, y + 21 );
            tft.drawString( "*", x + 19, y + 21 );
            break;

        case 80:
        case 81:
        case 82: // Showers
            tft.fillCircle( x + 20, y + 10, 7, TFT_YELLOW );
            tft.drawCircle( x + 20, y + 10, 8, shadowCol );
            tft.fillCircle( x + 8, y + 14, 6, cloudCol );
            tft.fillCircle( x + 14, y + 11, 7, cloudCol );
            tft.fillCircle( x + 20, y + 14, 5, cloudCol );
            tft.fillRoundRect( x + 8, y + 14, 15, 5, 2, cloudCol );
            tft.fillRoundRect( x + 14, y + 21, 2, 5, 1, TFT_SKYBLUE );
            break;

        case 85:
        case 86: // Snow showers
            tft.fillCircle( x + 20, y + 10, 7, TFT_YELLOW );
            tft.drawCircle( x + 20, y + 10, 8, shadowCol );
            tft.fillCircle( x + 8, y + 14, 6, cloudCol );
            tft.fillCircle( x + 14, y + 11, 7, cloudCol );
            tft.fillCircle( x + 20, y + 14, 5, cloudCol );
            tft.fillRoundRect( x + 8, y + 14, 15, 5, 2, cloudCol );
            tft.setTextColor( TFT_SKYBLUE );
            tft.drawString( "*", x + 14, y + 21 );
            break;

        case 95:
        case 96:
        case 99: // Storm
            tft.fillCircle( x + 9, y + 13, 6, TFT_BLUE );
            tft.fillCircle( x + 15, y + 10, 8, TFT_BLUE );
            tft.fillCircle( x + 22, y + 13, 6, TFT_BLUE );
            tft.fillRoundRect( x + 7, y + 13, 16, 3, 2, TFT_YELLOW );
            tft.drawLine( x + 15, y + 15, x + 12, y + 22, TFT_YELLOW );
            tft.drawLine( x + 12, y + 22, x + 17, y + 22, TFT_YELLOW );
            tft.drawLine( x + 17, y + 22, x + 14, y + 29, TFT_YELLOW );
            break;

        default:
            tft.fillCircle( x + 9, y + 13, 6, cloudCol );
            tft.fillCircle( x + 15, y + 10, 8, cloudCol );
            tft.fillCircle( x + 22, y + 13, 6, cloudCol );
            tft.fillRoundRect( x + 9, y + 13, 16, 6, 3, cloudCol );
            break;
    }
}

// ============================================
// NEW FEATURE FOR CORRECT DRAWING OF MOON PHASE
// ============================================
void drawMoonPhaseIcon( int mx, int my, int r, int phase, uint16_t textColor, uint16_t bgColor ) {
    uint16_t moonBg = ( themeMode == THEME_BLUE ) ? blueDark : ( themeMode == THEME_YELLOW ) ? yellowDark : ( isWhiteTheme ? 0xDEDB : 0x3186 );
    uint16_t moonColor = TFT_YELLOW;
    uint16_t shadowColor = moonBg;

    tft.drawCircle( mx, my, r, TFT_DARKGREY ); // Outer border

    switch ( phase ) {

        case 0: { // NEW MOON
            tft.fillCircle( mx, my, r - 1, shadowColor );
            break;
        }

        case 1: { // WAXING CRESCENT
            tft.fillCircle( mx, my, r - 1, shadowColor );
            int offset = r / 3;
            for ( int dy = -r; dy <= r; dy++ ) {
                int dx_max = sqrt( r * r - dy * dy );
                int light_boundary = sqrt( r * r - dy * dy - offset * offset ) - offset;
                if ( light_boundary < 0 ) {
                    light_boundary = 0;
                }
                for ( int dx = light_boundary; dx <= dx_max; dx++ ) {
                    tft.drawPixel( mx + dx, my + dy, moonColor );
                }
            }
            break;
        }

        case 2: { // FIRST QUARTER
            tft.fillCircle( mx, my, r - 1, shadowColor );
            for ( int dy = -r; dy <= r; dy++ ) {
                int dx_max = sqrt( r * r - dy * dy );
                for ( int dx = 0; dx <= dx_max; dx++ ) {
                    tft.drawPixel( mx + dx, my + dy, moonColor );
                }
            }
            break;
        }

        case 3: { // WAXING GIBBOUS
            tft.fillCircle( mx, my, r - 1, moonColor );
            int offset = r / 3;
            for ( int dy = -r; dy <= r; dy++ ) {
                int dx_max = sqrt( r * r - dy * dy );
                int shadow_boundary = -( sqrt( r * r - dy * dy - offset * offset ) - offset );
                if ( shadow_boundary > 0 ) {
                    shadow_boundary = 0;
                }
                for ( int dx = -dx_max; dx <= shadow_boundary; dx++ ) {
                    tft.drawPixel( mx + dx, my + dy, shadowColor );
                }
            }
            break;
        }

        case 4: { // FULL MOON
            tft.fillCircle( mx, my, r - 1, moonColor );
            break;
        }

        case 5: { // WANING GIBBOUS
            tft.fillCircle( mx, my, r - 1, moonColor );
            int offset = r / 3;
            for ( int dy = -r; dy <= r; dy++ ) {
                int dx_max = sqrt( r * r - dy * dy );
                int shadow_boundary = sqrt( r * r - dy * dy - offset * offset ) - offset;
                if ( shadow_boundary < 0 ) {
                    shadow_boundary = 0;
                }
                for ( int dx = shadow_boundary; dx <= dx_max; dx++ ) {
                    tft.drawPixel( mx + dx, my + dy, shadowColor );
                }
            }
            break;
        }

        case 6: { // LAST QUARTER
            tft.fillCircle( mx, my, r - 1, shadowColor );
            for ( int dy = -r; dy <= r; dy++ ) {
                int dx_max = sqrt( r * r - dy * dy );
                for ( int dx = -dx_max; dx <= 0; dx++ ) {
                    tft.drawPixel( mx + dx, my + dy, moonColor );
                }
            }
            break;
        }

        case 7: { // WANING CRESCENT
            tft.fillCircle( mx, my, r - 1, shadowColor );
            int offset = r / 3;
            for ( int dy = -r; dy <= r; dy++ ) {
                int dx_max = sqrt( r * r - dy * dy );
                int light_boundary = -( sqrt( r * r - dy * dy - offset * offset ) - offset );
                if ( light_boundary > 0 ) {
                    light_boundary = 0;
                }
                for ( int dx = -dx_max; dx <= light_boundary; dx++ ) {
                    tft.drawPixel( mx + dx, my + dy, moonColor );
                }
            }
            break;
        }

        default: {
            tft.drawCircle( mx, my, r, TFT_DARKGREY );
            break;
        }
    }
}

void drawWifiIndicator() {
    // Erase previous icon footprint
    tft.fillRect( 300, 10, 16, 12, getBgColor() );
    int wifiStatus = WiFi.status();
    if ( wifiStatus == WL_CONNECTED ) {
        int rssi = WiFi.RSSI();
        if ( rssi > -40 ) {
            tft.drawBitmap( 300, 10, icon_wifi_4, 16, 12, TFT_BLUE );
        } else if ( rssi > -60 ) {
            tft.drawBitmap( 300, 10, icon_wifi_3, 16, 12, TFT_BLUE );
        } else if ( rssi > -70 ) {
            tft.drawBitmap( 300, 10, icon_wifi_2, 16, 12, TFT_BLUE );
        } else if ( rssi > -80 ) {
            tft.drawBitmap( 300, 10, icon_wifi_1, 16, 12, TFT_ORANGE );
        } else {
            tft.drawBitmap( 300, 10, icon_wifi_1, 16, 12, TFT_RED ); // Very, very weak signal
        }
    }
    //tft.fillCircle( 305, 20, 4, color );
    //Serial.printf( "WiFi status: %d, RSSI: %d\n", wifiStatus, rssi );
}

    // Firmware Update-available indicator — upward triangle + stem, offset 4 px right of WiFi circle
void drawUpdateIndicator() {
    if ( !updateAvailable ) {
        return;
    }
    // Erase previous icon footprint before drawing (handles redraws without a prior fillScreen)
    tft.fillRect( 305, 79, 10, 12, getBgColor() );

    int iconX = 308;  // 4 px gap from WiFi circle right edge (x=309)
    int iconY = 80;   // Centres the 10px-tall icon at y=20 (matching WiFi circle centre)

    // Upward-pointing filled triangle (6 px wide, 5 px tall — less pointy)
    tft.fillTriangle( iconX, iconY + 5, iconX + 3, iconY, iconX + 6, iconY + 5, TFT_GREEN );
    // Stem extending down from triangle base (centred, 2 px wide, 5 px tall)
    tft.fillRect( iconX + 2, iconY + 5, 2, 5, TFT_GREEN );
}

void drawSettingsIcon( uint16_t color ) {
    int ix = 300, iy = 220;
    int rIn = 3, rMid = 6, rOut = 8;
    tft.fillCircle( ix, iy, rMid, color );
    tft.fillCircle( ix, iy, rIn, getBgColor() );
    for ( int i = 0; i < 8; i++ ) {
        float a = i * 45 * DEGTORAD_ICONS;
        float aL = a - 0.2;
        float aR = a + 0.2;
        tft.fillTriangle( ix + cos( aL ) * rMid, iy + sin( aL ) * rMid, ix + cos( aR ) * rMid, iy + sin( aR ) * rMid, ix + cos( a ) * rOut, iy + sin( a ) * rOut, color );
    }
}

void drawArrowBack( int x, int y, uint16_t color ) {
    tft.drawRoundRect( x, y, 50, 50, 4, color );
    tft.drawLine( x + 35, y + 15, x + 20, y + 25, color );
    tft.drawLine( x + 35, y + 35, x + 20, y + 25, color );
    tft.drawLine( x + 34, y + 15, x + 19, y + 25, color );
    tft.drawLine( x + 34, y + 35, x + 19, y + 25, color );
}

void drawArrowDown( int x, int y, uint16_t color ) {
    tft.drawRoundRect( x, y, 50, 50, 4, color );
    tft.drawLine( x + 15, y + 20, x + 25, y + 35, color );
    tft.drawLine( x + 35, y + 20, x + 25, y + 35, color );
    tft.drawLine( x + 15, y + 21, x + 25, y + 36, color );
    tft.drawLine( x + 35, y + 21, x + 25, y + 36, color );
}

void drawArrowUp( int x, int y, uint16_t color ) {
    tft.drawRoundRect( x, y, 50, 50, 4, color );
    tft.drawLine( x + 15, y + 35, x + 25, y + 20, color );
    tft.drawLine( x + 35, y + 35, x + 25, y + 20, color );
    tft.drawLine( x + 15, y + 34, x + 25, y + 19, color );
    tft.drawLine( x + 35, y + 34, x + 25, y + 19, color );
}

void fillGradientVertical( int x, int y, int w, int h, uint16_t colorTop, uint16_t colorBottom ) {
    for ( int i = 0; i < h; i++ ) {
        uint8_t r1 = ( colorTop >> 11 ) & 0x1F;
        uint8_t g1 = ( colorTop >> 5 ) & 0x3F;
        uint8_t b1 = colorTop & 0x1F;

        uint8_t r2 = ( colorBottom >> 11 ) & 0x1F;
        uint8_t g2 = ( colorBottom >> 5 ) & 0x3F;
        uint8_t b2 = colorBottom & 0x1F;

        float ratio = ( float )i / h;
        uint8_t r = r1 + ( r2 - r1 ) * ratio;
        uint8_t g = g1 + ( g2 - g1 ) * ratio;
        uint8_t b = b1 + ( b2 - b1 ) * ratio;

        uint16_t color = ( r << 11 ) | ( g << 5 ) | b;
        //tft.drawFastHLine( x, y + i, w, color );
    }
}

// ============================================
// HELPER FUNCTION FOR DRAWING DEGREE SYMBOL
// ============================================
void drawDegreeCircle( int x, int y, int r, uint16_t color ) {
    tft.drawCircle( x, y, r, color );
    if ( r > 1 ) {
        tft.drawCircle( x, y, r - 1, color );
    }
}

