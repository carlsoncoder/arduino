#include <ReceiveOnlySoftwareSerial.h> // Available at http://gammon.com.au/Arduino/ReceiveOnlySoftwareSerial.zip

// Possible MIDI PC commands that could be sent
#define CLEAN_CHANNEL_MARSHALL                1
#define CLEAN_CHANNEL_FENDER                  2
#define OVERDRIVE_CHANNEL_VINTAGE             3
#define OVERDRIVE_CHANNEL_MODERN              4
#define KOKO_BOOST_OFF                        5
#define KOKO_BOOST_CLEAN                      6
#define KOKO_BOOST_MIDS                       7
#define TUNER_ENGAGED                         8
#define TUNER_DISENGAGED                      9

// Possible amp voicings
#define CLEAN_MARSHALL                        1
#define CLEAN_FENDER                          2
#define OVERDRIVE_VINTAGE                     3
#define OVERDRIVE_MODERN                      4

// Possible amp channels
#define CLEAN_CHANNEL                         1
#define OVERDRIVE_CHANNEL                     2

// Possible Koko Boost states
#define STATE_KOKO_BOOST_OFF                  1
#define STATE_KOKO_BOOST_CLEAN                2
#define STATE_KOKO_BOOST_MIDS                 3

// MIDI constants
#define MIDI_WAIT_TIME                        125
#define MIDI_LISTEN_CHANNEL                   8      // MIDI channel to listen on - We listen on MIDI Channel 9, but is zero-indexed so we look for 8.  MIDI controller should send commands on channel 9!
#define MIDI_PROGRAM_CHANGE                   192    // The number that represents a MIDI PC (Program Change) command

/* Other constants */
#define PIN_PULSE_LENGTH                      3
#define DELAY_BETWEEN_RING_TIP_PULSE          25

/* Arduino PIN definitions */
#define MIDI_RECEIVE_PIN                      11    // Used for the Arduino to receive data from the MIDI-IN port

// PIN definitions for the two relays that control the amp channel switching
#define CHANNEL_TQRELAY_POSITIVE_PIN          3     // Connect this pin (3) on Arduino to the "+" pin (Pin 10) on the TQ2-L-5V relay controlling the channel - Connect COM and NO (no continuity) (Pins 7&8 or 3&4 on relay) to Sleeve & Tip on TRS output jack
#define CHANNEL_TQRELAY_NEGATIVE_PIN          4     // Connect this pin (4) on Arduino to the "-" pin (Pin 1) on the TQ2-L-5V relay controlling the channel - Connect COM and NO (no continuity) (Pins 7&8 or 3&4 on relay) to Sleeve & Tip on TRS output jack
#define VOICE_TQRELAY_POSITIVE_PIN            5     // Connect this pin (5) on Arduino to the "+" pin (Pin 10) on the TQ2-L-5V relay controlling the voicing - Connect COM and NO (no continuity) (Pins 7&8 or 3&4 on relay) to Sleeve & Ring on TRS output jack
#define VOICE_TQRELAY_NEGATIVE_PIN            6     // Connect this pin (6) on Arduino to the "-" pin (Pin 1) on the TQ2-L-5V relay controlling the voicing - Connect COM and NO (no continuity) (Pins 7&8 or 3&4 on relay) to Sleeve & Ring on TRS output jack

// PIN definitions for the two relays that control the Koko Boost switching
#define KOKO_CLEANBOOST_TQRELAY_POSITIVE_PIN  7     // Connect this pin (7) on Arduino to the "+" pin (Pin 10) on the TQ2-L-5V relay controlling the Koko Boost Clean Boost - Connect COM and NO (no continuity) (Pins 7&8 or 3&4 on relay) to Sleeve & Tip on TRS output jack
#define KOKO_CLEANBOOST_TQRELAY_NEGATIVE_PIN  8     // Connect this pin (8) on Arduino to the "-" pin (Pin 1) on the TQ2-L-5V relay controlling the Koko Boost Clean Boost - Connect COM and NO (no continuity) (Pins 7&8 or 3&4 on relay) to Sleeve & Tip on TRS output jack
#define KOKO_MIDBOOST_TQRELAY_POSITIVE_PIN    9     // Connect this pin (9) on Arduino to the "+" pin (Pin 10) on the TQ2-L-5V relay controlling the Koko Boost Mid Boost - Connect COM and NO (no continuity) (Pins 7&8 or 3&4 on relay) to Sleeve & Ring on TRS output jack
#define KOKO_MIDBOOST_TQRELAY_NEGATIVE_PIN    10    // Connect this pin (10) on Arduino to the "-" pin (Pin 1) on the TQ2-L-5V relay controlling the Koko Boost Mid Boost - Connect COM and NO (no continuity) (Pins 7&8 or 3&4 on relay) to Sleeve & Ring on TRS output jack

