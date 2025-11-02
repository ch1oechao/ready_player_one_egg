#include <Arduino.h>

int TRIG_PIN = 26;
int ECHO_PIN = 2;
long DURATION;
int DISTANCE;

void setup() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  Serial.println("HC-SR04 distance sensor ready!");
  Serial.begin(115200);
}

void loop() {
  // Clear the trigger pin
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  
  // Send 10us pulse
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Read echo pulse duration (in microseconds)
  DURATION = pulseIn(ECHO_PIN, HIGH, 30000); // Timeout 30ms

  // Calculate distance in cm
  DISTANCE = DURATION * 0.0343 / 2;

  Serial.println(DISTANCE);
  delay(100);
}
