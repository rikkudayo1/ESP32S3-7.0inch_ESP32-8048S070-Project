#include <Arduino_GFX_Library.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <time.h>

// WiFi passwords
const char* ssid = "";
const char* password = "";

// NTP Client setup for Thailand timezone
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7*3600, 60000);

// Display setup (pin configurations)
#define TFT_BL 2
#define TFT_DE    41
#define TFT_VSYNC 40
#define TFT_HSYNC 39
#define TFT_PCLK  42

#define TFT_R1 45
#define TFT_R2 48
#define TFT_R3 47
#define TFT_R4 21
#define TFT_R5 14

#define TFT_G0 1
#define TFT_G1 16
#define TFT_G2 8
#define TFT_G3 3
#define TFT_G4 46
#define TFT_G5 9

#define TFT_B1 4
#define TFT_B2 5
#define TFT_B3 6
#define TFT_B4 7
#define TFT_B5 15

Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(
  TFT_DE, TFT_VSYNC, TFT_HSYNC, TFT_PCLK,
  TFT_R1, TFT_R2, TFT_R3, TFT_R4, TFT_R5,
  TFT_G0, TFT_G1, TFT_G2, TFT_G3, TFT_G4, TFT_G5,
  TFT_B1, TFT_B2, TFT_B3, TFT_B4, TFT_B5,
  1, 40, 48, 40, 1, 13, 3, 32
);

Arduino_RGB_Display *gfx = new Arduino_RGB_Display(800, 480, rgbpanel);

// High Contrast Color Palette
#define COLOR_BACKGROUND 0x0000
#define COLOR_CARD_BG    0x2104
#define COLOR_CARD_LIGHT 0x4208
#define COLOR_CURRENT    0xFFE0
#define COLOR_NEXT       0x07FF
#define COLOR_TIME       0xF81F
#define COLOR_TEXT_WHITE 0xFFFF
#define COLOR_TEXT_BLACK 0x0000
#define COLOR_ACCENT     0xF800
#define COLOR_SUCCESS    0x07E0
#define COLOR_WARNING    0xFD20
#define COLOR_INACTIVE   0x8410
#define COLOR_HEADER     0x001F
#define COLOR_SHADOW     0x18C3

// Subject colors
#define COLOR_MATH       0x07FF
#define COLOR_SCIENCE    0x07E0
#define COLOR_ENGLISH    0xF81F
#define COLOR_TECH       0xFD20
#define COLOR_PHYSICS    0x001F
#define COLOR_HISTORY    0xF800
#define COLOR_CHEMISTRY  0xFFE0
#define COLOR_BIOLOGY    0x07E0
#define COLOR_LANGUAGE   0xF81F
#define COLOR_ART        0xFD20
#define COLOR_GEO        0x07E0
#define COLOR_PE         0xF800
#define COLOR_MUSIC      0xFFE0

// English day names
const char* englishDays[] = {
  "Sunday",
  "Monday",
  "Tuesday",
  "Wednesday",
  "Thursday",
  "Friday",
  "Saturday"
};

const char* englishMonths[] = {
  "January", "February", "March", "April",
  "May", "June", "July", "August",
  "September", "October", "November", "December"
};

// Class schedule structure
struct ClassInfo {
  String subjectCode;
  const char* subjectName;
  const char* teacherName;
  int startHour;
  int startMinute;
  int endHour;
  int endMinute;
  uint16_t subjectColor;
};

// Weekly schedule
ClassInfo schedule[7][10];
int classCount[7] = {0, 0, 0, 0, 0, 0, 0};

// Anti-flicker variables
String lastDisplayHash = "";
unsigned long lastUpdate = 0;
bool displayNeedsUpdate = true;
bool isInitialized = false;

// Display regions for partial updates
struct DisplayRegion {
  int x, y, w, h;
  String lastContent;
  bool needsUpdate;
};

DisplayRegion headerRegion = {20, 20, 760, 80, "", true};
DisplayRegion currentClassRegion = {20, 120, 370, 150, "", true};
DisplayRegion nextClassRegion = {410, 120, 370, 150, "", true};
DisplayRegion scheduleRegion = {20, 290, 760, 170, "", true};

