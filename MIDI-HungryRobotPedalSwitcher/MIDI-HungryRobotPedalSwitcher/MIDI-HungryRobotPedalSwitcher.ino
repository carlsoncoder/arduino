#include <SoftwareSerial.h>

// Possible MIDI PC commands that could be sent
#define ALL_OFF 1
#define LOW_GAIN_ONLY 2
#define HIGH_GAIN_ONLY 3
#define BOTH_HIGH_AND_LOW_GAIN 4
#define RESET_SETTINGS 5

// MIDI constants
#define MIDI_LISTEN_CHANNEL 7      // MIDI channel to listen on - We listen on MIDI Channel 8, but is zero-indexed so we look for 7.  MIDI controller should send commands on channel 8!
#define MIDI_PROGRAM_CHANGE 192   // The number that represents a MIDI PC (Program Change) command

// Arduino PIN definitions
// TODO: JUSTIN: FIGURE OUT HOW MANY RELAY PINS WE NEED!!!
//#define CHANNEL_RELAY_PIN 8         // Connect to IN1 on 2-relay board, controls channel switching (Tip+Sleeve) - Relay connected as NO - Middle and Right screw terminals
//#define VOICE_RELAY_PIN 9           // Connect to IN2 on 2-relay board, controls voice switching (Ring+Sleeve) - Relay connected as NO - Middle and Right screw terminals
#define MIDI_RECEIVE_PIN 2          // Used for the Arduino to receive data from the MIDI-IN port
#define MIDI_TRANSFER_PIN 3         // Used for the Arduino to transfer data to the MIDI-THRU port
#define LED_PIN 5

// state management
bool lowGainOn = false;
bool highGainOn = false;

// Incoming MIDI values
byte midiByte;
byte midiChannel;
byte midiCommand;
byte programChangeByte;

// Setup the MIDI serial connection
SoftwareSerial midiSerial(MIDI_RECEIVE_PIN, MIDI_TRANSFER_PIN); // RX, TX

void setup() {
  // setup relay pins and LED pin as OUTPUT
  // TODO: JUSTIN: FIGURE OUT WHAT PINS FOR WHAT RELAYS
  //pinMode(CHANNEL_RELAY_PIN, OUTPUT);
  //pinMode(VOICE_RELAY_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  // set default values for state variables and pull relay pins to appropriate starting points
  initializeDefaultState(false);

  // set the LED to on - this is our "power" / "MIDI stuff" LED
  digitalWrite(LED_PIN, HIGH);

  // setup SoftSerial for MIDI control
  midiSerial.begin(31250);
  delay(250); // give MIDI-device a short time to "digest" MIDI messages

  // Setup our regular serial channel to use for debugging messages
  Serial.begin(9600);
  Serial.println();
  Serial.println("MIDI Connection Active!");
}

