#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <NTPClient.h>
#include <FS.h>       // For LittleFS (or SPIFFS)
#include <LittleFS.h> // Explicitly include LittleFS
#include <ArduinoJson.h> // For JSON serialization/deserialization


// #define LED_PIN 2  // change later this is for internal LED so it might freak out!
#define LED_PIN 13  // GPIO14 = D5 on ESP8266
#define NUM_ROWS 12
#define NUM_COLS 12
#define NUM_LEDS (NUM_ROWS * NUM_COLS)
#define MODE_BTN    12    // GPIO12  D6
#define INC_BTN     14   // GPIO14  D5 
#define CONFIG_FILE "/config.json" 
#define COLOR_CONFIG_FILE "/color_config.json" 

const int modeButtonPin = 12;
const int incButtonPin = 14;

struct Config {
  int timezoneHours;
  int timezoneMinutes;
  // Add any other settings you want to save here (e.g., currentFontColorIndex, currentColorIndex)
};

Config settings;

// Create custom parameters
WiFiManagerParameter tzHoursParam("tzHours", "Timezone Hours (e.g. 5 or -5)", "5", 3);
WiFiManagerParameter tzMinutesParam("tzMinutes", "Timezone Minutes (e.g. 30)", "30", 3);

int timezoneHours = 0;
int timezoneMinutes = 0;

// Define UDP instance
WiFiUDP ntpUDP;


Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Variables to store time
int hours = 0;
int minutes = 0;
int seconds = 0;

bool settingTime = false;
bool settingHours = true;
bool lastModeBtnState = HIGH;
bool lastIncBtnState = HIGH;

// For tracking elapsed time
unsigned long previousMillis = 0;
const unsigned long interval = 1000; // 1 second


uint32_t colors[] = {
  strip.Color(8, 8, 8),      // Very soft grey  
  strip.Color(4, 0, 10),     // Deep violet  
  strip.Color(2, 8, 6),      // Dim jade green  
  strip.Color(6, 2, 0),      // Burnt dim orange  
  strip.Color(0, 6, 6),      // Soft cyan/teal  
  strip.Color(6, 0, 6),      // Dim magenta  
  strip.Color(3, 3, 1),      // Faint olive tint  
  strip.Color(10, 2, 5),     // Muted pinkish red  
  strip.Color(0, 3, 10),     // Deep ocean blue  
  strip.Color(1, 6, 3),      // Dim mint green  
  strip.Color(5, 1, 5),      // Subtle purple  
  strip.Color(2, 4, 6),      // Cool steel blue  
  strip.Color(6, 3, 0),      // Faint gold  
  strip.Color(3, 1, 0),      // Dim copper  
  strip.Color(0, 4, 2),      // Dark teal green  
  strip.Color(1, 2, 5),      // Navy-like blue  
  strip.Color(2, 0, 2),      // Dim lavender  
  strip.Color(3, 3, 5),      // Dark steel grey-blue  
  strip.Color(0, 2, 2),      // Very dim aqua  
  strip.Color(2, 2, 2),      // Very soft white
  strip.Color(0, 0, 0)  // OFF
};
const int colorCount = sizeof(colors) / sizeof(colors[0]);

uint32_t colorsFont[] = {
  strip.Color(255, 0, 0),     // Red
  strip.Color(0, 255, 0),     // Green
  strip.Color(0, 0, 255),     // Blue
  strip.Color(255, 255, 0),   // Yellow
  strip.Color(0, 255, 255),   // Cyan
  strip.Color(255, 0, 255),   // Magenta
  strip.Color(255, 128, 0),   // Orange
  strip.Color(0, 255, 128),   // Spring Green
  strip.Color(128, 0, 255),   // Purple
  strip.Color(255, 0, 128),   // Rose
  strip.Color(0, 128, 255),   // Sky Blue
  strip.Color(128, 255, 0),   // Lime
  strip.Color(255, 64, 64),   // Light Red
  strip.Color(64, 255, 64),   // Light Green
  strip.Color(64, 64, 255),   // Light Blue
  strip.Color(255, 255, 128), // Light Yellow
  strip.Color(128, 255, 255), // Light Cyan
  strip.Color(255, 128, 255), // Light Magenta
  strip.Color(192, 192, 192), // Silver
  strip.Color(255, 255, 255),  // White
  strip.Color(0, 0, 0)  // OFF
};
const int fontColorCount = sizeof(colorsFont) / sizeof(colorsFont[0]);