// Content tracking variables
String lastHeaderContent = "";
String lastCurrentClassContent = "";
String lastNextClassContent = "";
String lastScheduleContent = "";

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("=== English Class Schedule Display ===");
  
  // Initialize display
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  
  if (!gfx->begin()) {
    Serial.println("Display initialization FAILED!");
    while(1) delay(1000);
  }
  
  Serial.println("Display initialized successfully!");
  
  // Show loading screen
  showLoadingScreen();
  
  // Initialize schedule
  initializeSchedule();
  
  // Connect to WiFi
  connectToWiFi();
  
  // Initialize NTP
  timeClient.begin();
  timeClient.update();
  
  // Initial full screen draw
  gfx->fillScreen(COLOR_BACKGROUND);
  isInitialized = true;
  displayNeedsUpdate = true;
  
  Serial.println("System ready!");
}

void loop() {
  static unsigned long lastNTPUpdate = 0;
  static int lastMinute = -1;
  static unsigned long lastScreenUpdate = 0;
  
  // Update NTP every 5 minutes
  if (millis() - lastNTPUpdate > 300000) {
    lastNTPUpdate = millis();
    timeClient.update();
  }
  
  // Check if minute changed or force update
  time_t now = timeClient.getEpochTime();
  struct tm *timeinfo = localtime(&now);
  int currentMinute = timeinfo->tm_min;
  
  bool forceUpdate = (millis() - lastScreenUpdate > 30000);
  
  if (currentMinute != lastMinute || displayNeedsUpdate || forceUpdate) {
    lastMinute = currentMinute;
    lastScreenUpdate = millis();
    updateDisplay();
    displayNeedsUpdate = false;
  }
  
  delay(1000);
}

void initializeSchedule() {
  Serial.println("Initializing schedule...");
  
  // MONDAY
  schedule[1][0] = {"MATH01", "Mathematics", "Mr. Johnson", 8, 0, 9, 30, COLOR_MATH};
  schedule[1][1] = {"SCI01", "Science", "Ms. Davis", 9, 30, 11, 0, COLOR_SCIENCE};
  schedule[1][2] = {"ENG01", "English", "Mrs. Smith", 13, 0, 14, 30, COLOR_ENGLISH};
  schedule[1][3] = {"TECH01", "Technology", "Mr. Wilson", 14, 30, 16, 0, COLOR_TECH};
  classCount[1] = 4;
  
  // TUESDAY
  schedule[2][0] = {"MATH02", "Mathematics", "Mr. Johnson", 8, 0, 9, 30, COLOR_MATH};
  schedule[2][1] = {"PHYS01", "Physics", "Dr. Brown", 9, 30, 11, 0, COLOR_PHYSICS};
  schedule[2][2] = {"HIST01", "History", "Ms. Garcia", 13, 0, 14, 30, COLOR_HISTORY};
  classCount[2] = 3;
  
  // WEDNESDAY
  schedule[3][0] = {"CHEM01", "Chemistry", "Dr. Miller", 8, 0, 9, 30, COLOR_CHEMISTRY};
  schedule[3][1] = {"BIO01", "Biology", "Ms. Taylor", 9, 30, 11, 0, COLOR_BIOLOGY};
  schedule[3][2] = {"LANG01", "Language Arts", "Mrs. Anderson", 13, 0, 14, 30, COLOR_LANGUAGE};
  schedule[3][3] = {"ART01", "Art", "Mr. Clark", 14, 30, 16, 0, COLOR_ART};
  classCount[3] = 4;
  
  // THURSDAY
  schedule[4][0] = {"MATH03", "Mathematics", "Mr. Johnson", 8, 0, 9, 30, COLOR_MATH};
  schedule[4][1] = {"GEO01", "Geography", "Ms. Rodriguez", 9, 30, 11, 0, COLOR_GEO};
  schedule[4][2] = {"ENG02", "English", "Mrs. Smith", 13, 0, 14, 30, COLOR_ENGLISH};
  classCount[4] = 3;
  
  // FRIDAY
  schedule[5][0] = {"SCI02", "Science", "Ms. Davis", 8, 0, 9, 30, COLOR_SCIENCE};
  schedule[5][1] = {"PE01", "Physical Education", "Coach Thompson", 9, 30, 11, 0, COLOR_PE};
  schedule[5][2] = {"MUSIC01", "Music", "Ms. White", 13, 0, 14, 30, COLOR_MUSIC};
  classCount[5] = 3;
  
  Serial.println("Schedule initialized!");
}

