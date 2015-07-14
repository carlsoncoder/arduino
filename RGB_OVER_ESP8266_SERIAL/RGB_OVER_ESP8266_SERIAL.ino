#include <Adafruit_NeoPixel.h>

#define NEOPIXEL_PIN 12
#define NUMPIXELS 3

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, NEOPIXEL_PIN);

String serialInput;
int r, g, b;
int currentPixelIndex = 0;

void setup()
{
  Serial.begin(115200);
  
  strip.begin();
  strip.setBrightness(80);
  strip.show();

  initColors();
  strip.show();
}

void loop()
{ 
  if (Serial.available() > 0) {
    serialInput = Serial.readStringUntil('\n');
    Serial.println(serialInput);
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
  
  delay(100);
}

void initColors() {
  strip.setPixelColor(0, 0, 255, 170);
  strip.setPixelColor(1, 0, 255, 170);
  strip.setPixelColor(2, 0, 255, 170);  
}

void setColors(int red, int green, int blue) {
  strip.setPixelColor(currentPixelIndex, red, green, blue);
  currentPixelIndex++;

  if (currentPixelIndex > 2) {
    currentPixelIndex = 0;
  }
  
  strip.show();
}