// state management
int currentAmpChannel;
int currentAmpCleanVoicing;
int currentAmpOverdriveVoicing;
int currentKokoBoostState;
int savedKokoBoostStateForTunerReset;
bool isChannelRelayInResetMode;
bool isVoiceRelayInResetMode;
bool isCleanBoostRelayInResetMode;
bool isMidBoostRelayInResetMode;

// Incoming MIDI values
byte midiByte;
byte midiChannel;
byte midiCommand;
byte programChangeByte;

// Setup the MIDI serial connection
ReceiveOnlySoftwareSerial midiSerial(MIDI_RECEIVE_PIN);

// NOTES
  // When the amp first starts up, we expect it to be in the following state
  // The OD channel should have the "Vintage" voicing enabled (NO red LED)
  // The Clean channel should have the "Fender" voicing enabled (Green LED on)
  // The current selected channel should be the "Clean" channel

// NOTES
  // Tested with Port 4 on MC8 MIDI Controller, Tip to Resistor/Ring to Diode
      
void setup() {
  // set all relay pins to be OUTPUT
  pinMode(CHANNEL_TQRELAY_POSITIVE_PIN, OUTPUT);
  pinMode(CHANNEL_TQRELAY_NEGATIVE_PIN, OUTPUT);
  pinMode(VOICE_TQRELAY_POSITIVE_PIN, OUTPUT);
  pinMode(VOICE_TQRELAY_NEGATIVE_PIN, OUTPUT);
  pinMode(KOKO_CLEANBOOST_TQRELAY_POSITIVE_PIN, OUTPUT);
  pinMode(KOKO_CLEANBOOST_TQRELAY_NEGATIVE_PIN, OUTPUT);
  pinMode(KOKO_MIDBOOST_TQRELAY_POSITIVE_PIN, OUTPUT);
  pinMode(KOKO_MIDBOOST_TQRELAY_NEGATIVE_PIN, OUTPUT);

  // set default values for state variables
  currentAmpChannel = CLEAN_CHANNEL;
  currentAmpCleanVoicing = CLEAN_FENDER;
  currentAmpOverdriveVoicing = OVERDRIVE_VINTAGE;
  currentKokoBoostState = STATE_KOKO_BOOST_OFF;
  isChannelRelayInResetMode = false;
  isVoiceRelayInResetMode = false;
  isCleanBoostRelayInResetMode = false;
  isMidBoostRelayInResetMode = false;
  savedKokoBoostStateForTunerReset = -1;

  // setup ReceiveOnlySoftwareSerial for MIDI control
  midiSerial.begin(31250);
  delay(MIDI_WAIT_TIME); // give MIDI-device a short time to "digest" MIDI messages

  // Setup our regular serial channel to use for debugging messages
  Serial.begin(9600);
  Serial.println();
  Serial.println("MIDI Connection Active!");
}

