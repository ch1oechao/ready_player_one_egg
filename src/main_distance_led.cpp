#include <Arduino.h>
#include <stdlib.h>
#include <time.h>
#include <Adafruit_NeoPixel.h>

#define LED_PIN 4
#define MAX_LED 30

Adafruit_NeoPixel strip = Adafruit_NeoPixel(MAX_LED, LED_PIN, NEO_RGB + NEO_KHZ800);

int R = 0;
int G = 0;
int B = 0;

int TRIG_PIN = 26;
int ECHO_PIN = 2;
long DURATION;
int DISTANCE;

void setup() {
    Serial.begin(115200);
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    Serial.println("HC-SR04 distance sensor ready!");
    strip.begin();
    strip.show(); // Initialize all pixels to 'off'
}

void RGB_Light(int i, int R, int G, int B) {
    uint32_t color = strip.Color(G, R, B);
    strip.setPixelColor(i, color);
    strip.show();
}

void RGB_OFF() {
    uint8_t i = 0;
    uint32_t color = strip.Color(0, 0, 0);
    for (i = 0; i < MAX_LED; i++) {
        strip.setPixelColor(i, color);
    }
    strip.show();
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
    DURATION = pulseIn(ECHO_PIN, HIGH, 10000); // Timeout 10ms

    // Calculate distance in cm
    if (DURATION == 0) {
        DISTANCE = 0;
    } else {
        DISTANCE = DURATION * 0.0343 / 2;
        Serial.println(DISTANCE);
    }

    if (DISTANCE > 0 && DISTANCE <= 10) {
        for (int i = 0; i < MAX_LED; i++) {
            R = rand() % 255 + 1;
            G = rand() % 255 + 1;
            B = rand() % 255 + 1;
            RGB_Light(i, R, G, B);
            delay(10);
            RGB_OFF();
        }
        RGB_OFF();
        delay(30);
    } else {
        RGB_OFF();
        delay(30);
        return;
    }
}
