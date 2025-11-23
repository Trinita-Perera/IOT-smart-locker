#include <HX711.h>

const int LOADCELL_DOUT_PIN = 26;
const int LOADCELL_SCK_PIN = 25;
HX711 scale;
float calibration_factor = -96650; // Calibrate this for your load cell
float weight = 0;
float packageWeight = 0;

void initializeLoadCell() {
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(calibration_factor);
  scale.tare();
  Serial.println("Load Cell Initialized");
}

void readWeight() {
  if (scale.is_ready()) {
    weight = scale.get_units(5); // Average 5 readings
    if (weight < 0) weight = 0;
    if (currentState == BOX_CLOSED && weight > 50) {
      packageWeight = weight;
    }
  }
}

void setupPart3() {
  initializeLoadCell();
}
