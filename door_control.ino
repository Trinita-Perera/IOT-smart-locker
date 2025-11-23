#include <ESP32Servo.h>

const int solenoidPin = 4;
bool isLocked = true;

enum SystemState { 
  DOOR_CLOSED, 
  DOOR_OPEN, 
  PACKAGE_DETECTED, 
  BOX_OPEN, 
  BOX_CLOSED, 
  CANT_ACCESS,
  WEIGHT_MEASURING
};
SystemState currentState = DOOR_CLOSED;

unsigned long doorOpenTime = 0;
const unsigned long AUTO_CLOSE_TIME = 10000;

void initializeSolenoid() {
  pinMode(solenoidPin, OUTPUT);
  digitalWrite(solenoidPin, HIGH);
}

void updateSystemState() {
  if (currentState == DOOR_OPEN && millis() - doorOpenTime > AUTO_CLOSE_TIME) {
    closeDoors();
  }
  if (frontDistance < DETECTION_DISTANCE && currentState == DOOR_CLOSED) {
    currentState = PACKAGE_DETECTED;
  }
  if (packageInBox && currentState == BOX_CLOSED) {
    currentState = PACKAGE_DETECTED;
  }
}

void updateDoors() {
  if (currentState == DOOR_OPEN && millis() - doorOpenTime > AUTO_CLOSE_TIME) {
    closeDoors();
  }
}

void openDoors() {
  if (currentState != DOOR_OPEN && isLocked == false) {
    leftDoor.write(LEFT_DOOR_OPEN);
    rightDoor.write(RIGHT_DOOR_OPEN);
    currentState = DOOR_OPEN;
    doorOpenTime = millis();
    displayStatus("Door Open", "Place Package");
  } else if (isLocked) {
    displayStatus("Cannot Open", "Box is Locked");
    currentState = CANT_ACCESS;
  }
}

void closeDoors() {
  if (currentState != DOOR_CLOSED) {
    leftDoor.write(LEFT_DOOR_CLOSED);
    rightDoor.write(RIGHT_DOOR_CLOSED);
    currentState = BOX_CLOSED;
    displayStatus("Box Closed", "Measuring...");
    delay(1000);
    readWeight();
    if (weight > 50) {
      packageWeight = weight;
      displayStatus("Package Detected", "Weight: " + String(weight) + "g");
    }
  }
}

void toggleDoors() {
  if (currentState == DOOR_OPEN) {
    closeDoors();
  } else {
    openDoors();
  }
}

void unlockBox() {
  digitalWrite(solenoidPin, LOW);
  isLocked = false;
  displayStatus("Box Unlocked", "Ready for Access");
}

void lockBox() {
  digitalWrite(solenoidPin, HIGH);
  isLocked = true;
  displayStatus("Box Locked", "Secure");
}

void updateDisplay() {
  display.clearDisplay();
  display.setCursor(0,0);
  switch(currentState) {
    case DOOR_CLOSED:
      display.println(">> DOOR CLOSED");
      display.println("Status: Secure");
      break;
    case DOOR_OPEN:
      display.println(">> DOOR OPEN");
      display.println("Place Package");
      break;
    case PACKAGE_DETECTED:
      display.println(">> PACKAGE DETECTED");
      display.println("Weight: " + String(packageWeight) + "g");
      break;
    case BOX_OPEN:
      display.println(">> BOX OPEN");
      display.println("Access Granted");
      break;
    case BOX_CLOSED:
      display.println(">> BOX CLOSED");
      display.println("Weight: " + String(weight) + "g");
      break;
    case CANT_ACCESS:
      display.println(">> ACCESS DENIED");
      display.println("Unauthorized");
      break;
    case WEIGHT_MEASURING:
      display.println(">> MEASURING");
      display.println("Weight: " + String(weight) + "g");
      break;
  }
  display.println("---------------");
  display.println("Lock: " + String(isLocked ? "ON" : "OFF"));
  display.println("WiFi: " + String(WiFi.status() == WL_CONNECTED ? "ON" : "OFF"));
  display.display();
}

void handleRoot() {
  server.send(200, "text/plain", "Smart Delivery Box Web Interface");
}

void handleStatus() {
  StaticJsonDocument<200> jsonDoc;
  jsonDoc["state"] = (int)currentState;
  jsonDoc["weight"] = packageWeight;
  String response;
  serializeJson(jsonDoc, response);
  server.send(200, "application/json", response);
}

void handleOpen() {
  openDoors();
  server.send(200, "text/plain", "Door Opened");
}

void handleClose() {
  closeDoors();
  server.send(200, "text/plain", "Door Closed");
}

void handleLock() {
  lockBox();
  server.send(200, "text/plain", "Box Locked");
}

void handleUnlock() {
  unlockBox();
  server.send(200, "text/plain", "Box Unlocked");
}

void handleWeight() {
  StaticJsonDocument<200> jsonDoc;
  jsonDoc["weight"] = packageWeight;
  String response;
  serializeJson(jsonDoc, response);
  server.send(200, "application/json", response);
}

void initializeWebServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/open", HTTP_GET, handleOpen);
  server.on("/close", HTTP_GET, handleClose);
  server.on("/lock", HTTP_GET, handleLock);
  server.on("/unlock", HTTP_GET, handleUnlock);
  server.on("/weight", HTTP_GET, handleWeight);
  server.begin();
}

void setupPart5() {
  initializeSolenoid();
  initializeWebServer();
}

void loopPart5() {
  server.handleClient();
  readUltrasonicSensors();
  readWeight();
  checkRFID();
  updateSystemState();
  updateDoors();
  updateDisplay();
  delay(100);
}
