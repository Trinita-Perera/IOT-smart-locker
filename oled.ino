#include <ESP32Servo.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <WebServer.h>

// WiFi & Web Server
const char* ssid = "Sanch";
const char* password = "abcdefgh";
WebServer server(80);

//OLED Display
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//Servo Doors
#include <MFRC522.h>
Servo leftDoor;
Servo rightDoor;
const int leftDoorPin = 13;
const int rightDoorPin = 12;
const int LEFT_DOOR_CLOSED = 0;
const int LEFT_DOOR_OPEN = 90;
const int RIGHT_DOOR_CLOSED = 180;
const int RIGHT_DOOR_OPEN = 90;

void initializeOLED() {
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("Smart Delivery Box");
  display.display();
  delay(2000);
}

void initializeServos() {
  leftDoor.attach(leftDoorPin);
  rightDoor.attach(rightDoorPin);
  leftDoor.write(LEFT_DOOR_CLOSED);
  rightDoor.write(RIGHT_DOOR_CLOSED);
}

void connectToWiFi() {
  displayStatus("WiFi", "Connecting...");
  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(1000);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    displayStatus("WiFi Connected", IPToString(WiFi.localIP()));
    Serial.println("IP: " + IPToString(WiFi.localIP()));
    delay(2000);
  } else {
    displayStatus("WiFi Failed", "Using Standalone");
    delay(2000);
  }
}

String IPToString(IPAddress ip) {
  return String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
}

void setupPart1() {
  Serial.begin(115200);
  initializeOLED();
  initializeServos();
  connectToWiFi();
}
