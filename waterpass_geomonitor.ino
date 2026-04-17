#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#define SDA_PIN D2
#define SCL_PIN D1
#define BUZZER_PIN D6
#define ZERO_BUTTON D5

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_MPU6050 mpu;

bool mpu_ok = false;
bool oled_ok = false;
bool buzzer_ok = false;

unsigned long lastOLED = 0;
const unsigned long OLED_INTERVAL_MS = 350;

static const float WARNING_DEG = 0.12f;
static const float DANGER_DEG  = 0.29f;

#define STATE_NORMAL   0
#define STATE_WARNING  1
#define STATE_DANGER   2
#define STATE_UNSTABLE 3

uint8_t tilt_state = STATE_NORMAL;

float zeroOffset = 0.0f;
bool zeroJustSet = false;
unsigned long zeroMsgTime = 0;

unsigned long stateChangeTime = 0;
const unsigned long STATE_LOCK_MS = 300;

// =========================
static inline float radToDeg(float r) {
  return r * 180.0f / 3.14159265f;
}

static inline float degToMmPerM(float deg) {
  return tan(deg * 3.14159265f / 180.0f) * 1000.0f;
}

float estimateTilt(float ax, float ay, float az) {

  float roll  = atan2(ay, az);
  float pitch = atan2(-ax, sqrt(ay * ay + az * az));

  float rollDeg  = radToDeg(roll);
  float pitchDeg = radToDeg(pitch);

  float tilt = sqrt(rollDeg * rollDeg + pitchDeg * pitchDeg);

  if (tilt < 0.05f) tilt = 0.0f;

  return tilt;
}

// =========================
// BUZZER STABLE
// =========================
void buzzerUpdate(uint8_t state) {

  static uint8_t lastState = 255;
  static unsigned long lastToggle = 0;
  static bool buzzerOn = false;

  if (state != lastState) {
    lastState = state;
    lastToggle = millis();
    buzzerOn = false;
  }

  if (state == STATE_NORMAL || state == STATE_UNSTABLE) {
    digitalWrite(BUZZER_PIN, HIGH);
    return;
  }

  if (state == STATE_DANGER) {
    digitalWrite(BUZZER_PIN, LOW);
    return;
  }

  if (state == STATE_WARNING) {
    if (millis() - lastToggle > 600) {
      lastToggle = millis();
      buzzerOn = !buzzerOn;
      digitalWrite(BUZZER_PIN, buzzerOn ? LOW : HIGH);
    }
  }
}

// =========================
// OLED UI
// =========================
void renderUI(float tiltDeg, float slopeMm, float ax, float ay, float az, bool unstable) {

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // Header
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("GeoMonitor Waterpass");

  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  // Status Row
  display.setCursor(0, 12);
  display.print("MPU:");
  display.print(mpu_ok ? "OK " : "ER ");
  display.print("OLED:");
  display.print(oled_ok ? "OK " : "ER ");
  display.print("BZ:");
  display.print(buzzer_ok ? "OK" : "ER");

  // Tilt Display (besar)
  display.setTextSize(2);
  display.setCursor(0, 22);
  display.print(tiltDeg, 2);
  display.print((char)247);

  display.setTextSize(1);

  // Slope
  display.setCursor(0, 46);
  display.print("Slope:");
  display.print(slopeMm, 1);
  display.print("mm/m");

  // Status kanan
  display.setCursor(95, 46);

  if (zeroJustSet) display.print("ZERO");
  else if (unstable) display.print("UNS");
  else if (tilt_state == STATE_NORMAL) display.print("OK ");
  else if (tilt_state == STATE_WARNING) display.print("WRN");
  else display.print("DNG");

  // AX AY AZ tetap
  display.setCursor(0, 56);
  display.print("AX:");
  display.print(ax, 2);
  display.print(" AY:");
  display.print(ay, 2);

  display.display();
}

// =========================
void setup() {

  Serial.begin(115200);

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, HIGH);
  buzzer_ok = true;

  pinMode(ZERO_BUTTON, INPUT_PULLUP);

  oled_ok = display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  mpu_ok  = mpu.begin();
}

// =========================
void loop() {

  sensors_event_t a, g, t;
  mpu.getEvent(&a, &g, &t);

  float tiltRaw = estimateTilt(
    a.acceleration.x,
    a.acceleration.y,
    a.acceleration.z
  );

  static float tiltSmooth = 0;
  tiltSmooth = 0.97f * tiltSmooth + 0.03f * tiltRaw;

  float tiltDeg = tiltSmooth - zeroOffset;
  float slopeMm = degToMmPerM(tiltDeg);

  // ===== ZERO CALIBRATION (AVERAGING)
  static bool lastBtnState = HIGH;
  bool btnNow = digitalRead(ZERO_BUTTON);

  if (lastBtnState == HIGH && btnNow == LOW) {

    delay(50);

    if (digitalRead(ZERO_BUTTON) == LOW) {

      float sum = 0;
      for (int i = 0; i < 20; i++) {
        sensors_event_t a2, g2, t2;
        mpu.getEvent(&a2, &g2, &t2);
        sum += estimateTilt(
          a2.acceleration.x,
          a2.acceleration.y,
          a2.acceleration.z
        );
        delay(20);
      }

      zeroOffset = sum / 20.0f;

      zeroJustSet = true;
      zeroMsgTime = millis();
    }
  }

  lastBtnState = btnNow;

  if (zeroJustSet && millis() - zeroMsgTime > 1000)
    zeroJustSet = false;

  // ===== UNSTABLE detection
  float gmag = sqrt(
    a.acceleration.x * a.acceleration.x +
    a.acceleration.y * a.acceleration.y +
    a.acceleration.z * a.acceleration.z
  );

  bool unstable = (abs(gmag - 9.81f) > 2.0f);

  // ===== STATE DECISION dengan lock
  if (!unstable && millis() - stateChangeTime > STATE_LOCK_MS) {

    uint8_t newState;

    if (tiltDeg <= WARNING_DEG) newState = STATE_NORMAL;
    else if (tiltDeg <= DANGER_DEG) newState = STATE_WARNING;
    else newState = STATE_DANGER;

    if (newState != tilt_state) {
      tilt_state = newState;
      stateChangeTime = millis();
    }
  }

  buzzerUpdate(unstable ? STATE_UNSTABLE : tilt_state);

  if (millis() - lastOLED >= OLED_INTERVAL_MS) {
    lastOLED = millis();
    renderUI(
      tiltDeg,
      slopeMm,
      a.acceleration.x,
      a.acceleration.y,
      a.acceleration.z,
      unstable
    );
  }
}