// Color details
uint32_t wordColor = strip.Color(200, 50, 0);
uint32_t bgColor   = strip.Color(0, 10, 10); 
uint32_t textColor   = strip.Color(150, 150, 150); 
uint32_t startUpColor   = strip.Color(100, 0, 20);
uint32_t logoColor   = strip.Color(255, 255, 255); 


int currentColorIndex = 0;
int currentFontColorIndex = 0;

//Character mapping
const int A[][2] = {{1,3}};
const int IT[][2] = {{0,0}, {0,1}};
const int IS[][2] = {{0,3}, {0,4}};
const int HALF[][2] = {{1,2}, {1,3}, {1,4}, {1,5}};
const int TEN1[][2] = {{1,7},{1,8},{1,9}};
const int QUARTER[][2] = {{2,2},{2,3},{2,4},{2,5},{2,6},{2,7},{2,8}};
const int TWENTY[][2] = {{3,0}, {3,1}, {3,2}, {3,3}, {3,4}, {3,5}};
const int FIVE1[][2] = {{3,7}, {3,8}, {3,9}, {3,10}};
const int TO[][2] = {{4,1}, {4,2}};
const int PAST[][2] = {{4,4}, {4,5}, {4,6}, {4,7}};
const int ONE[][2] = {{4,9}, {4,10}, {4,11}};
const int TWO[][2] = {{5,0}, {5,1}, {5,2}};
const int THREE[][2] = {{5,4}, {5,5}, {5,6}, {5,7}, {5,8}};
const int FOUR[][2] = {{6,0}, {6,1}, {6,2}, {6,3}};
const int FIVE2[][2] = {{6,5}, {6,6}, {6,7}, {6,8}};
const int SIX[][2] = {{6,9}, {6,10}, {6,11}};
const int SEVEN[][2] = {{7,1}, {7,2}, {7,3}, {7,4}, {7,5}};
const int EIGHT[][2] = {{7,6}, {7,7}, {7,8}, {7,9}, {7,10}};
const int NINE[][2] = {{8,2}, {8,3}, {8,4}, {8,5}};
const int TEN2[][2] = {{8,7}, {8,8}, {8,9}};
const int ELEVEN[][2] = {{9,0}, {9,1}, {9,2}, {9,3}, {9,4}, {9,5}};
const int TWELVE[][2] = {{9,6}, {9,7}, {9,8}, {9,9}, {9,10}, {9,11}};
const int OCLOCK[][2] = {{10,5}, {10,6}, {10,7}, {10,8}, {10,9}, {10,10}};

const int AM[][2] = {{11,0}, {11,1}};
const int PM[][2] = {{11,10}, {11,11}};
const int H[][2] = {{0,10}};
const int M[][2] = {{0,11}};
const int SEMORA[][2] = {{11,3}, {11,4},{11,5}, {11,6}, {11,7}, {11,8}};
const int BGLETTERS[][2] = {{11,0}, {11,1},{11,2}, {11,9}, {11,10},{11,11} ,{10,0},{10,1},{10,2},{10,3},{10,4},{10,5},{10,6},{10,7},{10,8},{10,9},{10,10},{10,11}};
const int B[][2] = {{0,0},{1,0},{2,0},{3,0},{4,0},{5,0},{6,0},{3,1},{3,2},{3,3},{4,4},{5,4},{6,1},{6,2},{6,3},{0,1},{0,2},{0,3},{1,4},{2,4}};
const int C[][2] = {{0,9},{0,10},{0,11},{1,8},{2,7},{3,7},{4,7},{5,8},{6,9},{6,10},{6,11}};
const int F[][2] = {{0,0},{1,0},{2,0},{3,0},{4,0},{5,0},{6,0},{0,0},{0,1},{0,2},{0,3},{0,4},{3,0},{3,1},{3,2},{3,3}};
const int W[][2] = {{3,1}};
const int WBOX[][2] = {{0,2},{0,3},{0,4},{1,2},{1,4},{2,2},{2,3},{2,4}};

