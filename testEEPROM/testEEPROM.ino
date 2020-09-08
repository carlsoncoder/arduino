#include <SPI.h>
#include <Wire.h>
#include <EEPROM.h>
#include <ReceiveOnlySoftwareSerial.h>  //http://gammon.com.au/Arduino/ReceiveOnlySoftwareSerial.zip

// Possible MIDI CC commands that could be sent
#define MIDI_CC_EFFECT              1           // CC number to turn the effect On or Off - Sending a value of 127 enages the pedal, sending a value of 0 disengages the pedal
#define MIDI_CC_BOOST               2           // CC number to turn the boost On or Off - Sending a value of 127 enages the boost, sending a value of 0 disengages the boost
#define MIDI_CC_NORMALPOT           3           // CC number to change the value of the "Normal" pot - 0 is fully CCW (Min value), 127 is fully CW (Max value)
#define MIDI_CC_BRITEPOT            4           // CC number to change the value of the "Brite" pot - 0 is fully CCW (Min value), 127 is fully CW (Max value)
#define MIDI_CC_VOLUMEPOT           5           // CC number to change the value of the "Volume" pot - 0 is fully CCW (Min value), 127 is fully CW (Max value)
#define MIDI_CC_BASSPOT             6           // CC number to change the value of the "Bass" pot - 0 is fully CCW (Min value), 127 is fully CW (Max value)
#define MIDI_CC_MIDPOT              7           // CC number to change the value of the "Mid" pot - 0 is fully CCW (Min value), 127 is fully CW (Max value)
#define MIDI_CC_TREBLEPOT           8           // CC number to change the value of the "Treble" pot - 0 is fully CCW (Min value), 127 is fully CW (Max value)

// MIDI constants
#define DEFAULT_MIDI_CHANNEL        8           // By default, we listen on MIDI channel 8, unless otherwise specified
#define MIDI_PROGRAM_CHANGE         192         // The number that represents a MIDI PC (Program Change) command
#define MIDI_CONTROL_CHANGE         176         // The number that represents a MIDI CC (Control Change) command

// Potentiometer Definitions for Multiplexer
#define NORMAL_POT                  1         // Must be wired to channel 0 on the mux
#define BRITE_POT                   2         // Must be wired to channel 1 on the mux
#define VOLUME_POT                  3         // Must be wired to channel 2 on the mux
#define BASS_POT                    4         // Must be wired to channel 3 on the mux
#define MID_POT                     5         // Must be wired to channel 4 on the mux
#define TREBLE_POT                  6         // Must be wired to channel 5 on the mux

// Atmega 328p PIN definitions (all definitions for the TQFP-32 package)
// Note that we do not use all of these in code, but I've included them here for wiring reference
#define MAX_SWITCH_EFFECT           1           // Used to switch the effect on/off - Goes to the IN1/IN2 Pin on MAX4701 (Pin 4)
#define MAX_SWITCH_BOOST            2           // Used to switch the boost on/off - Goes to the IN3/IN4 Pin on MAX4701 (Pin 12)
#define GND_ONE                     3           // The first GND pin on the Atmega328p -- NOT USED IN CODE
#define VCC_ONE                     4           // The first VCC pin on the Atmega328p -- NOT USED IN CODE
#define GND_TWO                     5           // The second GND pin on the Atmega328p -- NOT USED IN CODE
#define VCC_TWO                     6           // The second VCC pin on the Atmega328p -- NOT USED IN CODE
#define CRYSTAL_IN                  7           // The input pin for the 16 Mhz external crystal -- NOT USED IN CODE
#define CRYSTAL_IN                  8           // The output pin for the 16 Mhz external crystal -- NOT USED IN CODE
#define BASSPOT_ADSEL_PIN           9           // Used to set the address select pin on ONE of the 1M ohm Digi-pots (the one used for ONLY the Bass control). This should be hooked up to AD0 on the digi-pot IC
#define CS_SELECTPIN_25K            10          // Used as the slave select pin for the 25k ohm Digi-pot connected over SPI (Mids)
#define CS_SELECTPIN_250K           11          // Used as the slave select pin for the 250k ohm Digi-pot connected over SPI (Treble)
#define EFFECT_FOOTSWITCH_PIN       12          // Used to read the state of the footswitch for turning the effect on/off
#define BOOST_FOOTSWITCH_PIN        13          // Used to read the state of the footswitch for enabling/disabling the BOOST
#define MIDI_RECEIVE_PIN            14          // Used to receive data from the MIDI-IN port
#define SPI_MOSI                    15          // The SPI "Master Out, Slave In" (MOSI) pin on the Atmega328p -- NOT USED IN CODE
#define SPI_MISO                    16          // The SPI "Master In, Slave Out" (MISO) pin on the Atmega328p -- NOT USED IN CODE
#define SPI_SCK                     17          // The SPI Clock (SCK) pin on the Atmega328p -- NOT USED IN CODE
#define AVCC                        18          // The AVCC (Analog voltage input) pin on the Atmega328p -- NOT USED IN CODE
#define ANALOG_MULTIPLEX_PIN        19          // The main analog input/output pin for the 4051 multiplexer
#define AREF                        20          // The AREF (Analog reference) pin on the Atmega328p -- NOT USED IN CODE
#define GND_THREE                   21          // The third GND pin (Likely AGND) on the Atmega328p -- NOT USED IN CODE
#define MUX_SELECT_PIN_A            22          // The first address select pin for the 4051 multiplexer - "A" pin
#define MUX_SELECT_PIN_B            23          // The second address select pin for the 4051 multiplexer - "B" pin
#define MUX_SELECT_PIN_C            24          // The third address select pin for the 4051 multiplexer - "C" pin
#define PRESET_LED_PIN              25          // Used to indicate the state of the currently selected preset, if any
#define BOOST_LED_PIN               26          // Used to indicate if the boost is enabled or not
#define I2C_SDA                     27          // The I2C "Data Line" (SDA) pin on the Atmega328p -- NOT USED IN CODE
#define I2C_SCL                     28          // The I2C "Clock Line" (SCL) pin on the Atmega328p -- NOT USED IN CODE
#define RESET_PIN                   29          // The Reset pin on the Atmega328p -- NOT USED IN CODE
#define SERIAL_TX_PIN               30          // The serial transmit (TX) pin on the Atmega328p -- NOT USED IN CODE
#define SERIAL_RX_PIN               31          // The serial receive (RX) pin on the Atmega328p -- NOT USED IN CODE
#define EFFECT_LED_PIN              32          // Used to indicate if the effect is on or off