void loop() {
  if (midiSerial.available() > 0) {
    
    // read MIDI byte
    midiByte = midiSerial.read();

    // immediately write the byte to the MIDI-THRU port
    midiSerial.write(midiByte);
    
    Serial.println("RAW MIDI Byte");
    Serial.println(midiByte);
    
    // remove channel info
    midiChannel = midiByte & B00001111;
    midiCommand = midiByte & B11110000;

    Serial.println("Channel:");
    Serial.println(midiChannel);
    Serial.println("Command:");
    Serial.println(midiCommand);

    // We only want to process Program Change (PC) commands sent specifically to our channel
    if (midiChannel == MIDI_LISTEN_CHANNEL && midiCommand == MIDI_PROGRAM_CHANGE) {
      
      // read the next immediate byte to get what PC we should execute
      programChangeByte = midiSerial.read();

      // immediately write the byte to the MIDI-THRU port
      midiSerial.write(programChangeByte);

      bool wasValidProgramChange = true;
      
      Serial.println("Program Change Byte:");
      Serial.println(programChangeByte);
      Serial.println("Current State Values:");
      Serial.println("--------------------------");
      Serial.println("lowGainOn:");
      Serial.println(lowGainOn);
      Serial.println("highGainOn:");
      Serial.println(highGainOn);
      
      bool switchLowGain = false;
      bool switchHighGain = false;
      bool isResetCommand = false;

      switch (programChangeByte) {
        case ALL_OFF: {
          Serial.println("Processing ALL_OFF program change...");
          if (lowGainOn) {
            lowGainOn = false;
            switchLowGain = true;
          }

          if (highGainOn) {
            highGainOn = false;
            switchHighGain = true;
          }
          
          break;
        }
        case LOW_GAIN_ONLY: {
          Serial.println("Processing LOW_GAIN_ONLY program change...");
          if (!lowGainOn) {
            lowGainOn = true;
            switchLowGain = true;
          }

          if (highGainOn) {
            highGainOn = false;
            switchHighGain = true;
          }
          
          break;
        }
        case HIGH_GAIN_ONLY: {
          Serial.println("Processing HIGH_GAIN_ONLY program change...");
          if (lowGainOn) {
            lowGainOn = false;
            switchLowGain = true;
          }

          if (!highGainOn) {
            highGainOn = true;
            switchHighGain = true;
          }

          break;
        }
        case BOTH_HIGH_AND_LOW_GAIN: {
          Serial.println("Processing BOTH_HIGH_AND_LOW_GAIN program change...");
          if (!lowGainOn) {
            lowGainOn = true;
            switchLowGain = true;
          }

          if (!highGainOn) {
            highGainOn = true;
            switchHighGain = true;
          }
          
          break;
        }
        case RESET_SETTINGS: {
          Serial.println("Processing RESET_SETTINGS program change...");          
          isResetCommand = true;
          break;
        }
        default: {
          wasValidProgramChange = false;
        }
    }
    
    if (isResetCommand) {
      initializeDefaultState(true);
    }
    else {
      toggleRelays(switchLowGain, switchHighGain);
    }

    if (wasValidProgramChange) {
      blinkLed(programChangeByte);
    }
  }
 }
}

void toggleRelays(bool switchLowGain, bool switchHighGain) {
  if (switchLowGain) {
    Serial.println("Switching low gain");
    // TODO: JUSTIN: FIGURE OUT WHAT PIN/RELAY TO USE
    //digitalWrite(CHANNEL_RELAY_PIN, !digitalRead(CHANNEL_RELAY_PIN));
    delay(5);
  }

  if (switchHighGain) {
    Serial.println("Switching high gain");
    // TODO: JUSTIN: FIGURE OUT WHAT PIN/RELAY TO USE
    //digitalWrite(CHANNEL_RELAY_PIN, !digitalRead(CHANNEL_RELAY_PIN));
    delay(5);
  }
}

void initializeDefaultState(bool fromProgramChange) {
  // When the pedal first starts up, we put it in the "Low Gain Only" state - that is also what the "RESET_SETTINGS" command should do

  if (fromProgramChange) {
    bool switchLowGain = false;
    bool switchHighGain = false;

    if (!lowGainOn) {
      lowGainOn = true;
      switchLowGain = true;
    }

    if (highGainOn) {
      highGainOn = false;
      switchHighGain = true;
    }

    toggleRelays(switchLowGain, switchHighGain);
  }
  else {
    // just initialize the values (this would only get called on program startup)
    lowGainOn = true;
    highGainOn = false;

    // TODO: JUSTIN: FIGURE OUT HOW TO SET LOW GAIN RELAY PIN TO HIGH AND HIGH GAIN RELAY PIN TO LOW!
    //digitalWrite(CHANNEL_RELAY_PIN, LOW);
    //digitalWrite(VOICE_RELAY_PIN, LOW);
  }
}

void blinkLed(byte programChangeNumber) {
  for (int i = 0; i < programChangeNumber; i++) {
    digitalWrite(LED_PIN, LOW);
    delay(250);
    digitalWrite(LED_PIN, HIGH);
    delay(250);
  }

  // always set the LED back to on (HIGH)
  digitalWrite(LED_PIN, HIGH);
}