enum Mode { CLOCK_MODE, FONT_COLOR_MODE, BG_COLOR_MODE,  WIFI_MODE };
Mode currentMode = CLOCK_MODE;

NTPClient timeClient(ntpUDP, "pool.ntp.org", 0); // Use 0 offset initially, set later

// Thing that lights up the actual LEDs as words
#define LIGHT_WORD(coords, color) lightWord(coords, sizeof(coords)/sizeof(coords[0]), color)
void lightWord(const int coords[][2], int length, uint32_t color) {
  for (int i = 0; i < length; i++) {
    int index = getLEDIndex(coords[i][0], coords[i][1]);
    strip.setPixelColor(index, color);
  }
}


void saveColorConfig() {
  DynamicJsonDocument doc(128); // Smaller document size is fine for just two ints
  doc["currentColorIndex"] = currentColorIndex;
  doc["currentFontColorIndex"] = currentFontColorIndex;

  File configFile = LittleFS.open(COLOR_CONFIG_FILE, "w");
  if (!configFile) {
    Serial.println("Failed to open color config file for writing");
    return;
  }

  if (serializeJson(doc, configFile) == 0) {
    Serial.println("Failed to write to color config file");
  } else {
    Serial.println("Color configuration saved.");
  }
  configFile.close();
}

// Function to load color configuration from LittleFS
bool loadColorConfig() {
  File configFile = LittleFS.open(COLOR_CONFIG_FILE, "r");
  if (!configFile) {
    Serial.println("Failed to open color config file for reading. Creating default color configuration...");
    // If file doesn't exist, set default values
    currentColorIndex = 0; // Default background color index
    currentFontColorIndex = 0; // Default font color index
    saveColorConfig(); // Save these defaults
    return false; // Indicate that defaults were loaded/created
  }

  size_t size = configFile.size();
  if (size > 256) { // Safety check for large file, adjust if needed
    Serial.println("Color config file size is too large");
    configFile.close();
    return false;
  }

  DynamicJsonDocument doc(128); // Must match size used in saveColorConfig()
  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close();

  if (error) {
    Serial.print(F("Failed to read color file, using default color configuration: "));
    Serial.println(error.c_str());
    // In case of error, load defaults
    currentColorIndex = 0;
    currentFontColorIndex = 0;
    saveColorConfig(); // Save these defaults
    return false;
  }

  currentColorIndex = doc["currentColorIndex"] | 0; // Use | 0 to provide a default if key is missing
  currentFontColorIndex = doc["currentFontColorIndex"] | 0;

  Serial.println("Color configuration loaded.");
  return true;
}

void setColor(int index) {
  bgColor = colors[index];         // Update bgColor with the new color
  strip.setPixelColor(0, bgColor); // Example: setting pixel 0 to bgColor
  strip.show();
  // --- OLD: writeColorIndexToRTC(index, SRAM_ADDR_COLOR_BG);
  currentColorIndex = index; // Update the global index
  saveColorConfig();         // Save the updated color settings
}

void setColorFont(int index) {
  wordColor = colorsFont[index];       // Update wordColor with the new color
  strip.setPixelColor(0, wordColor);   // Example: setting pixel 0 to wordColor
  strip.show();
  // --- OLD: writeColorIndexToRTC(index, SRAM_ADDR_COLOR_FONT);
  currentFontColorIndex = index; // Update the global index
  saveColorConfig();             // Save the updated color settings
}

