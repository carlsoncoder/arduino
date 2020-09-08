#include <SPI.h>
#include <Wire.h>
#include <EEPROM.h>
#include <ReceiveOnlySoftwareSerial.h>  //http://gammon.com.au/Arduino/ReceiveOnlySoftwareSerial.zip

// MIDI constants
#define DEFAULT_MIDI_CHANNEL        8           // By default, we listen on MIDI channel 8, unless otherwise specified
#define MIDI_PROGRAM_CHANGE         192         // The number that represents a MIDI PC (Program Change) command
#define MIDI_CONTROL_CHANGE         176         // The number that represents a MIDI CC (Control Change) command

// Atmega 328p PIN definitions
#define MIDI_RECEIVE_PIN            5           // Used to receive data from the MIDI-IN port
#define MIDI_STATUS_PIN       3           // Used to read the state of the footswitch for turning the effect on/off

// Constants used elsewhere
#define MIDI_WAIT_TIME              250         // Delay time to use when starting up the MIDI ReceiveOnlySoftwareSerial channel

// state management
byte expectedMidiChannel = 7;


// Incoming MIDI values
byte midiByte;
byte midiChannel;
byte midiCommand;
byte midiProgramChangeNumberByte;
byte midiControlChangeNumberByte;
byte midiControlChangeValueByte;

// Setup the MIDI serial connection
ReceiveOnlySoftwareSerial midiSerial(MIDI_RECEIVE_PIN);

// Setup function - initialize all pins, libraries, and state variables, as well as start up the MIDI serial connection
void setup() {
  
  // Setup all LED pins as OUTPUT
  pinMode(MIDI_STATUS_PIN, OUTPUT);

  // blink LED three times on startup - know that it's working
  blinkLed(3);

  Serial.begin(9600);
  
  // setup ReceiveOnlySoftwareSerial for MIDI control
  midiSerial.begin(31250);
  delay(MIDI_WAIT_TIME); // give MIDI-device a short time to "digest" MIDI messages
}


// Reads the serial MIDI channel and acts on any messages received
void processMidiCommands() {
  if (midiSerial.available() > 0) {
    
    // read MIDI byte
    midiByte = midiSerial.read();
    
    // parse channel and command values
    midiChannel = midiByte & B00001111;
    midiCommand = midiByte & B11110000;

    Serial.println("CHANNEL: ");
    Serial.println(midiChannel);
    Serial.println("COMMAND: ");
    Serial.println(midiCommand);

    // Only move forward if the channel is ours
    if (midiChannel == expectedMidiChannel) {
      
      // process PC and CC commands differently
      if (midiCommand == MIDI_PROGRAM_CHANGE) {
        
        // read the PC number byte
        midiProgramChangeNumberByte = midiSerial.read();        
        Serial.println("PC NUMBER: ");
        Serial.println(midiProgramChangeNumberByte);
        //blinkLed(midiProgramChangeNumberByte);
        // Just blink 3 times for now so we know we got SOME command
        blinkLed(3);
      }
    }
 }
}

void blinkLed(int presetNumber) {
  digitalWrite(MIDI_STATUS_PIN, LOW);

  for (int i = 0; i < presetNumber; i++) {
    digitalWrite(MIDI_STATUS_PIN, HIGH);
    delay(250);

    digitalWrite(MIDI_STATUS_PIN, LOW);
    delay(250);
  }

  digitalWrite(MIDI_STATUS_PIN, LOW);
}


// Standard program loop
void loop() {
  processMidiCommands();
}
