# Stock Ticker for T-Display-S3-AMOLED

A real-time stock ticker display for NVDA and AAPL stocks using the LilyGO T-Display-S3-AMOLED board.

## Features

- Real-time stock price display for NVIDIA (NVDA) and Apple (AAPL)
- Color-coded price changes (green for gains, red for losses)
- Mini chart visualization showing price trends
- Day high/low range display
- Market status indicator (OPEN/CLOSED)
- Automatic cycling between stocks every 3 seconds
- Smooth gradient backgrounds and modern UI

## Files

- `stock_ticker_demo.ino` - Demo version with simulated price movements (no API key needed)
- `stock_ticker.ino.backup` - Full version with real API integration (requires API key)
- `rm67162.cpp/h` - Display driver for the AMOLED screen
- `pins_config.h` - Pin configuration for the T-Display-S3-AMOLED board

**Important**: Arduino compiles all `.ino` files in a folder together. Only have one `.ino` file active at a time. To switch versions:
- For demo: Keep `stock_ticker_demo.ino`
- For API version: Rename `stock_ticker.ino.backup` to `stock_ticker.ino` and remove/rename the demo file

## Hardware Requirements

- LilyGO T-Display-S3-AMOLED (non-touch version)
- USB-C cable for programming and power

## Software Requirements

1. Arduino IDE with ESP32 board support (version 2.0.3 or above)
2. Required Libraries:
   - TFT_eSPI (copy from T-Display-S3-AMOLED/lib folder)
   - ArduinoJson (for the full API version)
   - OneButton (optional, for button controls)

## Installation

1. Copy the entire `stock_ticker` folder to your Arduino projects directory

2. **IMPORTANT: Configure your credentials:**
   ```bash
   cp config.h.template config.h
   # Edit config.h with your WiFi and API credentials
   ```

3. Copy the required libraries from the T-Display-S3-AMOLED repository:
   ```
   T-Display-S3-AMOLED/lib/TFT_eSPI → Arduino/libraries/
   ```

4. Configure Arduino IDE:
   - Board: "ESP32S3 Dev Module"
   - USB CDC On Boot: "Enabled"
   - PSRAM: "OPI PSRAM"
   - Upload Speed: "921600"
   - USB Mode: "Hardware CDC and JTAG"

4. For the demo version (`stock_ticker_demo.ino`):
   - No configuration needed, it works out of the box with simulated data
   - Optionally add WiFi credentials for time synchronization

5. For the full API version (`stock_ticker.ino`):
   - Get a free API key from [Alpha Vantage](https://www.alphavantage.co/support/#api-key)
   - Update the following in the code:
     ```cpp
     const char* ssid = "YOUR-SSID";
     const char* password = "YOUR-PASSWORD";
     const char* apiKey = "YOUR_API_KEY";
     ```

## Display Layout

The display shows:
- **Stock Symbol**: Large text with colored background (green/red based on trend)
- **Current Price**: Large white text showing current price
- **Price Change**: Shows absolute and percentage change with arrow indicator
- **Day Range**: High and low prices for the day
- **Mini Chart**: 20-point trend visualization
- **Market Status**: Indicator showing if market is open or closed
- **Update Time**: Last update timestamp (when WiFi is connected)

## Customization

### Change Stock Symbols
Edit the stocks array in the code:
```cpp
const char* stocks[] = {"NVDA", "AAPL", "GOOGL", "MSFT"};
const int numStocks = 4;
```

### Adjust Display Timing
```cpp
const unsigned long displayInterval = 3000; // Time between stock switches (ms)
const unsigned long priceUpdateInterval = 5000; // Price update frequency (ms)
```

### Color Scheme
The display uses:
- Green for positive changes
- Red for negative changes
- Cyan for titles
- White for primary text
- Grey for secondary information

## Troubleshooting

1. **Display not working**: 
   - Ensure PSRAM is enabled in Arduino IDE
   - Check that USB CDC On Boot is enabled for serial debugging

2. **WiFi connection fails**:
   - Verify SSID and password are correct
   - The demo version works without WiFi

3. **No price updates** (API version):
   - Check API key is valid
   - Free tier has rate limits (5 calls/minute)
   - Use demo version for testing

## Demo Mode

The `stock_ticker_demo.ino` provides a fully functional demonstration without requiring an API key. It:
- Simulates realistic price movements
- Updates every 5 seconds with small random changes
- Maintains price trends in the chart
- Perfect for testing and development

## API Limitations

The free Alpha Vantage API has the following limits:
- 5 API requests per minute
- 500 requests per day
- Real-time data may have 15-minute delay

For production use, consider premium API services or use the demo version for display purposes.

## License

This project uses code from the T-Display-S3-AMOLED examples and is provided as-is for educational purposes.