void handleColorChange() {
  if (digitalRead(incButtonPin) == LOW) {
    delay(200);
    currentColorIndex = (currentColorIndex + 1) % colorCount;
    setColor(currentColorIndex);
  }
}

void handleFontColorChange() {
  if (digitalRead(incButtonPin) == LOW) {
    delay(200);
    currentFontColorIndex = (currentFontColorIndex + 1) % fontColorCount;
    setColorFont(currentFontColorIndex);
  }
}



void saveConfig() {
  DynamicJsonDocument doc(256);
  doc["timezoneHours"] = settings.timezoneHours;
  doc["timezoneMinutes"] = settings.timezoneMinutes;

  File configFile = LittleFS.open(CONFIG_FILE, "w"); // Open file in write mode
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return;
  }

  // Serialize JSON to file. Check if writing was successful.
  if (serializeJson(doc, configFile) == 0) {
    Serial.println("Failed to write to config file");
  } else {
    Serial.println("Configuration saved.");
  }
  configFile.close(); // Always close the file
}

// Function to load configuration from LittleFS
bool loadConfig() {
  File configFile = LittleFS.open(CONFIG_FILE, "r"); // Open file in read mode
  if (!configFile) {
    Serial.println("Failed to open config file for reading. Creating default configuration...");
    // If the file doesn't exist, set default values and save them
    settings.timezoneHours = 0;
    settings.timezoneMinutes = 0;
    // Set defaults for other settings if you add them:
    // settings.fontColorIndex = 0;
    // settings.bgColorIndex = 0;
    saveConfig(); // Save these default settings
    return false; // Indicate that defaults were loaded/created
  }

  // Check file size to prevent potential memory issues if the file is too large
  size_t size = configFile.size();
  if (size > 512) { // You can adjust this max size as needed
    Serial.println("Config file size is too large");
    configFile.close();
    return false;
  }

  // Deserialize JSON from file
  DynamicJsonDocument doc(256); // Must match the size used in saveConfig()
  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close(); // Close the file immediately after reading

  if (error) {
    Serial.print(F("Failed to read config file, using default configuration: "));
    Serial.println(error.c_str());
    // In case of a parsing error, load defaults
    settings.timezoneHours = 0;
    settings.timezoneMinutes = 0;
    saveConfig(); // Save these default settings
    return false;
  }

  // Extract values from the JSON document
  settings.timezoneHours = doc["timezoneHours"];
  settings.timezoneMinutes = doc["timezoneMinutes"];
  // If you add more settings, load them here:
  // settings.fontColorIndex = doc["fontColorIndex"] | 0; // The | 0 provides a default if key is missing
  // settings.bgColorIndex = doc["bgColorIndex"] | 0;

  Serial.println("Configuration loaded.");
  return true; // Indicate that config was successfully loaded
}