// I2C addresses
#define NORMAL_BRITE_POT_I2C_ADDR   44          // The I2C address to communicate with the digi-pot for the Normal/Brite settings (should have AD0 and AD1 pulled low (default/ground) so no pins needed from Atmega to set address)
#define BASS_POT_I2C_ADDR           45          // The I2C address to communicate with the digi-pot for Bass settings (should have AD0 pulled HIGH at program startup by an Atmega GPIO pin)
#define VOLUME_POT_I2C_ADDR         42          // The I2C address to communicate with the digi-pot for Volume settings

// Constants used elsewhere
#define MIDI_WAIT_TIME              250         // Delay time to use when starting up the MIDI ReceiveOnlySoftwareSerial channel
#define PRESET_BYTE_LENGTH          7          // The amount of bytes used when saving a preset
#define PRESET_LED_BLINK_TIME       250         // The time in milliseconds between blinking the Preset LED in "Preset Save" mode
#define ALLOWABLE_POT_VARIANCE      10          // The amount of variation in the analog pot values (from 0-1023) that we'll allow without considering it a "change" (due to minor noise during analogRead operations)
#define DEBOUNCE_THRESHOLD          50          // The time in milliseconds for a button press to be considered valid (and not noise)
#define LONG_PRESS_THRESHOLD        1500        // The time in milliseconds for a button press to be considered a "long press" (to go into "Preset Save" mode)
#define PRESET_MODE_TIMEOUT         30000       // The amount of time in milliseconds we will wait in "Preset Save" mode before giving up
#define SPI_DELAY_TIME              50          // The time in milliseconds to delay operations after pulling an SPI CS pin HIGH for SPI communications
#define SPI_MAXIMUM_SPEED           20000000    // The maximum clock speed for the SPI digi-pot IC's (20000000 = 20 Mhz)

// state management
byte expectedMidiChannel; // set in "seedEEPROMIfNeeded(...)" - Note that this value is actually 1 less than the MIDI channel we send commands on, since the MIDI channel byte is 0-15, not 1-16 like the controller itself would send
int normalPotReading;
int britePotReading;
int volumePotReading;
int bassPotReading;
int midPotReading;
int treblePotReading;
unsigned long effectButtonPressDuration;
unsigned long boostButtonPressDuration;
unsigned long inPresetSaveModeStartTime;
bool effectEnabled = false;
bool boostEnabled = false;
bool inPresetSaveMode = false;
bool isPresetCurrentlyApplied = false;
bool effectButtonActive = false;
bool effectButtonLongPressActive = false;
bool boostButtonActive = false;
bool boostButtonLongPressActive = false;

// timers
unsigned long presetLedBlinkTimer;
unsigned long effectButtonTimer;
unsigned long boostButtonTimer;

// multiplexer pin state
uint8_t pin_A_value = 0;
uint8_t pin_B_value = 0;
uint8_t pin_C_value = 0;
int lastReadPotValue = 0;

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
  Serial.begin(9600);
  
  // Seed EEPROM if we have to - this also sets the MIDI channel to listen for
  seedEEPROMIfNeeded();
}