void connectToWiFi() {
  Serial.println("Connecting to WiFi...");
  
  gfx->fillScreen(COLOR_BACKGROUND);
  drawHighContrastCard(200, 180, 400, 120, COLOR_CARD_BG, COLOR_TEXT_WHITE);
  
  gfx->setTextSize(2);
  gfx->setTextColor(COLOR_TEXT_WHITE);
  gfx->setCursor(280, 220);
  gfx->print("Connecting to WiFi...");
  
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    
    // Loading animation
    for(int i = 0; i < 3; i++) {
      uint16_t color = (attempts % 3 == i) ? COLOR_CURRENT : COLOR_INACTIVE;
      gfx->fillCircle(350 + i * 30, 260, 8, color);
    }
    
    attempts++;
  }
  
  gfx->fillRect(200, 240, 400, 40, COLOR_CARD_BG);
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    gfx->setTextSize(2);
    gfx->setTextColor(COLOR_SUCCESS);
    gfx->setCursor(320, 250);
    gfx->print("Connected!");
    delay(2000);
  } else {
    Serial.println("\nWiFi failed!");
    gfx->setTextSize(2);
    gfx->setTextColor(COLOR_ACCENT);
    gfx->setCursor(310, 250);
    gfx->print("Failed!");
    delay(2000);
  }
}

void showLoadingScreen() {
  gfx->fillScreen(COLOR_BACKGROUND);
  
  // Title card
  drawHighContrastCard(150, 80, 500, 100, COLOR_HEADER, COLOR_TEXT_WHITE);
  
  gfx->setTextSize(4);
  gfx->setTextColor(COLOR_TEXT_WHITE);
  gfx->setCursor(200, 110);
  gfx->print("Class Schedule");
  
  gfx->setTextSize(2);
  gfx->setTextColor(COLOR_CURRENT);
  gfx->setCursor(310, 150);
  gfx->print("Display System");
  
  // Loading animation
  drawHighContrastCard(200, 220, 400, 80, COLOR_CARD_BG, COLOR_TEXT_WHITE);
  
  gfx->setTextSize(3);
  gfx->setTextColor(COLOR_TEXT_WHITE);
  gfx->setCursor(290, 240);
  gfx->print("Loading...");
  
  // Progress bar
  drawHighContrastCard(200, 320, 400, 40, COLOR_CARD_BG, COLOR_TEXT_WHITE);
  
  for(int i = 0; i <= 100; i += 10) {
    gfx->fillRect(210, 325, 380, 30, COLOR_CARD_BG);
    gfx->fillRect(210, 335, 380, 10, COLOR_INACTIVE);
    
    int fillWidth = (380 * i) / 100;
    gfx->fillRect(210, 335, fillWidth, 10, COLOR_CURRENT);
    
    gfx->setTextSize(2);
    gfx->setTextColor(COLOR_TEXT_WHITE);
    gfx->setCursor(380, 325);
    gfx->printf("%d%%", i);
    
    delay(100);
  }
  
  gfx->fillRect(210, 335, 380, 10, COLOR_SUCCESS);
  delay(500);
}

void drawHighContrastCard(int x, int y, int w, int h, uint16_t bgColor, uint16_t textColor) {
  gfx->fillRect(x - 2, y - 2, w + 4, h + 4, COLOR_TEXT_WHITE);
  gfx->fillRect(x, y, w, h, bgColor);
}

void updateDisplay() {
  if (!isInitialized) return;
  
  time_t now = timeClient.getEpochTime();
  struct tm *timeinfo = localtime(&now);
  
  int currentDay = timeinfo->tm_wday;
  int currentHour = timeinfo->tm_hour;
  int currentMinute = timeinfo->tm_min;
  
  String currentHash = String(currentDay) + ":" + String(currentHour) + ":" + String(currentMinute);
  
  if (currentHash != lastDisplayHash) {
    lastDisplayHash = currentHash;
    
    updateHeader(currentDay, currentHour, currentMinute, timeinfo);
    
    ClassInfo currentClass = findCurrentClass(currentDay, currentHour, currentMinute);
    ClassInfo nextClass = findNextClass(currentDay, currentHour, currentMinute);
    
    updateCurrentClassCard(currentClass);
    updateNextClassCard(nextClass);
    updateTodaySchedule(currentDay, currentHour, currentMinute);
  }
}

