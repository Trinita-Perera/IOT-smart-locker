// RFID
// Checks the scanned RFID card matches the UID.

#define SS_PIN   5
#define RST_PIN  27
MFRC522 rfid(SS_PIN, RST_PIN);

// UID definition
byte allowedUID[] = {0x57, 0xB6, 0x68, 0x62};
byte uidLength = 4;

// Setup for RFID
void setupRFID() {
  SPI.begin();
  rfid.PCD_Init();
}

// UID check function
bool checkUID() {
  for (int i = 0; i < uidLength; i++) {
    if (rfid.uid.uidByte[i] != allowedUID[i]) {
      return false;
    }
  }
  return true;
}