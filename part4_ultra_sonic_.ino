void setup() {

}

void loop() {

}
const int frontUltraTrig = 14;
const int frontUltraEcho = 27;
const int boxUltraTrig = 33;
const int boxUltraEcho = 32;
int frontDistance = 0;
int boxDistance = 0;
const int DETECTION_DISTANCE = 30;
bool packageInBox = false;

void initializeUltrasonic() {
  pinMode(frontUltraTrig, OUTPUT);
  pinMode(frontUltraEcho, INPUT);
  pinMode(boxUltraTrig, OUTPUT);
  pinMode(boxUltraEcho, INPUT);
}

int readUltrasonic(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH);
  return duration * 0.034 / 2;
}

void readUltrasonicSensors() {
  frontDistance = readUltrasonic(frontUltraTrig, frontUltraEcho);
  boxDistance = readUltrasonic(boxUltraTrig, boxUltraEcho);
  packageInBox = (boxDistance < 15 && boxDistance > 2);
}

void setupPart4() {
  initializeUltrasonic();
}

