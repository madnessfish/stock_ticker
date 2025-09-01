#include "pins_config.h"
#include "rm67162.h"
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

#if ARDUINO_USB_CDC_ON_BOOT != 1
#warning "If you need to monitor printed data, be sure to set USB CDC On boot to ENABLE"
#endif

#ifndef BOARD_HAS_PSRAM
#error "Detected that PSRAM is not turned on. Please set PSRAM to OPI PSRAM in ArduinoIDE"
#endif

// Display setup
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);

#define WIDTH  536
#define HEIGHT 240

// ===== CONFIGURATION - EDIT THESE =====
const char* ssid = "YOUR-WIFI-SSID";        // Your WiFi name
const char* password = "YOUR-WIFI-PASSWORD"; // Your WiFi password

// Choose your API provider (uncomment one):

// Option 2: Alpha Vantage (5 calls/minute)
#define USE_ALPHAVANTAGE  
const char* apiKey = "YOUR_API_KEY"; // Get free at https://www.alphavantage.co/support/#api-key

// Option 3: Yahoo Finance (Unofficial, no key needed but less reliable)
// #define USE_YAHOO_FINANCE
// =====================================

// Stock data structure
struct StockData {
    String symbol;
    float price;
    float previousClose;
    float change;
    float changePercent;
    float dayHigh;
    float dayLow;
    float openPrice;
    int trend[20];
    unsigned long lastUpdate;
};

StockData stocks[3] = {
    {"NVDA", 0, 0, 0, 0, 0, 0, 0, {50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50}, 0},
    {"AAPL", 0, 0, 0, 0, 0, 0, 0, {50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50}, 0},
    {"POPMART", 0, 0, 0, 0, 0, 0, 0, {50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50}, 0}  // 9992.HK
};

int currentStockIndex = 0;
unsigned long lastDisplayUpdate = 0;
const unsigned long displayInterval = 4000; // Switch display every 4 seconds
unsigned long lastFetchTime = 0;
const unsigned long fetchInterval = 30000; // Fetch new data every 30 seconds

WiFiClientSecure client;

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("Real-time Stock Ticker Starting...");
    
    // Initialize display
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, HIGH);
    
    rm67162_init();
    lcd_setRotation(1);
    spr.createSprite(WIDTH, HEIGHT);
    spr.setSwapBytes(1);
    
    // Show splash screen briefly
    displaySplashScreen();
    delay(1500);
    
    // Start showing stocks immediately with placeholder data
    Serial.println("Starting display with placeholder data...");
    
    // Connect to WiFi in background
    WiFi.begin(ssid, password);
    Serial.println("WiFi connecting in background...");
    
    // Configure time
    configTime(0, 0, "pool.ntp.org");
    
    // Skip certificate verification for HTTPS
    client.setInsecure();
    
    // Don't wait for data, start displaying immediately
}

void loop() {
    // Check WiFi and fetch data when connected
    static bool firstFetch = true;
    if (WiFi.status() == WL_CONNECTED) {
        if (firstFetch || millis() - lastFetchTime > fetchInterval) {
            fetchAllStockData();
            lastFetchTime = millis();
            firstFetch = false;
        }
    }
    
    // Always update display - show whatever data we have
    if (millis() - lastDisplayUpdate > displayInterval) {
        displayStockInfo(currentStockIndex);
        currentStockIndex = (currentStockIndex + 1) % 3;
        lastDisplayUpdate = millis();
    }
    
    delay(100);
}

// Removed the blocking WiFi connection screen - WiFi connects in background

void fetchAllStockData() {
    for (int i = 0; i < 3; i++) {
        fetchStockData(i);
        delay(1000); // Small delay between API calls
    }
}

