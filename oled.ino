
#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <WebServer.h>

// OLED
#define SDA_PIN 21
#define SCL_PIN 22
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
bool oledOK = false;

// Servo
#define SERVO_PIN   14
Servo myServo;

// LEDs & Buzzer
#define GRANTED_LED 12
#define DENIED_LED  13
#define BUZZER_PIN  26

// WiFi
const char* ssid = "Sanch";
const char* password = "abcdefgh"; 
WebServer server(80);

// Helper
void showMessage(String line1, String line2);

// Setup for core hardware, display, WiFi & webserver
void setupCore() {
  Wire.begin(SDA_PIN, SCL_PIN);
  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C) || display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
    oledOK = true;
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
  }
  pinMode(GRANTED_LED, OUTPUT);
  pinMode(DENIED_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(GRANTED_LED, LOW);
  digitalWrite(DENIED_LED, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  myServo.attach(SERVO_PIN, 500, 2400);
  myServo.write(0);   // Closed

  // WiFi Connect
  showMessage("Connecting", "WiFi...");
  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    showMessage("WiFi Connected", WiFi.localIP().toString());
    delay(1500);
  } else {
    showMessage("No WiFi", "Standalone");
    delay(1000);
  }

  // Webserver Endpoints
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/plain", "SMART BOX Web Interface - Online!");
  });
  server.on("/status", HTTP_GET, []() {
    server.send(200, "text/plain", "System is running.");
  });
  server.begin();
}

// WebServer loop handler (call in main loop)
void webServerHandler() {
  server.handleClient();
}

// Display helper
void showMessage(String line1, String line2) {
  if (!oledOK) return;
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(10, 10); display.print(line1);
  display.setTextSize(2);
  display.setCursor(10, 30); display.print(line2);
  display.display();
}