void handleAmpSwitcherProgramChange(byte programChangeByte) {
  bool wasValidProgramChange = true;
  bool switchChannel = false;
  bool switchVoicing = false;

  switch (programChangeByte) {
    case CLEAN_CHANNEL_MARSHALL: {
      if (currentAmpChannel == CLEAN_CHANNEL && currentAmpCleanVoicing == CLEAN_FENDER) {
        // just need to switch voicing
        switchVoicing = true;
        currentAmpCleanVoicing = CLEAN_MARSHALL;
      }
      else if (currentAmpChannel == OVERDRIVE_CHANNEL) {
        // definitely need to change channel
        switchChannel = true;
        currentAmpChannel = CLEAN_CHANNEL;

        // MIGHT need to switch clean voicing
        if (currentAmpCleanVoicing == CLEAN_FENDER) {
          // switch voicing
          switchVoicing = true;
          currentAmpCleanVoicing = CLEAN_MARSHALL;
        }
      }
          
      break;
    }
    case CLEAN_CHANNEL_FENDER: {
      if (currentAmpChannel == CLEAN_CHANNEL && currentAmpCleanVoicing == CLEAN_MARSHALL) {
        // just need to switch voicing
        switchVoicing = true;
        currentAmpCleanVoicing = CLEAN_FENDER;
      }
      else if (currentAmpChannel == OVERDRIVE_CHANNEL) {
        // definitely need to change channel
        switchChannel = true;
        currentAmpChannel = CLEAN_CHANNEL;

        // MIGHT need to switch clean voicing
        if (currentAmpCleanVoicing == CLEAN_MARSHALL) {
          // switch voicing
          switchVoicing = true;
          currentAmpCleanVoicing = CLEAN_FENDER;
        }
      }

      break;
    }
    case OVERDRIVE_CHANNEL_VINTAGE: {
      if (currentAmpChannel == OVERDRIVE_CHANNEL && currentAmpOverdriveVoicing == OVERDRIVE_MODERN) {
        // just need to switch voicing
        switchVoicing = true;
        currentAmpOverdriveVoicing = OVERDRIVE_VINTAGE;
      }
      else if (currentAmpChannel == CLEAN_CHANNEL) {
        // definitely need to change channel
        switchChannel = true;
        currentAmpChannel = OVERDRIVE_CHANNEL;

        // MIGHT need to switch overdrive voicing
        if (currentAmpOverdriveVoicing == OVERDRIVE_MODERN) {
          // switch voicing
          switchVoicing = true;
          currentAmpOverdriveVoicing = OVERDRIVE_VINTAGE;
        }
      }

      break;
    }
    case OVERDRIVE_CHANNEL_MODERN: {
      if (currentAmpChannel == OVERDRIVE_CHANNEL && currentAmpOverdriveVoicing == OVERDRIVE_VINTAGE) {
        // just need to switch voicing
        switchVoicing = true;
        currentAmpOverdriveVoicing = OVERDRIVE_MODERN;
      }
      else if (currentAmpChannel == CLEAN_CHANNEL) {
        // definitely need to change channel
        switchChannel = true;
        currentAmpChannel = OVERDRIVE_CHANNEL;

        // MIGHT need to switch overdrive voicing
        if (currentAmpOverdriveVoicing == OVERDRIVE_VINTAGE) {
          // switch voicing
          switchVoicing = true;
          currentAmpOverdriveVoicing = OVERDRIVE_MODERN;
        }
      }

      break;
    }
    default: {
      wasValidProgramChange = false;
    }
  }
  
  if (wasValidProgramChange) {
    setRelaysForAmpChannelVoiceSwitch(switchChannel, switchVoicing);
  }
}

void setRelaysForAmpChannelVoiceSwitch(bool switchChannel, bool switchVoicing) {
  // ALWAYS SWITCH CHANNEL BEFORE SWITCHING VOICING!
  if (switchChannel) {
    toggleAmpChannelRelay();
    
    if (switchVoicing) {
      // if we're going to switch voicing next, put a short delay to ensure the channel successfully switches on the amp first!
      delay(MIDI_WAIT_TIME);
    }
  }

  if (switchVoicing) {
    toggleAmpVoiceRelay();
  }
}


void toggleCleanBoostRelay() {
  Serial.println("Toggling clean boost relay");
  toggleRelay(isCleanBoostRelayInResetMode, KOKO_CLEANBOOST_TQRELAY_NEGATIVE_PIN, KOKO_CLEANBOOST_TQRELAY_POSITIVE_PIN);
}

void toggleMidBoostRelay() {
  Serial.println("Toggling mid boost relay");
  toggleRelay(isMidBoostRelayInResetMode, KOKO_MIDBOOST_TQRELAY_NEGATIVE_PIN, KOKO_MIDBOOST_TQRELAY_POSITIVE_PIN);
}

void toggleAmpChannelRelay() {
  Serial.println("Toggling amp channel relay");
  toggleRelay(isChannelRelayInResetMode, CHANNEL_TQRELAY_NEGATIVE_PIN, CHANNEL_TQRELAY_POSITIVE_PIN);
}

void toggleAmpVoiceRelay() {
  Serial.println("Toggling amp voice relay");
  toggleRelay(isVoiceRelayInResetMode, VOICE_TQRELAY_NEGATIVE_PIN, VOICE_TQRELAY_POSITIVE_PIN);
}

void toggleRelay(boolean predicate, bool truePin, bool falsePin) {
  // determine polarity (which pin to pulse on relay)
  int pinToPulse = predicate ? truePin : falsePin;

  // pulse the pin
  digitalWrite(pinToPulse, HIGH);
  delay(PIN_PULSE_LENGTH);
  digitalWrite(pinToPulse, LOW);

  // set the state of the predicate boolean to the opposite of what it currently is
  predicate = !predicate;
}
 
void setRelaysForKokoBoostModeSwitch(bool cleanBoostOffOperation, bool midBoostOffOperation, bool cleanBoostOnOperation, bool midBoostOnOperation) {
  if (!cleanBoostOffOperation && !midBoostOffOperation && !cleanBoostOnOperation && !midBoostOnOperation) {
    // nothing to do
    return;
  }
  
  // Perform "Off" operations first
  if (cleanBoostOffOperation) {
    toggleCleanBoostRelay();
  }
  else if (midBoostOffOperation) {
    toggleMidBoostRelay();
  }

  // check if a delay is needed
  bool delayNeeded = (cleanBoostOffOperation || midBoostOffOperation) && (cleanBoostOnOperation || midBoostOnOperation);
  if (delayNeeded) {
    delay(DELAY_BETWEEN_RING_TIP_PULSE);
  }

  // Perform "On" operations last
  if (cleanBoostOnOperation) {
    toggleCleanBoostRelay();
  }
  else if (midBoostOnOperation) {
    toggleMidBoostRelay();
  }
}

