#include <SoftwareSerial.h>

// Possible MIDI PC commands that could be sent
#define CLEAN_CHANNEL_MARSHALL 1
#define CLEAN_CHANNEL_FENDER 2
#define OVERDRIVE_CHANNEL_VINTAGE 3
#define OVERDRIVE_CHANNEL_MODERN 4
#define RESET_SETTINGS 5

// Possible Voicings
#define CLEAN_FENDER 6
#define CLEAN_MARSHALL 7
#define OVERDRIVE_VINTAGE 8
#define OVERDRIVE_MODERN 9

// Possible channels
#define CLEAN_CHANNEL 10
#define OVERDRIVE_CHANNEL 11

// MIDI constants
#define MIDI_LISTEN_CHANNEL 8      // MIDI channel to listen on - We listen on MIDI Channel 9, but is zero-indexed so we look for 8.  MIDI controller should send commands on channel 9!
#define MIDI_PROGRAM_CHANGE 192   // The number that represents a MIDI PC (Program Change) command

// Arduino PIN definitions
#define CHANNEL_RELAY_PIN 8         // Connect to IN1 on 2-relay board, controls channel switching (Tip+Sleeve) - Relay connected as NO - Middle and Right screw terminals
#define VOICE_RELAY_PIN 9           // Connect to IN2 on 2-relay board, controls voice switching (Ring+Sleeve) - Relay connected as NO - Middle and Right screw terminals
#define MIDI_RECEIVE_PIN 2          // Used for the Arduino to receive data from the MIDI communication link
#define MIDI_TRANSFER_PIN 3         // Used for the Arduino to transfer data to the MIDI communication link (UNUSED)
#define LED_PIN 5

// state management
int currentChannel;
int currentCleanVoicing;
int currentOverdriveVoicing;

// Incoming MIDI values
byte midiByte;
byte midiChannel;
byte midiCommand;
byte programChangeByte;

// Setup the MIDI serial connection
SoftwareSerial midiSerial(MIDI_RECEIVE_PIN, MIDI_TRANSFER_PIN); // RX, TX

