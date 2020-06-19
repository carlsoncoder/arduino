#include <Adafruit_NeoPixel.h>

#ifdef __AVR__
 #include <avr/power.h>
#endif

// Which pin on the Arduino is connected to the NeoPixels?
#define NEOPIXEL_PIN 8

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS 16

// Declare our NeoPixel strip object:
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

String serialInput;
int r, g, b, brightness;
bool useFadingMode;

void setup() {
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif

  Serial.begin(9600);
  strip.begin();

  useFadingMode = true;
  
  // default brightness/color values
  setBrightnessValues(64);
  setColorValues(255, 0, 0);
}


void loop() {  
  checkSerialInput();
  delay(100);
  
  if (useFadingMode) {
    colorFade(255, 0, 0); //red
    colorFade(255, 165, 0); //orange
    colorFade(255, 255, 0); //yellow
    colorFade(0,255,0); // green
    colorFade(0,0,255); // blue
    colorFade(75,0,130); //indigo
    colorFade(238,130,238); // violet 
  }
}

void checkSerialInput() {
  if (Serial.available() > 0) {
    serialInput = Serial.readStringUntil('\n');
    Serial.print("DATA: ");
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
        else if (counter == 4) {
          brightness = atoi(color);
        }
        else {
          break;
        }
      }

      if (useFadingMode && r != 999) {
        // we were in fading mode but now need to leave and set the new color
        useFadingMode = false;
        setColorValues(r, g, b);
      }
      else if (!useFadingMode && r == 999) {
        // we were NOT in fading mode but now need to go into it
        useFadingMode = true;
      }
      else if (!useFadingMode && r != 999) {
        // normal (non-fading mode), and we have a new value to set
        setColorValues(r, g, b);
      }

      // always set brightness value, even in fading loop
      setBrightnessValues(brightness);
    }
  }
}

void colorFade(uint8_t r, uint8_t g, uint8_t b) {
  if (!useFadingMode) {
    return;
  }
  
  // assume all pixels are the same color
  uint8_t curr_r, curr_g, curr_b;
  uint32_t curr_col = strip.getPixelColor(0); // get the current colour of first pixel
  curr_b = curr_col & 0xFF; curr_g = (curr_col >> 8) & 0xFF; curr_r = (curr_col >> 16) & 0xFF;  // separate into RGB components

  while ((curr_r != r) || (curr_g != g) || (curr_b != b)){  // while the curr color is not yet the target color
        if (curr_r < r) curr_r++; else if (curr_r > r) curr_r--;  // increment or decrement the old color values
        if (curr_g < g) curr_g++; else if (curr_g > g) curr_g--;
        if (curr_b < b) curr_b++; else if (curr_b > b) curr_b--;
        for(uint16_t i = 0; i < strip.numPixels(); i++) {
           strip.setPixelColor(i, curr_r, curr_g, curr_b);  // set the color
        }
        
        strip.show();
        delay(75);  // add a delay if its too fast
        checkSerialInput();
        if (!useFadingMode) {
          return;
       }
  }
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
