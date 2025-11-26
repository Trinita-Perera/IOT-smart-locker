#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "HX711.h"

// WIFI 
#define WIFI_SSID     "Pixel7P10iii"
#define WIFI_PASSWORD "123456789"

//  FIREBASE 
#define DATABASE_URL    "https://box2-fc0d0-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define DATABASE_SECRET "u49JdDhGOpHU96HpFUHtPNjRw9rRHL4drvDvvibA"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

//  OLED 
#define SDA_PIN 21
#define SCL_PIN 22
#define OLED_RESET -1

Adafruit_SSD1306 display(128, 64, &Wire, OLED_RESET);
bool oledOK = false;

// RFID 
#define SS_PIN  5
#define RST_PIN 27
MFRC522 rfid(SS_PIN, RST_PIN);

//  SERVO 
#define SERVO1_PIN 14
#define SERVO2_PIN 16
Servo servo1;
Servo servo2;

//  LEDs & BUZZER 
#define GRANTED_LED 2
#define DENIED_LED  15
#define BUZZER_PIN  26
#define BOX_LIGHT   12

//  ULTRASONIC
#define BOTTOM_TRIG 33
#define BOTTOM_ECHO 32
#define TOP_TRIG    17
#define TOP_ECHO    35   

// LOWER DETECTION DISTANCES (1-3cm range)
#define DETECT_DISTANCE 3      
#define VERY_CLOSE_DISTANCE 1  


#define LOADCELL_DOUT 34  
#define LOADCELL_SCK  25
HX711 scale;
float calibration_factor = -7050.0;

// RFID 
#define MAX_RFID_TAGS 30
String authorizedUIDs[MAX_RFID_TAGS];
int authorizedCount = 0;
String rfidMode = "IDLE";

// SYSTEM 
bool lidOpen = false;
unsigned long lidOpenTime = 0;
const unsigned long AUTO_CLOSE_TIME = 50000;
unsigned long lastControlCheck = 0;
const unsigned long CONTROL_CHECK_INTERVAL = 1000;

// UID FUNCTION 
String uidToString() {
  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  return uid;
}

// OLED DISPLAY 
void showMessage(String line1, String line2) {
  if (!oledOK) return;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println(line1);

  display.setTextSize(2);
  display.setCursor(0, 32);
  display.println(line2);

  display.display();
}

//  WIFI 
void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(1500);

  Serial.println("Connecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 25) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n WiFi Connected");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n WiFi Failed - Restarting...");
    delay(2000);
    ESP.restart();
  }
}

//FIREBASE 
void connectFirebase() {
  config.database_url = DATABASE_URL;
  config.signer.tokens.legacy_token = DATABASE_SECRET;
  Firebase.reconnectNetwork(true);
  Firebase.begin(&config, &auth);

  // Initialize control path
  Firebase.RTDB.setString(&fbdo, "/box/control/action", "NONE");
  Firebase.RTDB.setString(&fbdo, "/box/status", "ONLINE");
  Serial.println(" Firebase Connected");
}

// LOAD RFID 
void loadAuthorizedUIDs() {
  if (Firebase.RTDB.getJSON(&fbdo, "/rfids/list")) {
    FirebaseJson &json = fbdo.jsonObject();
    authorizedCount = 0;

    size_t len = json.iteratorBegin();
    for (size_t i = 0; i < len; i++) {
      String key, value;
      int type;
      json.iteratorGet(i, type, key, value);

      value.replace("\"", "");
      value.toUpperCase();

      if (authorizedCount < MAX_RFID_TAGS)
        authorizedUIDs[authorizedCount++] = value;
    }
    json.iteratorEnd();
    Serial.println(" Loaded " + String(authorizedCount) + " RFID tags");
  } else {
    Serial.println(" Failed to load RFID tags");
  }
}

//  RFID MODE 
void checkRFIDMode() {
  if (Firebase.RTDB.getString(&fbdo, "/rfids/mode")) {
    String newMode = fbdo.stringData();
    if (newMode != rfidMode) {
      rfidMode = newMode;
      Serial.println("RFID Mode: " + rfidMode);
    }
  }
}

//  RFID AUTH 
bool isAuthorized(String uid) {
  for (int i = 0; i < authorizedCount; i++) {
    if (uid == authorizedUIDs[i]) return true;
  }
  return false;
}