void fetchStockData(int index) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi disconnected!");
        return;
    }
    
    HTTPClient https;
    String url;
    String symbol = stocks[index].symbol;
    
    // For POPMART, use Yahoo Finance (Alpha Vantage doesn't support HK stocks)
    if (symbol == "POPMART") {
        symbol = "9992.HK";
        // Use Yahoo Finance for Hong Kong stocks
        url = "https://query1.finance.yahoo.com/v8/finance/chart/" + symbol;
    } else {
        // Use Alpha Vantage for US stocks
#ifdef USE_ALPHAVANTAGE
        url = "https://www.alphavantage.co/query?function=GLOBAL_QUOTE&symbol=" + 
              symbol + "&apikey=" + apiKey;
#endif

#ifdef USE_FINNHUB
        url = "https://finnhub.io/api/v1/quote?symbol=" + symbol + "&token=" + apiKey;
#endif

#ifdef USE_YAHOO_FINANCE
        url = "https://query1.finance.yahoo.com/v8/finance/chart/" + symbol;
#endif
    }
    
    Serial.print("Fetching: ");
    Serial.print(stocks[index].symbol);
    Serial.print(" URL: ");
    Serial.println(url);
    
    if (https.begin(client, url)) {
        int httpCode = https.GET();
        Serial.print("HTTP Response Code: ");
        Serial.println(httpCode);
        
        if (httpCode == HTTP_CODE_OK) {
            String payload = https.getString();
            parseStockData(payload, index);
            stocks[index].lastUpdate = millis();
            
            // Update trend data
            updateTrendData(index);
        } else {
            Serial.print("HTTP Error: ");
            Serial.println(httpCode);
        }
        https.end();
    } else {
        Serial.println("HTTPS connection failed");
    }
}

void parseStockData(String json, int index) {
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) {
        Serial.print("JSON parse error: ");
        Serial.println(error.c_str());
        return;
    }
    
    // Check if this is POPMART (will use Yahoo Finance format)
    bool isHKStock = (stocks[index].symbol == "POPMART");
    
    if (isHKStock || json.indexOf("chart") != -1) {
        // Parse Yahoo Finance response (for POPMART or if using Yahoo for all)
        JsonObject result = doc["chart"]["result"][0];
        JsonObject meta = result["meta"];
        
        stocks[index].price = meta["regularMarketPrice"] | 0.0;
        stocks[index].previousClose = meta["previousClose"] | 0.0;
        stocks[index].dayHigh = meta["regularMarketDayHigh"] | 0.0;
        stocks[index].dayLow = meta["regularMarketDayLow"] | 0.0;
        
        if (stocks[index].price > 0 && stocks[index].previousClose > 0) {
            stocks[index].change = stocks[index].price - stocks[index].previousClose;
            stocks[index].changePercent = (stocks[index].change / stocks[index].previousClose) * 100;
        }
    } else {
#ifdef USE_ALPHAVANTAGE
        // Parse Alpha Vantage response (for US stocks)
        JsonObject quote = doc["Global Quote"];
        if (quote) {
            stocks[index].price = atof(quote["05. price"] | "0");
            stocks[index].openPrice = atof(quote["02. open"] | "0");
            stocks[index].dayHigh = atof(quote["03. high"] | "0");
            stocks[index].dayLow = atof(quote["04. low"] | "0");
            stocks[index].previousClose = atof(quote["08. previous close"] | "0");
            
            String changeStr = quote["09. change"] | "0";
            stocks[index].change = atof(changeStr.c_str());
            
            String changePctStr = quote["10. change percent"] | "0%";
            changePctStr.replace("%", "");
            stocks[index].changePercent = atof(changePctStr.c_str());
        }
#endif

#ifdef USE_FINNHUB
        // Parse Finnhub response
        stocks[index].price = doc["c"] | 0.0;
        stocks[index].previousClose = doc["pc"] | 0.0;
        stocks[index].openPrice = doc["o"] | 0.0;
        stocks[index].dayHigh = doc["h"] | 0.0;
        stocks[index].dayLow = doc["l"] | 0.0;
        
        if (stocks[index].price > 0 && stocks[index].previousClose > 0) {
            stocks[index].change = stocks[index].price - stocks[index].previousClose;
            stocks[index].changePercent = (stocks[index].change / stocks[index].previousClose) * 100;
        }
#endif
    }
    
    Serial.print(stocks[index].symbol);
    Serial.print(" - Price: $");
    Serial.print(stocks[index].price);
    Serial.print(" Change: ");
    Serial.print(stocks[index].changePercent);
    Serial.print("% (from ");
    Serial.print(stocks[index].previousClose);
    Serial.println(")");
}

void updateTrendData(int index) {
    // Shift trend data left
    for (int i = 0; i < 19; i++) {
        stocks[index].trend[i] = stocks[index].trend[i + 1];
    }
    
    // Add new data point based on change percentage
    int newPoint = 50 + (int)(stocks[index].changePercent * 5);
    stocks[index].trend[19] = constrain(newPoint, 10, 90);
}

// No longer needed - we show stocks immediately with placeholder data

