#include <Arduino.h> // Essential for Arduino sketches
#include <WiFi.h>
#include <WebServer.h>
#include <SD.h>
#include <SPI.h>       // Required for SD Card SPI communication
#include <Wire.h>      // Required for I2C communication with OLED
#include <Preferences.h> // For NVS to store settings

// Libraries for the OLED Display (SH1106)
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h> // Correct library for SH1106G
#include <vector>            // For dynamic file list on OLED
#include <map>               // For localization strings
#include <algorithm>         // For std::min and std::sort

// IMPORTANT: Include the qrcode.h library within an extern "C" block
// This helps prevent name mangling issues when using a C library in a C++ project.
// Make sure you have installed the "QRCode by Richard Moore" library via the Arduino Library Manager.
extern "C" {
  #include <QRCode.h>          // For QR code generation
}

// --- WiFi and Server Configuration ---
const char* ssid = "VFT SERVER";
const char* password = "JAIDEEPVFT1";
WebServer server(80);

// --- SD Card Pin Configuration (VSPI) ---
#define SD_CS_PIN 5

// Define the SPI bus for the SD card
SPIClass spi = SPIClass(VSPI); // Use VSPI for the SD card

// --- 1.3\" OLED Display Pin Configuration (I2C) ---
#define OLED_SDA 21
#define OLED_SCL 22
#define i2c_Address 0x3c // Initialize with the I2C addr 0x3C
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1    // QT-PY / XIAO - Use -1 for ESP32 if not explicitly connected

// Create an instance of the SH1106G display driver
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Global variable to hold the file being uploaded
static File uploadFile;

// Global variable to track the current directory for both web and OLED
String currentPath = "/";

// --- Custom Splash Screen Data (128x64 pixels, monochromatic) ---
// This is your custom 128x64 monochromatic image data
// (Thunder logo + "Made by Jaideep" text, white on black)
// IMPORTANT: Paste your 1024 bytes of PROGMEM data here.
const unsigned char PROGMEM jaideep_thunder_logo[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x03, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x0f, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x3f, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x04, 0xff, 0xff, 0xff, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x09, 0xff, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x1b, 0x3f, 0xf9, 0xff, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x1c, 0x03, 0x83, 0x3f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x1c, 0x00, 0x00, 0x7f, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x18, 0x00, 0x01, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x11, 0xc0, 0x1f, 0xf8, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x60, 0xe0, 0x03, 0xc0, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x01, 0xe0, 0x70, 0xc0, 0x00, 0x18, 0x00, 0x00, 0x4f, 0xbf, 0x7d, 0xf3, 0x7c, 0x10, 0x00,
  0x00, 0x03, 0xc0, 0x38, 0x30, 0x00, 0x08, 0x00, 0x00, 0xcd, 0xb3, 0x61, 0x9b, 0x38, 0x30, 0x00,
  0x00, 0x03, 0xc0, 0x00, 0x0c, 0x00, 0x40, 0x00, 0x00, 0xcd, 0xb3, 0x61, 0x9b, 0x30, 0x30, 0x00,
  0x00, 0x02, 0x80, 0x00, 0x00, 0x03, 0x40, 0x00, 0x00, 0x8c, 0x33, 0x61, 0x9b, 0x30, 0x20, 0x00,
  0x00, 0x04, 0x82, 0x80, 0x80, 0x03, 0x00, 0x00, 0x01, 0x8c, 0x33, 0x79, 0x9b, 0x30, 0x60, 0x00,
  0x00, 0x01, 0x81, 0x7f, 0x00, 0x03, 0x00, 0x00, 0x01, 0x8c, 0x3f, 0x79, 0x9b, 0x30, 0x60, 0x00,
  0x00, 0x01, 0x80, 0x1e, 0x00, 0x03, 0x00, 0x00, 0x03, 0x0c, 0x3e, 0x61, 0x9b, 0x30, 0xc0, 0x00,
  0x00, 0x01, 0x80, 0x0c, 0x00, 0x02, 0x00, 0x00, 0x03, 0x0d, 0xb6, 0x61, 0x9b, 0x30, 0xc0, 0x00,
  0x00, 0x01, 0xc0, 0x00, 0x00, 0x03, 0x00, 0x00, 0x02, 0x0f, 0xb2, 0x7d, 0xfb, 0x30, 0x80, 0x00,
  0x00, 0x00, 0xc0, 0x01, 0x00, 0x03, 0xc0, 0x00, 0x06, 0x0f, 0xb3, 0x7d, 0xf3, 0x11, 0x80, 0x00,
  0x00, 0x00, 0xc0, 0x21, 0x80, 0x03, 0xe0, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x00,
  0x00, 0x00, 0xc0, 0xf0, 0x80, 0x03, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x40, 0x80, 0x80, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x40, 0x80, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x40, 0xc4, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0xc2, 0x00, 0x01, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x10, 0x76, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x18, 0x3e, 0x00, 0x01, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x08, 0x0e, 0x00, 0x00, 0x40, 0x00, 0x00, 0x18, 0xe3, 0x7d, 0xfb, 0xef, 0x80, 0x00,
  0x00, 0x00, 0x04, 0x00, 0x00, 0x01, 0xc0, 0x00, 0x00, 0x18, 0xe3, 0x6d, 0x83, 0x0d, 0xc0, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xc0, 0x00, 0x00, 0x18, 0xe3, 0x6d, 0x83, 0x0c, 0xc0, 0x00,
  0x00, 0x00, 0x00, 0x80, 0x00, 0x01, 0xc0, 0x00, 0x00, 0x19, 0xa3, 0x6d, 0x83, 0x0c, 0xc0, 0x00,
  0x00, 0x00, 0x00, 0x40, 0x04, 0x00, 0xc0, 0x00, 0x00, 0x19, 0xb3, 0x6d, 0xf3, 0xcc, 0xc0, 0x00,
  0x00, 0x00, 0x00, 0x20, 0x01, 0xff, 0x80, 0x00, 0x00, 0x19, 0xb3, 0x6d, 0xf3, 0xcf, 0x80, 0x00,
  0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x19, 0xf3, 0x6d, 0x83, 0x0f, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd9, 0xf3, 0x6d, 0x83, 0x0c, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x10, 0x00, 0x40, 0x00, 0x00, 0x00, 0xfb, 0x13, 0x7d, 0xf3, 0xec, 0x00, 0x00,
  0x00, 0x00, 0x03, 0x90, 0x00, 0x40, 0x00, 0x00, 0x00, 0x7b, 0x1b, 0x7d, 0xfb, 0xec, 0x00, 0x00,
  0x00, 0x00, 0x0f, 0xa0, 0x20, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x1c, 0x20, 0x21, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x1e, 0x00, 0x33, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x7f, 0x00, 0x1f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0xff, 0x80, 0x1f, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0xfe, 0x00, 0x9f, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x0f, 0x00, 0x7f, 0x99, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x7f, 0xc0, 0x3f, 0x8d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x01, 0xff, 0xe0, 0x0f, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x02, 0x03, 0xf0, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x06, 0x03, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x01, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// --- OLED UI State Management ---
enum OLEDScreenState {
    SCREEN_IDLE,
    SCREEN_FILE_TREE,
    SCREEN_UPLOAD_PROGRESS,
    SCREEN_CONFIRM_DELETE,
    SCREEN_CONFIRM_FORMAT,
    SCREEN_AUTH_PIN,
    SCREEN_FILE_PREVIEW,
    SCREEN_SETTINGS_MENU,
    SCREEN_THEME_SELECT,
    SCREEN_LANGUAGE_SELECT,
    SCREEN_SLEEP_ANIMATION,
    SCREEN_QR_CODE,            // New: For displaying QR codes
    SCREEN_ANALYTICS,          // New: For displaying data analytics
    SCREEN_ACTIVE_UPLOADS,     // New: For displaying multi-device uploads
    SCREEN_USER_INFO,          // New: For displaying connected user info
    SCREEN_USER_INPUT_CREATE,  // New: For user creation input
    SCREEN_USER_INPUT_LOGIN    // New: For user login input
};
OLEDScreenState currentOLEDState = SCREEN_IDLE;

// --- Physical Button Configuration ---
// Assuming 3 buttons for UI navigation and interaction
#define BTN_UP_PIN 15    // GPIO for "Up" or "Increment"
#define BTN_DOWN_PIN 16  // GPIO for "Down" or "Decrement"
#define BTN_SELECT_PIN 17 // GPIO for "Select" or "Confirm" / "Wake-up"

// Debounce variables for buttons
unsigned long lastButtonPressTime = 0;
const unsigned long BUTTON_DEBOUNCE_DELAY = 200; // milliseconds

// --- OLED File Tree Variables ---
std::vector<String> oledFileList; // Stores names of files/folders in current view
int oledScrollIndex = 0;         // Starting index for items displayed on OLED
int oledSelectedIndex = 0;       // Index of the currently highlighted item
int oledMenuLevel = 0;           // 0 for main menu, 1 for sub-menus

// --- Security / PIN Access Variables ---
const char* DEFAULT_PIN = "1234"; // Default PIN. In a real product, do NOT hardcode!
String enteredPIN = "";           // PIN currently being entered by user
int pinDigitIndex = 0;            // Current digit position being entered (0 for first digit)
int currentDigitValue = 0;        // Value of the current digit being selected (0-9)
bool isServerLocked = true;       // True if server access requires PIN unlock

// --- Inactivity / Power Management Variables ---
unsigned long lastActivityTime = 0;
const unsigned long LOCK_TIMEOUT_MS = 5 * 60 * 1000; // 5 minutes to auto-lock server
const unsigned long IDLE_SLEEP_TIMEOUT_MS = 10 * 60 * 1000; // 10 minutes to go to sleep (AP disconnect)
bool isAPActive = true;           // True if WiFi AP is currently active

// --- Preferences (NVS) ---
Preferences preferences;
String currentTheme = "dark";   // "dark" or "light"
String currentLanguage = "en";  // "en", "hi", "de" (English, Hindi, German)

// --- Localization Maps ---
// Using std::map for simple key-value string translations
std::map<String, String> en_strings = {
    {"SERVER_NAME", "VFT Server"},
    {"ENTER_PIN", "Enter PIN:"},
    {"INCORRECT_PIN", "Incorrect PIN!"},
    {"SERVER_UNLOCKED", "Server Unlocked!"},
    {"SERVER_LOCKED_INACTIVE", "Server Locked (Idle)"},
    {"GO_TO_SLEEP", "Going to Sleep..."},
    {"WAKING_UP", "Waking Up..."},
    {"SD_INIT_FAIL", "SD Init FAIL"},
    {"AP_ACTIVE", "AP Mode Active!"},
    {"FILE_RECEIVED", "File Received"},
    {"FILE_DELETED", "File Deleted!"},
    {"FOLDER_DELETED", "Folder Deleted!"},
    {"FOLDER_NOT_EMPTY", "Folder Not Empty!"},
    {"DELETE_FAILED", "Delete Failed!"},
    {"ITEM_NOT_FOUND", "Item Not Found!"},
    {"FILE_RENAMED", "File Renamed!"},
    {"RENAME_FAILED", "Rename Failed!"},
    {"FOLDER_CREATED", "Folder Created!"},
    {"CREATE_FAILED", "Create Failed!"},
    {"PREVIEW", "Preview:"},
    {"CANNOT_PREVIEW", "Cannot preview."},
    {"SETTINGS", "--- Settings ---"},
    {"THEME", "Theme:"},
    {"LANGUAGE", "Language:"},
    {"CHANGE_THEME", "1. Change Theme"},
    {"CHANGE_LANGUAGE", "2. Change Language"},
    {"DARK", "Dark"},
    {"LIGHT", "Light"},
    {"ENGLISH", "English"},
    {"HINDI", "Hindi"},
    {"GERMAN", "German"},
    {"CONFIRM_DELETE", "Confirm Delete?"},
    {"YES", "YES"},
    {"NO", "NO"},
    {"SERVING", "Serving:"},
    {"RECEIVING", "Receiving:"},
    {"STATUS", "Status:"},
    {"DIR", "Dir:"},
    {"FILE", "File:"},
    {"PARENT_DIR", ".. (Parent)"},
    {"FORMAT_SD", "Format SD Card?"},
    {"FORMAT_FAIL", "Format Failed!"},
    {"FORMAT_SUCCESS", "Format Complete!"},
    {"FORMATTING", "Formating..."},
    {"QR_SCAN_DL", "Scan QR to DL"}, // New
    {"ANALYTICS", "--- Analytics ---"}, // New
    {"TOTAL_UPLOADS", "Uploads:"}, // New
    {"TOTAL_DOWNLOADS", "Downloads:"}, // New
    {"MOST_DOWNLOADED", "Most DL:"}, // New
    {"STORAGE_USED", "Used:"}, // New
    {"STORAGE_FREE", "Free:"}, // New
    {"ACTIVE_UPLOADS", "Active Uploads"}, // New
    {"BROADCAST_MODE", "Broadcast Mode"}, // New
    {"BROADCASTING", "Broadcasting"}, // New
    {"USER_MGMT", "--- User Accounts ---"}, // New
    {"CREATE_USER", "1. Create User"}, // New
    {"LOGIN_USER", "2. Login User"}, // New
    {"LOGGED_IN_AS", "Logged in as:"}, // New
    {"UPLOAD_RIGHTS", "Upload Rights:"}, // New
    {"ENABLED", "Enabled"}, // New
    {"DISABLED", "Disabled"}, // New
    {"USER_CREATED", "User Created!"}, // New
    {"LOGIN_SUCCESS", "Login Success!"}, // New
    {"LOGIN_FAIL", "Login Failed!"}, // New
    {"ENTER_USERNAME", "Username:"}, // New
    {"ENTER_PASSWORD", "Password:"}, // New
    {"USER_NOT_FOUND", "User Not Found"}, // New
    {"INSUFFICIENT_PERMS", "Insufficient Permissions!"} // New
};

std::map<String, String> hi_strings = {
    {"SERVER_NAME", "वीएफटी सर्वर"},
    {"ENTER_PIN", "पिन दर्ज करें:"},
    {"INCORRECT_PIN", "गलत पिन!"},
    {"SERVER_UNLOCKED", "सर्वर खुला!"},
    {"SERVER_LOCKED_INACTIVE", "सर्वर बंद (निष्क्रिय)"},
    {"GO_TO_SLEEP", "सो रहा है..."},
    {"WAKING_UP", "जाग रहा है..."},
    {"SD_INIT_FAIL", "एसडी इनिट विफल!"},
    {"AP_ACTIVE", "एपी मोड सक्रिय!"},
    {"FILE_RECEIVED", "फ़ाइल प्राप्त हुई"},
    {"FILE_DELETED", "फ़ाइल हटाई गई!"},
    {"FOLDER_DELETED", "फ़ोल्डर हटाई गई!"},
    {"FOLDER_NOT_EMPTY", "फ़ोल्डर खाली नहीं!"},
    {"DELETE_FAILED", "हटाना विफल!"},
    {"ITEM_NOT_FOUND", "आइटम नहीं मिला!"},
    {"FILE_RENAMED", "फ़ाइल का नाम बदला!"},
    {"RENAME_FAILED", "नाम बदलना विफल!"},
    {"FOLDER_CREATED", "फ़ोल्डर बनाया!"},
    {"CREATE_FAILED", "बनाना विफल!"},
    {"PREVIEW", "पूर्वावलोकन:"},
    {"CANNOT_PREVIEW", "पूर्वावलोकन नहीं हो सकता।"},
    {"SETTINGS", "--- सेटिंग्स ---"},
    {"THEME", "थीम:"},
    {"LANGUAGE", "भाषा:"},
    {"CHANGE_THEME", "1. थीम बदलें"},
    {"CHANGE_LANGUAGE", "2. भाषा बदलें"},
    {"DARK", "गहरा"},
    {"LIGHT", "हल्का"},
    {"ENGLISH", "अंग्रेजी"},
    {"HINDI", "हिंदी"},
    {"GERMAN", "जर्मन"},
    {"CONFIRM_DELETE", "हटाने की पुष्टि करें?"},
    {"YES", "हाँ"},
    {"NO", "नहीं"},
    {"SERVING", "सेवा दे रहा है:"},
    {"RECEIVING", "प्राप्त कर रहा है:"},
    {"STATUS", "स्थिति:"},
    {"DIR", "निर्देशिका:"},
    {"FILE", "फ़ाइल:"},
    {"PARENT_DIR", ".. (अभिभावक)"},
    {"FORMAT_SD", "एसडी कार्ड फॉर्मेट करें?"},
    {"FORMAT_FAIL", "फॉर्मेट विफल!"},
    {"FORMAT_SUCCESS", "फॉर्मेट पूर्ण!"},
    {"FORMATTING", "फॉर्मेट कर रहा है..."},
    {"QR_SCAN_DL", "डाउनलोड के लिए QR स्कैन करें"}, // New
    {"ANALYTICS", "--- विश्लेषण ---"}, // New
    {"TOTAL_UPLOADS", "अपलोड:"}, // New
    {"TOTAL_DOWNLOADS", "डाउनलोड:"}, // New
    {"MOST_DOWNLOADED", "सबसे ज्यादा डाउनलोड:"}, // New
    {"STORAGE_USED", "प्रयुक्त:"}, // New
    {"STORAGE_FREE", "रिक्त:"}, // New
    {"ACTIVE_UPLOADS", "सक्रिय अपलोड"}, // New
    {"BROADCAST_MODE", "प्रसारण मोड"}, // New
    {"BROADCASTING", "प्रसारण हो रहा है"}, // New
    {"USER_MGMT", "--- उपयोगकर्ता खाते ---"}, // New
    {"CREATE_USER", "1. उपयोगकर्ता बनाएँ"}, // New
    {"LOGIN_USER", "2. उपयोगकर्ता लॉग इन करें"}, // New
    {"LOGGED_IN_AS", "के रूप में लॉग इन:"}, // New
    {"UPLOAD_RIGHTS", "अपलोड अधिकार:"}, // New
    {"ENABLED", "सक्षम"}, // New
    {"DISABLED", "अक्षम"}, // New
    {"USER_CREATED", "उपयोगकर्ता बनाया गया!"}, // New
    {"LOGIN_SUCCESS", "लॉगिन सफल!"}, // New
    {"LOGIN_FAIL", "लॉगिन विफल!"}, // New
    {"ENTER_USERNAME", "उपयोगकर्ता नाम:"}, // New
    {"ENTER_PASSWORD", "पासवर्ड:"}, // New
    {"USER_NOT_FOUND", "उपयोगकर्ता नहीं मिला"}, // New
    {"INSUFFICIENT_PERMS", "अपर्याप्त अनुमतियाँ!"} // New
};

std::map<String, String> de_strings = {
    {"SERVER_NAME", "VFT Server"},
    {"ENTER_PIN", "PIN eingeben:"},
    {"INCORRECT_PIN", "Falscher PIN!"},
    {"SERVER_UNLOCKED", "Server entsperrt!"},
    {"SERVER_LOCKED_INACTIVE", "Server gesperrt (inaktiv)"},
    {"GO_TO_SLEEP", "Schlafe ein..."},
    {"WAKING_UP", "Wache auf..."},
    {"SD_INIT_FAIL", "SD-Initialisierung fehlgeschlagen!"},
    {"AP_ACTIVE", "AP-Modus aktiv!"},
    {"FILE_RECEIVED", "Datei empfangen"},
    {"FILE_DELETED", "Datei gelöscht!"},
    {"FOLDER_DELETED", "Ordner gelöscht!"},
    {"FOLDER_NOT_EMPTY", "Ordner nicht leer!"},
    {"DELETE_FAILED", "Löschen fehlgeschlagen!"},
    {"ITEM_NOT_FOUND", "Element nicht gefunden!"},
    {"FILE_RENAMED", "Datei umbenannt!"},
    {"RENAME_FAILED", "Umbenennen fehlgeschlagen!"},
    {"FOLDER_CREATED", "Ordner erstellt!"},
    {"CREATE_FAILED", "Erstellen fehlgeschlagen!"},
    {"PREVIEW", "Vorschau:"},
    {"CANNOT_PREVIEW", "Kann nicht vorschauen."},
    {"SETTINGS", "--- Einstellungen ---"},
    {"THEME", "Thema:"},
    {"LANGUAGE", "Sprache:"},
    {"CHANGE_THEME", "1. Thema ändern"},
    {"CHANGE_LANGUAGE", "2. Sprache ändern"},
    {"DARK", "Dunkel"},
    {"LIGHT", "Hell"},
    {"ENGLISH", "Englisch"},
    {"HINDI", "Hindi"},
    {"GERMAN", "Deutsch"},
    {"CONFIRM_DELETE", "Löschen bestätigen?"},
    {"YES", "JA"},
    {"NO", "NEIN"},
    {"SERVING", "Serviere:"},
    {"RECEIVING", "Empfange:"},
    {"STATUS", "स्थिति:"},
    {"DIR", "Verzeichnis:"},
    {"FILE", "Datei:"},
    {"PARENT_DIR", ".. (Übergeordnetes)"},
    {"FORMAT_SD", "SD-Karte formatieren?"},
    {"FORMAT_FAIL", "Format fehlgeschlagen!"},
    {"FORMAT_SUCCESS", "Format abgeschlossen!"},
    {"FORMATTING", "Formatiere..."},
    {"QR_SCAN_DL", "QR zum Download scannen"}, // New
    {"ANALYTICS", "--- Analysen ---"}, // New
    {"TOTAL_UPLOADS", "Uploads:"}, // New
    {"TOTAL_DOWNLOADS", "Downloads:"}, // New
    {"MOST_DOWNLOADED", "Meist heruntergeladen:"}, // New
    {"STORAGE_USED", "Genutzt:"}, // New
    {"STORAGE_FREE", "Frei:"}, // New
    {"ACTIVE_UPLOADS", "Aktive Uploads"}, // New
    {"BROADCAST_MODE", "Broadcast-Modus"}, // New
    {"BROADCASTING", "Sende"}, // New
    {"USER_MGMT", "--- Benutzerkonten ---"}, // New
    {"CREATE_USER", "1. Benutzer erstellen"}, // New
    {"LOGIN_USER", "2. Benutzer anmelden"}, // New
    {"LOGGED_IN_AS", "Angemeldet als:"}, // New
    {"UPLOAD_RIGHTS", "Upload-Rechte:"}, // New
    {"ENABLED", "Aktiviert"}, // New
    {"DISABLED", "Deaktiviert"}, // New
    {"USER_CREATED", "Benutzer erstellt!"}, // New
    {"LOGIN_SUCCESS", "Anmeldung erfolgreich!"}, // New
    {"LOGIN_FAIL", "Anmeldung fehlgeschlagen!"}, // New
    {"ENTER_USERNAME", "Benutzername:"}, // New
    {"ENTER_PASSWORD", "Passwort:"}, // New
    {"USER_NOT_FOUND", "Benutzer nicht gefunden"}, // New
    {"INSUFFICIENT_PERMS", "Ungenügende Berechtigungen!"} // New
};

// Pointer to the current language map
std::map<String, String>* current_lang_map = &en_strings;

// Function to get a localized string
String getOLEDString(String key) {
    if (current_lang_map->count(key)) {
        return (*current_lang_map)[key];
    }
    return key; // Return key itself if not found
}

// --- Data Analytics Variables ---
unsigned long totalUploads = 0;
unsigned long totalDownloads = 0;
// Map to store download counts for each file, limited to top X for persistence
// When saving, only store top few to NVS.
std::map<String, unsigned int> fileDownloadCounts;

// --- Multi-Device Real-Time Upload Display Variables ---
struct UploadInfo {
    String filename;
    String clientIP;
    int percentage;
    String clientUserAgent; // For device fingerprinting
};
std::vector<UploadInfo> activeUploads;

// --- Device Fingerprinting / Connected Devices ---
// Stores IP -> User-Agent (simplified fingerprint)
std::map<String, String> connectedClientFingerprints;

// --- Broadcast Mode Variables ---
String broadcastFilePath = ""; // Path of the file currently set for broadcast

// --- Offline Temporary User Accounts Variables ---
struct TempUserAccount {
    String username;
    String pinHash; // Simple string PIN, not truly hashed. For more security, use a proper hashing algorithm.
    bool uploadRights;
};
std::vector<TempUserAccount> tempUserAccounts;
String currentLoggedInUser = ""; // Tracks the username of the user currently logged in via web UI
bool currentLoggedInUserUploadRights = false; // Tracks upload rights of the current logged in user

// --- Forward Declarations for OLED, Button, and Web functions ---
void initDisplay();
void displayIdleScreen();
void displayOperation(String message);
void displayProgress(int percentage, String filename);
void displayPINEntryScreen();
void displayConfirmation(String message, void (*onYes)(), void (*onNo)());
void displayFileTree();
void populateOLEDFileList(String path);
char getFileIconChar(String filename);
void displayFilePreview(String filename);
void displaySettingsMenu();
void displayThemeSelectMenu();
void displayLanguageSelectMenu();
void displaySleepAnimation();
void displayQRCode(String urlToEncode); // New: QR code display
void displayAnalyticsScreen(); // New: Analytics display
void displayActiveUploadsScreen(); // New: Active uploads display
void displayUserInfoScreen(); // New: User info display
void handleOLEDButtons(); // Master button handler
void handlePINEntry();
void handleFileTreeNavigation();
void handleConfirmationButtons();
void handleSettingsNavigation();
void handleThemeSelection();
void handleLanguageSelection();
void handleUserAccountNavigation(); // New: User account menu handling
void handleUserLoginPrompt(); // New: User login prompt handling
void handleUserCreatePrompt(); // New: User create prompt handling

void handleRoot();
void handleFileUpload();
void handleDelete();
void handleDeleteConfirmed(); // Callback for OLED delete confirmation
void handleRename();
void handleMkdir();
void handleFileRequest(String filename);
void handleFormatSD();
void formatSDConfirmed(); // Callback for OLED format confirmation
void handleSetBroadcast(); // New: Set file for broadcast
void handleBroadcastDownload(); // New: Serve broadcast file
void handleCreateUser(); // New: Create temporary user
void handleLogin(); // New: Login temporary user
void handleLogout(); // New: Logout user

String getContentType(String filename);
String formatBytes(size_t bytes);
bool isViewable(String filename);
String generateThemedHtml(); // Generates HTML with correct theme CSS

void enterSleepMode();
void wakeUpFromSleep();

void loadPreferences();
void savePreferences();

// --- Confirmation Callback Variables ---
void (*confirmationYesCallback)() = nullptr;
void (*confirmationNoCallback)() = nullptr;
String confirmationMessage = "";
String pendingDeleteItem = ""; // Stores the item to be deleted if confirmation is pending

// --- User Input for Account Creation/Login ---
String tempUsernameInput = "";
String tempPasswordInput = "";
bool tempUserHasUploadRights = false;
int userDigitIndex = 0;
int userCharIndex = 0; // Current character index for username/password entry

// =======================================================================
//    OLED DISPLAY HELPER FUNCTIONS
// =======================================================================

void initDisplay() {
  Wire.begin(OLED_SDA, OLED_SCL);
  delay(250); // wait for the OLED to power up

  if (!display.begin(i2c_Address, true)) {
    Serial.println(F("SH110X allocation failed"));
    for (;;); // Don't proceed, loop forever
  }

  // --- Custom Splash Screen Logic ---
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setRotation(0);
  display.drawBitmap(0, 0, jaideep_thunder_logo, SCREEN_WIDTH, SCREEN_HEIGHT, SH110X_WHITE);
  display.display();
  delay(3000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setRotation(0);
  display.display();
}

void displayIdleScreen() {
  currentOLEDState = SCREEN_IDLE;
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);

  // "Jai's" - Smaller text, centered
  const char* line1 = "Jai's";
  int line1_char_count = strlen(line1);
  int line1_width_pixels = line1_char_count * 6;
  int line1_x = (SCREEN_WIDTH - line1_width_pixels) / 2;
  display.setCursor(line1_x, 15);
  display.println(line1);

  // "VFT Server" - Larger text, centered
  display.setTextSize(2);
  String line2_str = getOLEDString("SERVER_NAME");
  int line2_char_count = line2_str.length();
  int line2_width_pixels = line2_char_count * 12;
  int line2_x = (SCREEN_WIDTH - line2_width_pixels) / 2;
  display.setCursor(line2_x, 35);
  display.println(line2_str);

  display.display();
}

void displayOperation(String message) {
  currentOLEDState = SCREEN_IDLE; // Temporary state for showing status
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 20);
  display.print(getOLEDString("STATUS") + " ");
  display.println(message);
  display.display();
}

