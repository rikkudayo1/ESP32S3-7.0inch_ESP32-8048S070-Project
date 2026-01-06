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
static lv_color_t *buf1;
static lv_color_t *buf2;
static lv_disp_drv_t disp_drv;

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
  uint32_t w = area->x2 - area->x1 + 1;
  uint32_t h = area->y2 - area->y1 + 1;
  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)color_p, w, h);
  lv_disp_flush_ready(disp);
}

// ===================== WiFi & NTP =====================
const char *ssid = "Jurapipat_2.4G";
const char *password = "168020130";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7 * 3600, 60000);

// ===================== Thai Font =====================
#include "noto_sans_thai_28.c"

// ===================== Data =====================
const char *thaiDays[] = {
    "วันอาทิตย์", "วันจันทร์", "วันอังคาร",
    "วันพุธ", "วันพฤหัสบดี", "วันศุกร์", "วันเสาร์"};

const char *periodTimes[9] = {
    "08:30 - 09:20", "09:20 - 10:10", "10:10 - 11:00",
    "11:00 - 11:50", "11:50 - 12:40", "12:40 - 13:30",
    "13:30 - 14:20", "14:20 - 15:10", "15:10 - 16:00"};

struct Subject
{
  const char *name;
  const char *teacher;
};

Subject schedule[7][9] = {
    {{"ไม่มีเรียน",""},{"ไม่มีเรียน",""},{"ไม่มีเรียน",""},{"",""},{"",""},{"",""},{"",""},{"",""},{"",""}},
    {{"คณิต","ครูเอก"},{"วิทย์","ครูบี"},{"อังกฤษ","ครูซี"},{"ศิลปะ","ครูดี"},{"คอม","ครูเอฟ"},{"ไทย","ครูจี"},{"สังคม","ครูเอช"},{"พลศึกษา","ครูไอ"},{"ดนตรี","ครูเจ"}},
    {{"ไทย","ครูเอ"},{"ประวัติ","ครูบี"},{"คอม","ครูซี"},{"สุขศึกษา","ครูดี"},{"วิทย์","ครูอี"},{"ศิลปะ","ครูจี"},{"อังกฤษ","ครูแฮ"},{"คณิต","ครูเค"},{"สังคม","ครูดี"}},
    {{"วิทย์","ครูบี"},{"คณิต","ครูเอ"},{"อังกฤษ","ครูซี"},{"ดนตรี","ครูดี"},{"พลศึกษา","ครูอี"},{"ศิลปะ","ครูจี"},{"คอม","ครูเอช"},{"ไทย","ครูไอ"},{"สังคม","ครูเจ"}},
    {{"คณิตศาสตร์","ครูอากร"},{"สังคมศีกษา","ครูลักษณา"},{"IS","ครูณัษฐพง"},{"IS","ครูณัษฐพง"},{"พักกลางวัน",""},{"ฟิสิกส์","ครูสุวะนิด"},{"ฟิสิกส์","ครูสุวะนิด"},{"ภาษาไทย","ครูวิทยานัน"},{"ไม่มีเรียน",""}},
    {{"คณิต","ครูเอ"},{"สังคม","ครูบี"},{"อังกฤษ","ครูซี"},{"คอม","ครูดี"},{"พลศึกษา","ครูอี"},{"ไทย","ครูจี"},{"วิทย์","ครูเอช"},{"ศิลปะ","ครูไอ"},{"ดนตรี","ครูเจ"}},
    {{"ไม่มีเรียน",""},{"ไม่มีเรียน",""},{"ไม่มีเรียน",""},{"",""},{"",""},{"",""},{"",""},{"",""},{"",""}}
};

lv_color_t dayColors[] = {
    lv_color_hex(0xFFCCCC),
    lv_color_hex(0xFFFACD),
    lv_color_hex(0xFFD6EB),
    lv_color_hex(0xCCFFCC),
    lv_color_hex(0xFFD580),
    lv_color_hex(0xCCE5FF),
    lv_color_hex(0xE6CCFF)
};

// ===================== UI =====================
lv_obj_t *label_time;
lv_obj_t *periodBoxes[9];
lv_style_t thai_style;

// ===================== Helpers =====================
int timeToMinutes(const char *t)
{
  int h = atoi(t);
  int m = atoi(t + 3);
  return h * 60 + m;
}

int getCurrentPeriod()
{
  static int cachedHour = -1;
  static int cachedMinute = -1;
  static int cachedPeriod = -1;
  
  int currentHour = timeClient.getHours();
  int currentMinute = timeClient.getMinutes();
  
  // Return cached value if time hasn't changed
  if (currentHour == cachedHour && currentMinute == cachedMinute)
  {
    return cachedPeriod;
  }
  
  cachedHour = currentHour;
  cachedMinute = currentMinute;
  
  int now = currentHour * 60 + currentMinute;
  for (int i = 0; i < 9; i++)
  {
    int start = timeToMinutes(periodTimes[i]);
    int end = timeToMinutes(periodTimes[i] + 8);
    if (now >= start && now < end)
    {
      cachedPeriod = i;
      return i;
    }
  }
  cachedPeriod = -1;
  return -1;
}