// RFID HANDLER 
void checkRFID() {
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  String uid = uidToString();
  Serial.println("Scanned: " + uid);

  digitalWrite(BUZZER_PIN, HIGH);
  delay(100);
  digitalWrite(BUZZER_PIN, LOW);

  if (rfidMode == "WAITING") {
    Firebase.RTDB.setString(&fbdo, "/rfids/captured_uid", uid);
    Firebase.RTDB.setString(&fbdo, "/rfids/mode", "IDLE");
    showMessage("NEW RFID", uid);
    Serial.println(" New RFID captured: " + uid);
    delay(2000);
    return;
  }

  if (isAuthorized(uid)) {
    Serial.println(" Access granted for: " + uid);
    toggleLid("Delivery BOY");
  } else {
    Serial.println(" Access denied for: " + uid);
    showMessage("ACCESS", "DENIED");
    digitalWrite(DENIED_LED, HIGH);
    delay(1000);
    digitalWrite(DENIED_LED, LOW);
  }
}

// LID CONTROL 
void toggleLid(String who) {
  lidOpen = !lidOpen;
  digitalWrite(BOX_LIGHT, lidOpen ? HIGH : LOW);

  Serial.println(" Toggling lid - New state: " + String(lidOpen ? "OPEN" : "CLOSED"));

  if (lidOpen) {
    lidOpenTime = millis();
    showMessage("OPENED BY", who);
    digitalWrite(GRANTED_LED, HIGH);

    // Open servos smoothly
    Serial.println(" Opening servos...");
    for (int i = 500; i <= 1400; i += 10) {
      servo1.writeMicroseconds(i);
      servo2.writeMicroseconds(i);
      delay(5);
    }
    delay(1000);
    digitalWrite(GRANTED_LED, LOW);
  } else {
    showMessage("LID", "CLOSED");
    Serial.println(" Closing servos...");

    // Close servos smoothly
    for (int i = 1400; i >= 500; i -= 10) {
      servo1.writeMicroseconds(i);
      servo2.writeMicroseconds(i);
      delay(5);
    }
  }

  // Update Firebase
  if (Firebase.RTDB.setString(&fbdo, "/box/lid_status", lidOpen ? "OPEN" : "CLOSED")) {
    Serial.println(" Updated lid status in Firebase");
  } else {
    Serial.println(" Failed to update lid status in Firebase");
  }
  
  if (Firebase.RTDB.setString(&fbdo, "/box/last_access", who)) {
    Serial.println(" Updated last access in Firebase");
  } else {
    Serial.println(" Failed to update last access in Firebase");
  }
}

// AUTO CLOSE 
void checkAutoClose() {
  if (lidOpen && millis() - lidOpenTime > AUTO_CLOSE_TIME) {
    Serial.println(" Auto-closing lid due to timeout");
    toggleLid("AUTO TIMEOUT");
  }
}

//  LEVEL SENSOR 
String readLevelStatus() {
  long bottom = readUltrasonic(BOTTOM_TRIG, BOTTOM_ECHO);
  long top    = readUltrasonic(TOP_TRIG, TOP_ECHO);

  // Print raw distances for debugging
  Serial.println("📏 Bottom: " + String(bottom) + "cm, Top: " + String(top) + "cm");

  bool bottomDetected = bottom > 0 && bottom <= DETECT_DISTANCE;
  bool topDetected = top > 0 && top <= DETECT_DISTANCE;
  bool bottomVeryClose = bottom > 0 && bottom <= VERY_CLOSE_DISTANCE;
  bool topVeryClose = top > 0 && top <= VERY_CLOSE_DISTANCE;

  // Enhanced level detection with lower thresholds
  if (bottomVeryClose && topVeryClose) {
    return "100%";  // Both sensors detect very close object
  } else if (bottomDetected && topDetected) {
    return "75%";   // Both sensors detect object within 3cm
  } else if (bottomDetected) {
    return "50%";   // Only bottom sensor detects object
  } else if (topDetected) {
    return "25%";   // Only top sensor detects object
  } else {
    return "EMPTY"; // No object detected
  }
}

// ULTRASONIC 
long readUltrasonic(int trig, int echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  long duration = pulseIn(echo, HIGH, 30000);
  if (duration == 0) return -1;

  long distance = duration * 0.034 / 2;
  
  // Filter out unrealistic readings (ultrasonic sensors can be noisy at close range)
  if (distance < 0 || distance > 50) {
    return -1; // Invalid reading
  }
  
  return distance;
}

//  WEIGHT
float readWeight() {
  if (scale.is_ready()) {
    float weight = scale.get_units(5);
    // Ensure weight is not negative and filter noise
    return (weight < 0.05) ? 0.0 : weight;
  }
  return 0.0;
}