// Updated displayProgress to handle multiple active uploads for the new screen
void displayProgress(int percentage, String filename, String clientIP) {
    currentOLEDState = SCREEN_UPLOAD_PROGRESS; // Still primarily for single upload on OLED
    display.clearDisplay();

    // Draw file name
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 0);
    display.print(getOLEDString("RECEIVING") + " ");
    display.setCursor(0, 10);
    if (filename.length() > 21) {
        display.print(filename.substring(0, 18) + "...");
    } else {
        display.print(filename);
    }

    // Draw progress bar outline
    display.drawRect(5, 30, SCREEN_WIDTH - 10, 10, SH110X_WHITE);

    // Draw filled part of progress bar
    int barWidth = map(percentage, 0, 100, 0, SCREEN_WIDTH - 12);
    display.fillRect(6, 31, barWidth, 8, SH110X_WHITE);

    // Draw percentage text
    display.setTextSize(2);
    display.setCursor(45, 45);
    display.print(percentage);
    display.print("%");

    display.display();
}


void displayPINEntryScreen() {
  currentOLEDState = SCREEN_AUTH_PIN;
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 10);
  display.println(getOLEDString("ENTER_PIN"));
  display.setCursor(0, 35);
  for (int i = 0; i < enteredPIN.length(); i++) {
    display.print("*");
  }
  // Show the current digit being selected, next to the asterisks
  if (pinDigitIndex < strlen(DEFAULT_PIN)) {
      display.print(currentDigitValue);
  }
  display.display();
}

// Universal confirmation screen for OLED
void displayConfirmation(String message, void (*onYesCallback)(), void (*onNoCallback)()) {
    currentOLEDState = SCREEN_CONFIRM_DELETE; // Can generalize to SCREEN_CONFIRMATION
    confirmationMessage = message;
    confirmationYesCallback = onYesCallback;
    confirmationNoCallback = onNoCallback;

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 10);
    display.println(confirmationMessage);

    // Add YES/NO buttons indication
    display.setTextSize(2);
    display.setCursor(10, 40);
    display.print(getOLEDString("YES")); // For BTN_UP
    display.setCursor(SCREEN_WIDTH - 10 - (getOLEDString("NO").length() * 12), 40); // For BTN_DOWN
    display.print(getOLEDString("NO"));

    display.display();
}

char getFileIconChar(String filename) {
    filename.toLowerCase();
    if (filename.endsWith("/")) return 'D'; // Directory
    if (filename.endsWith(".txt") || filename.endsWith(".log") || filename.endsWith(".md")) return 'T'; // Text file
    if (filename.endsWith(".jpg") || filename.endsWith(".png") || filename.endsWith(".gif") || filename.endsWith(".bmp")) return 'P'; // Image file
    if (filename.endsWith(".mp3") || filename.endsWith(".wav") || filename.endsWith(".flac")) return 'A'; // Audio file
    if (filename.endsWith(".zip") || filename.endsWith(".rar") || filename.endsWith(".7z")) return 'Z'; // Archive file
    if (filename.endsWith(".html") || filename.endsWith(".htm")) return 'H'; // HTML file
    if (filename.endsWith(".css")) return 'C'; // CSS file
    if (filename.endsWith(".js")) return 'J'; // JavaScript file
    if (filename.endsWith(".pdf")) return 'F'; // PDF file
    if (filename.endsWith(".doc") || filename.endsWith(".docx")) return 'W'; // Word document
    if (filename.endsWith(".xls") || filename.endsWith(".xlsx")) return 'S'; // Spreadsheet
    if (filename.endsWith(".ppt") || filename.endsWith(".pptx")) return 'O'; // Presentation
    if (filename.endsWith(".ino") || filename.endsWith(".cpp") || filename.endsWith(".c") || filename.endsWith(".h")) return 'X'; // Code file
    return '?'; // Generic/Unknown file
}

void populateOLEDFileList(String path) {
    oledFileList.clear();
    File root = SD.open(path);
    if (!root || !root.isDirectory()) {
        displayOperation("Dir Error!"); // Show error on OLED
        delay(1000);
        return;
    }

    if (path != "/") {
        oledFileList.push_back(getOLEDString("PARENT_DIR") + "/");
    }

    File entry = root.openNextFile();
    while (entry) {
        String entryName = String(entry.name());
        // For subdirectories, the name() function returns full path.
        // We only want the name relative to the current path.
        if (entryName.startsWith(path)) {
            entryName = entryName.substring(path.length());
        }

        if (entry.isDirectory()) {
            oledFileList.push_back(entryName + "/");
        } else {
            oledFileList.push_back(entryName);
        }
        entry.close();
        entry = root.openNextFile();
    }
    root.close();
    oledScrollIndex = 0;
    oledSelectedIndex = 0;
}

void displayFileTree() {
    currentOLEDState = SCREEN_FILE_TREE;
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);

    display.setCursor(0, 0);
    display.print(getOLEDString("DIR") + " ");
    // Truncate path for display if too long
    String displayPath = currentPath;
    if (displayPath.length() > 20) {
        displayPath = "..." + displayPath.substring(displayPath.length() - 17);
    }
    display.println(displayPath);
    display.drawFastHLine(0, 9, SCREEN_WIDTH, SH110X_WHITE); // Separator

    int maxVisibleItems = (SCREEN_HEIGHT - 12) / 8; // Calculate how many lines fit
    for (int i = 0; i < maxVisibleItems; i++) {
        int itemIndex = oledScrollIndex + i;
        if (itemIndex < oledFileList.size()) {
            if (itemIndex == oledSelectedIndex) {
                display.setTextColor(SH110X_BLACK, SH110X_WHITE); // Invert colors for selected item
            } else {
                display.setTextColor(SH110X_WHITE);
            }
            display.setCursor(0, 12 + (i * 8)); // Start below the separator

            String itemName = oledFileList[itemIndex];
            char icon = getFileIconChar(itemName);

            // Truncate filename if too long to fit
            if (itemName.length() > 18) { // Adjust based on icon + desired chars
                itemName = itemName.substring(0, 15) + "...";
            }
            display.print(icon);
            display.print(" ");
            display.println(itemName);
        }
    }
    display.display();
}

