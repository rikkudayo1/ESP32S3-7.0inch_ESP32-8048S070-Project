#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <lvgl.h>

// ===================== Display config =====================
#define TFT_BL 2
#define TFT_DE 41
#define TFT_VSYNC 40
#define TFT_HSYNC 39
#define TFT_PCLK 42
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
    1, 40, 48, 40, 1, 13, 3, 32);

Arduino_RGB_Display *gfx = new Arduino_RGB_Display(800, 480, rgbpanel);

// ===================== LVGL Buffer =====================
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[800 * 40];
static lv_disp_drv_t disp_drv;

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);
  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)color_p, w, h);
  lv_disp_flush_ready(disp);
}

// ===================== WiFi & NTP =====================
const char *ssid = "YOUR WIFI NAME";
const char *password = "YOUR WIFI PASSWORD";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7 * 3600, 60000); // UTC+7

// ===================== Thai Font =====================
#include "noto_sans_thai_28.c" // LVGL v8 font file

// ===================== Data =====================
const char *thaiDays[] = {
    "วันอาทิตย์", "วันจันทร์", "วันอังคาร", "วันพุธ",
    "วันพฤหัสบดี", "วันศุกร์", "วันเสาร์"};

const char *periodTimes[9] = {
    "08:00 - 08:50",
    "08:50 - 09:40",
    "09:40 - 10:30",
    "10:30 - 11:20",
    "11:20 - 12:10",
    "12:10 - 13:00",
    "13:00 - 13:50",
    "13:50 - 14:40",
    "14:40 - 15:30"};

struct Subject
{
  const char *name;
  const char *teacher;
};

// Schedule for each day
Subject schedule[7][9] = {
    {{"ไม่มีเรียน", ""}, {"ไม่มีเรียน", ""}, {"ไม่มีเรียน", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""}},
    {{"คณิต", "ครูเอก"}, {"วิทย์", "ครูบี"}, {"อังกฤษ", "ครูซี"}, {"ศิลปะ", "ครูดี"}, {"คอม", "ครูเอฟ"}, {"ไทย", "ครูจี"}, {"สังคม", "ครูเอช"}, {"พลศึกษา", "ครูไอ"}, {"ดนตรี", "ครูเจ"}},
    {{"ไทย", "ครูเอ"}, {"ประวัติ", "ครูบี"}, {"คอม", "ครูซี"}, {"สุขศึกษา", "ครูดี"}, {"วิทย์", "ครูอี"}, {"ศิลปะ", "ครูจี"}, {"อังกฤษ", "ครูแฮ"}, {"คณิต", "ครูเค"}, {"สังคม", "ครูดี"}},
    {{"วิทย์", "ครูบี"}, {"คณิต", "ครูเอ"}, {"อังกฤษ", "ครูซี"}, {"ดนตรี", "ครูดี"}, {"พลศึกษา", "ครูอี"}, {"ศิลปะ", "ครูจี"}, {"คอม", "ครูเอช"}, {"ไทย", "ครูไอ"}, {"สังคม", "ครูเจ"}},
    {{"ศิลปะ", "ครูเอ"}, {"ไทย", "ครูบี"}, {"วิทย์", "ครูซี"}, {"พลศึกษา", "ครูดี"}, {"คณิต", "ครูอี"}, {"อังกฤษ", "ครูจี"}, {"สุขศึกษา", "ครูเอช"}, {"คอม", "ครูไอ"}, {"ดนตรี", "ครูเจ"}},
    {{"คณิต", "ครูเอ"}, {"สังคม", "ครูบี"}, {"อังกฤษ", "ครูซี"}, {"คอม", "ครูดี"}, {"พลศึกษา", "ครูอี"}, {"ไทย", "ครูจี"}, {"วิทย์", "ครูเอช"}, {"ศิลปะ", "ครูไอ"}, {"ดนตรี", "ครูเจ"}},
    {{"ไม่มีเรียน", ""}, {"ไม่มีเรียน", ""}, {"ไม่มีเรียน", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""}},
};

// Background colors per day
lv_color_t dayColors[] = {
    lv_color_hex(0xFFCCCC), // Sunday - red
    lv_color_hex(0xFFFACD), // Monday - yellow
    lv_color_hex(0xFFD6EB), // Tuesday - pink
    lv_color_hex(0xCCFFCC), // Wednesday - green
    lv_color_hex(0xFFD580), // Thursday - orange
    lv_color_hex(0xCCE5FF), // Friday - blue
    lv_color_hex(0xE6CCFF)  // Saturday - purple
};

// ===================== UI =====================
lv_obj_t *label_header, *label_time;
lv_obj_t *periodBoxes[9];
lv_style_t thai_style;