void setup() {
  strip.begin();
  strip.show();

  // Initialize LittleFS filesystem
  if (!LittleFS.begin()) {
    LittleFS.format(); // This will erase all files on LittleFS if it's the first time or corrupted
    if (!LittleFS.begin()) {
      delay(3000);
      ESP.restart();
    }
  }

  // Load configuration (timezone, etc.) from LittleFS
  loadConfig(); // This will load existing settings or create/save defaults
  loadColorConfig(); 
  bgColor = colors[currentColorIndex];
  wordColor = colorsFont[currentFontColorIndex];

  WiFiManager wm;
  pinMode(MODE_BTN, INPUT_PULLUP);
  pinMode(INC_BTN, INPUT_PULLUP);

  // Set WiFiManager parameters with the currently loaded/default settings
  char tzHoursStr[4];
  sprintf(tzHoursStr, "%d", settings.timezoneHours);
  tzHoursParam.setValue(tzHoursStr, 4);

  char tzMinutesStr[4];
  sprintf(tzMinutesStr, "%d", settings.timezoneMinutes);
  tzMinutesParam.setValue(tzMinutesStr, 4);

  wm.addParameter(&tzHoursParam);
  wm.addParameter(&tzMinutesParam);

  // Automatically connect or start config portal
  LIGHT_WORD(W, strip.Color(0, 200, 0)); // Indicate starting WiFi connection
  strip.show();
  bool res = wm.autoConnect("MightyClock");

  if (!res) {
    Serial.println("Failed to connect to WiFi or configure. Restarting...");
    LIGHT_WORD(W, strip.Color(200, 0, 0)); // Red W for failure
    strip.show();
    delay(3000);
    ESP.restart();
  } else {
    LIGHT_WORD(W, strip.Color(0, 0, 200)); // Blue W for success
    strip.show();
    delay(1000);
    strip.clear(); // Clear the W after successful connection

    int newTzHours = atoi(tzHoursParam.getValue());
    int newTzMinutes = atoi(tzMinutesParam.getValue());

    if (newTzHours != settings.timezoneHours || newTzMinutes != settings.timezoneMinutes) {
      settings.timezoneHours = newTzHours;
      settings.timezoneMinutes = newTzMinutes;
      saveConfig(); // Save the newly configured values to LittleFS
    }
  }

  // Set the NTP client time offset using the (loaded or newly configured) timezone values
  long utcOffsetInSeconds = settings.timezoneHours * 3600 + settings.timezoneMinutes * 60;
  timeClient.setTimeOffset(utcOffsetInSeconds);
  // Initialize NTP client
  timeClient.begin();

  LIGHT_WORD(WBOX, strip.Color(0, 50, 0)); // Indicate waiting for time, maybe a pulsing box
  strip.show();
  unsigned long startSyncMillis = millis();
  const unsigned long syncTimeout = 15000; // 15 seconds timeout for NTP sync

  // Keep updating NTP client until time is valid or timeout occurs
  while (!timeClient.update()) {
    timeClient.forceUpdate(); // Request an update if the previous one wasn't complete
    delay(500); // Wait a bit before trying again
    if (millis() - startSyncMillis > syncTimeout) {
      // Optionally, show an error state or proceed with default time (00:00)
      // or restart
      break; // Exit the loop if timeout occurs
    }
    // You could also add some animation here to indicate waiting
  }
  strip.clear();
  strip.show();
  Serial.println("NTP time synchronized!");

  hours = timeClient.getHours();
  minutes = timeClient.getMinutes();

  // startUpAnimation(wordColor);
}


int getLEDIndex(int row, int col) {
  if (row % 2 == 0) {
    return row * NUM_COLS + col;
  } else {
    return row * NUM_COLS + (NUM_COLS - 1 - col);
  }
}


void fadeWord(const int coords[][2], int length, uint32_t color1, uint32_t color2, int delayTime = 20) {
  uint8_t r1 = (color1 >> 16) & 0xFF;
  uint8_t g1 = (color1 >> 8) & 0xFF;
  uint8_t b1 = color1 & 0xFF;

  uint8_t r2 = (color2 >> 16) & 0xFF;
  uint8_t g2 = (color2 >> 8) & 0xFF;
  uint8_t b2 = color2 & 0xFF;

  for (int cycle = 0; cycle < 3; cycle++) {
    for (int b = 255; b >= 0; b -= 5) {
      uint32_t interpolatedColor = strip.Color(
        r1 + ((r2 - r1) * b / 255),
        g1 + ((g2 - g1) * b / 255),
        b1 + ((b2 - b1) * b / 255)
      );
      for (int i = 0; i < length; i++) {
        int index = getLEDIndex(coords[i][0], coords[i][1]);
        strip.setPixelColor(index, interpolatedColor);
      }
      strip.show();
      delay(delayTime);
    }
    for (int b = 0; b <= 255; b += 5) {
      uint32_t interpolatedColor = strip.Color(
        r1 + ((r2 - r1) * b / 255),
        g1 + ((g2 - g1) * b / 255),
        b1 + ((b2 - b1) * b / 255)
      );
      for (int i = 0; i < length; i++) {
        int index = getLEDIndex(coords[i][0], coords[i][1]);
        strip.setPixelColor(index, interpolatedColor);
      }
      strip.show();
      delay(delayTime);
    }
  }
  for (int i = 0; i < length; i++) {
    int index = getLEDIndex(coords[i][0], coords[i][1]);
    strip.setPixelColor(index, color2);
  }
  strip.show();
}



