#include <Wire.h>
#include "DFRobot_BloodOxygen_S.h"

// I2C object for version 1.0.0
DFRobot_BloodOxygen_S_I2C oximeter(&Wire, 0x57);

// LED pin
const int LED_PIN = 13;

// AI learning parameters
#define MAX_HISTORY 50
float heartRateHistory[MAX_HISTORY];
int historyCount = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.println("--- Initializing DFRobot MAX30102 Module (v1.0.0) ---");

  if (!oximeter.begin()) {
    Serial.println("!!! Sensor initialization failed, please check wiring !!!");
    while (1) delay(1000);
  }

  Serial.println("Sensor initialized successfully, starting data collection...");
  oximeter.sensorStartCollect();
}

void loop() {
  oximeter.getHeartbeatSPO2();  // Get data

  // Read from struct members
  int hr = oximeter._sHeartbeatSPO2.Heartbeat;
  int spo2 = oximeter._sHeartbeatSPO2.SPO2;

  if (hr > 20 && spo2 > 50) {
    // Save historical heart rate
    if (historyCount < MAX_HISTORY) {
      heartRateHistory[historyCount++] = hr;
    } else {
      for (int i = 1; i < MAX_HISTORY; i++) {
        heartRateHistory[i - 1] = heartRateHistory[i];
      }
      heartRateHistory[MAX_HISTORY - 1] = hr;
    }

    // Calculate mean and standard deviation
    float mean = 0;
    for (int i = 0; i < historyCount; i++) mean += heartRateHistory[i];
    mean /= historyCount;

    float variance = 0;
    for (int i = 0; i < historyCount; i++) {
      float diff = heartRateHistory[i] - mean;
      variance += diff * diff;
    }
    variance /= historyCount;
    float stdDev = sqrt(variance);

    // LED blinks with heart rate
    digitalWrite(LED_PIN, HIGH);
    delay(80);
    digitalWrite(LED_PIN, LOW);

    // Print info
    Serial.print("Heart Rate: ");
    Serial.print(hr);
    Serial.print(" BPM, SpO2: ");
    Serial.print(spo2);
    Serial.print(" % | Avg: ");
    Serial.print(mean, 1);
    Serial.print(" ± ");
    Serial.print(stdDev, 1);

    // Abnormality check
    if (hr > mean + 2 * stdDev || hr < mean - 2 * stdDev) {
      Serial.println(" ⚠️ Abnormal heart rate warning!");
    } else {
      Serial.println(" ✅ Heart rate normal.");
    }
  } else {
    Serial.println("Heart Rate: --- BPM, SpO2: --- % (Finger not detected or measuring...)");
    digitalWrite(LED_PIN, LOW);
  }

  delay(800);
}