void updateHeader(int day, int hour, int minute, struct tm *timeinfo) {
  String headerContent = String(day) + ":" + String(hour) + ":" + String(minute) + ":" + String(timeinfo->tm_mday);
  
  if (headerContent != lastHeaderContent) {
    lastHeaderContent = headerContent;
    
    gfx->fillRect(headerRegion.x, headerRegion.y, headerRegion.w, headerRegion.h, COLOR_BACKGROUND);
    drawHighContrastCard(headerRegion.x, headerRegion.y, headerRegion.w, headerRegion.h, COLOR_HEADER, COLOR_TEXT_WHITE);
    
    // Day name
    gfx->setTextSize(3);
    gfx->setTextColor(COLOR_TEXT_WHITE);
    gfx->setCursor(40, 40);
    gfx->print(englishDays[day]);
    
    // Date
    gfx->setTextSize(2);
    gfx->setTextColor(COLOR_TEXT_WHITE);
    gfx->setCursor(40, 70);
    gfx->printf("%s %d, %d", englishMonths[timeinfo->tm_mon], 
                timeinfo->tm_mday, 
                timeinfo->tm_year + 1900);
    
    // Time
    gfx->setTextSize(5);
    gfx->setTextColor(COLOR_CURRENT);
    gfx->setCursor(580, 35);
    gfx->printf("%02d:%02d", hour, minute);
  }
}

ClassInfo findCurrentClass(int day, int hour, int minute) {
  ClassInfo noClass = {"", "No Current Class", "", 0, 0, 0, 0, COLOR_INACTIVE};
  
  if (day == 0 || day == 6) return noClass;
  
  int currentTime = hour * 60 + minute;
  
  for (int i = 0; i < classCount[day]; i++) {
    int startTime = schedule[day][i].startHour * 60 + schedule[day][i].startMinute;
    int endTime = schedule[day][i].endHour * 60 + schedule[day][i].endMinute;
    
    if (currentTime >= startTime && currentTime < endTime) {
      return schedule[day][i];
    }
  }
  
  return noClass;
}

ClassInfo findNextClass(int day, int hour, int minute) {
  ClassInfo noClass = {"", "No Next Class", "", 0, 0, 0, 0, COLOR_INACTIVE};
  
  if (day == 0 || day == 6) return noClass;
  
  int currentTime = hour * 60 + minute;
  
  for (int i = 0; i < classCount[day]; i++) {
    int startTime = schedule[day][i].startHour * 60 + schedule[day][i].startMinute;
    
    if (currentTime < startTime) {
      return schedule[day][i];
    }
  }
  
  return noClass;
}

void updateCurrentClassCard(ClassInfo classInfo) {
  String currentContent = classInfo.subjectCode + ":" + String(classInfo.subjectName) + ":" + String(classInfo.teacherName);
  
  if (currentContent != lastCurrentClassContent) {
    lastCurrentClassContent = currentContent;
    
    gfx->fillRect(currentClassRegion.x, currentClassRegion.y, currentClassRegion.w, currentClassRegion.h, COLOR_BACKGROUND);
    drawHighContrastCard(currentClassRegion.x, currentClassRegion.y, currentClassRegion.w, currentClassRegion.h, classInfo.subjectColor, COLOR_TEXT_WHITE);
    
    // Header
    gfx->setTextSize(2);
    gfx->setTextColor(COLOR_TEXT_WHITE);
    gfx->setCursor(35, 135);
    gfx->print("Current Class");
    
    if (classInfo.subjectCode != "") {
      gfx->setTextSize(3);
      gfx->setTextColor(COLOR_TEXT_WHITE);
      gfx->setCursor(35, 165);
      gfx->println(classInfo.subjectCode);
      
      gfx->setTextSize(2);
      gfx->setTextColor(COLOR_TEXT_WHITE);
      gfx->setCursor(35, 195);
      gfx->println(classInfo.subjectName);
      
      gfx->setCursor(35, 220);
      gfx->println(classInfo.teacherName);
      
      gfx->setTextSize(2);
      gfx->setTextColor(COLOR_CURRENT);
      gfx->setCursor(35, 245);
      gfx->printf("%02d:%02d - %02d:%02d", 
                  classInfo.startHour, classInfo.startMinute,
                  classInfo.endHour, classInfo.endMinute);
    } else {
      gfx->setTextSize(2);
      gfx->setTextColor(COLOR_TEXT_WHITE);
      gfx->setCursor(35, 195);
      gfx->print(classInfo.subjectName);
    }
  }
}

