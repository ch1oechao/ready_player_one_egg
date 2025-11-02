#include <Arduino.h>
#include <ESP32Servo.h>

const int SERVO_PIN  = 18;   // D15 → GPIO18 (servo motor)
const int SENSOR_PIN = 4;    // D12 → GPIO4  (magnetic sensor)

const bool TRIGGER_ON_LOW = false;  // Output HIGH when near magnet
const int ANGLE_CLOSED = 0;
const int ANGLE_OPEN   = 90;
const int OPEN_MS      = 5000;      // Keep open for 5 seconds
const int COOLDOWN_MS  = 2000;      // Wait 2 seconds after closing
const int STEP_DELAY   = 15;        // Step delay (milliseconds), controls speed
const int STEP_SIZE    = 2;         // Angle step size per movement (degrees)

Servo lid;

void smoothOpen() {
  for (int angle = ANGLE_CLOSED; angle <= ANGLE_OPEN; angle += STEP_SIZE) {
    lid.write(angle);
    delay(STEP_DELAY);
  }
}

void smoothClose() {
  for (int angle = ANGLE_OPEN; angle >= ANGLE_CLOSED; angle -= STEP_SIZE) {
    lid.write(angle);
    delay(STEP_DELAY);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(SENSOR_PIN, INPUT);

  lid.attach(SERVO_PIN, 500, 2500);
  lid.write(ANGLE_CLOSED);
  delay(500);
  Serial.println("System ready (Smooth motion enabled).");
}

void loop() {
  int v = digitalRead(SENSOR_PIN);
  bool triggered = TRIGGER_ON_LOW ? (v == LOW) : (v == HIGH);

  if (triggered) {
    Serial.println("Magnet detected → OPENING...");
    smoothOpen();
    delay(OPEN_MS);
    Serial.println("CLOSING...");
    smoothClose();
    delay(COOLDOWN_MS);
  } else {
    delay(50);
  }
}