// FIREBASE CONTROL HANDLER 
void checkFirebaseControl() {
  if (millis() - lastControlCheck > CONTROL_CHECK_INTERVAL) {
    lastControlCheck = millis();
    
    if (Firebase.RTDB.getString(&fbdo, "/box/control/action")) {
      String action = fbdo.stringData();
      Serial.println(" Checking Firebase control: " + action);
      
      if (action == "toggle_lid") {
        Serial.println(" Web control: Toggle lid received");
        toggleLid("WEB ADMIN");
        if (Firebase.RTDB.setString(&fbdo, "/box/control/action", "NONE")) {
          Serial.println(" Reset control action to NONE");
        } else {
          Serial.println(" Failed to reset control action");
        }
      }
      else if (action == "emergency_close" && lidOpen) {
        Serial.println(" Web control: Emergency close received");
        toggleLid("EMERGENCY");
        Firebase.RTDB.setString(&fbdo, "/box/control/action", "NONE");
      }
      else if (action == "calibrate_scale") {
        Serial.println("🎛️ Web control: Calibrate scale received");
        scale.tare();
        showMessage("SCALE", "CALIBRATED");
        Firebase.RTDB.setString(&fbdo, "/box/control/action", "NONE");
        delay(2000);
      }
    }

    // Check for OLED message
    if (Firebase.RTDB.getBool(&fbdo, "/box/oled_show")) {
      bool oledShow = fbdo.boolData();
      if (oledShow) {
        if (Firebase.RTDB.getString(&fbdo, "/box/oled_message")) {
          String message = fbdo.stringData();
          Serial.println("📺 OLED message received: " + message);
          showMessage("MESSAGE:", message);
          Firebase.RTDB.setBool(&fbdo, "/box/oled_show", false);
          delay(5000); // Show message for 5 seconds
        }
      }
    }
  }
}

// SETUP
void setup() {
  Serial.begin(115200);
  Serial.println("\n Smart Box System Starting...");
  
  Wire.begin(SDA_PIN, SCL_PIN);

  // Initialize OLED
  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    oledOK = true;
    showMessage("SMART BOX", "STARTING...");
    Serial.println(" OLED Display Initialized");
  } else {
    Serial.println(" OLED Display Failed");
  }

  // Initialize RFID
  SPI.begin();
  rfid.PCD_Init();
  Serial.println(" RFID Reader Initialized");

  // Initialize Servos
  servo1.attach(SERVO1_PIN, 500, 2400);
  servo2.attach(SERVO2_PIN, 500, 2400);
  
  // Close lid on startup
  servo1.writeMicroseconds(500);
  servo2.writeMicroseconds(500);
  Serial.println(" Servos Initialized - Lid Closed");

  // Initialize Pins
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BOX_LIGHT, OUTPUT);
  pinMode(GRANTED_LED, OUTPUT);
  pinMode(DENIED_LED, OUTPUT);

  pinMode(BOTTOM_TRIG, OUTPUT);
  pinMode(BOTTOM_ECHO, INPUT);
  pinMode(TOP_TRIG, OUTPUT);
  pinMode(TOP_ECHO, INPUT);
  Serial.println(" GPIO Pins Initialized");

  // Initialize Scale
  scale.begin(LOADCELL_DOUT, LOADCELL_SCK);
  scale.set_scale(calibration_factor);
  scale.tare();
  Serial.println("Weight Scale Initialized");

  // Connect to WiFi and Firebase
  connectWiFi();
  connectFirebase();
  loadAuthorizedUIDs();

  showMessage("SMART BOX", "READY");
  Serial.println(" System Ready - All components initialized");
  Serial.println(" Ultrasonic Detection Ranges:");
  Serial.println("   - VERY_CLOSE: 0-1cm (100% full)");
  Serial.println("   - DETECT_DISTANCE: 1-3cm (25-75% full)");
  Serial.println("   - >3cm: EMPTY");
}

//  LOOP 
void loop() {
  static unsigned long lastDataUpdate = 0;
  static unsigned long lastUIDReload = 0;

  // Check for control commands from web
  checkFirebaseControl();

  // Check RFID mode and cards
  checkRFIDMode();
  checkRFID();
  
  // Auto-close lid if open too long
  checkAutoClose();

  // Update data to Firebase every 2 seconds
  if (millis() - lastDataUpdate > 2000) {
    String level = readLevelStatus();
    float weight = readWeight();

    if (Firebase.RTDB.setString(&fbdo, "/box/level", level)) {
      Serial.println(" Level updated: " + level);
    }
    
    if (Firebase.RTDB.setFloat(&fbdo, "/box/weight", weight)) {
      Serial.println("⚖️ Weight updated: " + String(weight, 2) + " kg");
    }

    // Update OLED display
    if (oledOK) {
      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor(0, 0);
      display.println("LEVEL:");
      display.println(level);

      display.setTextSize(1);
      display.setCursor(0, 40);
      display.print("Weight: ");
      display.print(weight, 2);
      display.println("kg");

      display.display();
    }

    lastDataUpdate = millis();
  }

  // Reload authorized UIDs every 30 seconds
  if (millis() - lastUIDReload > 30000) {
    loadAuthorizedUIDs();
    lastUIDReload = millis();
  }

  // Small delay to prevent overwhelming the system
  delay(100);
}