void displayFilePreview(String filename) {
    currentOLEDState = SCREEN_FILE_PREVIEW;
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 0);

    display.print(getOLEDString("PREVIEW") + " ");
    String displayFilename = filename;
    int lastSlash = displayFilename.lastIndexOf('/');
    if (lastSlash != -1) {
        displayFilename = displayFilename.substring(lastSlash + 1);
    }
    if (displayFilename.length() > 15) {
        display.println(displayFilename.substring(0, 12) + "...");
    } else {
        display.println(displayFilename);
    }
    display.drawFastHLine(0, 9, SCREEN_WIDTH, SH110X_WHITE);

    File file = SD.open(filename, FILE_READ);
    if (!file) {
        display.setCursor(0, 20);
        display.println(getOLEDString("CANNOT_PREVIEW"));
        display.display();
        return;
    }

    int linesRead = 0;
    while (file.available() && linesRead < 6) { // Read first 6 lines
        String line = file.readStringUntil('\n');
        line.trim(); // Remove newline chars
        display.setCursor(0, 12 + (linesRead * 8)); // Adjust Y position
        // Truncate line if too long for screen
        if (line.length() > 21) {
            line = line.substring(0, 18) + "...";
        }
        display.println(line);
        linesRead++;
    }
    file.close();
    display.display();
}

void displaySettingsMenu() {
    currentOLEDState = SCREEN_SETTINGS_MENU;
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 0);
    display.println(getOLEDString("SETTINGS"));
    display.drawFastHLine(0, 9, SCREEN_WIDTH, SH110X_WHITE);

    // Items for settings menu
    String menuItems[] = {
        getOLEDString("CHANGE_THEME"),
        getOLEDString("CHANGE_LANGUAGE"),
        getOLEDString("ANALYTICS"), // New: Analytics menu item
        getOLEDString("USER_MGMT") // New: User Management menu item
    };
    int numItems = sizeof(menuItems) / sizeof(menuItems[0]);

    for (int i = 0; i < numItems; ++i) {
        if (i == oledSelectedIndex) { // Use oledSelectedIndex for menu navigation
            display.setTextColor(SH110X_BLACK, SH110X_WHITE);
        } else {
            display.setTextColor(SH110X_WHITE);
        }
        display.setCursor(0, 12 + (i * 8));
        display.println(menuItems[i]);
    }
    display.display();
}

void displayThemeSelectMenu() {
    currentOLEDState = SCREEN_THEME_SELECT;
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 0);
    display.println(getOLEDString("THEME"));
    display.drawFastHLine(0, 9, SCREEN_WIDTH, SH110X_WHITE);

    String themes[] = {getOLEDString("DARK"), getOLEDString("LIGHT")};
    int numThemes = sizeof(themes) / sizeof(themes[0]);

    for (int i = 0; i < numThemes; ++i) {
        if (i == oledSelectedIndex) {
            display.setTextColor(SH110X_BLACK, SH110X_WHITE);
        } else {
            display.setTextColor(SH110X_WHITE);
        }
        display.setCursor(0, 12 + (i * 8));
        display.print("> ");
        display.println(themes[i]);
    }
    display.display();
}

void displayLanguageSelectMenu() {
    currentOLEDState = SCREEN_LANGUAGE_SELECT;
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 0);
    display.println(getOLEDString("LANGUAGE"));
    display.drawFastHLine(0, 9, SCREEN_WIDTH, SH110X_WHITE);

    String languages[] = {getOLEDString("ENGLISH"), getOLEDString("HINDI"), getOLEDString("GERMAN")};
    int numLanguages = sizeof(languages) / sizeof(languages[0]);

    for (int i = 0; i < numLanguages; ++i) {
        if (i == oledSelectedIndex) {
            display.setTextColor(SH110X_BLACK, SH110X_WHITE);
        } else {
            display.setTextColor(SH110X_WHITE);
        }
        display.setCursor(0, 12 + (i * 8));
        display.print("> ");
        display.println(languages[i]);
    }
    display.display();
}

void displaySleepAnimation() {
    currentOLEDState = SCREEN_SLEEP_ANIMATION;
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 25);
    display.println(getOLEDString("GO_TO_SLEEP"));
    display.display();
}

// New: Display QR Code - Modified to show only QR code, no text
void displayQRCode(String urlToEncode) {
    currentOLEDState = SCREEN_QR_CODE;
    display.clearDisplay();
    // No text display here, so we remove the text-related lines.

    // Ensure the URL isn't too long for QR code version 3 (max 42 chars)
    // For larger URLs, a higher QR version would be needed, or truncation.
    // Given OLED size, QR version 3 is a good fit (29x29 pixels).
    if (urlToEncode.length() > 42) {
        urlToEncode = urlToEncode.substring(0, 42); // Truncate if too long
    }

    // Create the QR code
    QRCode qrcode; // This struct comes from qrcode.h
    uint8_t qrcodeBytes[qrcode_getBufferSize(3)]; // This function comes from qrcode.h

    // Arguments are: qrcode_object, modules_buffer, version, ECC_level, data_string
    qrcode_initText(&qrcode, qrcodeBytes, 3, ECC_LOW, urlToEncode.c_str()); // Version 3, ECC_LOW

    // Calculate QR code size and position to center it
    int qrSize = qrcode.size; // Should be 29 for version 3
    int scale = 2; // Scale QR code up to make it more visible (29 * 2 = 58 pixels)
    int xOffset = (SCREEN_WIDTH - (qrSize * scale)) / 2;
    // New yOffset: Center vertically on the entire screen, as no header text is present.
    int yOffset = (SCREEN_HEIGHT - (qrSize * scale)) / 2;

    // Draw the QR code
    for (int y = 0; y < qrSize; y++) {
        for (int x = 0; x < qrSize; x++) {
            if (qrcode_getModule(&qrcode, x, y)) { // This function comes from qrcode.h
                display.fillRect(x * scale + xOffset, y * scale + yOffset, scale, scale, SH110X_WHITE);
            }
        }
    }
    display.display();
}
// New: Display Data Analytics
void displayAnalyticsScreen() {
    currentOLEDState = SCREEN_ANALYTICS;
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 0);
    display.println(getOLEDString("ANALYTICS"));
    display.drawFastHLine(0, 9, SCREEN_WIDTH, SH110X_WHITE);

    display.setCursor(0, 12);
    display.print(getOLEDString("TOTAL_UPLOADS") + " " + String(totalUploads));
    display.setCursor(0, 20);
    display.print(getOLEDString("TOTAL_DOWNLOADS") + " " + String(totalDownloads));

    // Get SD card usage
    uint64_t totalBytes = SD.totalBytes();
    uint64_t usedBytes = SD.usedBytes();
    if (totalBytes > 0) {
        display.setCursor(0, 28);
        display.print(getOLEDString("STORAGE_USED") + " " + formatBytes(usedBytes));
        display.setCursor(0, 36);
        display.print(getOLEDString("STORAGE_FREE") + " " + formatBytes(totalBytes - usedBytes));
    } else {
        display.setCursor(0, 28);
        display.println("SD Not Ready");
    }

    // Find Most Downloaded File (top 1)
    String mostDownloadedFile = "N/A";
    unsigned int maxDownloads = 0;
    for (auto const& [filename, count] : fileDownloadCounts) {
        if (count > maxDownloads) {
            maxDownloads = count;
            mostDownloadedFile = filename;
        }
    }
    display.setCursor(0, 44);
    display.print(getOLEDString("MOST_DOWNLOADED") + " ");
    if (mostDownloadedFile.length() > 10) { // Truncate if too long
        display.print(mostDownloadedFile.substring(0, 7) + "...");
    } else {
        display.print(mostDownloadedFile);
    }
    display.print(" (");
    display.print(maxDownloads);
    display.print(")");

    display.display();
}

// New: Display Active Uploads (Multi-Device)
void displayActiveUploadsScreen() {
    currentOLEDState = SCREEN_ACTIVE_UPLOADS;
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 0);
    display.println(getOLEDString("ACTIVE_UPLOADS"));
    display.drawFastHLine(0, 9, SCREEN_WIDTH, SH110X_WHITE);

    if (activeUploads.empty()) {
        display.setCursor(0, 20);
        display.println("None active.");
    } else {
        int maxVisibleUploads = 2; // Display up to 2 uploads at a time
        for (int i = 0; i < std::min((int)activeUploads.size(), maxVisibleUploads); ++i) {
            UploadInfo info = activeUploads[i];
            display.setCursor(0, 12 + (i * 20)); // Each upload takes 2 lines
            display.print(info.clientIP.substring(info.clientIP.lastIndexOf('.') + 1) + ": "); // Show last octet of IP
            if (info.filename.length() > 10) {
                display.print(info.filename.substring(0, 7) + "...");
            } else {
                display.print(info.filename);
            }
            display.print(" (");
            display.print(info.percentage);
            display.print("%)");
        }
    }
    display.display();
}

// New: Display Current User Info (for temporary accounts)
void displayUserInfoScreen() {
    currentOLEDState = SCREEN_USER_INFO;
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 0);
    display.println(getOLEDString("USER_MGMT"));
    display.drawFastHLine(0, 9, SCREEN_WIDTH, SH110X_WHITE);

    display.setCursor(0, 12);
    display.print(getOLEDString("LOGGED_IN_AS") + " ");
    display.println(currentLoggedInUser.isEmpty() ? "Guest" : currentLoggedInUser);

    display.setCursor(0, 20);
    display.print(getOLEDString("UPLOAD_RIGHTS") + " ");
    display.println(currentLoggedInUserUploadRights ? getOLEDString("ENABLED") : getOLEDString("DISABLED"));

    display.setCursor(0, 35);
    display.print(getOLEDString("BROADCAST_MODE") + ": ");
    if (broadcastFilePath.isEmpty()) {
        display.println("Disabled");
    } else {
        String displayBcastFile = broadcastFilePath;
        int lastSlash = displayBcastFile.lastIndexOf('/');
        if (lastSlash != -1) {
            displayBcastFile = displayBcastFile.substring(lastSlash + 1);
        }
        if (displayBcastFile.length() > 12) {
            display.print(getOLEDString("BROADCASTING") + " " + displayBcastFile.substring(0, 9) + "...");
        } else {
            display.print(getOLEDString("BROADCASTING") + " " + displayBcastFile);
        }
    }
    display.display();
}


// New: Common character selection logic for user input (username/password)
char getCharFromDigit(int digitValue) {
    if (digitValue >= 0 && digitValue <= 9) return '0' + digitValue;
    if (digitValue >= 10 && digitValue < 36) return 'A' + (digitValue - 10);
    if (digitValue >= 36 && digitValue < 62) return 'a' + (digitValue - 36);
    return '_'; // Example special character for 62
}
// New: Display for creating/logging in user accounts
void displayUserAccountInputScreen(bool isCreate, bool isUsernamePhase) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 0);
    display.println(isCreate ? getOLEDString("CREATE_USER") : getOLEDString("LOGIN_USER"));
    display.drawFastHLine(0, 9, SCREEN_WIDTH, SH110X_WHITE);

    display.setCursor(0, 12);
    display.print(isUsernamePhase ? getOLEDString("ENTER_USERNAME") : getOLEDString("ENTER_PASSWORD"));
    display.println(" "); // Space for input

    display.setCursor(0, 20);
    String currentInput = isUsernamePhase ? tempUsernameInput : tempPasswordInput;
    if (isUsernamePhase) {
        display.print(currentInput);
    } else {
        for (int i = 0; i < currentInput.length(); ++i) display.print("*");
    }

    // Display the current selected character for the input field
    // Only display if we are still building the string (userCharIndex is within bounds)
    // and if we are not in the 'upload rights confirmation' phase of creation
    if (userCharIndex < 16) { // Max length for username/password
        display.print(getCharFromDigit(currentDigitValue));
    }

    // For create user, show upload rights option *after* password
    if (isCreate && !isUsernamePhase && (currentOLEDState == SCREEN_USER_INPUT_CREATE && userCharIndex >= 16)) {
        display.setCursor(0, 40);
        display.print(getOLEDString("UPLOAD_RIGHTS") + ": ");
        display.print(tempUserHasUploadRights ? getOLEDString("ENABLED") : getOLEDString("DISABLED"));
    }
    display.display();
}

// =======================================================================
//    BUTTON HANDLING
// =======================================================================

// Master function to handle all OLED-related button interactions based on current state
void handleOLEDButtons() {
    static unsigned long lastButtonStateChange = 0; // For debounce
    static int lastBtnUpState = HIGH;
    static int lastBtnDownState = HIGH;
    static int lastBtnSelectState = HIGH;

    int currentBtnUpState = digitalRead(BTN_UP_PIN);
    int currentBtnDownState = digitalRead(BTN_DOWN_PIN);
    int currentBtnSelectState = digitalRead(BTN_SELECT_PIN);

    unsigned long currentTime = millis();

    // Check for button press (LOW indicates pressed with INPUT_PULLUP)
    bool btnUpPressed = (currentBtnUpState == LOW && lastBtnUpState == HIGH);
    bool btnDownPressed = (currentBtnDownState == LOW && lastBtnDownState == HIGH);
    bool btnSelectPressed = (currentBtnSelectState == LOW && lastBtnSelectState == HIGH);

    // Apply debounce
    if ((btnUpPressed || btnDownPressed || btnSelectPressed) && (currentTime - lastButtonPressTime < BUTTON_DEBOUNCE_DELAY)) {
        // Ignore rapid presses
        lastBtnUpState = currentBtnUpState;
        lastBtnDownState = currentBtnDownState;
        lastBtnSelectState = currentBtnSelectState;
        return;
    }

    // Update last button press time if any button was just pressed
    if (btnUpPressed || btnDownPressed || btnSelectPressed) {
        lastButtonPressTime = currentTime;
    }

    // Handle button logic based on current OLED state
    switch (currentOLEDState) {
        case SCREEN_AUTH_PIN:
            handlePINEntry();
            break;
        case SCREEN_FILE_TREE:
            handleFileTreeNavigation();
            break;
        case SCREEN_CONFIRM_DELETE: // Fall through for any confirmation
        case SCREEN_CONFIRM_FORMAT:
            handleConfirmationButtons();
            break;
        case SCREEN_FILE_PREVIEW:
        case SCREEN_QR_CODE: // New: Any button to exit QR code
            if (btnSelectPressed || btnUpPressed || btnDownPressed) { // Any button to exit preview/QR
                populateOLEDFileList(currentPath); // Go back to file tree
                displayFileTree();
            }
            break;
        case SCREEN_IDLE: // From idle, select button can go to file tree
            if (btnSelectPressed) {
                populateOLEDFileList(currentPath);
                displayFileTree();
            } else if (btnUpPressed) { // Option to enter settings from idle
                oledSelectedIndex = 0; // Reset selection for settings menu
                displaySettingsMenu();
            } else if (btnDownPressed) { // Option to enter analytics from idle
                oledSelectedIndex = 0; // Reset selection for analytics menu
                displayAnalyticsScreen();
            }
            break;
        case SCREEN_SETTINGS_MENU:
            handleSettingsNavigation();
            break;
        case SCREEN_THEME_SELECT:
            handleThemeSelection();
            break;
        case SCREEN_LANGUAGE_SELECT:
            handleLanguageSelection();
            break;
        case SCREEN_ANALYTICS: // New: Analytics screen, any button to exit
        case SCREEN_ACTIVE_UPLOADS: // New: Active uploads screen, any button to exit
        case SCREEN_USER_INFO: // New: User info screen, any button to exit
            if (btnSelectPressed || btnUpPressed || btnDownPressed) { // Any button to exit these screens
                displayIdleScreen(); // Return to idle
            }
            break;
        case SCREEN_USER_INPUT_CREATE:
            handleUserCreatePrompt();
            break;
        case SCREEN_USER_INPUT_LOGIN:
            handleUserLoginPrompt();
            break;
        case SCREEN_SLEEP_ANIMATION: // If in sleep mode, any button should wake it up
            if (btnUpPressed || btnDownPressed || btnSelectPressed) {
                wakeUpFromSleep();
            }
            break;
        default:
            // Do nothing or return to idle if in an unexpected state
            break;
    }

    // Store current button states for next debounce cycle
    lastBtnUpState = currentBtnUpState;
    lastBtnDownState = currentBtnDownState;
    lastBtnSelectState = currentBtnSelectState;
}

void handlePINEntry() {
    if (digitalRead(BTN_UP_PIN) == LOW && millis() - lastButtonPressTime >= BUTTON_DEBOUNCE_DELAY) {
        lastButtonPressTime = millis();
        currentDigitValue = (currentDigitValue + 1) % 10;
        displayPINEntryScreen();
    }
    if (digitalRead(BTN_DOWN_PIN) == LOW && millis() - lastButtonPressTime >= BUTTON_DEBOUNCE_DELAY) {
        lastButtonPressTime = millis();
        currentDigitValue = (currentDigitValue - 1 + 10) % 10; // Ensure positive modulo
        displayPINEntryScreen();
    }
    if (digitalRead(BTN_SELECT_PIN) == LOW && millis() - lastButtonPressTime >= BUTTON_DEBOUNCE_DELAY) {
        lastButtonPressTime = millis();
        enteredPIN += String(currentDigitValue);
        pinDigitIndex++;
        currentDigitValue = 0; // Reset for next digit

        if (pinDigitIndex < strlen(DEFAULT_PIN)) {
            displayPINEntryScreen();
        } else {
            // PIN entry complete, check it
            if (enteredPIN == DEFAULT_PIN) {
                isServerLocked = false;
                Serial.println("Server Unlocked via OLED!");
                displayOperation(getOLEDString("SERVER_UNLOCKED"));
                delay(1000);
                lastActivityTime = millis(); // Reset activity timer on unlock

                // Transition based on client connection after unlock
                if (WiFi.softAPgetStationNum() > 0) {
                    populateOLEDFileList(currentPath); // Go to file tree
                    displayFileTree();
                } else {
                    displayIdleScreen(); // Go to idle screen
                }
            } else {
                displayOperation(getOLEDString("INCORRECT_PIN"));
                delay(1500);
                enteredPIN = ""; // Reset for re-entry
                pinDigitIndex = 0;
                currentDigitValue = 0;
                displayPINEntryScreen();
            }
        }
    }
}

