#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "HX711.h"

// ---------- OLED ----------
#define SDA_PIN 21
#define SCL_PIN 22
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
bool oledOK = false;

// ---------- RFID ----------
#define SS_PIN   5
#define RST_PIN  27

// ---------- Servo, LEDs, Buzzer ----------
#define SERVO1_PIN    14
#define SERVO2_PIN    16

#define GRANTED_LED   2
#define DENIED_LED    15
#define BUZZER_PIN    26
#define BOX_LIGHT_LED 12   // ✅ CHANGED TO GPIO 12

// ---------- Ultrasonic ----------
#define BOTTOM_TRIG_PIN 33
#define BOTTOM_ECHO_PIN 32
#define TOP_TRIG_PIN    17
#define TOP_ECHO_PIN    35
#define DETECT_DISTANCE 10

// ---------- Vibration ----------
#define VIBRATION_PIN 4

// ---------- HX711 ----------
#define LOADCELL_DOUT_PIN 34
#define LOADCELL_SCK_PIN  25

HX711 scale;
float calibration_factor = -7050;

float lastWeight = 0;
unsigned long weightDisplayTime = 0;
bool weightShowing = false;

// ---------- RFID UID ----------
byte allowedUID[] = {0x57, 0xB6, 0x68, 0x62};
byte uidLength = 4;

// ---------- Objects ----------
MFRC522 rfid(SS_PIN, RST_PIN);
Servo servo1;
Servo servo2;

// ---------- System ----------
bool lidOpen = false;
unsigned long lidOpenTime = 0;
const unsigned long AUTO_CLOSE_TIME = 50000;

// ---------- Vibration ----------
volatile bool tamperDetected = false;
unsigned long lastVibrationTime = 0;
const unsigned long VIBRATION_COOLDOWN = 2000;

// ---------- ISR ----------
void IRAM_ATTR vibrationTriggered() {
  if (!lidOpen) tamperDetected = true;
}

// ---------- Setup ----------
void setup() {

  Serial.begin(9600);

  Wire.begin(SDA_PIN, SCL_PIN);
  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    oledOK = true;
    display.setTextColor(SSD1306_WHITE);
  }

  SPI.begin();
  rfid.PCD_Init();

  servo1.attach(SERVO1_PIN, 500, 2400);
  servo2.attach(SERVO2_PIN, 500, 2400);

  servo1.writeMicroseconds(500);
  servo2.writeMicroseconds(500);

  pinMode(GRANTED_LED, OUTPUT);
  pinMode(DENIED_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BOX_LIGHT_LED, OUTPUT);

  digitalWrite(BOX_LIGHT_LED, LOW); // light off initially

  pinMode(BOTTOM_TRIG_PIN, OUTPUT);
  pinMode(BOTTOM_ECHO_PIN, INPUT);
  pinMode(TOP_TRIG_PIN, OUTPUT);
  pinMode(TOP_ECHO_PIN, INPUT);

  pinMode(VIBRATION_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(VIBRATION_PIN), vibrationTriggered, FALLING);

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(calibration_factor);
  scale.tare();

  showMessage("SMART BOX", "READY");
}

