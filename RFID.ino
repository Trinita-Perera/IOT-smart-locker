#include <MFRC522.h>
#include <SPI.h>

#define SS_PIN 5
#define RST_PIN 17
MFRC522 mfrc522(SS_PIN, RST_PIN);
String authorizedUID = "A1B2C3D4"; // Change to your RFID card UID

void initializeRFID() {
  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("RFID Reader Initialized");
}

void checkRFID() {
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    return;
  }
  String uid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    uid += String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
    uid += String(mfrc522.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  Serial.println("RFID Detected: " + uid);
  if (uid == authorizedUID) {
    toggleDoors();
    displayStatus("Access Granted", "RFID Verified");
  } else {
    displayStatus("Access Denied", "Invalid RFID");
    currentState = CANT_ACCESS;
  }
  delay(1000);
}

void setupPart2() {
  initializeRFID();
}