void handleFileTreeNavigation() {
    // New: Handle context menu for file tree items
    static bool inContextMenu = false;
    static int contextMenuSelectedIndex = 0;
    std::vector<String> contextMenuItems;

    if (oledFileList.size() > 0) {
        String selectedItemPath = currentPath;
        if (oledSelectedIndex < oledFileList.size()) {
            selectedItemPath += oledFileList[oledSelectedIndex];
        } else {
            selectedItemPath = ""; // Invalid selection
        }

        if (inContextMenu) {
            // Context menu items for files (simplified: QR, Preview, Delete, Broadcast)
            if (!selectedItemPath.endsWith("/")) { // It's a file
                 contextMenuItems.push_back(getOLEDString("PREVIEW"));
                 contextMenuItems.push_back(getOLEDString("QR_SCAN_DL"));
                 contextMenuItems.push_back(getOLEDString("BROADCAST_MODE")); // Broadcast option
                 contextMenuItems.push_back(getOLEDString("CONFIRM_DELETE")); // Direct delete via OLED
            } else if (selectedItemPath.endsWith("/")) { // It's a directory
                 contextMenuItems.push_back(getOLEDString("CONFIRM_DELETE")); // Only delete for directories
            }


            // Handle context menu button presses
            if (digitalRead(BTN_UP_PIN) == LOW && millis() - lastButtonPressTime >= BUTTON_DEBOUNCE_DELAY) {
                lastButtonPressTime = millis();
                contextMenuSelectedIndex--;
                if (contextMenuSelectedIndex < 0) contextMenuSelectedIndex = contextMenuItems.size() - 1;
                // Re-display context menu
                display.clearDisplay();
                display.setCursor(0,0);
                display.print(getOLEDString("FILE") + " ");
                display.println(oledFileList[oledSelectedIndex]);
                display.drawFastHLine(0, 9, SCREEN_WIDTH, SH110X_WHITE);
                for (int i = 0; i < contextMenuItems.size(); ++i) {
                    if (i == contextMenuSelectedIndex) {
                        display.setTextColor(SH110X_BLACK, SH110X_WHITE);
                    } else {
                        display.setTextColor(SH110X_WHITE);
                    }
                    display.setCursor(0, 12 + (i * 8));
                    display.println(contextMenuItems[i]);
                }
                display.display();

            } else if (digitalRead(BTN_DOWN_PIN) == LOW && millis() - lastButtonPressTime >= BUTTON_DEBOUNCE_DELAY) {
                lastButtonPressTime = millis();
                contextMenuSelectedIndex++;
                if (contextMenuSelectedIndex >= contextMenuItems.size()) contextMenuSelectedIndex = 0;
                // Re-display context menu
                display.clearDisplay();
                display.setCursor(0,0);
                display.print(getOLEDString("FILE") + " ");
                display.println(oledFileList[oledSelectedIndex]);
                display.drawFastHLine(0, 9, SCREEN_WIDTH, SH110X_WHITE);
                for (int i = 0; i < contextMenuItems.size(); ++i) {
                    if (i == contextMenuSelectedIndex) {
                        display.setTextColor(SH110X_BLACK, SH110X_WHITE);
                    } else {
                        display.setTextColor(SH110X_WHITE);
                    }
                    display.setCursor(0, 12 + (i * 8));
                    display.println(contextMenuItems[i]);
                }
                display.display();

            } else if (digitalRead(BTN_SELECT_PIN) == LOW && millis() - lastButtonPressTime >= BUTTON_DEBOUNCE_DELAY) {
                lastButtonPressTime = millis();
                inContextMenu = false; // Exit context menu
                String selectedOption = contextMenuItems[contextMenuSelectedIndex];

                if (selectedOption == getOLEDString("PREVIEW")) {
                     displayFilePreview(selectedItemPath);
                } else if (selectedOption == getOLEDString("QR_SCAN_DL")) {
                    IPAddress ip = WiFi.softAPIP();
                    String url = "http://" + ip.toString() + selectedItemPath;
                    displayQRCode(url);
                } else if (selectedOption == getOLEDString("BROADCAST_MODE")) {
                    broadcastFilePath = selectedItemPath;
                    displayOperation(getOLEDString("BROADCASTING") + " " + broadcastFilePath.substring(broadcastFilePath.lastIndexOf('/') + 1));
                    Serial.println("File set for broadcast: " + broadcastFilePath);
                    delay(1500);
                    populateOLEDFileList(currentPath);
                    displayFileTree();
                }
                else if (selectedOption == getOLEDString("CONFIRM_DELETE")) {
                    pendingDeleteItem = selectedItemPath;
                    displayConfirmation(getOLEDString("CONFIRM_DELETE"), handleDeleteConfirmed, [](){
                        populateOLEDFileList(currentPath);
                        displayFileTree();
                    });
                }
            }
            return; // Don't process file tree navigation if in context menu
        }
    }


    if (digitalRead(BTN_UP_PIN) == LOW && millis() - lastButtonPressTime >= BUTTON_DEBOUNCE_DELAY) {
        lastButtonPressTime = millis();
        oledSelectedIndex--;
        if (oledSelectedIndex < 0) oledSelectedIndex = oledFileList.size() - 1; // Wrap around
        int maxVisibleItems = (SCREEN_HEIGHT - 12) / 8;
        if (oledSelectedIndex < oledScrollIndex) { // If selected item scrolls off top
            oledScrollIndex = oledSelectedIndex;
        } else if (oledSelectedIndex >= oledScrollIndex + maxVisibleItems) { // If selected item scrolls off bottom
            oledScrollIndex = oledSelectedIndex - maxVisibleItems + 1;
        }
        displayFileTree();
    }
    if (digitalRead(BTN_DOWN_PIN) == LOW && millis() - lastButtonPressTime >= BUTTON_DEBOUNCE_DELAY) {
        lastButtonPressTime = millis();
        oledSelectedIndex++;
        if (oledSelectedIndex >= oledFileList.size()) oledSelectedIndex = 0; // Wrap around
        int maxVisibleItems = (SCREEN_HEIGHT - 12) / 8;
        if (oledSelectedIndex >= oledScrollIndex + maxVisibleItems) { // If selected item scrolls off bottom
            oledScrollIndex = oledSelectedIndex - maxVisibleItems + 1;
        } else if (oledSelectedIndex < oledScrollIndex) { // If selected item scrolls off top
             oledScrollIndex = oledSelectedIndex;
        }
        displayFileTree();
    }
    if (digitalRead(BTN_SELECT_PIN) == LOW && millis() - lastButtonPressTime >= BUTTON_DEBOUNCE_DELAY) {
        lastButtonPressTime = millis();
        if (oledFileList.size() > 0) {
            String selectedItem = oledFileList[oledSelectedIndex];
            if (selectedItem.endsWith("/")) { // It's a directory
                String newPath = currentPath;
                if (selectedItem.equals(getOLEDString("PARENT_DIR") + "/")) { // ".." entry
                    if (newPath != "/") {
                         int lastSlash = newPath.lastIndexOf('/', newPath.length() - 2); // Find second to last slash
                         if (lastSlash == -1) newPath = "/";
                         else newPath = newPath.substring(0, lastSlash + 1);
                    }
                } else {
                    newPath += selectedItem;
                }
                currentPath = newPath; // Update global path
                populateOLEDFileList(currentPath); // Re-populate for new directory
                displayFileTree();
            } else { // It's a file - show context menu
                inContextMenu = true;
                contextMenuSelectedIndex = 0;
                // Re-display context menu (same logic as above)
                display.clearDisplay();
                display.setCursor(0,0);
                display.print(getOLEDString("FILE") + " ");
                display.println(oledFileList[oledSelectedIndex]);
                display.drawFastHLine(0, 9, SCREEN_WIDTH, SH110X_WHITE);
                // Context menu items for files (simplified: QR, Preview, Delete, Broadcast)
                contextMenuItems.clear();
                contextMenuItems.push_back(getOLEDString("PREVIEW"));
                contextMenuItems.push_back(getOLEDString("QR_SCAN_DL"));
                contextMenuItems.push_back(getOLEDString("BROADCAST_MODE"));
                contextMenuItems.push_back(getOLEDString("CONFIRM_DELETE")); // Direct delete via OLED
                for (int i = 0; i < contextMenuItems.size(); ++i) {
                    if (i == contextMenuSelectedIndex) {
                        display.setTextColor(SH110X_BLACK, SH110X_WHITE);
                    } else {
                        display.setTextColor(SH110X_WHITE);
                    }
                    display.setCursor(0, 12 + (i * 8));
                    display.println(contextMenuItems[i]);
                }
                display.display();
            }
        }
    }
}

void handleConfirmationButtons() {
    if (digitalRead(BTN_UP_PIN) == LOW && millis() - lastButtonPressTime >= BUTTON_DEBOUNCE_DELAY) { // YES
        lastButtonPressTime = millis();
        if (confirmationYesCallback) {
            confirmationYesCallback();
        }
        confirmationYesCallback = nullptr; // Clear callback
        confirmationNoCallback = nullptr;
    }
    if (digitalRead(BTN_DOWN_PIN) == LOW && millis() - lastButtonPressTime >= BUTTON_DEBOUNCE_DELAY) { // NO
        lastButtonPressTime = millis();
        if (confirmationNoCallback) {
            confirmationNoCallback();
        }
        confirmationYesCallback = nullptr; // Clear callback
        confirmationNoCallback = nullptr;
    }
}

void handleSettingsNavigation() {
    if (digitalRead(BTN_UP_PIN) == LOW && millis() - lastButtonPressTime >= BUTTON_DEBOUNCE_DELAY) {
        lastButtonPressTime = millis();
        oledSelectedIndex--;
        if (oledSelectedIndex < 0) oledSelectedIndex = 3; // 4 items in settings menu (0-3)
        displaySettingsMenu();
    }
    if (digitalRead(BTN_DOWN_PIN) == LOW && millis() - lastButtonPressTime >= BUTTON_DEBOUNCE_DELAY) {
        lastButtonPressTime = millis();
        oledSelectedIndex++;
        if (oledSelectedIndex > 3) oledSelectedIndex = 0;
        displaySettingsMenu();
    }
    if (digitalRead(BTN_SELECT_PIN) == LOW && millis() - lastButtonPressTime >= BUTTON_DEBOUNCE_DELAY) {
        lastButtonPressTime = millis();
        // Go to selected submenu
        if (oledSelectedIndex == 0) { // Change Theme
            oledSelectedIndex = (currentTheme == "dark" ? 0 : 1); // Select current theme by default
            displayThemeSelectMenu();
        } else if (oledSelectedIndex == 1) { // Change Language
            if (currentLanguage == "en") oledSelectedIndex = 0;
            else if (currentLanguage == "hi") oledSelectedIndex = 1;
            else if (currentLanguage == "de") oledSelectedIndex = 2;
            displayLanguageSelectMenu();
        } else if (oledSelectedIndex == 2) { // Analytics
            oledSelectedIndex = 0; // Reset for analytics display
            displayAnalyticsScreen();
        } else if (oledSelectedIndex == 3) { // User Management
            oledSelectedIndex = 0; // Reset for user management menu
            handleUserAccountNavigation(); // Go to user account menu
        }
    }
    // Option to go back to idle from settings? (e.g., long press select or a dedicated button)
    // For now, after any setting change, it will return to file tree.
}

void handleThemeSelection() {
    if (digitalRead(BTN_UP_PIN) == LOW && millis() - lastButtonPressTime >= BUTTON_DEBOUNCE_DELAY) {
        lastButtonPressTime = millis();
        oledSelectedIndex = (oledSelectedIndex == 0) ? 1 : 0; // Toggle between 0 (Dark) and 1 (Light)
        displayThemeSelectMenu();
    }
    if (digitalRead(BTN_DOWN_PIN) == LOW && millis() - lastButtonPressTime >= BUTTON_DEBOUNCE_DELAY) {
        lastButtonPressTime = millis();
        oledSelectedIndex = (oledSelectedIndex == 0) ? 1 : 0; // Toggle
        displayThemeSelectMenu();
    }
    if (digitalRead(BTN_SELECT_PIN) == LOW && millis() - lastButtonPressTime >= BUTTON_DEBOUNCE_DELAY) {
        lastButtonPressTime = millis();
        currentTheme = (oledSelectedIndex == 0) ? "dark" : "light";
        savePreferences();
        displayOperation(getOLEDString("THEME") + " " + (currentTheme == "dark" ? getOLEDString("DARK") : getOLEDString("LIGHT")));
        delay(1000);
        populateOLEDFileList(currentPath); // Return to file tree
        displayFileTree();
    }
}

void handleLanguageSelection() {
    if (digitalRead(BTN_UP_PIN) == LOW && millis() - lastButtonPressTime >= BUTTON_DEBOUNCE_DELAY) {
        lastButtonPressTime = millis();
        oledSelectedIndex--;
        if (oledSelectedIndex < 0) oledSelectedIndex = 2; // 3 languages: 0,1,2
        displayLanguageSelectMenu();
    }
    if (digitalRead(BTN_DOWN_PIN) == LOW && millis() - lastButtonPressTime >= BUTTON_DEBOUNCE_DELAY) {
        lastButtonPressTime = millis();
        oledSelectedIndex++;
        if (oledSelectedIndex > 2) oledSelectedIndex = 0;
        displayLanguageSelectMenu();
    }
    if (digitalRead(BTN_SELECT_PIN) == LOW && millis() - lastButtonPressTime >= BUTTON_DEBOUNCE_DELAY) {
        lastButtonPressTime = millis();
        if (oledSelectedIndex == 0) {
            currentLanguage = "en";
            current_lang_map = &en_strings;
        } else if (oledSelectedIndex == 1) {
            currentLanguage = "hi";
            current_lang_map = &hi_strings;
        } else if (oledSelectedIndex == 2) {
            currentLanguage = "de";
            current_lang_map = &de_strings;
        }
        savePreferences();
        String tempLangName; // Temporary variable for the language name
        if (currentLanguage == "en") tempLangName = getOLEDString("ENGLISH");
        else if (currentLanguage == "hi") tempLangName = getOLEDString("HINDI");
        else if (currentLanguage == "de") tempLangName = getOLEDString("GERMAN");
        displayOperation(getOLEDString("LANGUAGE") + " " + tempLangName);
        delay(1000);
        populateOLEDFileList(currentPath); // Return to file tree
        displayFileTree();
    }
}

// New: Handle navigation within the User Account management menu
void handleUserAccountNavigation() {
    currentOLEDState = SCREEN_SETTINGS_MENU; // Reuse settings screen state for menu, or create new one
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 0);
    display.println(getOLEDString("USER_MGMT"));
    display.drawFastHLine(0, 9, SCREEN_WIDTH, SH110X_WHITE);

    String menuItems[] = {
        getOLEDString("CREATE_USER"),
        getOLEDString("LOGIN_USER"),
        getOLEDString("LOGGED_IN_AS") // Option to view current user info
    };
    int numItems = sizeof(menuItems) / sizeof(menuItems[0]);

    // Ensure oledSelectedIndex is within bounds for this menu
    if (oledSelectedIndex >= numItems) oledSelectedIndex = 0;
    if (oledSelectedIndex < 0) oledSelectedIndex = numItems - 1;


    for (int i = 0; i < numItems; ++i) {
        if (i == oledSelectedIndex) {
            display.setTextColor(SH110X_BLACK, SH110X_WHITE);
        } else {
            display.setTextColor(SH110X_WHITE);
        }
        display.setCursor(0, 12 + (i * 8));
        display.println(menuItems[i]);
    }
    display.display();

    if (digitalRead(BTN_UP_PIN) == LOW && millis() - lastButtonPressTime >= BUTTON_DEBOUNCE_DELAY) {
        lastButtonPressTime = millis();
        oledSelectedIndex--;
        if (oledSelectedIndex < 0) oledSelectedIndex = numItems - 1;
        handleUserAccountNavigation(); // Refresh display
    }
    if (digitalRead(BTN_DOWN_PIN) == LOW && millis() - lastButtonPressTime >= BUTTON_DEBOUNCE_DELAY) {
        lastButtonPressTime = millis();
        oledSelectedIndex++;
        if (oledSelectedIndex >= numItems) oledSelectedIndex = 0;
        handleUserAccountNavigation(); // Refresh display
    }
    if (digitalRead(BTN_SELECT_PIN) == LOW && millis() - lastButtonPressTime >= BUTTON_DEBOUNCE_DELAY) {
        lastButtonPressTime = millis();
        if (oledSelectedIndex == 0) { // Create User
            tempUsernameInput = "";
            tempPasswordInput = "";
            tempUserHasUploadRights = false;
            userCharIndex = 0;
            currentDigitValue = 0;
            currentOLEDState = SCREEN_USER_INPUT_CREATE; // Set specific state for input
            displayUserAccountInputScreen(true, true); // Display initial username input for create
        } else if (oledSelectedIndex == 1) { // Login User
            tempUsernameInput = "";
            tempPasswordInput = "";
            userCharIndex = 0;
            currentDigitValue = 0;
            currentOLEDState = SCREEN_USER_INPUT_LOGIN; // Set specific state for input
            displayUserAccountInputScreen(false, true); // Display initial username input for login
        } else if (oledSelectedIndex == 2) { // View User Info
            oledSelectedIndex = 0; // Reset for user info display
            displayUserInfoScreen();
        }
    }
}

