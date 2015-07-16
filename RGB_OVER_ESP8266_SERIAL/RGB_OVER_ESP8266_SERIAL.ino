#include <Adafruit_NeoPixel.h>

#define POT_PIN 3
#define NEOPIXEL_PIN 5
#define NUMPIXELS 3

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, NEOPIXEL_PIN);

String serialInput;
int r, g, b;
int lastBrightness = -1;

void setup()
{
  Serial.begin(115200);
  
  strip.begin();
  strip.setBrightness(64);
  strip.show();

  initColors();
  strip.show();
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
        else {
          break;
        }
      }

      setColors(r, g, b);
    }
  }

  determineBrightness();
  delay(100);
}

void determineBrightness() {
  int brightness = analogRead(POT_PIN) / 4;
    
  if (brightness == lastBrightness) {
    // no change, nothing to do
    return;
  }
  else if (lastBrightness != -1) {
      if (brightness == 0) {
        brightness = 1;
      }
      
      strip.setBrightness(brightness);
      strip.show();
      delay(100);
  }
  
  lastBrightness = brightness;
}

void initColors() {
  strip.setPixelColor(0, 255, 0, 0);    // red
  strip.setPixelColor(1, 255, 255, 0);  // yellow
  strip.setPixelColor(2, 0, 255, 0);    // green
}

void setColors(int red, int green, int blue) {
  for (int i = 0; i < NUMPIXELS; i++) {
    strip.setPixelColor(i, red, green, blue);  
  }
  
  strip.show();
}
