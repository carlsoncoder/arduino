#include <ReceiveOnlySoftwareSerial.h>
//http://gammon.com.au/Arduino/ReceiveOnlySoftwareSerial.zip

// Possible MIDI PC commands that could be sent
#define CLEAN_CHANNEL_MARSHALL 1
#define CLEAN_CHANNEL_FENDER 2
#define OVERDRIVE_CHANNEL_VINTAGE 3
#define OVERDRIVE_CHANNEL_MODERN 4

// Possible Voicings
#define CLEAN_FENDER 6
#define CLEAN_MARSHALL 7
#define OVERDRIVE_VINTAGE 8
#define OVERDRIVE_MODERN 9

// Possible channels
#define CLEAN_CHANNEL 10
#define OVERDRIVE_CHANNEL 11

// MIDI constants
#define MIDI_LISTEN_CHANNEL 8              // MIDI channel to listen on - We listen on MIDI Channel 9, but is zero-indexed so we look for 8.  MIDI controller should send commands on channel 9!
#define MIDI_PROGRAM_CHANGE 192            // The number that represents a MIDI PC (Program Change) command

// Arduino PIN definitions
#define CHANNEL_TQRELAY_POSITIVE_PIN 1    // Connect this pin (1) on Arduino to the "+" pin (Pin 10) on the TQ2-L-5V relay controlling the channel - Connect COM and NO (no continuity) (Pins 7&8 or 3&4 on relay) to Sleeve & Tip on TRS output jack
#define CHANNEL_TQRELAY_NEGATIVE_PIN 2    // Connect this pin (2) on Arduino to the "-" pin (Pin 1) on the TQ2-L-5V relay controlling the channel - Connect COM and NO (no continuity) (Pins 7&8 or 3&4 on relay) to Sleeve & Tip on TRS output jack
#define VOICE_TQRELAY_POSITIVE_PIN 3      // Connect this pin (3) on Arduino to the "+" pin (Pin 10) on the TQ2-L-5V relay controlling the voicing - Connect COM and NO (no continuity) (Pins 7&8 or 3&4 on relay) to Sleeve & Ring on TRS output jack
#define VOICE_TQRELAY_NEGATIVE_PIN 4      // Connect this pin (4) on Arduino to the "-" pin (Pin 1) on the TQ2-L-5V relay controlling the voicing - Connect COM and NO (no continuity) (Pins 7&8 or 3&4 on relay) to Sleeve & Ring on TRS output jack
#define MIDI_RECEIVE_PIN 0                // Used for the Arduino to receive data from the MIDI-IN port

// Constants used elsewhere
#define RELAY_PULSE_TIME 3
#define MIDI_WAIT_TIME 125 // Was originally 250 in the first iteration

// state management
int currentChannel;
int currentCleanVoicing;
int currentOverdriveVoicing;
bool isChannelRelayInResetMode;
bool isVoiceRelayInResetMode;

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
  // setup relay pins and LED pins as OUTPUT
  pinMode(CHANNEL_TQRELAY_POSITIVE_PIN, OUTPUT);
  pinMode(CHANNEL_TQRELAY_NEGATIVE_PIN, OUTPUT);
  pinMode(VOICE_TQRELAY_POSITIVE_PIN, OUTPUT);
  pinMode(VOICE_TQRELAY_NEGATIVE_PIN, OUTPUT);

  // set default values for state variables
  currentChannel = CLEAN_CHANNEL;
  currentCleanVoicing = CLEAN_FENDER;
  currentOverdriveVoicing = OVERDRIVE_VINTAGE;
  isChannelRelayInResetMode = true;
  isVoiceRelayInResetMode = true;
  
  // setup ReceiveOnlySoftwareSerial for MIDI control
  midiSerial.begin(31250);
  delay(MIDI_WAIT_TIME); // give MIDI-device a short time to "digest" MIDI messages
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

      bool wasValidProgramChange = true;
      bool switchChannel = false;
      bool switchVoicing = false;

      switch (programChangeByte) {
        case CLEAN_CHANNEL_MARSHALL: {
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
        default: {
          wasValidProgramChange = false;
        }
    }

    if (wasValidProgramChange) {
      toggleRelays(switchChannel, switchVoicing);
    }
  }
 }
}

void toggleRelays(bool switchChannel, bool switchVoicing) {
  // ALWAYS SWITCH CHANNEL BEFORE SWITCHING VOICING!
  if (switchChannel) {
    // switch the channel relay to be the opposite of what it currently is
    // determine polarity (which pin to pulse on relay)
    int channelPinToPulse = 0;
    if (isChannelRelayInResetMode) {
      channelPinToPulse = CHANNEL_TQRELAY_NEGATIVE_PIN;
    }
    else {
      channelPinToPulse = CHANNEL_TQRELAY_POSITIVE_PIN;
    }

    // send the pulse to the pin
    digitalWrite(channelPinToPulse, HIGH);
    delay(RELAY_PULSE_TIME);
    digitalWrite(channelPinToPulse, LOW);

    // set the state to the opposite value
    isChannelRelayInResetMode = !isChannelRelayInResetMode;
    
    if (switchVoicing) {
      // if we're going to switch voicing next, put a short delay to ensure the channel successfully switches on the amp first!
      delay(MIDI_WAIT_TIME);
    }
  }

  if (switchVoicing) {
    // switch the voicing relay to be the opposite of what it currently is
    // determine polarity (which pin to pulse on relay)
    int voicingPinToPulse = 0;
    if (isVoiceRelayInResetMode) {
      voicingPinToPulse = VOICE_TQRELAY_NEGATIVE_PIN;
    }
    else {
      voicingPinToPulse = VOICE_TQRELAY_POSITIVE_PIN;
    }

    // send the pulse to the pin
    digitalWrite(voicingPinToPulse, HIGH);
    delay(RELAY_PULSE_TIME);
    digitalWrite(voicingPinToPulse, LOW);

    // set the state to the opposite value
    isVoiceRelayInResetMode = !isVoiceRelayInResetMode;
  }
}