// New: Handle user input for creating a user account on OLED
void handleUserCreatePrompt() {
    static int inputPhase = 0; // 0: username, 1: password, 2: upload rights confirm

    // Reset phase if entering from a different state (e.g., navigating back to this menu)
    if (currentOLEDState != SCREEN_USER_INPUT_CREATE) {
        inputPhase = 0;
        return;
    }

    // Display the correct input screen based on the current phase
    if (inputPhase == 0) { // Username input
        displayUserAccountInputScreen(true, true);
    } else if (inputPhase == 1) { // Password input
        displayUserAccountInputScreen(true, false);
    } else if (inputPhase == 2) { // Upload Rights confirmation
        // Only show upload rights option *after* password entry is complete
        displayUserAccountInputScreen(true, false); // Still in password phase for display, but show rights below
        display.setCursor(0, 40);
        display.print(getOLEDString("UPLOAD_RIGHTS") + ": ");
        display.print(tempUserHasUploadRights ? getOLEDString("ENABLED") : getOLEDString("DISABLED"));
        display.display(); // Update again to show rights
    }

    if (digitalRead(BTN_UP_PIN) == LOW && millis() - lastButtonPressTime >= BUTTON_DEBOUNCE_DELAY) {
        lastButtonPressTime = millis();
        if (inputPhase == 2) { // Toggle upload rights
            tempUserHasUploadRights = !tempUserHasUploadRights;
        } else { // Increment character value
            currentDigitValue = (currentDigitValue + 1) % 63; // 0-9, A-Z, a-z, _
        }
        // Re-display immediately
        displayUserAccountInputScreen(true, inputPhase == 0);
        if (inputPhase == 2) { // If in rights phase, redraw the rights line
            display.setCursor(0, 40);
            display.print(getOLEDString("UPLOAD_RIGHTS") + ": ");
            display.print(tempUserHasUploadRights ? getOLEDString("ENABLED") : getOLEDString("DISABLED"));
            display.display();
        }
    }
    if (digitalRead(BTN_DOWN_PIN) == LOW && millis() - lastButtonPressTime >= BUTTON_DEBOUNCE_DELAY) {
        lastButtonPressTime = millis();
        if (inputPhase == 2) { // Toggle upload rights
            tempUserHasUploadRights = !tempUserHasUploadRights;
        } else { // Decrement character value
            currentDigitValue = (currentDigitValue - 1 + 63) % 63; // Ensure positive modulo
        }
        // Re-display immediately
        displayUserAccountInputScreen(true, inputPhase == 0);
        if (inputPhase == 2) { // If in rights phase, redraw the rights line
            display.setCursor(0, 40);
            display.print(getOLEDString("UPLOAD_RIGHTS") + ": ");
            display.print(tempUserHasUploadRights ? getOLEDString("ENABLED") : getOLEDString("DISABLED"));
            display.display();
        }
    }
    if (digitalRead(BTN_SELECT_PIN) == LOW && millis() - lastButtonPressTime >= BUTTON_DEBOUNCE_DELAY) {
        lastButtonPressTime = millis();
        if (inputPhase == 0) { // Confirm username digit/char or complete username entry
            if (userCharIndex < 16) { // Max username length (arbitrary, can be adjusted)
                tempUsernameInput += getCharFromDigit(currentDigitValue);
                userCharIndex++;
            }
            // If user presses SELECT and string is not empty, or max length reached, move to next phase
            if (userCharIndex >= 16 || (tempUsernameInput.length() > 0 && digitalRead(BTN_SELECT_PIN) == LOW)) {
                inputPhase = 1; // Move to password phase
                userCharIndex = 0; // Reset for next input field
                currentDigitValue = 0; // Reset for next digit
                displayUserAccountInputScreen(true, false); // Display password input
            } else {
                 displayUserAccountInputScreen(true, true); // Keep displaying username input
            }
        } else if (inputPhase == 1) { // Confirm password digit/char or complete password entry
            if (userCharIndex < 16) { // Max password length
                tempPasswordInput += getCharFromDigit(currentDigitValue);
                userCharIndex++;
            }
            // If user presses SELECT and string is not empty, or max length reached, move to next phase
            if (userCharIndex >= 16 || (tempPasswordInput.length() > 0 && digitalRead(BTN_SELECT_PIN) == LOW)) {
                inputPhase = 2; // Move to upload rights confirmation
                currentDigitValue = 0; // Reset for toggling rights
                displayUserAccountInputScreen(true, false); // Keep password display, but ready for rights
            } else {
                displayUserAccountInputScreen(true, false); // Keep displaying password input
            }
        } else if (inputPhase == 2) { // Final confirmation for upload rights, finish creation
            if (tempUsernameInput.isEmpty() || tempPasswordInput.isEmpty()) {
                displayOperation("Name/Pass empty!"); // Error if fields are empty
                delay(1500);
                inputPhase = 0; // Restart
                tempUsernameInput = "";
                tempPasswordInput = "";
                userCharIndex = 0;
                currentDigitValue = 0;
                displayUserAccountInputScreen(true, true);
                return;
            }

            // Check if username already exists on OLED side
            for (const auto& user : tempUserAccounts) {
                if (user.username == tempUsernameInput) {
                    displayOperation("User exists!");
                    delay(1500);
                    inputPhase = 0; // Restart
                    tempUsernameInput = "";
                    tempPasswordInput = "";
                    userCharIndex = 0;
                    currentDigitValue = 0;
                    displayUserAccountInputScreen(true, true);
                    return;
                }
            }


            TempUserAccount newUser;
            newUser.username = tempUsernameInput;
            newUser.pinHash = tempPasswordInput; // Storing as plain text for simplicity
            newUser.uploadRights = tempUserHasUploadRights;
            tempUserAccounts.push_back(newUser);

            displayOperation(getOLEDString("USER_CREATED"));
            delay(1500);
            inputPhase = 0; // Reset for next time
            currentOLEDState = SCREEN_SETTINGS_MENU; // Exit submenu
            oledSelectedIndex = 0; // Reset selected index for settings menu
            displaySettingsMenu(); // Return to settings menu
        }
    }
}

// New: Handle user input for logging in user account on OLED
void handleUserLoginPrompt() {
    static int inputPhase = 0; // 0: username, 1: password

    // Reset phase if entering from a different state (e.g., navigating back to this menu)
    if (currentOLEDState != SCREEN_USER_INPUT_LOGIN) {
        inputPhase = 0;
        return;
    }

    // Display the correct input screen based on the current phase
    if (inputPhase == 0) { // Username input
        displayUserAccountInputScreen(false, true);
    } else if (inputPhase == 1) { // Password input
        displayUserAccountInputScreen(false, false);
    }

    if (digitalRead(BTN_UP_PIN) == LOW && millis() - lastButtonPressTime >= BUTTON_DEBOUNCE_DELAY) {
        lastButtonPressTime = millis();
        currentDigitValue = (currentDigitValue + 1) % 63;
        displayUserAccountInputScreen(false, inputPhase == 0);
    }
    if (digitalRead(BTN_DOWN_PIN) == LOW && millis() - lastButtonPressTime >= BUTTON_DEBOUNCE_DELAY) {
        lastButtonPressTime = millis();
        currentDigitValue = (currentDigitValue - 1 + 63) % 63;
        displayUserAccountInputScreen(false, inputPhase == 0);
    }
    if (digitalRead(BTN_SELECT_PIN) == LOW && millis() - lastButtonPressTime >= BUTTON_DEBOUNCE_DELAY) {
        lastButtonPressTime = millis();
        if (inputPhase == 0) { // Confirm username digit/char or complete username entry
            if (userCharIndex < 16) { // Max username length
                tempUsernameInput += getCharFromDigit(currentDigitValue);
                userCharIndex++;
            }
            // If user presses SELECT and string is not empty, or max length reached, move to next phase
            if (userCharIndex >= 16 || (tempUsernameInput.length() > 0 && digitalRead(BTN_SELECT_PIN) == LOW)) {
                inputPhase = 1; // Move to password phase
                userCharIndex = 0; // Reset for next input field
                currentDigitValue = 0; // Reset for next digit
                displayUserAccountInputScreen(false, false); // Display password input
            } else {
                 displayUserAccountInputScreen(false, true); // Keep displaying username input
            }
        } else if (inputPhase == 1) { // Confirm password digit/char, attempt login
            if (userCharIndex < 16) { // Max password length
                tempPasswordInput += getCharFromDigit(currentDigitValue);
                userCharIndex++;
            }

            // If user presses SELECT and string is not empty, or max length reached, attempt login
            if (userCharIndex >= 16 || (tempPasswordInput.length() > 0 && digitalRead(BTN_SELECT_PIN) == LOW)) {
                // Check credentials
                bool loginSuccess = false;
                for (const auto& user : tempUserAccounts) {
                    if (user.username == tempUsernameInput && user.pinHash == tempPasswordInput) {
                        currentLoggedInUser = user.username;
                        currentLoggedInUserUploadRights = user.uploadRights;
                        loginSuccess = true;
                        break;
                    }
                }

                if (loginSuccess) {
                    displayOperation(getOLEDString("LOGIN_SUCCESS"));
                    Serial.println("User " + currentLoggedInUser + " logged in via OLED.");
                } else {
                    displayOperation(getOLEDString("LOGIN_FAIL"));
                    Serial.println("OLED Login failed for " + tempUsernameInput);
                }
                delay(1500);
                inputPhase = 0; // Reset for next time
                currentOLEDState = SCREEN_SETTINGS_MENU; // Exit submenu
                oledSelectedIndex = 0; // Reset selected index for settings menu
                displaySettingsMenu(); // Return to settings menu
            } else {
                displayUserAccountInputScreen(false, false); // Keep displaying password input
            }
        }
    }
}


// =======================================================================
//    WEB SERVER UI (HTML & CSS)
// =======================================================================

