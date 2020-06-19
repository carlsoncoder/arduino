#include <Adafruit_NeoPixel.h>

#ifdef __AVR__
 #include <avr/power.h>
#endif

// Which pin on the Arduino is connected to the NeoPixels?
#define NEOPIXEL_PIN 8

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS 30

class NeoPatterns : public Adafruit_NeoPixel
{
    public:
    
    unsigned long Interval;   // milliseconds between updates
    unsigned long OriginalInterval; // used to track the original interval when we extend interval to wait between color changes
    unsigned long IntervalBetweenColors; // milliseconds to wait between starting the next color fade pattern
    unsigned long LastUpdate; // last update of position
    
    uint32_t Color1, Color2;  // What colors are in use
    uint16_t TotalSteps;  // total number of steps in the pattern
    uint16_t Index;  // current step within the pattern

    uint16_t CurrentFadeColorIndex = 0;

    bool WaitingAfterColorChange = false;

    uint32_t FadeColors[12] =
    {
      Color(255, 0, 0),
      Color(255, 128, 0),
      Color(255, 255, 0),
      Color(128,255,0),
      Color(0,255,0),
      Color(0,255,128),
      Color(0,255,255),
      Color(0,128,255),
      Color(0,0,255),
      Color(128,0,255),
      Color(255,0,255),
      Color(255,0,128)
    };

    // Constructor - calls base-class constructor to initialize strip
    NeoPatterns(uint16_t pixels, uint8_t pin, uint8_t type)
    :Adafruit_NeoPixel(pixels, pin, type)
    {
    }
    
    // Update the pattern
    void Update()
    {
      if((millis() - LastUpdate) > Interval) // time to update
      {
        if (WaitingAfterColorChange)
        {
          WaitingAfterColorChange = false;
          Interval = OriginalInterval;
        }
        
        LastUpdate = millis();
        
        uint8_t red = ((Red(Color1) * (TotalSteps - Index)) + (Red(Color2) * Index)) / TotalSteps;
        uint8_t green = ((Green(Color1) * (TotalSteps - Index)) + (Green(Color2) * Index)) / TotalSteps;
        uint8_t blue = ((Blue(Color1) * (TotalSteps - Index)) + (Blue(Color2) * Index)) / TotalSteps;
            
        ColorSet(Color(red, green, blue));
        show();
        Increment();
      }
    }
  
    // Increment the Index and reset at the end
    void Increment()
    {
      Index++;
      if (Index >= TotalSteps)
      {
        Index = 0;

        WaitingAfterColorChange = true;
        Interval = IntervalBetweenColors;
            
        // increment CurrentFadeColorIndex by 1 - that will correspond to the current color
        if (CurrentFadeColorIndex <= 9)
        {
          // just increment it and assign new values
          CurrentFadeColorIndex++;
          Color1 = FadeColors[CurrentFadeColorIndex];
          Color2 = FadeColors[CurrentFadeColorIndex + 1];
        }
        else if (CurrentFadeColorIndex == 10)
        {
          // current index is 10, meaning Color1 = 10 and Color2 = 11
          // Next step should be: Color1 = 11 and Color 2 = 0
          Color1 = FadeColors[11];
          Color2 = FadeColors[0];

          // increment the index
          CurrentFadeColorIndex++;
        }
        else if (CurrentFadeColorIndex == 11)
        {
          // current index is 11, meaning Color1 = 11 and Color2 = 0
          // Next step should be: Color1 = 0 and Color2 = 1
          Color1 = FadeColors[0];
          Color2 = FadeColors[1];

          // reset index back to 0
          CurrentFadeColorIndex = 0;
        }
      }
    }

    // Initialize for a Fade
    void Fade(uint16_t fadeSteps, uint16_t fadeInterval, uint16_t intervalBetweenColors)
    {
      Interval = fadeInterval;
      OriginalInterval = fadeInterval;
      TotalSteps = fadeSteps;
      IntervalBetweenColors = intervalBetweenColors;
      CurrentFadeColorIndex = 0;
      Color1 = FadeColors[0];
      Color2 = FadeColors[1];
      Index = 0;

      ColorSet(FadeColors[0]);
    }

    // Set all pixels to a color (synchronously)
    void ColorSet(uint32_t color)
    {
      uint32_t gammaColor = gamma32(color);
      for (int i = 0; i < numPixels(); i++)
      {
          setPixelColor(i, gammaColor);
      }
       
      show();
    }

    void BrightnessSet(uint32_t brightness)
    {
      setBrightness(brightness);
      show();
    }

    // Returns the Red component of a 32-bit color
    uint8_t Red(uint32_t color)
    {
      return (color >> 16) & 0xFF;
    }

    // Returns the Green component of a 32-bit color
    uint8_t Green(uint32_t color)
    {
      return (color >> 8) & 0xFF;
    }

    // Returns the Blue component of a 32-bit color
    uint8_t Blue(uint32_t color)
    {
      return color & 0xFF;
    }
};

// Declare our NeoPixel strip object:
NeoPatterns strip = NeoPatterns(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

String serialInput;
int r, g, b, brightness;
bool useFadingMode = true;
uint16_t fadeSteps = 100;
uint16_t fadeInterval = 100;
uint16_t intervalBetweenColors = 20000;
unsigned long lastSerialScan;
unsigned long serialScanInterval = 100;

void setup() {
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif

  Serial.begin(9600);
  strip.begin();

  strip.Fade(fadeSteps, fadeInterval, intervalBetweenColors);

  // default values
  strip.BrightnessSet(255);
}

void loop() {  
  
  if((millis() - lastSerialScan) > serialScanInterval) // time to update
  {
    lastSerialScan = millis();
    checkSerialInput();
  }

  // always update the strip if we're in fading mode
  if (useFadingMode) {
    strip.Update(); 
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
        strip.ColorSet(strip.Color(r, g, b));        
      }
      else if (!useFadingMode && r == 999) {
        // we were NOT in fading mode but now need to go into it
        strip.Fade(fadeSteps, fadeInterval, intervalBetweenColors);
        useFadingMode = true;
      }
      else if (!useFadingMode && r != 999) {
        // normal (non-fading mode), and we have a new value to set
        strip.ColorSet(strip.Color(r, g, b));
      }

      // always set brightness value, even in fading loop
      strip.BrightnessSet(brightness);
    }
  }
}
