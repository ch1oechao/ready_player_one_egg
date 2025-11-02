#include <stdlib.h>
#include <time.h>
#include <Adafruit_NeoPixel.h>
#define PIN 4
#define MAX_LED 30

Adafruit_NeoPixel strip = Adafruit_NeoPixel(MAX_LED, PIN, NEO_RGB + NEO_KHZ800);

int R = 0;
int G = 0;
int B = 0;

void setup() {
    // put your setup code here, it will run once:
    strip.begin();
    strip.show();
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
    for (int i = 0; i < MAX_LED; i++) {
        R = rand() % 255 + 1;
        G = rand() % 255 + 1;
        B = rand() % 255 + 1;
        RGB_Light(i, R, G, B);
        delay(100);
        RGB_OFF();
    }
    RGB_OFF();
    delay(300);
}