// Dynamic HTML content with placeholders for theme and file list
const char* HTML_TEMPLATE = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>JAIDEEP'S VFT SERVER</title>
    <style id="dynamic-theme-css">
        /* THEME_CSS_PLACEHOLDER */

        /* For consistent spacing around inputs */
        .form-group {
            margin-bottom: 15px; /* Adjust spacing as needed */
        }

        /* Ensure inputs inside form-group get the styling */
        .form-group input[type="text"],
        .form-group input[type="password"] {
            margin-bottom: 0; /* Important: reset this for inputs inside form-group */
            width: 100%; /* Ensure they take full width of their container */
            box-sizing: border-box; /* Include padding and border in the element's total width */
        }

        /* Delete button specific styling (for circular) */
        .button.delete-btn {
            background-color: var(--delete-color);
            padding: 8px; /* Smaller padding to make it compact for circular shape */
            border-radius: 50%; /* Make it perfectly circular */
            width: 36px; /* Fixed width */
            height: 36px; /* Fixed height */
            display: flex; /* Use flexbox to center content */
            align-items: center; /* Center vertically */
            justify-content: center; /* Center horizontally */
            font-size: 1.2em; /* Make the '❌' icon larger */
            box-shadow: 0 2px 5px rgba(0,0,0,0.2); /* Keep consistent shadow */
            flex-shrink: 0; /* Prevent shrinking when space is limited */
        }
        .button.delete-btn:hover {
            background-color: #b32a3b;
            transform: translateY(-1px);
            box-shadow: 0 4px 8px rgba(0,0,0,0.3);
        }

        /* New styling for format SD card button (rectangular, still red) */
        .button.delete-action-btn {
            background-color: var(--delete-color);
            border-radius: 6px; /* Standard rounded corners, not fully circular */
            padding: 10px 15px; /* Standard button padding */
            font-weight: bold;
            box-shadow: 0 2px 5px rgba(0,0,0,0.2);
            transition: background-color 0.2s ease, transform 0.1s ease, box-shadow 0.2s ease;
            display: inline-flex;
            align-items: center;
            justify-content: center;
            gap: 5px;
            text-transform: uppercase;
            font-size: 0.9em;
            cursor: pointer;
            border: none;
        }
        .button.delete-action-btn:hover {
            background-color: #b32a3b;
            transform: translateY(-1px);
            box-shadow: 0 4px 8px rgba(0,0,0,0.3);
        }


        /* Responsive adjustments */
        @media (max-width: 600px) {
            body {
                padding: 10px;
            }
            .container {
                border-radius: 0;
                padding: 15px;
            }
            h2 {
                font-size: 1.5em;
            }
            li {
                flex-direction: column;
                align-items: flex-start;
                padding: 10px;
            }
            .file-info {
                width: 100%;
                margin-bottom: 10px;
            }
            .actions {
                width: 100%;
                display: flex;
                flex-wrap: wrap;
                gap: 5px;
            }
            .actions form, .actions a {
                flex: 1 1 auto; /* Allow items to grow/shrink */
                width: auto; /* Override inline-block width */
                margin-left: 0;
            }
            /* Specific adjustment for delete button on mobile to maintain circle */
            .actions .button.delete-btn {
                width: 40px;
                height: 40px;
                font-size: 1.4em;
                flex: none; /* Prevent flex from distorting it */
            }

            input[type="text"], input[type="file"], input[type="password"], input[type="submit"], button {
                width: 100%;
                box-sizing: border-box; /* Include padding and border in the element's total width */
            }
            .upload-section form {
                display: flex;
                flex-direction: column;
                gap: 10px; /* Use gap for spacing between form elements in column layout */
            }
            .upload-section label {
                margin-top: 5px;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <h2>📁 JAIDEEP'S VFT SERVER</h2>
        <div class="current-path" id="currentPath"></div>
        <div class="user-status" id="userStatus"></div> <!-- New: User Status -->

        <section class="file-browser-section">
            <h3>📂 Files & Folders</h3>
            <ul id="file-list">
                <!-- FILE_LIST_PLACEHOLDER -->
            </ul>
        </section>

        <section class="upload-section" id="uploadSection">
            <h3>⬆️ Upload File</h3>
            <form id="uploadForm" method="POST" enctype="multipart/form-data" action="/upload">
                <label for="file-input" class="file-input-label">Choose File</label>
                <input type="file" name="upload" id="file-input" onchange="uploadFile()">
                <progress id="progress" max="100" value="0"></progress>
                <input type="hidden" name="dir" id="uploadDir">
            </form>
        </section>

        <section class="upload-section" id="mkdirSection">
            <h3>➕ Create Folder</h3>
            <form method="POST" action="/mkdir">
                <div class="form-group">
                    <input name="folder" placeholder="Enter New Folder Name" required aria-label="New Folder Name">
                </div>
                <input type="hidden" name="dir" id="mkdirDir">
                <button type="submit" class="button">Create Folder</button>
            </form>
        </section>

        <section class="upload-section">
            <h3>⚙️ SD Card Actions</h3>
            <button onclick="confirmFormat()" class="button delete-action-btn">Format SD Card</button>
        </section>

        <section class="upload-section user-accounts-section">
            <h3>👥 User Accounts</h3>
            <div class="account-forms">
                <form method="POST" action="/create_user_web">
                    <h4>Register New User</h4>
                    <div class="form-group">
                        <input type="text" name="username" placeholder="Username" required aria-label="New Username">
                    </div>
                    <div class="form-group">
                        <input type="password" name="password" placeholder="Password" required aria-label="New Password">
                    </div>
                    <label class="checkbox-label">
                        <input type="checkbox" name="upload_rights" value="true"> Allow Uploads
                    </label>
                    <button type="submit" class="button">Create Account</button>
                </form>
                <form method="POST" action="/login_web">
                    <h4>Login</h4>
                    <div class="form-group">
                        <input type="text" name="username" placeholder="Username" required aria-label="Username">
                    </div>
                    <div class="form-group">
                        <input type="password" name="password" placeholder="Password" required aria-label="Password">
                    </div>
                    <button type="submit" class="button">Login</button>
                </form>
            </div>
            <button onclick="logout()" class="button logout-btn">Logout</button>
        </section>
    </div>
    <script>
        document.addEventListener('DOMContentLoaded', (event) => {
            const currentPathElement = document.getElementById('currentPath');
            const pathFromUrl = new URLSearchParams(window.location.search).get('path');
            const displayPath = pathFromUrl ? decodeURIComponent(pathFromUrl) : '/';
            currentPathElement.textContent = 'Current Directory: ' + displayPath;

            document.getElementById('uploadDir').value = displayPath;
            document.getElementById('mkdirDir').value = displayPath;

            // Update user status based on server response
            const userStatusElement = document.getElementById('userStatus');
            const loggedInUser = "<!-- LOGGED_IN_USER_PLACEHOLDER -->";
            const uploadRights = "<!-- UPLOAD_RIGHTS_PLACEHOLDER -->" === "true";
            userStatusElement.textContent = 'Logged in as: ' + (loggedInUser || 'Guest') +
                                            ' | Upload Rights: ' + (uploadRights ? 'Enabled' : 'Disabled');

            // Conditionally display upload/mkdir sections based on upload rights
            const uploadSection = document.getElementById('uploadSection');
            const mkdirSection = document.getElementById('mkdirSection');
            if (uploadRights) {
                uploadSection.style.display = 'block';
                mkdirSection.style.display = 'block';
            } else {
                uploadSection.style.display = 'none';
                mkdirSection.style.display = 'none';
            }

            // Custom file input label to make it look like a button
            const fileInput = document.getElementById('file-input');
            const fileInputLabel = document.querySelector('.file-input-label');
            if (fileInput && fileInputLabel) {
                fileInputLabel.addEventListener('click', () => {
                    fileInput.click();
                });
                fileInput.addEventListener('change', () => {
                    if (fileInput.files.length > 0) {
                        fileInputLabel.textContent = fileInput.files[0].name;
                    } else {
                        fileInputLabel.textContent = 'Choose File';
                    }
                });
            }
        });

        function uploadFile() {
            var form = document.getElementById('uploadForm');
            var formData = new FormData(form);
            var xhr = new XMLHttpRequest();
            xhr.open('POST', '/upload', true);
            xhr.upload.onprogress = function(e) {
                if (e.lengthComputable) {
                    document.getElementById('progress').value = (e.loaded / e.total) * 100;
                }
            };
            xhr.onload = function() {
                if (xhr.status == 200) {
                    location.reload();
                } else {
                    // Custom message box for error instead of alert
                    displayMessageBox('Upload failed: ' + xhr.responseText, 'error');
                }
            };
            xhr.onerror = function() {
                // Custom message box for error instead of alert
                displayMessageBox('Upload failed: Network error.', 'error');
            };
            xhr.send(formData);
        }

        // Custom message box function for web UI
        function displayMessageBox(message, type) {
            let messageBox = document.getElementById('messageBox');
            if (!messageBox) {
                messageBox = document.createElement('div');
                messageBox.id = 'messageBox';
                messageBox.style.cssText = `
                    position: fixed;
                    top: 20px;
                    left: 50%;
                    transform: translateX(-50%);
                    padding: 15px 25px;
                    border-radius: 8px;
                    font-weight: bold;
                    color: white;
                    z-index: 1000;
                    box-shadow: 0 4px 10px rgba(0,0,0,0.2);
                    opacity: 0;
                    transition: opacity 0.3s ease-in-out;
                    max-width: 80%;
                    text-align: center;
                `;
                document.body.appendChild(messageBox);
            }

            messageBox.textContent = message;
            if (type === 'error') {
                messageBox.style.backgroundColor = '#dc3545';
            } else if (type === 'success') {
                messageBox.style.backgroundColor = '#28a745';
            } else {
                messageBox.style.backgroundColor = '#007bff';
            }

            messageBox.style.opacity = 1;
            setTimeout(() => {
                messageBox.style.opacity = 0;
                // messageBox.remove(); // Remove after fade out
            }, 3000);
        }


        function confirmDelete(link) {
            // Use browser confirm for web UI, OLED confirm is for physical buttons
            if (confirm('Are you sure you want to delete this file/folder?')) {
                window.location.href = link;
            }
        }
        function confirmFormat() {
            if (confirm('WARNING: Formatting SD card will erase ALL DATA. Are you sure?')) {
                if (confirm('Are you absolutely sure? This cannot be undone.')) {
                    window.location.href = '/format';
                }
            }
        }
        function logout() {
            window.location.href = '/logout';
        }
    </script>
</body>
</html>
)rawliteral";

// Define CSS for dark and light themes
const char* DARK_THEME_CSS = R"rawliteral(
        :root {
            --bg-color: #1a1a1a;
            --primary-color: #2a2a2a;
            --secondary-color: #3a3a3a;
            --text-color: #e0e0e0;
            --accent-color: #007bff;
            --hover-color: #0056b3;
            --delete-color: #dc3545;
            --success-color: #28a745;
            --border-color: #444;
            --shadow-color: rgba(0,0,0,0.4);
        }
        body {
            font-family: 'Inter', -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif;
            background-color: var(--bg-color);
            color: var(--text-color);
            margin: 0;
            padding: 20px;
            line-height: 1.6;
        }
        .container {
            max-width: 800px;
            margin: 20px auto;
            background-color: var(--primary-color);
            padding: 30px;
            border-radius: 12px;
            box-shadow: 0 8px 20px var(--shadow-color);
            border: 1px solid var(--border-color);
        }
        h2, h3 {
            color: var(--accent-color);
            border-bottom: 2px solid var(--accent-color);
            padding-bottom: 10px;
            margin-top: 25px;
            margin-bottom: 20px;
            font-weight: 600;
        }
        h2 {
            font-size: 2em;
            text-align: center;
        }
        h3 {
            font-size: 1.4em;
        }
        ul {
            list-style: none;
            padding: 0;
        }
        li {
            background-color: var(--secondary-color);
            margin: 8px 0;
            padding: 12px 18px;
            border-radius: 8px;
            display: flex;
            align-items: center;
            justify-content: space-between;
            transition: background-color 0.2s ease, transform 0.1s ease;
            box-shadow: 0 2px 5px rgba(0,0,0,0.2);
            border: 1px solid var(--border-color);
        }
        li:hover {
            background-color: #4a4a4a;
            transform: translateY(-2px);
        }
        a {
            color: var(--accent-color);
            text-decoration: none;
            font-weight: 500;
            transition: color 0.2s ease;
        }
        a:hover {
            color: var(--hover-color);
        }
        .file-info {
            flex-grow: 1;
            overflow: hidden; /* Hide overflow */
            text-overflow: ellipsis; /* Add ellipsis */
            white-space: nowrap; /* Prevent wrapping */
            padding-right: 10px; /* Space before actions */
        }
        .actions {
            display: flex;
            gap: 8px; /* Space between buttons */
            flex-shrink: 0; /* Don't shrink actions */
        }
        .actions form, .actions a {
            display: inline-flex; /* Use flex for button alignment */
            align-items: center;
        }
        input[type="text"], input[type="password"] {
            background-color: var(--primary-color);
            color: var(--text-color);
            border: 1px solid var(--border-color);
            padding: 10px 12px;
            border-radius: 6px;
            width: calc(100% - 24px); /* Full width minus padding */
            margin-bottom: 0; /* Handled by form-group */
            box-sizing: border-box; /* Include padding and border in width */
        }
        .form-group {
            margin-bottom: 15px; /* Spacing between input fields */
        }
        input[type="file"] {
            display: none; /* Hide default file input */
        }
        .file-input-label {
            background-color: var(--accent-color);
            color: white;
            padding: 10px 15px;
            border-radius: 6px;
            cursor: pointer;
            transition: background-color 0.2s ease, transform 0.1s ease;
            display: inline-block;
            margin-bottom: 10px;
            font-weight: bold;
            box-shadow: 0 2px 5px rgba(0,0,0,0.2);
        }
        .file-input-label:hover {
            background-color: var(--hover-color);
            transform: translateY(-1px);
        }

        button, input[type="submit"] {
            background-color: var(--accent-color);
            color: white;
            border: none;
            padding: 10px 15px;
            border-radius: 6px;
            cursor: pointer;
            transition: background-color 0.2s ease, transform 0.1s ease, box-shadow 0.2s ease;
            font-weight: bold;
            box-shadow: 0 2px 5px rgba(0,0,0,0.2);
            display: inline-flex;
            align-items: center;
            justify-content: center;
            gap: 5px;
            text-transform: uppercase;
            font-size: 0.9em;
        }
        button:hover, input[type="submit"]:hover {
            background-color: var(--hover-color);
            transform: translateY(-1px);
            box-shadow: 0 4px 8px rgba(0,0,0,0.3);
        }
        .button.delete-btn {
            background-color: var(--delete-color);
            padding: 8px; /* Smaller padding to make it compact for circular shape */
            border-radius: 50%; /* Make it perfectly circular */
            width: 36px; /* Fixed width */
            height: 36px; /* Fixed height */
            display: flex; /* Use flexbox to center content */
            align-items: center; /* Center vertically */
            justify-content: center; /* Center horizontally */
            font-size: 1.2em; /* Make the '❌' icon larger */
            box-shadow: 0 2px 5px rgba(0,0,0,0.2); /* Keep consistent shadow */
            flex-shrink: 0; /* Prevent shrinking when space is limited */
        }
        .button.delete-btn:hover {
            background-color: #b32a3b;
        }
        .button.delete-action-btn { /* Specific style for Format SD Card and similar actions */
            background-color: var(--delete-color);
            border-radius: 6px; /* Standard rounded corners */
            padding: 10px 15px;
            font-weight: bold;
            box-shadow: 0 2px 5px rgba(0,0,0,0.2);
            transition: background-color 0.2s ease, transform 0.1s ease, box-shadow 0.2s ease;
            display: inline-flex;
            align-items: center;
            justify-content: center;
            gap: 5px;
            text-transform: uppercase;
            font-size: 0.9em;
            cursor: pointer;
            border: none;
        }
        .button.delete-action-btn:hover {
            background-color: #b32a3b;
            transform: translateY(-1px);
            box-shadow: 0 4px 8px rgba(0,0,0,0.3);
        }
        .button.logout-btn {
            background-color: #6c757d; /* Gray color for logout */
            margin-top: 15px;
            width: 100%;
        }
        .button.logout-btn:hover {
            background-color: #5a6268;
        }
        .upload-section {
            background-color: var(--secondary-color);
            padding: 25px;
            margin-top: 25px;
            border-radius: 10px;
            box-shadow: 0 2px 8px rgba(0,0,0,0.2);
            border: 1px solid var(--border-color);
        }
        progress {
            width: 100%;
            height: 25px;
            border-radius: 5px;
            margin-top: 15px;
            appearance: none; /* For cross-browser consistency */
            -webkit-appearance: none;
            overflow: hidden; /* Hide the default border on some browsers */
        }
        progress::-webkit-progress-bar {
            background-color: var(--primary-color);
            border-radius: 5px;
        }
        progress::-webkit-progress-value {
            background-color: var(--success-color);
            border-radius: 5px;
            transition: width 0.3s ease;
        }
        progress::-moz-progress-bar {
            background-color: var(--success-color);
            border-radius: 5px;
        }

        .current-path {
            font-size: 1.1em;
            margin-bottom: 20px;
            color: #b0b0b0;
            padding: 10px 15px;
            background-color: var(--secondary-color);
            border-radius: 8px;
            border: 1px solid var(--border-color);
        }
        .user-status {
            font-size: 0.95em;
            margin-bottom: 20px;
            color: #a0a0a0;
            padding: 8px 15px;
            background-color: var(--secondary-color);
            border-radius: 8px;
            border: 1px solid var(--border-color);
        }
        .user-accounts-section .account-forms {
            display: grid;
            grid-template-columns: 1fr;
            gap: 20px;
            margin-top: 20px;
        }
        .user-accounts-section form {
            background-color: #333; /* Slightly different background for forms */
            padding: 20px;
            border-radius: 8px;
            border: 1px solid #555;
            box-shadow: inset 0 1px 3px rgba(0,0,0,0.3);
        }
        .user-accounts-section form h4 {
            margin-top: 0;
            color: var(--text-color);
            font-weight: 500;
            border-bottom: 1px dashed #555;
            padding-bottom: 10px;
            margin-bottom: 15px;
        }
        .user-accounts-section input[type="checkbox"] {
            margin-right: 8px;
        }
        .user-accounts-section .checkbox-label {
            display: flex;
            align-items: center;
            margin-bottom: 15px;
            font-size: 0.95em;
        }

        @media (min-width: 768px) {
            .user-accounts-section .account-forms {
                grid-template-columns: 1fr 1fr; /* Two columns on larger screens */
            }
        }
    )rawliteral";

const char* LIGHT_THEME_CSS = R"rawliteral(
        :root {
            --bg-color: #f8f9fa;
            --primary-color: #ffffff;
            --secondary-color: #e9ecef;
            --text-color: #343a40;
            --accent-color: #007bff;
            --hover-color: #0056b3;
            --delete-color: #dc3545;
            --success-color: #28a745;
            --border-color: #ced4da;
            --shadow-color: rgba(0,0,0,0.1);
        }
        body {
            font-family: 'Inter', -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif;
            background-color: var(--bg-color);
            color: var(--text-color);
            margin: 0;
            padding: 20px;
            line-height: 1.6;
        }
        .container {
            max-width: 800px;
            margin: 20px auto;
            background-color: var(--primary-color);
            padding: 30px;
            border-radius: 12px;
            box-shadow: 0 8px 20px var(--shadow-color);
            border: 1px solid var(--border-color);
        }
        h2, h3 {
            color: var(--accent-color);
            border-bottom: 2px solid var(--accent-color);
            padding-bottom: 10px;
            margin-top: 25px;
            margin-bottom: 20px;
            font-weight: 600;
        }
        h2 {
            font-size: 2em;
            text-align: center;
        }
        h3 {
            font-size: 1.4em;
        }
        ul {
            list-style: none;
            padding: 0;
        }
        li {
            background-color: var(--secondary-color);
            margin: 8px 0;
            padding: 12px 18px;
            border-radius: 8px;
            display: flex;
            align-items: center;
            justify-content: space-between;
            transition: background-color 0.2s ease, transform 0.1s ease;
            box-shadow: 0 2px 5px rgba(0,0,0,0.08);
            border: 1px solid var(--border-color);
        }
        li:hover {
            background-color: #dee2e6;
            transform: translateY(-2px);
        }
        a {
            color: var(--accent-color);
            text-decoration: none;
            font-weight: 500;
            transition: color 0.2s ease;
        }
        a:hover {
            color: var(--hover-color);
        }
        .file-info {
            flex-grow: 1;
            overflow: hidden; /* Hide overflow */
            text-overflow: ellipsis; /* Add ellipsis */
            white-space: nowrap; /* Prevent wrapping */
            padding-right: 10px; /* Space before actions */
        }
        .actions {
            display: flex;
            gap: 8px; /* Space between buttons */
            flex-shrink: 0; /* Don't shrink actions */
        }
        .actions form, .actions a {
            display: inline-flex; /* Use flex for button alignment */
            align-items: center;
        }
        input[type="text"], input[type="password"] {
            background-color: var(--primary-color);
            color: var(--text-color);
            border: 1px solid var(--border-color);
            padding: 10px 12px;
            border-radius: 6px;
            width: calc(100% - 24px); /* Full width minus padding */
            margin-bottom: 0; /* Handled by form-group */
            box-sizing: border-box; /* Include padding and border in width */
        }
        .form-group {
            margin-bottom: 15px; /* Spacing between input fields */
        }
        input[type="file"] {
            display: none; /* Hide default file input */
        }
        .file-input-label {
            background-color: var(--accent-color);
            color: white;
            padding: 10px 15px;
            border-radius: 6px;
            cursor: pointer;
            transition: background-color 0.2s ease, transform 0.1s ease;
            display: inline-block;
            margin-bottom: 10px;
            font-weight: bold;
            box-shadow: 0 2px 5px rgba(0,0,0,0.1);
        }
        .file-input-label:hover {
            background-color: var(--hover-color);
            transform: translateY(-1px);
        }

        button, input[type="submit"] {
            background-color: var(--accent-color);
            color: white;
            border: none;
            padding: 10px 15px;
            border-radius: 6px;
            cursor: pointer;
            transition: background-color 0.2s ease, transform 0.1s ease, box-shadow 0.2s ease;
            font-weight: bold;
            box-shadow: 0 2px 5px rgba(0,0,0,0.1);
            display: inline-flex;
            align-items: center;
            justify-content: center;
            gap: 5px;
            text-transform: uppercase;
            font-size: 0.9em;
        }
        button:hover, input[type="submit"]:hover {
            background-color: var(--hover-color);
            transform: translateY(-1px);
            box-shadow: 0 4px 8px rgba(0,0,0,0.2);
        }
        .button.delete-btn {
            background-color: var(--delete-color);
            padding: 8px; /* Smaller padding to make it compact for circular shape */
            border-radius: 50%; /* Make it perfectly circular */
            width: 36px; /* Fixed width */
            height: 36px; /* Fixed height */
            display: flex; /* Use flexbox to center content */
            align-items: center; /* Center vertically */
            justify-content: center; /* Center horizontally */
            font-size: 1.2em; /* Make the '❌' icon larger */
            box-shadow: 0 2px 5px rgba(0,0,0,0.1); /* Keep consistent shadow */
            flex-shrink: 0; /* Prevent shrinking when space is limited */
        }
        .button.delete-btn:hover {
            background-color: #b32a3b;
        }
        .button.delete-action-btn { /* Specific style for Format SD Card and similar actions */
            background-color: var(--delete-color);
            border-radius: 6px; /* Standard rounded corners */
            padding: 10px 15px;
            font-weight: bold;
            box-shadow: 0 2px 5px rgba(0,0,0,0.1);
            transition: background-color 0.2s ease, transform 0.1s ease, box-shadow 0.2s ease;
            display: inline-flex;
            align-items: center;
            justify-content: center;
            gap: 5px;
            text-transform: uppercase;
            font-size: 0.9em;
            cursor: pointer;
            border: none;
        }
        .button.delete-action-btn:hover {
            background-color: #b32a3b;
            transform: translateY(-1px);
            box-shadow: 0 4px 8px rgba(0,0,0,0.2);
        }
        .button.logout-btn {
            background-color: #6c757d; /* Gray color for logout */
            margin-top: 15px;
            width: 100%;
        }
        .button.logout-btn:hover {
            background-color: #5a6268;
        }
        .upload-section {
            background-color: var(--secondary-color);
            padding: 25px;
            margin-top: 25px;
            border-radius: 10px;
            box-shadow: 0 2px 8px rgba(0,0,0,0.08);
            border: 1px solid var(--border-color);
        }
        progress {
            width: 100%;
            height: 25px;
            border-radius: 5px;
            margin-top: 15px;
            appearance: none; /* For cross-browser consistency */
            -webkit-appearance: none;
            overflow: hidden; /* Hide the default border on some browsers */
        }
        progress::-webkit-progress-bar {
            background-color: var(--primary-color);
            border-radius: 5px;
        }
        progress::-webkit-progress-value {
            background-color: var(--success-color);
            border-radius: 5px;
            transition: width 0.3s ease;
        }
        progress::-moz-progress-bar {
            background-color: var(--success-color);
            border-radius: 5px;
        }

        .current-path {
            font-size: 1.1em;
            margin-bottom: 20px;
            color: #6c757d;
            padding: 10px 15px;
            background-color: var(--secondary-color);
            border-radius: 8px;
            border: 1px solid var(--border-color);
        }
        .user-status {
            font-size: 0.95em;
            margin-bottom: 20px;
            color: #6c757d;
            padding: 8px 15px;
            background-color: var(--secondary-color);
            border-radius: 8px;
            border: 1px solid var(--border-color);
        }
        .user-accounts-section .account-forms {
            display: grid;
            grid-template-columns: 1fr;
            gap: 20px;
            margin-top: 20px;
        }
        .user-accounts-section form {
            background-color: #f1f3f5; /* Slightly different background for forms */
            padding: 20px;
            border-radius: 8px;
            border: 1px solid #dee2e6;
            box-shadow: inset 0 1px 3px rgba(0,0,0,0.05);
        }
        .user-accounts-section form h4 {
            margin-top: 0;
            color: var(--text-color);
            font-weight: 500;
            border-bottom: 1px dashed #adb5bd;
            padding-bottom: 10px;
            margin-bottom: 15px;
        }
        .user-accounts-section input[type="checkbox"] {
            margin-right: 8px;
        }
        .user-accounts-section .checkbox-label {
            display: flex;
            align-items: center;
            margin-bottom: 15px;
            font-size: 0.95em;
        }

        @media (min-width: 768px) {
            .user-accounts-section .account-forms {
                grid-template-columns: 1fr 1fr; /* Two columns on larger screens */
            }
        }
    )rawliteral";