void updateNextClassCard(ClassInfo classInfo) {
  String nextContent = classInfo.subjectCode + ":" + String(classInfo.subjectName) + ":" + String(classInfo.teacherName);
  
  if (nextContent != lastNextClassContent) {
    lastNextClassContent = nextContent;
    
    gfx->fillRect(nextClassRegion.x, nextClassRegion.y, nextClassRegion.w, nextClassRegion.h, COLOR_BACKGROUND);
    drawHighContrastCard(nextClassRegion.x, nextClassRegion.y, nextClassRegion.w, nextClassRegion.h, classInfo.subjectColor, COLOR_TEXT_WHITE);
    
    gfx->setTextSize(2);
    gfx->setTextColor(COLOR_TEXT_WHITE);
    gfx->setCursor(425, 135);
    gfx->print("Next Class");
    
    if (classInfo.subjectCode != "") {
      gfx->setTextSize(3);
      gfx->setTextColor(COLOR_TEXT_WHITE);
      gfx->setCursor(425, 165);
      gfx->println(classInfo.subjectCode);
      
      gfx->setTextSize(2);
      gfx->setTextColor(COLOR_TEXT_WHITE);
      gfx->setCursor(425, 195);
      gfx->println(classInfo.subjectName);
      
      gfx->setCursor(425, 220);
      gfx->println(classInfo.teacherName);
      
      gfx->setTextSize(2);
      gfx->setTextColor(COLOR_CURRENT);
      gfx->setCursor(425, 245);
      gfx->printf("%02d:%02d - %02d:%02d", 
                  classInfo.startHour, classInfo.startMinute,
                  classInfo.endHour, classInfo.endMinute);
    } else {
      gfx->setTextSize(2);
      gfx->setTextColor(COLOR_TEXT_WHITE);
      gfx->setCursor(425, 195);
      gfx->print(classInfo.subjectName);
    }
  }
}

void updateTodaySchedule(int day, int hour, int minute) {
  String scheduleContent = String(day) + ":" + String(hour) + ":" + String(minute);
  for (int i = 0; i < classCount[day]; i++) {
    scheduleContent += ":" + schedule[day][i].subjectCode;
  }
  
  if (scheduleContent != lastScheduleContent) {
    lastScheduleContent = scheduleContent;
    
    gfx->fillRect(scheduleRegion.x, scheduleRegion.y, scheduleRegion.w, scheduleRegion.h, COLOR_BACKGROUND);
    drawHighContrastCard(scheduleRegion.x, scheduleRegion.y, scheduleRegion.w, scheduleRegion.h, COLOR_CARD_BG, COLOR_TEXT_WHITE);
    
    gfx->setTextSize(2);
    gfx->setTextColor(COLOR_TEXT_WHITE);
    gfx->setCursor(35, 310);
    gfx->print("Today's Schedule");
    
    if (day == 0 || day == 6) {
      gfx->setTextSize(3);
      gfx->setTextColor(COLOR_CURRENT);
      gfx->setCursor(320, 360);
      gfx->print("Weekend!");
      
      gfx->setTextSize(2);
      gfx->setTextColor(COLOR_TEXT_WHITE);
      gfx->setCursor(320, 390);
      gfx->print("No classes today");
      return;
    }
    
    int currentTime = hour * 60 + minute;
    
    for (int i = 0; i < classCount[day] && i < 5; i++) {
      int startTime = schedule[day][i].startHour * 60 + schedule[day][i].startMinute;
      int endTime = schedule[day][i].endHour * 60 + schedule[day][i].endMinute;
      
      uint16_t textColor = COLOR_INACTIVE;
      String status = "";
      
      if (currentTime >= startTime && currentTime < endTime) {
        textColor = COLOR_CURRENT;
        status = ">>> ";
      } else if (currentTime < startTime) {
        textColor = COLOR_NEXT;
        status = "    ";
      } else {
        textColor = COLOR_SUCCESS;
        status = "✓   ";
      }
      
      gfx->setTextSize(2);
      gfx->setTextColor(textColor);
      gfx->setCursor(35, 340 + i * 25);
      gfx->printf("%s%02d:%02d-%02d:%02d %s - %s", 
                status.c_str(),
                schedule[day][i].startHour, schedule[day][i].startMinute,
                schedule[day][i].endHour, schedule[day][i].endMinute,
                schedule[day][i].subjectCode.c_str(),
                schedule[day][i].subjectName);
    }
  }
}