// Set background color to all LEDs
void setBackground(uint32_t color) {
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, color);
  }
}

const byte font3x5[10][7] = {
  {0b111, 0b101, 0b101, 0b101, 0b111}, // 0 (5 rows)
  {0b010, 0b110, 0b010, 0b010, 0b111}, // 1
  {0b111, 0b001, 0b111, 0b100, 0b111}, // 2
  {0b111, 0b001, 0b111, 0b001, 0b111}, // 3
  {0b101, 0b101, 0b111, 0b001, 0b001}, // 4
  {0b111, 0b100, 0b111, 0b001, 0b111}, // 5
  {0b111, 0b100, 0b111, 0b101, 0b111}, // 6
  {0b111, 0b001, 0b001, 0b001, 0b001}, // 7
  {0b111, 0b101, 0b111, 0b101, 0b111}, // 8
  {0b111, 0b101, 0b111, 0b001, 0b111}  // 9
};

void showNumber(int num, int offsetX = 3, int offsetY = 3, uint32_t color = strip.Color(255, 255, 255)) {
  int tens = num / 10;
  int ones = num % 10;

  // Width is 5 pixels (x from 0 to 4)
  for (int x = 0; x < 5; x++) {
    // Height is 3 pixels (y from 0 to 2)
    for (int y = 0; y < 3; y++) {
      // Tens digit on top at (offsetX + x, offsetY + y)
      if (font3x5[tens][x] & (1 << (2 - y))) {
        int index = getLEDIndex(offsetX + x, offsetY + y);
        strip.setPixelColor(index, color);
      }

      // Ones digit below tens, so offset Y by 4 (3 pixels + 1 pixel space)
      if (font3x5[ones][x] & (1 << (2 - y))) {
        int index = getLEDIndex(offsetX + x, offsetY + 4 + y);
        strip.setPixelColor(index, color);
      }
    }
  }
  strip.show();
}



void clearMatrix() {
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, 0);
  }
  strip.show();
}



unsigned long lastDebounce = 0;
bool lastButtonState = HIGH;



// Handle mode changes
void handleModeButton() {
  bool reading = digitalRead(modeButtonPin);
  if (reading == LOW && (millis() - lastDebounce) > 200) {
    lastDebounce = millis();
    // Cycle through modes
    if (currentMode == CLOCK_MODE) currentMode = BG_COLOR_MODE;
    else if (currentMode == BG_COLOR_MODE) currentMode = FONT_COLOR_MODE;
    else if (currentMode == BG_COLOR_MODE) currentMode = FONT_COLOR_MODE;
    else if (currentMode == FONT_COLOR_MODE) currentMode = WIFI_MODE;
    else currentMode = CLOCK_MODE;
  }
}



void handleIncrementButton(int &value, int maxValue) {
  if (digitalRead(incButtonPin) == LOW) {
    delay(200); // basic debounce
    value = (value + 1) % (maxValue + 1);
  }
}

void handleWifiReset() {
  if (digitalRead(incButtonPin) == LOW) {
    delay(200); // basic debounce
    LIGHT_WORD(W, strip.Color(200, 50, 0));
    WiFiManager wm;
    wm.resetSettings(); 
    LIGHT_WORD(WBOX, strip.Color(100, 100, 200));
  }
}