void handleKokoBoostProgramChange(byte programChangeByte) {
  if (programChangeByte == TUNER_ENGAGED) {
    if (currentKokoBoostState == STATE_KOKO_BOOST_OFF) {
      // nothing to do
      return;
    }
    else {
      // If the Koko Boost is currently enabled, and we're engaging the tuner, save the current value in state
      savedKokoBoostStateForTunerReset = currentKokoBoostState;

      // change the programChangeByte - this will force this function to turn the Koko Boost off when engaging the tuner
      programChangeByte = KOKO_BOOST_OFF;
    }
  }
  else if (programChangeByte == TUNER_DISENGAGED) {
    if (savedKokoBoostStateForTunerReset == -1) {
      // nothing to do
      return;
    }
    else {
      // turning off the tuner, so set the Koko Boost back to it's previous state
      programChangeByte = savedKokoBoostStateForTunerReset == STATE_KOKO_BOOST_CLEAN ? KOKO_BOOST_CLEAN : KOKO_BOOST_MIDS;
      
      // and clear out the saved state
      savedKokoBoostStateForTunerReset = -1;
    }
  }
  else {
    // regular (non-tuner) related change, clear out the saved state
    savedKokoBoostStateForTunerReset = -1;
  }

  bool cleanBoostOffOperation = false;
  bool midBoostOffOperation = false;
  bool cleanBoostOnOperation = false;
  bool midBoostOnOperation = false;
  
  switch (programChangeByte) {
    case KOKO_BOOST_OFF: {
          if (currentKokoBoostState == STATE_KOKO_BOOST_CLEAN) {
            cleanBoostOffOperation = true;
          }
          else if (currentKokoBoostState == STATE_KOKO_BOOST_MIDS) {
            midBoostOffOperation = true;
          }

          currentKokoBoostState = STATE_KOKO_BOOST_OFF;
          break;
        }
        case KOKO_BOOST_CLEAN: {
          if (currentKokoBoostState == STATE_KOKO_BOOST_MIDS) {
            midBoostOffOperation = true;
          }

          if (currentKokoBoostState != STATE_KOKO_BOOST_CLEAN) {
            cleanBoostOnOperation = true;
          }
          
          currentKokoBoostState = STATE_KOKO_BOOST_CLEAN;
          break;
        }
        case KOKO_BOOST_MIDS: {
          if (currentKokoBoostState == STATE_KOKO_BOOST_CLEAN) {
            cleanBoostOffOperation = true;
          }
          
          if (currentKokoBoostState != STATE_KOKO_BOOST_MIDS) {
            midBoostOnOperation = true;
          }
          
          currentKokoBoostState = STATE_KOKO_BOOST_MIDS;
          break;
        }
        default: {
          cleanBoostOffOperation = false;
          midBoostOffOperation = false;
          cleanBoostOnOperation = false;
          midBoostOnOperation = false;
        }
    }

    setRelaysForKokoBoostModeSwitch(cleanBoostOffOperation, midBoostOffOperation, cleanBoostOnOperation, midBoostOnOperation);
}

void printProgramChangeCommandDescription(byte programChangeByte) {
  Serial.println("Program Change #: ");
  Serial.println(programChangeByte);
}

void loop() {
  if (midiSerial.available() > 0) {
    // read MIDI byte
    midiByte = midiSerial.read();

    // parse channel and command values
    midiChannel = midiByte & B00001111;
    midiCommand = midiByte & B11110000;

    Serial.println("MIDI Channel: ");
    Serial.println(midiChannel);
    Serial.println("MIDI Command: ");
    Serial.println(midiCommand);

    // We only want to process Program Change (PC) commands sent specifically to our channel
    if (midiChannel == MIDI_LISTEN_CHANNEL && midiCommand == MIDI_PROGRAM_CHANGE) {
      // read the next immediate byte to get what PC we should execute
      programChangeByte = midiSerial.read();
      
      Serial.println("MIDI Program Change #: ");
      Serial.println(programChangeByte);

      if (programChangeByte >= 1 && programChangeByte <= 4) {
        handleAmpSwitcherProgramChange(programChangeByte);
      }
      else if (programChangeByte >= 5 && programChangeByte <= 9) {
        handleKokoBoostProgramChange(programChangeByte);
      }
    }
 }
}

  
