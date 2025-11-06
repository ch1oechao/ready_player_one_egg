#include <Arduino.h>
#include <ESP32Servo.h>

const int SERVO_PIN  = 18;   
const int SENSOR_PIN = 4;    

const bool TRIGGER_ON_LOW = false;  
const int ANGLE_CLOSED = 0;
const int ANGLE_OPEN   = 60;
const int STEP_DELAY   = 15;        
const int STEP_SIZE    = 2;         

Servo lid;


bool isOpen = false;        
bool lastTriggered = false; 

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

  Serial.println("System ready (Toggle mode).");
}

void loop() {

  int v = digitalRead(SENSOR_PIN);
  bool triggered = TRIGGER_ON_LOW ? (v == LOW) : (v == HIGH);

 
  if (triggered && !lastTriggered) {
    Serial.println("Magnet detected → TOGGLE ACTION");

    if (!isOpen) {
      Serial.println("Opening...");
      smoothOpen();
      isOpen = true;
    } else {
      Serial.println("Closing...");
      smoothClose();
      isOpen = false;
    }

    delay(300); 
  }

  lastTriggered = triggered; 
  delay(20);
}