// ===================== Helpers =====================
// Parse "HH:MM" into minutes
int timeToMinutes(const char *t)
{
  int h = atoi(t);
  int m = atoi(t + 3);
  return h * 60 + m;
}

// Determine which period is active
int getCurrentPeriod()
{
  int h = timeClient.getHours();
  int m = timeClient.getMinutes();
  int now = h * 60 + m;

  for (int i = 0; i < 9; i++)
  {
    int start = timeToMinutes(periodTimes[i]);
    int end = timeToMinutes(periodTimes[i] + 8); // end time after '-'
    if (now >= start && now < end)
      return i;
  }
  return -1;
}

// ===================== UI Setup =====================
void setup_lvgl_ui(int day)
{
  lv_obj_clean(lv_scr_act());
  lv_obj_t *scr = lv_scr_act();

  lv_style_init(&thai_style);
  lv_style_set_text_font(&thai_style, &noto_sans_thai_28);

  // Background color
  lv_obj_set_style_bg_color(scr, dayColors[day], 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

  // Header
  label_header = lv_label_create(scr);
  lv_obj_add_style(label_header, &thai_style, 0);
  String headerText = "ตารางเรียน " + String(thaiDays[day]);
  lv_label_set_text(label_header, headerText.c_str());
  lv_obj_align(label_header, LV_ALIGN_TOP_MID, 0, 10);

  // Time
  label_time = lv_label_create(scr);
  lv_obj_add_style(label_time, &thai_style, 0);
  lv_label_set_text(label_time, "--:--");
  lv_obj_align(label_time, LV_ALIGN_TOP_RIGHT, -30, 10);

  // Period rows
  int startY = 70;
  for (int i = 0; i < 9; i++)
  {
    lv_obj_t *cont = lv_obj_create(scr);
    lv_obj_set_size(cont, 740, 38);
    lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, startY + i * 43);
    lv_obj_set_style_radius(cont, 12, 0);
    lv_obj_set_style_bg_color(cont, lv_color_hex(0xFFFFFF), 0);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *timeLbl = lv_label_create(cont);
    lv_obj_add_style(timeLbl, &thai_style, 0);
    lv_label_set_text(timeLbl, periodTimes[i]);
    lv_obj_align(timeLbl, LV_ALIGN_LEFT_MID, 10, 0);

    String subjText = String(schedule[day][i].name);
    if (strlen(schedule[day][i].teacher) > 0)
      subjText += " (" + String(schedule[day][i].teacher) + ")";

    lv_obj_t *subjLbl = lv_label_create(cont);
    lv_obj_add_style(subjLbl, &thai_style, 0);
    lv_label_set_text(subjLbl, subjText.c_str());
    lv_obj_align(subjLbl, LV_ALIGN_LEFT_MID, 250, 0);

    periodBoxes[i] = cont;
  }
}

// ===================== Setup =====================
void setup()
{
  Serial.begin(115200);
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  gfx->begin();
  gfx->fillScreen(BLACK);

  lv_init();
  lv_disp_draw_buf_init(&draw_buf, buf1, NULL, 800 * 40);
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = 800;
  disp_drv.ver_res = 480;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");

  Serial.println("Getting time from NTP...");
  while (!timeClient.update())
  {
    timeClient.forceUpdate();
    delay(500);
  }
  Serial.println("Time synchronized!");

  int d = timeClient.getDay();
  if (d < 0 || d > 6)
    d = 0;
  setup_lvgl_ui(d);
}

// ===================== Loop =====================
void loop()
{
  static unsigned long lastUpdate = 0;
  static int currentDay = -1;
  static int highlighted = -1;

  lv_timer_handler();
  delay(5);

  if (millis() - lastUpdate > 1000)
  {
    timeClient.update();
    int h = timeClient.getHours();
    int m = timeClient.getMinutes();
    int d = timeClient.getDay();
    if (d < 0 || d > 6)
      d = 0;

    char buf[16];
    sprintf(buf, "%02d:%02d", h, m);
    lv_label_set_text(label_time, buf);

    if (d != currentDay)
    {
      currentDay = d;
      setup_lvgl_ui(d);
      highlighted = -1; // reset highlight
    }

    // Highlight current period
    int nowPeriod = getCurrentPeriod();
    if (nowPeriod != highlighted)
    {
      for (int i = 0; i < 9; i++)
      {
        lv_obj_set_style_bg_color(periodBoxes[i],
                                  (i == nowPeriod) ? lv_color_hex(0x99CCFF) : lv_color_hex(0xFFFFFF),
                                  0);
      }
      highlighted = nowPeriod;
    }

    lastUpdate = millis();
    Serial.printf("Time: %02d:%02d | Day: %d | Period: %d\n", h, m, d, highlighted);
  }
}