void savePreset(int preset, int normal, int brite, int volume, int bass, int mid, int treble, int boost) {
    // We need to subtract 1 from the PC number to find our starting index in EEPROM
  Serial.println("Saving preset...");
  int pc = preset - 1;
  int startIndex = (pc * PRESET_BYTE_LENGTH) + 1;

  // write all values to EEPROM
  EEPROM[startIndex] = normal;
  EEPROM[startIndex + 1] = brite;
  EEPROM[startIndex + 2] = volume;
  EEPROM[startIndex + 3] = bass;
  EEPROM[startIndex + 4] = mid;
  EEPROM[startIndex + 5] = treble;
  EEPROM[startIndex + 6] = boost;
}

void recallPreset(int preset) {
  // We need to subtract 1 from the PC number to find our starting index in EEPROM
  int pc = preset - 1;
  int startIndex = (pc * PRESET_BYTE_LENGTH) + 1;

  int normal = EEPROM[startIndex];
  int brite = EEPROM[startIndex + 1];
  int volume = EEPROM[startIndex + 2];
  int bass = EEPROM[startIndex + 3];
  int mid = EEPROM[startIndex + 4];
  int treble = EEPROM[startIndex + 5];
  int boost = EEPROM[startIndex + 6];

  Serial.println("Showing values for preset: (Preset, Normal, Brite, Volume, Bass, Mid, Treble, Boost)");
  Serial.println(preset);
  Serial.println(normal);
  Serial.println(brite);
  Serial.println(volume);
  Serial.println(bass);
  Serial.println(mid);
  Serial.println(treble);
  Serial.println(boost);
}

void seedEEPROMIfNeeded() {
  Serial.println("SEED EEPROM");
  byte savedMidiChannel = EEPROM.read(0);
  Serial.println("Found value at 0th byte of EEPROM:");
  Serial.println(savedMidiChannel);
  byte midiChannelToSet;
  if (savedMidiChannel == 255) {
    // If the first byte is 255, that means it's never been written to, and we can assume that the entire EEPROM is empty
    // First set the default MIDI channel
    EEPROM[0] = DEFAULT_MIDI_CHANNEL;
    midiChannelToSet = DEFAULT_MIDI_CHANNEL;
    
    // Now, set every other value in EEPROM to 0 - by default the EEPROM will return 255 if never written to, 
    // which would actually be MAX value on the potentiometers!  So if you accidentally recalled a preset that you 
    // had never actually saved, it would MAX ALL VALUES!  And we don't want that!
    for (int i = 1; i < 1024; i++) {
      EEPROM[i] = 0;
    }
  }
  else if (savedMidiChannel <= 0 || savedMidiChannel >= 17) {
    // Anything less than or equal to 0, or greater than or equal to 17, is an invalid MIDI channel (MIDI channel is between 1-16)
    // Reset it to the default MIDI channel, but assume all other EEPROM values are valid
    EEPROM[0] = DEFAULT_MIDI_CHANNEL;
    midiChannelToSet = DEFAULT_MIDI_CHANNEL;
  }
  else {
    midiChannelToSet = savedMidiChannel;
  }

  Serial.println("MIDI Channel To Set =");
  Serial.println(midiChannelToSet);
  
  // At this point, we know whatever is in "midiChannelToSet" is our valid MIDI channel, either because we fixed it above, or it was already set prior
  // Subtract 1 from it (MIDI Controller sends in 1-16, but the byte is zero-indexed so will be 0-15),
  // and set our local variable so we know what to listen for
  expectedMidiChannel = midiChannelToSet - 1;
}

// Standard program loop
// Test EEPROM - Seed EEPROM / Read from EEPROM / Write to EEPROM / Recall from EEPROM
void loop() {

}

// Logic remaining to test on Metro (test on larger Metro device before Atmega328p device)
  // Test both SPI devices (25K and 250K pots) hooked up at the same time
  // Test 1M digital pot (I2C) (Bass) by itself - Only the 1 register
  // Test 1M digital pot (I2C) (Normal/Brite) by itself - Both Registers
  // Test 100K digital pot (I2C) (Volume) by itself - Only the 1 register
  // Test all three I2C devices hooked up at the same time
  // Evaluate if the SPI communication delay time can be shorter than 50 ms
  // Read ONE pot from Analog Multiplexer (4051)
  // Read all 6 pots at once from Analog Multiplexer (4051) at the same time
  // Ensure each of the 6 analog pots appropriately changes the value on the digital pots
  // Sending MIDI CC’s set the right value to the Pots (or turn the boost/effect on/off)
  // Changing a preset turns off “preset” mode
  // EVERYTHING hooked up at once


// Logic already tested on Metro
  // Footswitch reading/behavior
  // LED Blinking/Behavior
  // MAX4701 Switching behavior
  // 25k SPI Digi-pot, by itself
  // 25k SPI Digi-pot, by itself
