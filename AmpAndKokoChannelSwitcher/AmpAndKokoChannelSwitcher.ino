#include <ReceiveOnlySoftwareSerial.h>
//http://gammon.com.au/Arduino/ReceiveOnlySoftwareSerial.zip

// Possible MIDI PC commands that could be sent
#define CLEAN_CHANNEL_MARSHALL        1
#define CLEAN_CHANNEL_FENDER          2
#define OVERDRIVE_CHANNEL_VINTAGE     3
#define OVERDRIVE_CHANNEL_MODERN      4
#define KOKO_BOOST_OFF                5
#define KOKO_BOOST_CLEAN              6
#define KOKO_BOOST_MIDS               7
#define TUNER_ENGAGED                 8
#define TUNER_DISENGAGED              9

// Possible amp voicings
#define CLEAN_MARSHALL                1
#define CLEAN_FENDER                  2
#define OVERDRIVE_VINTAGE             3
#define OVERDRIVE_MODERN              4

// Possible amp channels
#define CLEAN_CHANNEL                 1
#define OVERDRIVE_CHANNEL             2

// Possible Koko Boost states
#define STATE_KOKO_BOOST_OFF          1
#define STATE_KOKO_BOOST_CLEAN        2
#define STATE_KOKO_BOOST_MIDS         3

// MIDI constants
#define MIDI_WAIT_TIME                125
#define MIDI_LISTEN_CHANNEL           8      // MIDI channel to listen on - We listen on MIDI Channel 9, but is zero-indexed so we look for 8.  MIDI controller should send commands on channel 9!
#define MIDI_PROGRAM_CHANGE           192    // The number that represents a MIDI PC (Program Change) command

/* Other constants */
#define PIN_PULSE_LENGTH 3
#define DELAY_BETWEEN_RING_TIP_PULSE 25

/* Arduino PIN definitions */
#define MIDI_RECEIVE_PIN              11     // Used for the Arduino to receive data from the MIDI-IN port

/* Relay PIN definitions for the relays controlling the Koko Boost pedal */
// Tip (Clean Boost - Green LED)
#define TQRELAY_BOOST_TIP_COIL1_PIN         3     // Connect this pin (3) on Arduino to Pin 10 on the TQ2-L2-5V relay controlling the TIP.
#define TQRELAY_BOOST_TIP_COIL2_PIN         4     // Connect this pin (4) on Arduino to Pin 1 on the TQ2-L2-5V relay controlling the TIP. 
                                                  // Connect pin 5 and 6 on the relay to GND reference on the Arduino
                                                  // Connect pin 9 on relay to SLEEVE on TRS jack, and pin 8 to TIP on TRS jack

 // Ring (Mid Boost - Red LED)
#define TQRELAY_BOOST_RING_COIL1_PIN        5     // Connect this pin (5) on Arduino to Pin 10 on the TQ2-L2-5V relay controlling the RING.
#define TQRELAY_BOOST_RING_COIL2_PIN        6     // Connect this pin (6) on Arduino to Pin 1 on the TQ2-L2-5V relay controlling the RING.
                                                  // Connect pin 5 and 6 on this relay to GND reference on the Arduino
                                                  // Connect pin 9 on relay to SLEEVE on TRS jack, and pin 8 to RING on TRS jack

// TIP/SLEEVE = Channel, RING/SLEEVE = Voice
/* Relay PIN definitions for the relays controlling the Amp Switcher pedal */
// Tip (Controls channel changes)
#define TQRELAY_AMP_CHANNEL_TIP_COIL1_PIN         7   // Connect this pin (7) on Arduino to Pin 10 on the TQ2-L2-5V relay controlling the TIP.
#define TQRELAY_AMP_CHANNEL_TIP_COIL2_PIN         8   // Connect this pin (8) on Arduino to Pin 1 on the TQ2-L2-5V relay controlling the TIP. 
                                                      // Connect pin 5 and 6 on the relay to GND reference on the Arduino
                                                      // Connect pin 9 on relay to SLEEVE on TRS jack, and pin 8 to TIP on TRS jack

 // Ring (Controls voicing changes)
#define TQRELAY_AMP_VOICE_RING_COIL1_PIN          9    // Connect this pin (9) on Arduino to Pin 10 on the TQ2-L2-5V relay controlling the RING.
#define TQRELAY_AMP_VOICE_RING_COIL2_PIN          10   // Connect this pin (10) on Arduino to Pin 1 on the TQ2-L2-5V relay controlling the RING.
                                                       // Connect pin 5 and 6 on this relay to GND reference on the Arduino
                                                       // Connect pin 9 on relay to SLEEVE on TRS jack, and pin 8 to RING on TRS jack