// Generates the full HTML with the selected theme's CSS
String generateThemedHtml() {
    String html = HTML_TEMPLATE;
    String cssToInject;
    if (currentTheme == "light") {
        cssToInject = LIGHT_THEME_CSS;
    } else {
        cssToInject = DARK_THEME_CSS;
    }
    html.replace("/* THEME_CSS_PLACEHOLDER */", cssToInject);
    // Inject user info into HTML
    html.replace("<!-- LOGGED_IN_USER_PLACEHOLDER -->", currentLoggedInUser.isEmpty() ? "Guest" : currentLoggedInUser);
    html.replace("<!-- UPLOAD_RIGHTS_PLACEHOLDER -->", currentLoggedInUserUploadRights ? "true" : "false");
    return html;
}

// =======================================================================
//    SERVER HELPER FUNCTIONS
// =======================================================================

// Helper to determine if a file is viewable in browser (text-based) for OLED preview
bool isViewable(String filename) {
  filename.toLowerCase();
  if (filename.endsWith(".txt") ||
      filename.endsWith(".html") ||
      filename.endsWith(".css") ||
      filename.endsWith(".js") ||
      filename.endsWith(".json") ||
      filename.endsWith(".xml") ||
      filename.endsWith(".log") ||
      filename.endsWith(".c") ||
      filename.endsWith(".cpp") ||
      filename.endsWith(".h") ||
      filename.endsWith(".ino") ||
      filename.endsWith(".md") ||
      filename.endsWith(".py") ||
      filename.endsWith(".sh")) { // Added markdown, python, shell scripts
    return true;
  }
  return false;
}

// Helper to get MIME type for file serving
String getContentType(String filename) {
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".json")) return "application/json";
  else if (filename.endsWith(".xml")) return "application/xml";
  else if (filename.endsWith(".txt")) return "text/plain";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  else if (filename.endsWith(".jpeg")) return "image/jpeg";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".gif")) return "image/gif";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".svg")) return "image/svg+xml";
  else if (filename.endsWith(".pdf")) return "application/pdf";
  else if (filename.endsWith(".zip")) return "application/zip";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  // Add more as needed
  return "application/octet-stream"; // Default for unknown types (forces download)
}

// Formats file sizes for display
String formatBytes(size_t bytes) {
  if (bytes < 1024) return String(bytes) + " B";
  if (bytes < 1024 * 1024) return String(bytes / 1024.0, 2) + " KB";
  return String(bytes / 1024.0 / 1024.0, 2) + " MB";
}

// Function to handle listing files in a given path
void handleRoot() {
  if (isServerLocked) {
      server.send(401, "text/plain", "Unauthorized: Server Locked. Unlock via OLED.");
      return;
  }
  lastActivityTime = millis(); // Reset activity timer

  // New: Device Fingerprinting - Track connected client's IP and User-Agent
  String clientIP = server.client().remoteIP().toString();
  String userAgent = server.header("User-Agent");
  if (!clientIP.isEmpty()) {
      connectedClientFingerprints[clientIP] = userAgent;
      Serial.print("Client connected: ");
      Serial.print(clientIP);
      Serial.print(" (");
      Serial.print(userAgent);
      Serial.println(")");
      displayUserInfoScreen(); // Update OLED with connected client info (simplified)
  }


  String path = "/";
  if (server.hasArg("path")) {
    path = server.arg("path");
    if (!path.startsWith("/")) {
      path = "/" + path;
    }
    if (!path.endsWith("/") && SD.exists(path) && SD.open(path).isDirectory()) {
      path += "/"; // Ensure directory paths end with a slash
    }
  }
  currentPath = path; // Update the global current path

  File root = SD.open(path);
  if (!root) {
    server.send(500, "text/plain", "Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    server.send(500, "text/plain", "Not a directory");
    return;
  }

  String fileListHtml = "";

  // Add ".." (parent directory) link if not at root
  if (path != "/") {
    String parentPath = path.substring(0, path.lastIndexOf('/'));
    if (parentPath == "") parentPath = "/"; // Ensure root is '/'
    fileListHtml += "<li><div class='file-info'><a href='/?path=" + parentPath + "'>⬆️ .. (Parent Directory)</a></div></li>";
  }

  File entry = root.openNextFile();
  while (entry) {
    String entryName = String(entry.name());
    // Remove the leading path from entryName for display purposes
    // SD.openNextFile() might return full path for some filesystems or contexts
    // Ensure we only use the local name for display.
    if (entryName.startsWith(path)) {
      entryName = entryName.substring(path.length());
    }

    if (entry.isDirectory()) {
      String folderPath = path + entryName;
      if (!folderPath.endsWith("/")) folderPath += "/";
      fileListHtml += "<li><div class='file-info'>📁 <a href='/?path=" + folderPath + "'><strong>" + entryName + "/</strong></a></div>";
      // Add delete for empty folders on web UI
      File subDir = SD.open(folderPath);
      if (subDir && subDir.isDirectory() && !subDir.openNextFile()) { // Check if empty
        fileListHtml += "<div class='actions'><a href='javascript:void(0);' onclick=\"confirmDelete('/delete?name=" + folderPath + "')\" class='button delete-btn'>❌</a></div></li>";
      } else {
        fileListHtml += "</li>"; // Close li if not empty
      }
      subDir.close();

    } else {
      String fullPath = path + entryName; // Full path for download/delete
      String linkAttr = "";
      String icon = "📄"; // Default icon

      fileListHtml += "<li><div class='file-info'><a href='" + fullPath + "' " + linkAttr + ">" + icon + " " + entryName + "</a> (" + formatBytes(entry.size()) + ")</div>";
      fileListHtml += "<div class='actions'>";
      fileListHtml += "<form method='POST' action='/rename'><input type='hidden' name='old' value='" + fullPath + "'><input name='new' placeholder='New Name'><button type='submit' class='button'>✏️ Rename</button></form>";
      fileListHtml += "<a href='javascript:void(0);' onclick=\"confirmDelete('/delete?name=" + fullPath + "')\" class='button delete-btn'>❌</a>";
      // New: Add Broadcast button
      fileListHtml += "<a href='/set_broadcast?name=" + fullPath + "' class='button'>📢 Broadcast</a>";
      fileListHtml += "</div></li>";
    }
    entry.close();
    entry = root.openNextFile();
  }
  root.close();

  String finalHtml = generateThemedHtml(); // Get themed HTML template
  finalHtml.replace("<!-- FILE_LIST_PLACEHOLDER -->", fileListHtml);
  server.send(200, "text/html; charset=UTF-8", finalHtml);

  // Update OLED with current path (file tree view)
  populateOLEDFileList(currentPath);
  displayFileTree();
}

void handleFileRequest(String filename) {
  if (isServerLocked) {
      server.send(401, "text/plain", "Unauthorized: Server Locked. Unlock via OLED.");
      return;
  }
  lastActivityTime = millis(); // Reset activity timer

  // Check upload rights for delete/rename/mkdir, etc.
  // For file downloads, no specific rights needed, just server unlock.

  File file = SD.open(filename);
  if (!file || file.isDirectory()) {
    server.send(404, "text/plain", "File Not Found");
    return;
  }

  // New: Data Analytics - Increment download count
  totalDownloads++;
  fileDownloadCounts[filename]++;
  savePreferences(); // Save updated analytics

  // Display "Serving..." message on OLED
  String displayFilename = filename;
  int lastSlash = displayFilename.lastIndexOf('/');
  if (lastSlash != -1) {
    displayFilename = displayFilename.substring(lastSlash + 1);
  }
  if (displayFilename.length() > 15) { // Truncate for OLED display if needed
    displayFilename = displayFilename.substring(0, 12) + "...";
  }
  displayOperation(getOLEDString("SERVING") + " " + displayFilename); // Show status on OLED

  server.streamFile(file, getContentType(filename));
  file.close();

  // Return to previous OLED state (e.g., file tree) or idle after serving
  delay(500); // Give user a moment to see "Serving"
  // After serving, go back to the file tree view on OLED
  populateOLEDFileList(currentPath);
  displayFileTree();
}

void handleFileUpload() {
  if (isServerLocked) {
      server.send(401, "text/plain", "Unauthorized: Server Locked. Unlock via OLED.");
      return;
  }
  // New: Check user upload rights
  if (!currentLoggedInUserUploadRights) {
      server.send(403, "text/plain", getOLEDString("INSUFFICIENT_PERMS"));
      return;
  }
  lastActivityTime = millis(); // Reset activity timer

  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    String uploadDir = server.arg("dir");

    if (!uploadDir.endsWith("/") && uploadDir != "/") {
      uploadDir += "/";
    }
    String fullPath = uploadDir + filename;

    uploadFile = SD.open(fullPath, FILE_WRITE);
    if (!uploadFile) {
      Serial.println("File open failed");
      server.send(500, "text/plain", "Failed to create file on SD card");
      return;
    }
    Serial.printf("Upload Start: %s\n", fullPath.c_str());
    // New: Add to active uploads
    activeUploads.push_back({filename, server.client().remoteIP().toString(), 0, server.header("User-Agent")});
    displayProgress(0, upload.filename, server.client().remoteIP().toString()); // Show progress on OLED

  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) {
      uploadFile.write(upload.buf, upload.currentSize);
      int progress = (upload.totalSize > 0) ? (upload.currentSize * 100) / upload.totalSize : 0;
      // Update active uploads progress
      for (auto& info : activeUploads) {
          if (info.filename == upload.filename && info.clientIP == server.client().remoteIP().toString()) {
              info.percentage = progress;
              break;
          }
      }
      displayProgress(progress, upload.filename, server.client().remoteIP().toString()); // Update progress on OLED
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) {
      uploadFile.close();
      Serial.printf("Upload End: %s, %u bytes\n", upload.filename.c_str(), upload.totalSize);
      // New: Data Analytics - Increment upload count
      totalUploads++;
      savePreferences(); // Save updated analytics

      displayOperation(getOLEDString("FILE_RECEIVED")); // Indicate completion on OLED
      delay(1000); // Show message briefly

      // Remove from active uploads
      for (size_t i = 0; i < activeUploads.size(); ++i) {
          if (activeUploads[i].filename == upload.filename && activeUploads[i].clientIP == server.client().remoteIP().toString()) {
              activeUploads.erase(activeUploads.begin() + i);
              break;
          }
      }

      populateOLEDFileList(currentPath); // Refresh file list on OLED
      displayFileTree();
    }
    server.send(200, "text/plain", "Upload Successful"); // Send success response
  }
}

void handleDelete() {
  if (isServerLocked) {
      server.send(401, "text/plain", "Unauthorized: Server Locked. Unlock via OLED.");
      return;
  }
  // New: Check user upload rights (or specific delete rights)
  if (!currentLoggedInUserUploadRights) { // Assuming upload rights also grant delete
      server.send(403, "text/plain", getOLEDString("INSUFFICIENT_PERMS"));
      return;
  }
  lastActivityTime = millis(); // Reset activity timer

  if (!server.hasArg("name")) {
    server.send(400, "text/plain", "BAD REQUEST: Missing 'name' argument");
    return;
  }
  // Store the item to delete for the OLED confirmation callback
  pendingDeleteItem = server.arg("name");

  // Show OLED confirmation prompt for deletion
  displayConfirmation(getOLEDString("CONFIRM_DELETE"), handleDeleteConfirmed, [](){
      // On "No" from OLED, redirect back to current path without deleting
      server.sendHeader("Location", "/?path=" + currentPath, true);
      server.send(302, "text/plain", "");
      populateOLEDFileList(currentPath); // Refresh file list on OLED (no change)
      displayFileTree();
  });

  // Web server should wait for OLED confirmation or timeout, or simply indicate action taken
  // For simplicity, we'll send a "waiting for confirmation" or "redirecting" to web
  // and handle the actual deletion in handleDeleteConfirmed.
  server.send(200, "text/plain", "Waiting for OLED confirmation for deletion of " + pendingDeleteItem);
}

void handleDeleteConfirmed() {
    bool success = false;
    // For safety, ensure the path starts with a slash
    if (!pendingDeleteItem.startsWith("/")) {
        pendingDeleteItem = "/" + pendingDeleteItem;
    }

    File entry = SD.open(pendingDeleteItem);
    if (entry) {
        if (entry.isDirectory()) {
            if (!entry.openNextFile()) { // If openNextFile returns false, directory is empty
                success = SD.rmdir(pendingDeleteItem);
                if (success) {
                    displayOperation(getOLEDString("FOLDER_DELETED"));
                } else {
                    displayOperation(getOLEDString("DELETE_FAILED"));
                }
            } else {
                displayOperation(getOLEDString("FOLDER_NOT_EMPTY"));
            }
        } else { // It's a file
            success = SD.remove(pendingDeleteItem);
            if (success) {
                displayOperation(getOLEDString("FILE_DELETED"));
            } else {
                displayOperation(getOLEDString("DELETE_FAILED"));
            }
        }
        entry.close();
    } else {
        displayOperation(getOLEDString("ITEM_NOT_FOUND"));
    }

    delay(1000); // Show message briefly
    // After deletion, refresh the file list on OLED
    populateOLEDFileList(currentPath);
    displayFileTree();

    // The web browser needs to be redirected. Since this is an OLED callback,
    // we can't directly send a redirect to the browser that initiated the /delete request.
    // The user will need to manually refresh the web page after the OLED confirmation.
    // Or, for a better UX, implement AJAX polling on the web client.
    // For now, we rely on manual refresh for simplicity.
}


void handleRename() {
  if (isServerLocked) {
      server.send(401, "text/plain", "Unauthorized: Server Locked. Unlock via OLED.");
      return;
  }
  // New: Check user upload rights (assuming rename also requires upload rights)
  if (!currentLoggedInUserUploadRights) {
      server.send(403, "text/plain", getOLEDString("INSUFFICIENT_PERMS"));
      return;
  }
  lastActivityTime = millis(); // Reset activity timer

  if (!server.hasArg("old") || !server.hasArg("new")) {
    server.send(400, "text/plain", "BAD REQUEST: Missing arguments");
    return;
  }
  String oldName = server.arg("old");
  String newName = server.arg("new");

  // Ensure full paths for old and new names
  if (!oldName.startsWith("/")) oldName = "/" + oldName;
  if (!newName.startsWith("/")) {
    // If new name doesn't start with /, assume it's relative to current path
    newName = currentPath + newName;
  }

  if (SD.rename(oldName, newName)) {
    displayOperation(getOLEDString("FILE_RENAMED"));
  } else {
    displayOperation(getOLEDString("RENAME_FAILED"));
  }
  delay(1000); // Show message briefly
  populateOLEDFileList(currentPath); // Refresh file list on OLED
  displayFileTree();

  server.sendHeader("Location", "/?path=" + currentPath, true);
  server.send(302, "text/plain", "");
}

void handleMkdir() {
  if (isServerLocked) {
      server.send(401, "text/plain", "Unauthorized: Server Locked. Unlock via OLED.");
      return;
  }
  // New: Check user upload rights
  if (!currentLoggedInUserUploadRights) {
      server.send(403, "text/plain", getOLEDString("INSUFFICIENT_PERMS"));
      return;
  }
  lastActivityTime = millis(); // Reset activity timer

  if (!server.hasArg("folder") || !server.hasArg("dir")) {
    server.send(400, "text/plain", "BAD REQUEST: Missing arguments");
    return;
  }
  String folderName = server.arg("folder");
  String parentDir = server.arg("dir");

  // Ensure parentDir ends with a slash if it's not root
  if (!parentDir.endsWith("/") && parentDir != "/") {
    parentDir += "/";
  }
  String fullPath = parentDir + folderName;

  if (SD.mkdir(fullPath)) {
    displayOperation(getOLEDString("FOLDER_CREATED"));
  } else {
    displayOperation(getOLEDString("CREATE_FAILED"));
  }
  delay(1000); // Show message briefly
  populateOLEDFileList(currentPath); // Refresh file list on OLED
  displayFileTree();

  server.sendHeader("Location", "/?path=" + currentPath, true);
  server.send(302, "text/plain", "");
}

void handleFormatSD() {
    if (isServerLocked) {
        server.send(401, "text/plain", "Unauthorized: Server Locked. Unlock via OLED.");
        return;
    }
    // New: Check user upload rights (assuming format requires high privileges)
    if (!currentLoggedInUserUploadRights) {
        server.send(403, "text/plain", getOLEDString("INSUFFICIENT_PERMS"));
        return;
    }
    lastActivityTime = millis(); // Reset activity timer

    // Show OLED confirmation prompt for formatting
    displayConfirmation(getOLEDString("FORMAT_SD"), formatSDConfirmed, [](){
        // On "No" from OLED, redirect back to current path
        server.sendHeader("Location", "/?path=" + currentPath, true);
        server.send(302, "text/plain", "");
        populateOLEDFileList(currentPath); // Refresh file list on OLED (no change)
        displayFileTree();
    });

    server.send(200, "text/plain", "Waiting for OLED confirmation to format SD card.");
}

