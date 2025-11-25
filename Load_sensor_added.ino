#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "HX711.h"   // Added load cell

//RFID / SERVO
#define SS_PIN        5
#define RST_PIN       27
#define SERVO1_PIN    14
#define SERVO2_PIN    16

MFRC522 rfid(SS_PIN, RST_PIN);
Servo servo1, servo2;

// Allowed RFID Card UID
byte allowedUID[] = {0x57, 0xB6, 0x68, 0x62};
byte uidLength = 4;

// Lid State
bool lidOpen = false;
unsigned long lidOpenTime = 0;
const unsigned long AUTO_CLOSE_TIME = 50000;


//  OLED DISPLAY 
#define SDA_PIN 21
#define SCL_PIN 22
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
bool oledOK = false;


//  ULTRASONIC 
#define BOTTOM_TRIG_PIN 33
#define BOTTOM_ECHO_PIN 32
#define TOP_TRIG_PIN    17
#define TOP_ECHO_PIN    35
#define DETECT_DISTANCE 10


// HX711 LOAD CELL 
#define LOADCELL_DOUT_PIN 34
#define LOADCELL_SCK_PIN  26   // Changed to avoid pin conflict

HX711 scale;
float calibration_factor = -7050;

float lastWeight = 0;
unsigned long weightDisplayTime = 0;
bool weightShowing = false;


//  OLED MESSAGE FUNCTION 
void showMessage(String line1, String line2) {
  if (!oledOK) return;
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(8, 10);
  display.print(line1);

  display.setTextSize(2);
  display.setCursor(8, 30);
  display.print(line2);

  display.display();
}


//  RFID + SERVO SETUP 
void setupRFID() {
  SPI.begin();
  rfid.PCD_Init();

  servo1.attach(SERVO1_PIN, 500, 2400);
  servo2.attach(SERVO2_PIN, 500, 2400);

  servo1.writeMicroseconds(500);
  servo2.writeMicroseconds(500);
}

bool checkUID() {
  for (int i = 0; i < uidLength; i++) {
    if (rfid.uid.uidByte[i] != allowedUID[i])
      return false;
  }
  return true;
}


// SERVO MOVEMENTS
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

void grantAccess() {
  if (!lidOpen) {
    lidOpen = true;
    lidOpenTime = millis();
    slowOpen();
    showMessage("ACCESS", "GRANTED");
  } else {
    lidOpen = false;
    slowClose();
    showMessage("LID", "CLOSED");
  }
}


// OLED SETUP
void setupOLED() {
  Wire.begin(SDA_PIN, SCL_PIN);

  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    oledOK = true;
    showMessage("SMART BOX", "READY");
  }
}


//  ULTRASONIC FUNCTIONS
void setupUltrasonic() {
  pinMode(BOTTOM_TRIG_PIN, OUTPUT);
  pinMode(BOTTOM_ECHO_PIN, INPUT);
  pinMode(TOP_TRIG_PIN, OUTPUT);
  pinMode(TOP_ECHO_PIN, INPUT);
}

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

void loopUltrasonic() {
  long bottomDist = getDistance(BOTTOM_TRIG_PIN, BOTTOM_ECHO_PIN);
  long topDist = getDistance(TOP_TRIG_PIN, TOP_ECHO_PIN);
}



// HX711 FUNCTIONS 
void setupHX711() {
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(calibration_factor);
  scale.tare();
}

void loopHX711() {
  float currentWeight = scale.get_units(5);

  if (abs(currentWeight - lastWeight) > 0.2) {
    lastWeight = currentWeight;
    weightDisplayTime = millis();
    weightShowing = true;


  }

  if (weightShowing && millis() - weightDisplayTime > 10000) {
    weightShowing = false;
  }
}



//  RFID LOOP
void loopRFID() {
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  if (checkUID()) {
    grantAccess();
  } else {
    showMessage("ACCESS", "DENIED");
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  delay(800);
}



//  MAIN SETUP
void setup() {
  setupOLED();
  setupRFID();
  setupUltrasonic();
  setupHX711();     // Added load cell setup
}



// MAIN LOOP 
void loop() {
  loopRFID();
  loopUltrasonic();
  loopHX711();      // Added load cell loop

  // Auto close lid
  if (lidOpen && millis() - lidOpenTime >= AUTO_CLOSE_TIME) {
    slowClose();
    lidOpen = false;
    showMessage("AUTO", "CLOSED");
  }
}
