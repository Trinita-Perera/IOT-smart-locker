// Lid state/timer
bool lidOpen = false;
unsigned long lidOpenTime = 0;
const unsigned long AUTO_CLOSE_TIME = 60000;  // 1 minute

// Toggle the lid (open/close)
void toggleLid() {
  if (!lidOpen) {
    lidOpen = true;
    lidOpenTime = millis();
    myServo.write(90);
    if (oledOK) {
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(20, 10); display.print("OPENED BY");
      display.setTextSize(2);
      display.setCursor(0, 30); display.print("DELIVERY");
      display.setCursor(15, 50); display.print("RIDER");
      display.display();
    }
  } else {
    lidOpen = false;
    myServo.write(0);
    showMessage("LID STATUS", "CLOSED");
  }
  digitalWrite(GRANTED_LED, HIGH); digitalWrite(BUZZER_PIN, HIGH);
  delay(150);
  digitalWrite(BUZZER_PIN, LOW); digitalWrite(GRANTED_LED, LOW);
}

// Auto-close after one minute
void closeLidAuto() {
  lidOpen = false;
  myServo.write(0);
  if (oledOK) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(10, 10); display.print("AUTO CLOSING");
    display.setTextSize(2);
    display.setCursor(25, 30); display.print("LID");
    display.display();
  }
  // 2 beeps
  for (int i = 0; i < 2; i++) {
    digitalWrite(BUZZER_PIN, HIGH); delay(200);
    digitalWrite(BUZZER_PIN, LOW); delay(200);
  }
}
