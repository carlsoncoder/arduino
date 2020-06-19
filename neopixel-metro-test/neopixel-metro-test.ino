#include <Adafruit_NeoPixel.h>

// Which pin on the Arduino is connected to the NeoPixels?
#define NEOPIXEL_PIN 2

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS 16

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

String serialInput;
int r, g, b, brightness;

void setup() {
  Serial.begin(115200);

  // initialize NeoPixel strip
  strip.begin();

  // default brightness/color values
  setBrightnessValues(64);
  setColorValues(0, 0, 255);
}

void loop()
{ 
  if (Serial.available() > 0) {
    serialInput = Serial.readStringUntil('\n');
    if (serialInput.startsWith("RGB:")) {
      String rgbSubstring = serialInput.substring(4);
      char rgbCharArray[rgbSubstring.length() + 1];
      strcpy(rgbCharArray, rgbSubstring.c_str());
      
      char *p = rgbCharArray;
      char *color;
      int counter = 0;
      while ((color = strtok_r(p, ",", &p)) != NULL) {
        counter++;
        if (counter == 1) {
          r = atoi(color);
        }
        else if (counter == 2) {
          g = atoi(color);
        }
        else if (counter == 3) {
          b = atoi(color);
        }
        else if (counter == 4) {
          brightness = atoi(color);
        }
        else {
          break;
        }
      }

      setColorValues(r, g, b);
      setBrightnessValues(brightness);
    }
  }

  determineBrightness();
  delay(100);
}
void setColorValues(int red, int green, int blue) {
  for (int i = 0; i < NUMPIXELS; i++) {
    strip.setPixelColor(i, red, green, blue);  
  }
  
  strip.show();
  delay(100);
}

void setBrightnessValues(int brightness) {
  strip.setBrightness(brightness);
  strip.show();
  delay(100);
}