// ===================== UI Setup =====================
void setup_lvgl_ui(int day)
{
  lv_obj_clean(lv_scr_act());
  lv_obj_t *scr = lv_scr_act();

  lv_style_init(&thai_style);
  lv_style_set_text_font(&thai_style, &noto_sans_thai_28);

  lv_obj_set_style_bg_color(scr, dayColors[day], LV_PART_MAIN);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *header = lv_label_create(scr);
  lv_obj_add_style(header, &thai_style, LV_PART_MAIN);
  lv_label_set_text_fmt(header, "ตารางเรียน %s ห้อง xxx", thaiDays[day]);
  lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 10);

  label_time = lv_label_create(scr);
  lv_obj_add_style(label_time, &thai_style, LV_PART_MAIN);
  lv_obj_set_width(label_time, 150);  // Fixed width
  lv_label_set_text(label_time, "00:00");  // Initial text
  lv_obj_align(label_time, LV_ALIGN_TOP_RIGHT, -30, 10);

  int y = 70;
  for (int i = 0; i < 9; i++)
  {
    lv_obj_t *cont = lv_obj_create(scr);
    lv_obj_set_size(cont, 740, 38);
    lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, y + i * 43);
    lv_obj_set_style_radius(cont, 12, LV_PART_MAIN);
    lv_obj_set_style_bg_color(cont, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *t = lv_label_create(cont);
    lv_obj_add_style(t, &thai_style, LV_PART_MAIN);
    lv_label_set_text(t, periodTimes[i]);
    lv_obj_align(t, LV_ALIGN_LEFT_MID, 10, 0);

    lv_obj_t *s = lv_label_create(cont);
    lv_obj_add_style(s, &thai_style, LV_PART_MAIN);
    lv_label_set_text_fmt(s, "%s %s", schedule[day][i].name, schedule[day][i].teacher);
    lv_obj_align(s, LV_ALIGN_LEFT_MID, 250, 0);

    periodBoxes[i] = cont;
  }
}

int currentDay = -1;
// ===================== Setup =====================
void setup()
{
  Serial.begin(115200);

  Serial.print("PSRAM found: ");
  Serial.println(psramFound());
  Serial.print("PSRAM size: ");
  Serial.println(ESP.getPsramSize());

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  gfx->begin();
  gfx->fillScreen(BLACK);

  lv_init();

  buf1 = (lv_color_t *)ps_malloc(sizeof(lv_color_t) * 800 * 240);
  buf2 = (lv_color_t *)ps_malloc(sizeof(lv_color_t) * 800 * 240);

  if (!buf1 || !buf2) {
    Serial.println("❌ PSRAM allocation failed!");
    while (1);
  }
  lv_disp_draw_buf_init(&draw_buf, buf1, buf2, 800 * 240);
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = 800;
  disp_drv.ver_res = 480;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  // ❗ Disable LVGL default theme (VERY IMPORTANT)
  lv_disp_set_theme(lv_disp_get_default(), NULL);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(300);

  while (!timeClient.forceUpdate())
  {
    delay(200);
  }
  currentDay = timeClient.getDay();
  setup_lvgl_ui(currentDay);

  // Initialize time display to avoid first update
  lv_label_set_text_fmt(label_time, "%02d:%02d",
                        timeClient.getHours(),
                        timeClient.getMinutes());
}

// ===================== Loop =====================
void loop()
{
  static uint32_t lastTick = 0;
  static uint32_t lastUpdate = 0;
  static uint32_t lastNtpSync = 0;
  static uint32_t lastSecondUpdate = 0;
  static int highlighted = -1;
  static int lastHour = -1;
  static int lastMinute = -1;
  static int lastSecond = -1;
  static int localHour = 0;
  static int localMinute = 0;
  static int localSecond = 0;

  uint32_t now = millis();
  lv_tick_inc(now - lastTick);
  lastTick = now;

  lv_timer_handler();

  if (millis() - lastNtpSync >= 300000 || lastNtpSync == 0)
  {
    timeClient.update();
    localHour = timeClient.getHours();
    localMinute = timeClient.getMinutes();
    localSecond = timeClient.getSeconds();
    lastNtpSync = millis();
    lastSecondUpdate = millis();
  }

  if (millis() - lastSecondUpdate >= 1000)
  {
    localSecond++;
    if (localSecond >= 60)
    {
      localSecond = 0;
      localMinute++;
      if (localMinute >= 60)
      {
        localMinute = 0;
        localHour++;
        if (localHour >= 24)
        {
          localHour = 0;
        }
      }
    }
    lastSecondUpdate = millis();
  }

  if (millis() - lastUpdate >= 1000)
  {
     int today = timeClient.getDay();
    if (today != currentDay)
    {
      currentDay = today;
      setup_lvgl_ui(currentDay);
    }
    if (localHour != lastHour || localMinute != lastMinute)
    {
      static char timeStr[6];
      snprintf(timeStr, sizeof(timeStr), "%02d:%02d", localHour, localMinute);
      lv_label_set_text(label_time, timeStr);
      lastHour = localHour;
      lastMinute = localMinute;
    }

    int p = getCurrentPeriod();
    if (p != highlighted)
    {
      if (highlighted >= 0 && highlighted < 9)
      {
        lv_obj_set_style_bg_color(periodBoxes[highlighted], 
                                  lv_color_hex(0xFFFFFF), 
                                  LV_PART_MAIN);
      }
      
      if (p >= 0 && p < 9)
      {
        lv_obj_set_style_bg_color(periodBoxes[p], 
                                  lv_color_hex(0x99CCFF), 
                                  LV_PART_MAIN);
      }
      
      highlighted = p;
    }

    lastUpdate = millis();
  }
}