void displaySplashScreen() {
    spr.fillSprite(TFT_BLACK);
    
    // Gradient background
    for (int y = 0; y < HEIGHT; y++) {
        uint8_t brightness = (y * 50) / HEIGHT;
        uint16_t color = spr.color565(0, brightness, brightness/2);
        spr.drawFastHLine(0, y, WIDTH, color);
    }
    
    spr.setTextColor(TFT_CYAN, TFT_BLACK);
    spr.setTextSize(4);
    spr.setTextDatum(MC_DATUM);
    spr.drawString("STOCK", WIDTH/2, 60);
    spr.drawString("TICKER", WIDTH/2, 110);
    
    spr.setTextSize(2);
    spr.setTextColor(TFT_WHITE, TFT_BLACK);
    spr.drawString("NVDA | AAPL | POPMART", WIDTH/2, 160);
    
    spr.setTextSize(1);
    spr.setTextColor(TFT_YELLOW, TFT_BLACK);
    spr.drawString("Real-Time Market Data", WIDTH/2, 200);
    
    spr.setTextDatum(TL_DATUM);
    lcd_PushColors(0, 0, WIDTH, HEIGHT, (uint16_t *)spr.getPointer());
}

void displayStockInfo(int index) {
    StockData &stock = stocks[index];
    
    // Background gradient
    for (int y = 0; y < HEIGHT; y++) {
        uint8_t brightness = 10 + (y * 20) / HEIGHT;
        uint16_t bgColor = spr.color565(brightness/3, brightness/3, brightness/2);
        spr.drawFastHLine(0, y, WIDTH, bgColor);
    }
    
    // Stock symbol with colored background - all same width, left aligned
    uint16_t symbolBgColor = stock.change >= 0 ? spr.color565(0, 60, 0) : spr.color565(60, 0, 0);
    spr.fillRoundRect(15, 15, 150, 50, 8, symbolBgColor);
    spr.setTextSize(4);
    spr.setTextColor(TFT_WHITE, symbolBgColor);
    spr.setTextDatum(ML_DATUM);  // Middle Left alignment
    spr.drawString(stock.symbol, 25, 40);  // All symbols start at same X position
    
    // Current price - all use $ for consistency, left aligned
    spr.setTextDatum(TL_DATUM);
    spr.setTextSize(5);
    String priceStr;
    if (stock.price == 0) {
        priceStr = "N/A";
        spr.setTextColor(TFT_DARKGREY);
    } else {
        spr.setTextColor(TFT_WHITE);
        priceStr = "$" + String(stock.price, 2);
    }
    spr.drawString(priceStr, 20, 80);  // All prices start at same position
    
    // Change with arrow
    spr.setTextSize(2);
    uint16_t changeColor = stock.change >= 0 ? TFT_GREEN : TFT_RED;
    spr.setTextColor(changeColor);
    
    // Arrow indicator - all at same position
    int arrowX = 20;
    int arrowY = 140;
    if (stock.price != 0) {  // Only draw arrow if we have data
        if (stock.change >= 0) {
            spr.fillTriangle(arrowX, arrowY+10, arrowX-5, arrowY+20, arrowX+5, arrowY+20, changeColor);
        } else {
            spr.fillTriangle(arrowX, arrowY+20, arrowX-5, arrowY+10, arrowX+5, arrowY+10, changeColor);
        }
    }
    
    String changeStr;
    if (stock.price == 0) {
        changeStr = "--";
    } else {
        changeStr = (stock.change >= 0 ? "+" : "") + String(stock.change, 2) + 
                    " (" + (stock.change >= 0 ? "+" : "") + String(stock.changePercent, 2) + "%)";
    }
    spr.drawString(changeStr, 35, 135);  // All changes at same position
    
    // Day range
    spr.drawRoundRect(20, 170, 200, 50, 5, TFT_DARKGREY);
    spr.setTextSize(1);
    spr.setTextColor(TFT_LIGHTGREY);
    spr.drawString("Day Range", 25, 175);
    
    spr.setTextColor(TFT_WHITE);
    if (stock.price == 0) {
        spr.drawString("H: --", 25, 195);
        spr.drawString("L: --", 120, 195);
    } else {
        String highStr = "H: $" + String(stock.dayHigh, 2);
        String lowStr = "L: $" + String(stock.dayLow, 2);
        spr.drawString(highStr, 25, 195);
        spr.drawString(lowStr, 120, 195);
    }
    
    // Mini chart
    drawChart(280, 40, 230, 140, stock);
    
    // Update time
    drawUpdateTime(280, 190, stock.lastUpdate);
    
    // Market status
    drawMarketStatus(420, 15);
    
    // Show loading indicator if data hasn't been updated from API yet
    if (stock.lastUpdate == 0) {
        drawLoadingIndicator(480, 220);
    }
    
    lcd_PushColors(0, 0, WIDTH, HEIGHT, (uint16_t *)spr.getPointer());
}

