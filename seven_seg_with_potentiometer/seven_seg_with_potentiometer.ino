// Enable one of these two #includes and comment out the other.
// Conditional #include doesn't work due to Arduino IDE shenanigans.
#include <Wire.h> // Enable this line if using Arduino Uno, Mega, etc.
//#include <TinyWireM.h> // Enable this line if using Adafruit Trinket, Gemma, etc.

#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"

static float justinTestNumber = 88.5;
unsigned long currentTime;
unsigned long loopTime;
const int PIN_ENCODER_A = 10;
const int PIN_ENCODER_B = 13;

unsigned char encoder_A;
unsigned char encoder_B;
unsigned char encoder_A_prev = 0;

Adafruit_7segment matrix = Adafruit_7segment();

void setup() {
#ifndef __AVR_ATtiny85__
  Serial.begin(9600);
  Serial.println("7 Segment Backpack Test");
#endif
  matrix.begin(0x70);
  
  // print a floating point 
  matrix.print(justinTestNumber);
  matrix.writeDisplay();
  
  // set pins as input with internal pull-up resistors enabled
  pinMode(PIN_ENCODER_A, INPUT);
  pinMode(PIN_ENCODER_B, INPUT);
  digitalWrite(PIN_ENCODER_A, HIGH);
  digitalWrite(PIN_ENCODER_B, HIGH);
  
  currentTime = millis();
  loopTime = currentTime;
}

void loop()
{
  currentTime = millis();
  if (currentTime >= (loopTime + 5))
  {
    encoder_A = digitalRead(PIN_ENCODER_A);
    encoder_B = digitalRead(PIN_ENCODER_B);
    if ((!encoder_A) && (encoder_A_prev))
    {
      if (encoder_B)
      {
        // add a number
        justinTestNumber += 0.1;
        if (justinTestNumber > 108.0)
        {
          justinTestNumber = 87.5;
        }
      }
      else
      {
        // subtract a number
        justinTestNumber -= 0.1;
        if (justinTestNumber < 87.5)
        {
          justinTestNumber = 108.0;
        }
      }
    }
    
    encoder_A_prev = encoder_A;
    matrix.print(justinTestNumber);
    matrix.writeDisplay();
  }
}

//87.5-108