void setup() {
  // setup relay pins and LED pin as OUTPUT
  pinMode(CHANNEL_RELAY_PIN, OUTPUT);
  pinMode(VOICE_RELAY_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  // set default values for state variables and pull relay pins to LOW
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

      bool wasValidProgramChange = true;
      
      Serial.println("Program Change Byte:");
      Serial.println(programChangeByte);
      Serial.println("Current State Values:");
      Serial.println("--------------------------");
      Serial.println("currentChannel:");
      Serial.println(currentChannel);
      Serial.println("currentCleanVoicing:");
      Serial.println(currentCleanVoicing);
      Serial.println("currentOverdriveVoicing:");
      Serial.println(currentOverdriveVoicing);

      bool switchChannel = false;
      bool switchVoicing = false;
      bool isResetCommand = false;

      switch (programChangeByte) {
        case CLEAN_CHANNEL_MARSHALL: {
          Serial.println("Processing CLEAN_CHANNEL_MARSHALL program change...");
          if (currentChannel == CLEAN_CHANNEL && currentCleanVoicing == CLEAN_FENDER) {
            // just need to switch voicing
            switchVoicing = true;
            currentCleanVoicing = CLEAN_MARSHALL;
          }
          else if (currentChannel == OVERDRIVE_CHANNEL) {
            // definitely need to change channel
            switchChannel = true;
            currentChannel = CLEAN_CHANNEL;

            // MIGHT need to switch clean voicing
            if (currentCleanVoicing == CLEAN_FENDER) {
              // switch voicing
              switchVoicing = true;
              currentCleanVoicing = CLEAN_MARSHALL;
            }
          }
          
          break;
        }
        case CLEAN_CHANNEL_FENDER: {
          Serial.println("Processing CLEAN_CHANNEL_FENDER program change...");
          if (currentChannel == CLEAN_CHANNEL && currentCleanVoicing == CLEAN_MARSHALL) {
            // just need to switch voicing
            switchVoicing = true;
            currentCleanVoicing = CLEAN_FENDER;
          }
          else if (currentChannel == OVERDRIVE_CHANNEL) {
            // definitely need to change channel
            switchChannel = true;
            currentChannel = CLEAN_CHANNEL;

            // MIGHT need to switch clean voicing
            if (currentCleanVoicing == CLEAN_MARSHALL) {
              // switch voicing
              switchVoicing = true;
              currentCleanVoicing = CLEAN_FENDER;
            }
          }

          break;
        }
        case OVERDRIVE_CHANNEL_VINTAGE: {
          Serial.println("Processing OVERDRIVE_CHANNEL_VINTAGE program change...");
          if (currentChannel == OVERDRIVE_CHANNEL && currentOverdriveVoicing == OVERDRIVE_MODERN) {
            // just need to switch voicing
            switchVoicing = true;
            currentOverdriveVoicing = OVERDRIVE_VINTAGE;
          }
          else if (currentChannel == CLEAN_CHANNEL) {
            // definitely need to change channel
            switchChannel = true;
            currentChannel = OVERDRIVE_CHANNEL;

            // MIGHT need to switch overdrive voicing
            if (currentOverdriveVoicing == OVERDRIVE_MODERN) {
              // switch voicing
              switchVoicing = true;
              currentOverdriveVoicing = OVERDRIVE_VINTAGE;
            }
          }

          break;
        }
        case OVERDRIVE_CHANNEL_MODERN: {
          Serial.println("Processing OVERDRIVE_CHANNEL_MODERN program change...");          
          if (currentChannel == OVERDRIVE_CHANNEL && currentOverdriveVoicing == OVERDRIVE_VINTAGE) {
            // just need to switch voicing
            switchVoicing = true;
            currentOverdriveVoicing = OVERDRIVE_MODERN;
          }
          else if (currentChannel == CLEAN_CHANNEL) {
            // definitely need to change channel
            switchChannel = true;
            currentChannel = OVERDRIVE_CHANNEL;

            // MIGHT need to switch overdrive voicing
            if (currentOverdriveVoicing == OVERDRIVE_VINTAGE) {
              // switch voicing
              switchVoicing = true;
              currentOverdriveVoicing = OVERDRIVE_MODERN;
            }
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
      toggleRelays(switchChannel, switchVoicing);
    }

    if (wasValidProgramChange) {
      blinkLed(programChangeByte);
    }
  }
 }
}

void toggleRelays(bool switchChannel, bool switchVoicing) {
  // ALWAYS SWITCH CHANNEL BEFORE SWITCHING VOICING!
  if (switchChannel) {
    // switch the channel relay to be the opposite of what it currently is
    Serial.println("Switching channel relay");
    digitalWrite(CHANNEL_RELAY_PIN, !digitalRead(CHANNEL_RELAY_PIN));

    if (switchVoicing) {
      // if we're going to switch voicing next, put a short delay to ensure the channel successfully switches on the amp first!
      delay(300);
    }
  }

  if (switchVoicing) {
    // switch the voicing relay to be the opposite of what it currently is
    Serial.println("Switching voice relay");
    digitalWrite(VOICE_RELAY_PIN, !digitalRead(VOICE_RELAY_PIN));
  }
}

void initializeDefaultState(bool fromProgramChange) {
  // When the amp first starts up, we expect it to be in this state
    // The OD channel should have the "Vintage" voicing enabled (NO red LED)
    // The Clean channel should have the "Fender" voicing enabled (Green LED on)
    // The current selected channel should be the "Clean" channel

  if (fromProgramChange) {
    // get the voicing correct on whatever channel we're currently on
    if (currentChannel == CLEAN_CHANNEL && currentCleanVoicing != CLEAN_FENDER) {
      // switch voicing
      currentCleanVoicing = CLEAN_FENDER;
      toggleRelays(false, true);
    }
    else if (currentChannel == OVERDRIVE_CHANNEL && currentOverdriveVoicing != OVERDRIVE_VINTAGE) {
      // switch voicing
      currentOverdriveVoicing = OVERDRIVE_VINTAGE;
      toggleRelays(false, true);
    }

    delay(100);

    // now we now whatever channel we're on is correct.  See if we have to change the other channels voicing
    if (currentChannel == CLEAN_CHANNEL && currentOverdriveVoicing != OVERDRIVE_VINTAGE) {
      // need to switch to OD channel, and then switch the vintage voicing
      currentChannel = OVERDRIVE_CHANNEL;
      currentOverdriveVoicing = OVERDRIVE_VINTAGE;
      toggleRelays(true, true);
    }
    else if (currentChannel == OVERDRIVE_CHANNEL && currentCleanVoicing != CLEAN_FENDER) {
      // need to switch to clean channel, and then switch the fender voicing
      currentChannel = CLEAN_CHANNEL;
      currentCleanVoicing = CLEAN_FENDER;
      toggleRelays(true, true);
    }

    // now both channels have the correct voicing - but we have to make sure we're on the clean channel
    if (currentChannel != CLEAN_CHANNEL) {
      currentChannel = CLEAN_CHANNEL;
      toggleRelays(true, false);
    }
  }
  else {
    // just initialize the values (this would only get called on program startup)
    currentChannel = CLEAN_CHANNEL;
    currentCleanVoicing = CLEAN_FENDER;
    currentOverdriveVoicing = OVERDRIVE_VINTAGE;
    
    // start with both relays disengaged
    digitalWrite(CHANNEL_RELAY_PIN, LOW);
    digitalWrite(VOICE_RELAY_PIN, LOW);
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