// ---------- Main Loop ----------
void loop() {

  checkTamperInterrupt();
  checkWeightChange();

  if (lidOpen && millis() - lidOpenTime >= AUTO_CLOSE_TIME) {
    autoClose();
  }

  if (!lidOpen && !weightShowing) {
    displayBoxStatus();
  }

  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  digitalWrite(GRANTED_LED, HIGH);
  digitalWrite(BUZZER_PIN, HIGH);
  delay(300);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(GRANTED_LED, LOW);

  if (checkUID()) {
    grantAccess();
  } else {
    accessDenied();
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  delay(800);
}

// ---------- Grant Access ----------
void grantAccess() {

  showMessage("OPENED BY", "DELIVERY BOY");

  if (!lidOpen) {
    lidOpen = true;
    lidOpenTime = millis();

    digitalWrite(BOX_LIGHT_LED, HIGH);   // ✅ Light ON

    slowOpen();
  } 
  else {
    lidOpen = false;

    digitalWrite(BOX_LIGHT_LED, LOW);    // ✅ Light OFF

    slowClose();
    showMessage("LID", "CLOSED");
  }
}

// ---------- Auto Close ----------
void autoClose() {

  lidOpen = false;

  digitalWrite(BOX_LIGHT_LED, LOW);   // ✅ OFF on auto close

  slowClose();
  showMessage("AUTO", "CLOSED");
}

// ---------- Weight Detection ----------
void checkWeightChange() {

  float currentWeight = scale.get_units(5);

  if (abs(currentWeight - lastWeight) > 0.2) {
    lastWeight = currentWeight;
    weightDisplayTime = millis();
    weightShowing = true;

    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(10, 5);
    display.print("PACKAGE ADDED");

    display.setTextSize(2);
    display.setCursor(15, 30);
    display.print(currentWeight, 2);
    display.print(" KG");
    display.display();
  }

  if (weightShowing && millis() - weightDisplayTime > 10000) {
    weightShowing = false;
  }
}

// ---------- Box Status ----------
void displayBoxStatus() {

  long bottomDist = getDistance(BOTTOM_TRIG_PIN, BOTTOM_ECHO_PIN);
  long topDist = getDistance(TOP_TRIG_PIN, TOP_ECHO_PIN);

  bool bottomDetected = bottomDist > 0 && bottomDist < DETECT_DISTANCE;
  bool topDetected = topDist > 0 && topDist < DETECT_DISTANCE;

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(10, 5);
  display.print("BOX STATUS");

  display.setTextSize(2);
  display.setCursor(25, 30);

  if (bottomDetected && topDetected) display.print("100%");
  else if (bottomDetected || topDetected) display.print("50%");
  else display.print("EMPTY");

  display.display();
}

// ---------- Ultrasonic ----------
long getDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000);
  if (duration == 0) return -1;

  return duration * 0.034 / 2;
}

// ---------- Servo ----------
void slowOpen() {
  for (int i = 500; i <= 2400; i += 15) {
    servo1.writeMicroseconds(i);
    servo2.writeMicroseconds(i);
    delay(6);
  }
}

void slowClose() {
  for (int i = 2400; i >= 500; i -= 15) {
    servo1.writeMicroseconds(i);
    servo2.writeMicroseconds(i);
    delay(6);
  }
}

// ---------- RFID ----------
bool checkUID() {
  for (int i = 0; i < uidLength; i++) {
    if (rfid.uid.uidByte[i] != allowedUID[i]) return false;
  }
  return true;
}

// ---------- Denied ----------
void accessDenied() {
  showMessage("ACCESS", "DENIED");

  for (int i = 0; i < 3; i++) {
    digitalWrite(DENIED_LED, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    digitalWrite(DENIED_LED, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    delay(200);
  }
}

// ---------- OLED ----------
void showMessage(String line1, String line2) {
  if (!oledOK) return;

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(8, 10);
  display.print(line1);

  display.setTextSize(2);
  display.setCursor(8, 30);
  display.print(line2);

  display.display();
}

// ---------- Tamper ----------
void checkTamperInterrupt() {

  if (tamperDetected && (millis() - lastVibrationTime > VIBRATION_COOLDOWN)) {

    lastVibrationTime = millis();

    showMessage("SECURITY ALERT", "TAMPER!");

    for (int i = 0; i < 3; i++) {
      digitalWrite(DENIED_LED, HIGH);
      digitalWrite(BUZZER_PIN, HIGH);
      delay(200);
      digitalWrite(DENIED_LED, LOW);
      digitalWrite(BUZZER_PIN, LOW);
      delay(200);
    }

    tamperDetected = false;
  }
}