
// HC-SR04
#define TRIG_PIN 33
#define ECHO_PIN 32

// Setup for Ultrasonic sensor
void setupUltrasonic() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);
}

// Ultrasonic distance measurement
long getDistanceCm() {
  digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 30000); // 30ms timeout
  if (duration == 0) return -1;
  return duration * 0.034 / 2;
}

