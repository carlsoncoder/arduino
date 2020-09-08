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

// Possible amp voicings
#define CLEAN_FENDER                  1
#define CLEAN_MARSHALL                2
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
  
  // setup ReceiveOnlySoftwareSerial for MIDI control
  midiSerial.begin(31250);
  Serial.begin(9600);
  delay(MIDI_WAIT_TIME); // give MIDI-device a short time to "digest" MIDI messages
}

void handleAmpSwitcherProgramChange(byte programChangeByte) {
  // TODO: JUSTIN: FINISH THIS
}

void handleBoostProgramChange(byte programChangeByte) {
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

      // For some reason, with the current setup (Port 4 on MC8, Tip to resistor, Ring to Diode),
      // the Program Change is 128 higher than what is actually sent (so if you send "3" from the MC8, this shows as 131)
      // We handle this by simply subtracting 128 from the value, but this is hacky and would likely break with a different MIDI
      // controller, or even a different port on the MC8!
      programChangeByte = programChangeByte - 128;

      if (programChangeByte >= 1 && programChangeByte <= 4) {
        handleAmpSwitcherProgramChange(programChangeByte);
      }
      if (programChangeByte >= 5 && programChangeByte <= 7) {
        handleBoostProgramChange(programChangeByte);
      }
    }
 }
}
  