unsigned long previousNtpUpdate = 0;
const unsigned long ntpUpdateInterval = 10000; // 10 seconds

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousNtpUpdate >= ntpUpdateInterval) {
    previousNtpUpdate = currentMillis;
    timeClient.update();
    hours = timeClient.getHours();
    minutes = timeClient.getMinutes();
  }

 
  handleModeButton();
  strip.clear();
  if (currentMode == CLOCK_MODE) {
    
    clockWords();
    strip.show();

  // } else if (currentMode == SET_TIME_MINUTE) {
  //   LIGHT_WORD(M, wordColor);
  //   showNumber(minutes);
  //   // handleIncrementButton(minutes, 59); // 0-59

  } else if (currentMode == BG_COLOR_MODE){
    LIGHT_WORD(B, textColor);
    LIGHT_WORD(C, textColor);
    LIGHT_WORD(SEMORA, wordColor);
    LIGHT_WORD(BGLETTERS, bgColor);
    strip.show();
    handleColorChange();
    // handleColorChange();
  } else if (currentMode == FONT_COLOR_MODE){
    LIGHT_WORD(F, textColor);
    LIGHT_WORD(C, textColor);
    LIGHT_WORD(SEMORA, wordColor);
    LIGHT_WORD(BGLETTERS, bgColor);
    strip.show();
    handleFontColorChange();
    // handleFontColorChange();
  } else if (currentMode == WIFI_MODE){
    LIGHT_WORD(W, textColor);
    handleWifiReset();
    strip.show();
  }
}


void startUpAnimation(uint32_t color1) {
  setBackground(strip.Color(0, 0, 0));
  strip.show();

  int layer = 0;
  int size = NUM_COLS;
  while (layer < size / 2) {
    for (int i = layer; i < size - layer; i++) {
      int index = getLEDIndex(layer, i);
      strip.setPixelColor(index, color1);
      strip.show();
      delay(20);
    }
    for (int i = layer + 1; i < size - layer; i++) {
      int index = getLEDIndex(i, size - layer - 1);
      strip.setPixelColor(index, color1);
      strip.show();
      delay(20);
    }
    for (int i = size - layer - 2; i >= layer; i--) {
      int index = getLEDIndex(size - layer - 1, i);
      strip.setPixelColor(index, color1);
      strip.show();
      delay(20);
    }
    for (int i = size - layer - 2; i > layer; i--) {
      int index = getLEDIndex(i, layer);
      strip.setPixelColor(index, color1);
      strip.show();
      delay(20);
    }
    layer++;
  }
  delay(2000);
  fadeWord(SEMORA, sizeof(SEMORA)/sizeof(SEMORA[0]), logoColor, color1);
  strip.clear();
  delay(1000);
  strip.show();
}




