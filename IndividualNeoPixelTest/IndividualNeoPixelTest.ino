#include <Adafruit_NeoPixel.h>

#define NEOPIXEL_PIN 7
#define NUMPIXELS 3

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, NEOPIXEL_PIN);

void setup() {
  strip.begin();
  strip.setBrightness(64);
  strip.show();
}

void loop() {
  strip.setPixelColor(0, 255, 0, 0);
  strip.setPixelColor(1, 0, 255, 0);
  strip.setPixelColor(2, 0, 0, 255);
  strip.show();

  delay(2000);

  strip.clear();
  strip.show();

  delay(2000);
}
