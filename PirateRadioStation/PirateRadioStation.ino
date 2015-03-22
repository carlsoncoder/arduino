// includes
#include <Wire.h>
#include <Adafruit_Si4713.h>
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"

// constant definitions
const int RESETPIN = 12;
const int PIN_ENCODER_A = 4;
const int PIN_ENCODER_B = 5;

// local variables
int FMSTATION = 8920;     // 10230 == 102.30 MHz
int pendingFMStation = 8920;
int timer1_counter;
boolean isFrequencyChanging = false;
boolean hasFrequencyChanged = false;
unsigned char encoder_A;
unsigned char encoder_B;
unsigned char encoder_A_prev = 0;
unsigned long currentTime = 0;
unsigned long frequencyLastUpdatedTime = 0;

Adafruit_7segment matrix = Adafruit_7segment();
Adafruit_Si4713 radio = Adafruit_Si4713(RESETPIN);

// methods
void setup()
{
  Serial.begin(9600);
  Serial.println("Justin's Pirate Radio Project");

  // set timers - initialize timer1 
  noInterrupts();           // disable all interrupts
  TCCR1A = 0;
  TCCR1B = 0;
  
  int hertz = 200;

  // Set timer1_counter to the correct value for our interrupt interval
  timer1_counter = 65536 - (12000000 / 256 / hertz);
  
  TCNT1 = timer1_counter;   // preload timer
  TCCR1B |= (1 << CS12);    // 256 prescaler 
  TIMSK1 |= (1 << TOIE1);   // enable timer overflow interrupt
  interrupts();             // enable all interrupts
  
  // rotary encoder - set pins as input with internal pull-up resistors enabled
  pinMode(PIN_ENCODER_A, INPUT);
  pinMode(PIN_ENCODER_B, INPUT);
  digitalWrite(PIN_ENCODER_A, HIGH);
  digitalWrite(PIN_ENCODER_B, HIGH);
  
  if (!radio.begin())
  {
    // begin with address 0x63 (CS high default)
    Serial.println("Couldn't find radio?");
    while (1);
  }
  
  // start the Si4713 radio
  tuneRadio();
  
  // start the 7-segment matrix
  matrix.begin(0x70);
  
  // print a floating point value to the matrix
  writeFrequencyToMatrix(FMSTATION);
  
  currentTime = millis();
  frequencyLastUpdatedTime = millis();
}

ISR(TIMER1_OVF_vect)        // interrupt service routine 
{
  TCNT1 = timer1_counter;   // preload timer
  currentTime = millis();
  
  boolean didChangeFrequency = false;
  
  encoder_A = digitalRead(PIN_ENCODER_A);
  encoder_B = digitalRead(PIN_ENCODER_B);
  if ((!encoder_A) && (encoder_A_prev))
  {
    isFrequencyChanging = true;
    didChangeFrequency = true;
    
    if (encoder_B)
    {
      // increment by one
      pendingFMStation += 10;
      if (pendingFMStation > 10800)
      {
        pendingFMStation = 8750;
      }
    }
    else
    {
      // decrement by one
      pendingFMStation -= 10;
      if (pendingFMStation < 8750)
      {
        pendingFMStation = 10800;
      }
    }
  }
  
  encoder_A_prev = encoder_A;
  
  if (didChangeFrequency)
  {
    writeFrequencyToMatrix(pendingFMStation);
    frequencyLastUpdatedTime = millis();
  }
  else if (isFrequencyChanging && (currentTime >= (frequencyLastUpdatedTime + 2000)))
  {
    // it's been two seconds since the user modified the frequency, time to reset
    hasFrequencyChanged = true;
  }
}

void tuneRadio()
{
  radio.setTXpower(115);
  radio.tuneFM(FMSTATION);
  
  // begin the RDS/RDBS transmission
  radio.beginRDS();
  radio.setRDSstation("JCPirate");
  radio.setRDSbuffer( "Pirate Radio!");

  radio.setGPIOctrl(_BV(1) | _BV(2));  // set GP1 and GP2 to output
}

void writeFrequencyToMatrix(int frequencyToWrite)
{
  if (isFrequencyChanging)
  {
    matrix.blinkRate(2);
  }
  
  // print a floating point value to the matrix
  float selectedFrequencyFloat = frequencyToWrite / 100;
  matrix.print(selectedFrequencyFloat);
  matrix.writeDisplay();
}

void loop()
{
  if (isFrequencyChanging)
  {
    // intentionally do nothing
  }
  else if (hasFrequencyChanged)
  {
    // reset variables
    isFrequencyChanging = false;
    hasFrequencyChanged = false;
    
    FMSTATION = pendingFMStation;
    
    matrix.blinkRate(0);
    
    // need to retune the radio
    radio.reset();
    tuneRadio();
  }
  else
  {
    radio.readASQ();
    
    // toggle GPO1 and GPO2
    radio.setGPIO(_BV(1));
    delay(500);
    radio.setGPIO(_BV(2));
    delay(500);
  }
}
