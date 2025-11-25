#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED
#define SDA_PIN 21
#define SCL_PIN 22
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
bool oledOK = false;

// Display Helper
void showMessage(String line1, String line2) {
  if (!oledOK) return;
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(8, 10); display.print(line1);
  display.setTextSize(2);
  display.setCursor(8, 30); display.print(line2);
  display.display();
}

void setupOLED() {
  Wire.begin(SDA_PIN, SCL_PIN);
  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    oledOK = true;
    display.setTextColor(SSD1306_WHITE);
    display.clearDisplay();
    showMessage("SMART BOX", "READY");
  }
}

void loopOLED() 