// state management
int currentAmpChannel;
int currentAmpCleanVoicing;
int currentAmpOverdriveVoicing;
int currentKokoBoostState;
int savedKokoBoostStateForTunerReset;
bool isVoiceRingRelayInResetMode;
bool isChannelTipRelayInResetMode;

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
  pinMode(TQRELAY_BOOST_TIP_COIL1_PIN, OUTPUT);
  pinMode(TQRELAY_BOOST_TIP_COIL2_PIN, OUTPUT);
  pinMode(TQRELAY_BOOST_RING_COIL1_PIN, OUTPUT);
  pinMode(TQRELAY_BOOST_RING_COIL2_PIN, OUTPUT);
  pinMode(TQRELAY_AMP_CHANNEL_TIP_COIL1_PIN, OUTPUT);
  pinMode(TQRELAY_AMP_CHANNEL_TIP_COIL2_PIN, OUTPUT);
  pinMode(TQRELAY_AMP_VOICE_RING_COIL1_PIN, OUTPUT);
  pinMode(TQRELAY_AMP_VOICE_RING_COIL2_PIN, OUTPUT);

  // pulse COIL1 on all relays so we start in "off" mode
  pulsePin(TQRELAY_BOOST_TIP_COIL1_PIN);
  pulsePin(TQRELAY_BOOST_RING_COIL1_PIN);
  pulsePin(TQRELAY_AMP_CHANNEL_TIP_COIL1_PIN);
  pulsePin(TQRELAY_AMP_VOICE_RING_COIL1_PIN);

  // set default values for state variables
  currentAmpChannel = CLEAN_CHANNEL;
  currentAmpCleanVoicing = CLEAN_FENDER;
  currentAmpOverdriveVoicing = OVERDRIVE_VINTAGE;
  currentKokoBoostState = STATE_KOKO_BOOST_OFF;
  isVoiceRingRelayInResetMode = false;
  isChannelTipRelayInResetMode = false;
  savedKokoBoostStateForTunerReset = -1;
  
  // setup ReceiveOnlySoftwareSerial for MIDI control
  midiSerial.begin(31250);
  Serial.begin(9600);
  delay(MIDI_WAIT_TIME); // give MIDI-device a short time to "digest" MIDI messages
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

// IF the connection is OFF (the default state, i.e., if "!isVoiceRingRelayInResetMode" or "!isChannelTipRelayInResetMode"), pulse coil 2 to turn it on
// IF the connection is ON (non-default state, i.e., if "isVoiceRingRelayInResetMode" or "isChannelTipRelayInResetMode"), pulse coil 1 to turn it off
void setRelaysForAmpChannelVoiceSwitch(bool switchChannel, bool switchVoicing) {
  // ALWAYS switch the channel before switching voicing!
  if (switchChannel) {
    int pinToPulse = isChannelTipRelayInResetMode ? TQRELAY_AMP_CHANNEL_TIP_COIL1_PIN : TQRELAY_AMP_CHANNEL_TIP_COIL2_PIN;
    pulsePin(pinToPulse);

    // set the state to the opposite value
    isChannelTipRelayInResetMode = !isChannelTipRelayInResetMode;

    // if we plan to switch the voicing after the channel, add a short delay
    if (switchVoicing) {
      delay(MIDI_WAIT_TIME);
    }
  }

  if (switchVoicing) {
     int pinToPulse = isVoiceRingRelayInResetMode ? TQRELAY_AMP_VOICE_RING_COIL1_PIN : TQRELAY_AMP_VOICE_RING_COIL2_PIN;
     pulsePin(pinToPulse);

     // set the state to the opposite value
     isVoiceRingRelayInResetMode = !isVoiceRingRelayInResetMode;
  }
}

void handleBoostProgramChange(byte programChangeByte) {
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

  
  switch (programChangeByte) {
    case KOKO_BOOST_OFF: {
          Serial.println("Processing KOKO_BOOST_OFF program change...");
          if (currentKokoBoostState == STATE_KOKO_BOOST_CLEAN) {
            pulsePin(TQRELAY_BOOST_TIP_COIL1_PIN);
          }
          else if (currentKokoBoostState == STATE_KOKO_BOOST_MIDS) {
            pulsePin(TQRELAY_BOOST_RING_COIL1_PIN);
          }

          currentKokoBoostState = STATE_KOKO_BOOST_OFF;
          break;
        }
        case KOKO_BOOST_CLEAN: {
          Serial.println("Processing KOKO_BOOST_CLEAN program change...");
          if (currentKokoBoostState == STATE_KOKO_BOOST_MIDS) {
            pulsePin(TQRELAY_BOOST_RING_COIL1_PIN);
            delay(DELAY_BETWEEN_RING_TIP_PULSE);
          }

          if (currentKokoBoostState == STATE_KOKO_BOOST_OFF || currentKokoBoostState == STATE_KOKO_BOOST_MIDS) {
            pulsePin(TQRELAY_BOOST_TIP_COIL2_PIN);
          }

          currentKokoBoostState = STATE_KOKO_BOOST_CLEAN;
          break;
        }
        case KOKO_BOOST_MIDS: {
          Serial.println("Processing KOKO_BOOST_MIDS program change...");

          if (currentKokoBoostState == STATE_KOKO_BOOST_CLEAN) {
            pulsePin(TQRELAY_BOOST_TIP_COIL1_PIN);
            delay(DELAY_BETWEEN_RING_TIP_PULSE);
          }
          
          if (currentKokoBoostState == STATE_KOKO_BOOST_OFF || currentKokoBoostState == STATE_KOKO_BOOST_CLEAN) {
            pulsePin(TQRELAY_BOOST_RING_COIL2_PIN);
          }

          currentKokoBoostState = STATE_KOKO_BOOST_MIDS;
          break;
        }
        default: {
          Serial.println("Invalid program change byte");
        }
    }
}

void pulsePin(int pinNumber) {
  digitalWrite(pinNumber, HIGH);
  delay(PIN_PULSE_LENGTH);
  digitalWrite(pinNumber, LOW);
}

void loop() {
  if (midiSerial.available() > 0) {
    // read MIDI byte
    midiByte = midiSerial.read();

    // parse channel and command values
    midiChannel = midiByte & B00001111;
    midiCommand = midiByte & B11110000;

    // We only want to process Program Change (PC) commands sent specifically to our channel
    if (midiChannel == MIDI_LISTEN_CHANNEL && midiCommand == MIDI_PROGRAM_CHANGE) {
      // read the next immediate byte to get what PC we should execute
      programChangeByte = midiSerial.read();

      if (programChangeByte >= 1 && programChangeByte <= 4) {
        handleAmpSwitcherProgramChange(programChangeByte);
      }
      else if (programChangeByte >= 5 && programChangeByte <= 9) {
        handleBoostProgramChange(programChangeByte);
      }
    }
 }
}

  