void drawLoadingIndicator(int x, int y) {
    // Subtle loading indicator - small spinning dots
    static int animFrame = 0;
    animFrame = (animFrame + 1) % 8;
    
    // Draw WiFi status icon
    if (WiFi.status() == WL_CONNECTED) {
        spr.setTextSize(1);
        spr.setTextColor(TFT_GREEN);
        spr.drawString("LIVE", x, y);
    } else {
        spr.setTextSize(1);
        spr.setTextColor(TFT_YELLOW);
        spr.drawString("...", x, y);
        
        // Animated dot
        int dotX = x + (animFrame * 3);
        spr.fillCircle(dotX, y - 5, 1, TFT_YELLOW);
    }
}

void drawChart(int x, int y, int w, int h, StockData &stock) {
    spr.drawRoundRect(x-5, y-5, w+10, h+10, 5, TFT_DARKGREY);
    
    // Grid lines
    for (int i = 1; i < 4; i++) {
        int gridY = y + (h * i) / 4;
        spr.drawFastHLine(x, gridY, w, spr.color565(30, 30, 30));
    }
    
    uint16_t chartColor = stock.change >= 0 ? TFT_GREEN : TFT_RED;
    uint16_t fillColor = stock.change >= 0 ? spr.color565(0, 100, 0) : spr.color565(100, 0, 0);
    
    // Draw trend line
    int pointSpacing = w / 19;
    for (int i = 0; i < 19; i++) {
        int y1 = y + h - ((stock.trend[i] * h) / 100);
        int y2 = y + h - ((stock.trend[i + 1] * h) / 100);
        int x1 = x + i * pointSpacing;
        int x2 = x + (i + 1) * pointSpacing;
        
        // Fill area under line
        for (int fillY = y1; fillY <= y + h; fillY++) {
            spr.drawPixel(x1, fillY, fillColor);
        }
        
        spr.drawLine(x1, y1, x2, y2, chartColor);
        spr.drawLine(x1, y1+1, x2, y2+1, chartColor);
        
        if (i % 4 == 0) {
            spr.fillCircle(x1, y1, 2, chartColor);
        }
    }
    
    // Highlight last point
    int lastY = y + h - ((stock.trend[19] * h) / 100);
    spr.fillCircle(x + w, lastY, 3, TFT_WHITE);
    spr.drawCircle(x + w, lastY, 4, chartColor);
}

void drawUpdateTime(int x, int y, unsigned long lastUpdate) {
    spr.setTextSize(1);
    spr.setTextColor(TFT_LIGHTGREY);
    spr.drawString("Last Update:", x, y);
    
    if (lastUpdate > 0) {
        unsigned long elapsed = (millis() - lastUpdate) / 1000;
        String timeStr;
        if (elapsed < 60) {
            timeStr = String(elapsed) + "s ago";
        } else {
            timeStr = String(elapsed / 60) + "m ago";
        }
        spr.setTextColor(TFT_WHITE);
        spr.drawString(timeStr, x, y + 15);
    }
}

void drawMarketStatus(int x, int y) {
    // Check if market is open (NYSE: 9:30 AM - 4:00 PM EST)
    struct tm timeinfo;
    bool marketOpen = false;
    
    if (getLocalTime(&timeinfo)) {
        int hour = timeinfo.tm_hour; // Adjust for your timezone
        int minute = timeinfo.tm_min;
        int weekday = timeinfo.tm_wday;
        
        // Market open Mon-Fri, 9:30 AM - 4:00 PM EST
        // Adjust these hours for your timezone
        if (weekday >= 1 && weekday <= 5) {
            if ((hour > 9 || (hour == 9 && minute >= 30)) && hour < 16) {
                marketOpen = true;
            }
        }
    }
    
    spr.fillCircle(x, y + 8, 4, marketOpen ? TFT_GREEN : TFT_RED);
    spr.setTextSize(1);
    spr.setTextColor(TFT_WHITE);
    spr.drawString(marketOpen ? "OPEN" : "CLOSED", x + 10, y);
}