void formatSDConfirmed() {
    displayOperation(getOLEDString("FORMATTING"));
    Serial.println("Attempting to format SD card...");

    bool success = false;
    // Attempt to format by re-initializing SD.
    // The `true` parameter will attempt to format the card if it fails to mount.
    // This is not a direct SD.format() function but a common way to force re-initialization.
    SD.end(); // Unmount SD card
    // Use the mountpoint "/sd" as the 4th argument.
    success = SD.begin(SD_CS_PIN, spi, 4000000, "/sd", 5, true); // Re-initialize with format_if_failed = true

    if (success) {
        displayOperation(getOLEDString("FORMAT_SUCCESS"));
        Serial.println("SD card formatted and re-initialized successfully.");
    } else {
        displayOperation(getOLEDString("FORMAT_FAIL"));
        Serial.println("Failed to format or re-initialize SD card!");
    }
    delay(2000); // Show message briefly

    // After format, re-initialize SD card and display file tree
    if (!SD.begin(SD_CS_PIN, spi)) { // Final check for SD card
        Serial.println("SD Card re-initialization failed after format (final check)!");
        displayOperation(getOLEDString("SD_INIT_FAIL"));
        while(true) { delay(100); } // Halt execution if SD card fails again
    }
    populateOLEDFileList("/"); // Go to root after format
    displayFileTree();

    // No direct web redirect from here, user needs to refresh browser.
}

// New: Handle setting a file for broadcast
void handleSetBroadcast() {
    if (isServerLocked) {
        server.send(401, "text/plain", "Unauthorized: Server Locked. Unlock via OLED.");
        return;
    }
    if (!server.hasArg("name")) {
        server.send(400, "text/plain", "BAD REQUEST: Missing 'name' argument");
        return;
    }
    broadcastFilePath = server.arg("name");
    displayOperation(getOLEDString("BROADCASTING") + " " + broadcastFilePath.substring(broadcastFilePath.lastIndexOf('/') + 1));
    Serial.println("File set for broadcast: " + broadcastFilePath);
    delay(1500); // Show message briefly
    server.sendHeader("Location", "/?path=" + currentPath, true);
    server.send(302, "text/plain", "");
    populateOLEDFileList(currentPath);
    displayFileTree();
}

// New: Handle serving the broadcasted file
void handleBroadcastDownload() {
    if (isServerLocked) {
        server.send(401, "text/plain", "Unauthorized: Server Locked. Unlock via OLED.");
        return;
    }
    if (broadcastFilePath.isEmpty()) {
        server.send(404, "text/plain", "No file set for broadcast.");
        return;
    }
    handleFileRequest(broadcastFilePath); // Reuse existing file serving logic
}

// New: Handle web-based creation of temporary user accounts
void handleCreateUser() {
    if (isServerLocked) {
        server.send(401, "text/plain", "Unauthorized: Server Locked. Unlock via OLED.");
        return;
    }
    lastActivityTime = millis();

    if (!server.hasArg("username") || !server.hasArg("password")) {
        server.send(400, "text/plain", "Username and Password required.");
        return;
    }

    String username = server.arg("username");
    String password = server.arg("password"); // In a real app, hash this!
    bool uploadRights = server.hasArg("upload_rights") && server.arg("upload_rights") == "true";

    // Check if username already exists
    for (const auto& user : tempUserAccounts) {
        if (user.username == username) {
            server.send(409, "text/plain", "Username already exists.");
            return;
        }
    }

    TempUserAccount newUser;
    newUser.username = username;
    newUser.pinHash = password; // Storing as plain text for simplicity
    newUser.uploadRights = uploadRights;
    tempUserAccounts.push_back(newUser);

    Serial.println("New user created via web: " + username + ", Upload Rights: " + (uploadRights ? "Yes" : "No"));
    server.send(200, "text/plain", "User created successfully!");
    // Optional: Log in the newly created user automatically
    currentLoggedInUser = username;
    currentLoggedInUserUploadRights = uploadRights;
    displayOperation(getOLEDString("USER_CREATED"));
    delay(1000);
    server.sendHeader("Location", "/?path=" + currentPath, true); // Redirect to root
    server.send(302, "text/plain", "");
}

// New: Handle web-based user login
void handleLogin() {
    if (isServerLocked) {
        server.send(401, "text/plain", "Unauthorized: Server Locked. Unlock via OLED.");
        return;
    }
    lastActivityTime = millis();

    if (!server.hasArg("username") || !server.hasArg("password")) {
        server.send(400, "text/plain", "Username and Password required.");
        return;
    }

    String username = server.arg("username");
    String password = server.arg("password");

    bool loginSuccess = false;
    for (const auto& user : tempUserAccounts) {
        if (user.username == username && user.pinHash == password) {
            currentLoggedInUser = user.username;
            currentLoggedInUserUploadRights = user.uploadRights;
            loginSuccess = true;
            break;
        }
    }

    if (loginSuccess) {
        Serial.println("User " + currentLoggedInUser + " logged in via web.");
        server.send(200, "text/plain", "Login successful!");
        displayOperation(getOLEDString("LOGIN_SUCCESS"));
    } else {
        Serial.println("Web Login failed for " + username);
        server.send(401, "text/plain", getOLEDString("LOGIN_FAIL"));
        displayOperation(getOLEDString("LOGIN_FAIL"));
    }
    delay(1000);
    server.sendHeader("Location", "/?path=" + currentPath, true); // Redirect to root
    server.send(302, "text/plain", "");
}

// New: Handle user logout
void handleLogout() {
    if (isServerLocked) {
        server.send(401, "text/plain", "Unauthorized: Server Locked. Unlock via OLED.");
        return;
    }
    lastActivityTime = millis();
    currentLoggedInUser = ""; // Clear logged in user
    currentLoggedInUserUploadRights = false;
    Serial.println("User logged out.");
    server.send(200, "text/plain", "Logged out.");
    displayOperation("Logged Out");
    delay(1000);
    server.sendHeader("Location", "/?path=" + currentPath, true); // Redirect to root
    server.send(302, "text/plain", "");
}

// =======================================================================
//    POWER MANAGEMENT FUNCTIONS
// =======================================================================

void enterSleepMode() {
    if (!isAPActive) return; // Already asleep

    Serial.println(getOLEDString("GO_TO_SLEEP"));
    displaySleepAnimation(); // Show sleep animation
    delay(2000); // Let animation play

    WiFi.softAPdisconnect(true); // Disconnect and free resources
    isAPActive = false;
    Serial.println("AP disconnected.");

    // Turn off OLED pixels by clearing the display buffer and updating the display
    display.clearDisplay();
    display.display();
}

void wakeUpFromSleep() {
    if (isAPActive) return; // Already awake

    // Turn on OLED pixels (display will automatically update with new content)
    display.clearDisplay(); // Clear any previous sleep content
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 25);
    display.println(getOLEDString("WAKING_UP"));
    display.display();
    delay(1000); // Show message

    Serial.println(getOLEDString("WAKING_UP"));
    WiFi.softAP(ssid, password);
    IPAddress ip = WiFi.softAPIP();
    Serial.print("AP IP: ");
    Serial.println(ip);
    isAPActive = true;
    Serial.println("AP reconnected.");

    lastActivityTime = millis(); // Reset activity timer

    if (!isServerLocked) { // If server was unlocked before sleep
        if (WiFi.softAPgetStationNum() > 0) { // If clients connected immediately after waking
            populateOLEDFileList(currentPath);
            displayFileTree();
        } else { // No clients, go to idle
            displayIdleScreen();
        }
    } else { // Otherwise, go back to PIN entry
        displayPINEntryScreen();
    }
}

// =======================================================================
//    PREFERENCES (NVS) FUNCTIONS
// =======================================================================
void loadPreferences() {
    preferences.begin("vft-server", false); // Open preferences in read-write mode
    currentTheme = preferences.getString("theme", "dark");
    currentLanguage = preferences.getString("language", "en");
    totalUploads = preferences.getULong("total_uploads", 0);
    totalDownloads = preferences.getULong("total_downloads", 0);

    // Load file download counts (simplified: store top 5 as string)
    String downloadCountsJson = preferences.getString("dl_counts", ""); // Changed default to empty string
    if (!downloadCountsJson.isEmpty()) {
        // Parse JSON string into map. This is a very basic example and might need a proper JSON library for complex parsing.
        // For now, assume a simple format like "file1:10,file2:5".
        int start = 0;
        int end = downloadCountsJson.indexOf(',');
        while (end != -1) {
            String pair = downloadCountsJson.substring(start, end);
            int colon = pair.indexOf(':');
            if (colon != -1) {
                String filename = pair.substring(0, colon);
                unsigned int count = pair.substring(colon + 1).toInt();
                fileDownloadCounts[filename] = count;
            }
            start = end + 1;
            end = downloadCountsJson.indexOf(',', start);
        }
        // Handle the last entry if any
        if (start < downloadCountsJson.length()) { // Check if there's a last segment
            String lastPair = downloadCountsJson.substring(start);
            int colon = lastPair.indexOf(':');
            if (colon != -1) {
                String filename = lastPair.substring(0, colon);
                unsigned int count = lastPair.substring(colon + 1).toInt();
                fileDownloadCounts[filename] = count;
            }
        }
    }


    preferences.end();

    // Set the correct language map based on loaded preference
    if (currentLanguage == "hi") {
        current_lang_map = &hi_strings;
    } else if (currentLanguage == "de") {
        current_lang_map = &de_strings;
    } else {
        current_lang_map = &en_strings;
    }
    Serial.printf("Loaded preferences: Theme='%s', Language='%s', Uploads=%lu, Downloads=%lu\n",
                  currentTheme.c_str(), currentLanguage.c_str(), totalUploads, totalDownloads);
}

void savePreferences() {
    preferences.begin("vft-server", false); // Open preferences in read-write mode
    preferences.putString("theme", currentTheme);
    preferences.putString("language", currentLanguage);
    preferences.putULong("total_uploads", totalUploads);
    preferences.putULong("total_downloads", totalDownloads);

    // Save file download counts (simplified: store top 5 as string)
    String downloadCountsJson = "";
    std::vector<std::pair<String, unsigned int>> sortedCounts;
    for (auto const& [filename, count] : fileDownloadCounts) {
        sortedCounts.push_back({filename, count});
    }
    // Sort in descending order by count
    std::sort(sortedCounts.begin(), sortedCounts.end(), [](auto& left, auto& right) {
        return left.second > right.second;
    });

    for (size_t i = 0; i < std::min((size_t)5, sortedCounts.size()); ++i) { // Save top 5
        downloadCountsJson += sortedCounts[i].first + ":" + String(sortedCounts[i].second);
        if (i < std::min((size_t)5, sortedCounts.size()) - 1) {
            downloadCountsJson += ",";
        }
    }
    preferences.putString("dl_counts", downloadCountsJson);

    preferences.end();
    Serial.printf("Saved preferences: Theme='%s', Language='%s', Uploads=%lu, Downloads=%lu\n",
                  currentTheme.c_str(), currentLanguage.c_str(), totalUploads, totalDownloads);
}

// =======================================================================
//    SETUP & LOOP
// =======================================================================

void setup() {
  Serial.begin(115200);

  // --- Load Preferences ---
  loadPreferences();

  // --- Initialize Display ---
  initDisplay(); // This handles displaying your custom logo first

  // --- Initialize Physical Buttons ---
  pinMode(BTN_UP_PIN, INPUT_PULLUP);
  pinMode(BTN_DOWN_PIN, INPUT_PULLUP);
  pinMode(BTN_SELECT_PIN, INPUT_PULLUP);

  // --- Initialize WiFi ---
  Serial.print("Starting AP...");
  WiFi.softAP(ssid, password);
  IPAddress ip = WiFi.softAPIP();
  Serial.print("AP IP: ");
  Serial.println(ip);

  // Display IP address on OLED
  display.clearDisplay(); // Clear your logo before showing network info
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.println(getOLEDString("AP_ACTIVE"));
  display.print("SSID: ");
  display.println(ssid);
  display.print("Pass: ");
  display.println(password);
  display.print("IP: ");
  display.println(ip);
  display.display();
  delay(3000); // Show IP for a few seconds

  // After showing network info, transition to idle screen
  displayIdleScreen(); // New addition to show idle screen

  // --- Initialize SD Card ---
  if (!SD.begin(SD_CS_PIN, spi)) {
    Serial.println("SD Card initialization failed!");
    displayOperation(getOLEDString("SD_INIT_FAIL")); // OLED status
    while(true) { delay(100); } // Halt execution if SD card fails
  }
  Serial.println("SD card initialized.");

  // --- Setup Web Server Routes ---
  server.on("/", HTTP_GET, handleRoot);
  server.on("/upload", HTTP_POST, []() {
    server.send(200, "text/plain", "OK"); // Send a 200 OK immediately
  }, handleFileUpload);
  server.on("/delete", HTTP_GET, handleDelete);
  server.on("/rename", HTTP_POST, handleRename);
  server.on("/mkdir", HTTP_POST, handleMkdir);
  server.on("/format", HTTP_GET, handleFormatSD);
  server.on("/set_broadcast", HTTP_GET, handleSetBroadcast); // New: Set broadcast file
  server.on("/broadcast_download", HTTP_GET, handleBroadcastDownload); // New: Serve broadcast file
  server.on("/create_user_web", HTTP_POST, handleCreateUser); // New: Web-based user creation
  server.on("/login_web", HTTP_POST, handleLogin); // New: Web-based user login
  server.on("/logout", HTTP_GET, handleLogout); // New: Web-based logout

  // This handler must be last, it's a catch-all for file requests (downloads)
  server.onNotFound([]() {
    if (isServerLocked) {
        server.send(401, "text/plain", "Unauthorized: Server Locked. Unlock via OLED.");
        return;
    }
    lastActivityTime = millis(); // Reset activity timer

    // If the requested URI is a file, handle it as a download/view
    String requestedPath = server.uri();
    if (SD.exists(requestedPath) && !SD.open(requestedPath).isDirectory()) {
      handleFileRequest(requestedPath);
    } else {
      server.send(404, "text/plain", "Not Found");
    }
  });

  server.begin();
  Serial.println("Server started");

  // Initial OLED state based on security. If server is locked, display PIN entry.
  // Otherwise, it stays on the idle screen which was set above.
  if (isServerLocked) {
      displayPINEntryScreen();
  } else {
      // If server is not locked, it's already on the idle screen from displayIdleScreen() above.
      lastActivityTime = millis(); // Start activity timer if server is unlocked
  }
}

void loop() {
  server.handleClient(); // Process web requests
  handleOLEDButtons();   // Process physical button presses

  // --- Power Management Logic ---
  if (isAPActive) { // Only check for inactivity if AP is currently active
      if (isServerLocked) {
          // If server is locked, always display PIN entry screen.
          // This overrides other states to enforce security.
          if (currentOLEDState != SCREEN_AUTH_PIN) {
              displayPINEntryScreen();
          }
      } else { // Server is unlocked
          // Check for auto-lock due to inactivity
          if (millis() - lastActivityTime > LOCK_TIMEOUT_MS) {
              isServerLocked = true;
              enteredPIN = ""; // Clear entered PIN
              pinDigitIndex = 0;
              currentDigitValue = 0;
              Serial.println(getOLEDString("SERVER_LOCKED_INACTIVE"));
              displayOperation(getOLEDString("SERVER_LOCKED_INACTIVE")); // Show message briefly
              delay(1000);
              displayPINEntryScreen(); // Go to PIN entry screen
          }
          // Check for auto-sleep due to inactivity
          else if (millis() - lastActivityTime > IDLE_SLEEP_TIMEOUT_MS) {
              enterSleepMode(); // Go to light sleep (AP disconnect)
          }
          // Check for automatic transition between idle/file tree based on client connection
          else {
              int connectedClients = WiFi.softAPgetStationNum();
              // Only change state if not in a transient state (e.g., upload, confirm, settings, QR, analytics, user info, user input)
              if (currentOLEDState != SCREEN_UPLOAD_PROGRESS &&
                  currentOLEDState != SCREEN_CONFIRM_DELETE &&
                  currentOLEDState != SCREEN_CONFIRM_FORMAT &&
                  currentOLEDState != SCREEN_FILE_PREVIEW &&
                  currentOLEDState != SCREEN_SETTINGS_MENU &&
                  currentOLEDState != SCREEN_THEME_SELECT &&
                  currentOLEDState != SCREEN_LANGUAGE_SELECT &&
                  currentOLEDState != SCREEN_QR_CODE &&
                  currentOLEDState != SCREEN_ANALYTICS &&
                  currentOLEDState != SCREEN_ACTIVE_UPLOADS &&
                  currentOLEDState != SCREEN_USER_INFO &&
                  currentOLEDState != SCREEN_USER_INPUT_CREATE && // Added new states
                  currentOLEDState != SCREEN_USER_INPUT_LOGIN)    // Added new states
              {
                  if (connectedClients > 0) {
                      // If clients connected and not already in file tree
                      if (currentOLEDState != SCREEN_FILE_TREE) {
                          populateOLEDFileList(currentPath); // Refresh list for consistency
                          displayFileTree();
                      }
                  } else { // No clients connected
                      // If no clients and not already in idle screen
                      if (currentOLEDState != SCREEN_IDLE) {
                          displayIdleScreen();
                      }
                  }
              }
          }
      }
  } else { // AP is inactive (in simulated sleep mode)
      // Buttons are handled in handleOLEDButtons, which will call wakeUpFromSleep()
      // if any button is pressed. No other action needed here.
  }
}