void clockWords(){
  setBackground(bgColor);
  LIGHT_WORD(IT, wordColor);
  LIGHT_WORD(IS, wordColor);

  
  if(minutes>2 && minutes<8){ 
    LIGHT_WORD(FIVE1, wordColor);
    LIGHT_WORD(PAST, wordColor);
  }
  if(minutes>7 && minutes<13){
    LIGHT_WORD(TEN1, wordColor);
    LIGHT_WORD(PAST, wordColor);
  }
  if(minutes>12 && minutes<18){
    LIGHT_WORD(A, wordColor);
    LIGHT_WORD(QUARTER, wordColor);
    LIGHT_WORD(PAST, wordColor);
  }
  if(minutes>17 && minutes<23){
    LIGHT_WORD(TWENTY, wordColor);
    LIGHT_WORD(PAST, wordColor);
  }
  if(minutes>22 && minutes<28){
    LIGHT_WORD(TWENTY, wordColor);
    LIGHT_WORD(FIVE1, wordColor);
    LIGHT_WORD(PAST, wordColor);
  }
  if(minutes>27 && minutes<33){
    LIGHT_WORD(HALF, wordColor);
    LIGHT_WORD(PAST, wordColor);
  }
  if(minutes>32 && minutes<38){
    LIGHT_WORD(TWENTY, wordColor);
    LIGHT_WORD(FIVE1, wordColor);
    LIGHT_WORD(TO, wordColor);
  }
  if(minutes>37 && minutes<43){
    LIGHT_WORD(TWENTY, wordColor);
    LIGHT_WORD(TO, wordColor);
  }
  if(minutes>42 && minutes<48){
    LIGHT_WORD(A, wordColor);
    LIGHT_WORD(QUARTER, wordColor);
    LIGHT_WORD(TO, wordColor);
  }    
  if(minutes>47 && minutes<53){
    LIGHT_WORD(TEN1, wordColor);
    LIGHT_WORD(TO, wordColor);
  }
  if(minutes>52 && minutes<58){
    LIGHT_WORD(FIVE1, wordColor);
    LIGHT_WORD(TO, wordColor);
  }
  if(minutes<3){
    LIGHT_WORD(OCLOCK, wordColor);
  }

  if(hours==0 || hours==12){
    if(minutes>32){
      LIGHT_WORD(ONE, wordColor);
    }
    else
    {
      LIGHT_WORD(TWELVE, wordColor);
    }
  }
  if(hours==1 || hours==13){
    if(minutes>32){
      LIGHT_WORD(TWO, wordColor);
    }
    else
    {
      LIGHT_WORD(ONE, wordColor);
    }
  }
  if(hours==2 || hours==14){
    if(minutes>32){
      LIGHT_WORD(THREE, wordColor);
    }
    else
    {
      LIGHT_WORD(TWO, wordColor);
    }
  }
    if(hours==3 || hours==15){
    if(minutes>32){
      LIGHT_WORD(FOUR, wordColor);
    }
    else
    {
      LIGHT_WORD(THREE, wordColor);
    }
  }
  if(hours==4 || hours==16){
    if(minutes>32){
      LIGHT_WORD(FIVE2, wordColor);
    }
    else
    {
      LIGHT_WORD(FOUR, wordColor);
    }
  }
  if(hours==5 || hours==17){
    if(minutes>32){
      LIGHT_WORD(SIX, wordColor);
    }
    else
    {
      LIGHT_WORD(FIVE2, wordColor);
    }
  }
  if(hours==6 || hours==18){
    if(minutes>32){
      LIGHT_WORD(SEVEN, wordColor);
    }
    else
    {
      LIGHT_WORD(SIX, wordColor);
    }
  }
  if(hours==7 || hours==19){
    if(minutes>32){
      LIGHT_WORD(EIGHT, wordColor);
    }
    else
    {
      LIGHT_WORD(SEVEN, wordColor);
    }
  }
  if(hours==8 || hours==20){
    if(minutes>32){
      LIGHT_WORD(NINE, wordColor);
    }
    else
    {
      LIGHT_WORD(EIGHT, wordColor);
    }
  }
  if(hours==9 || hours==21){
    if(minutes>32){
      LIGHT_WORD(TEN2, wordColor);
    }
    else
    {
      LIGHT_WORD(NINE, wordColor);
    }
  }
  if(hours==10 || hours==22){
    if(minutes>32){
      LIGHT_WORD(ELEVEN, wordColor);
    }
    else
    {
      LIGHT_WORD(TEN2, wordColor);
    }
  }
  if(hours==11 || hours==23){
    if(minutes>32){
      LIGHT_WORD(TWELVE, wordColor);
    }
    else
    {
      LIGHT_WORD(ELEVEN, wordColor);
    }
  }


  //AM PM Selector
  if(hours<=12)
      LIGHT_WORD(AM, wordColor);
    else
      LIGHT_WORD(PM, wordColor);
  

}