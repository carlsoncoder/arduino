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
  // Seed EEPROM if we have to - this also sets the MIDI channel to listen for
  seedEEPROMIfNeeded();
  
  // Setup all LED pins as OUTPUT
  pinMode(EFFECT_LED_PIN, OUTPUT);
  pinMode(BOOST_LED_PIN, OUTPUT);
  pinMode(PRESET_LED_PIN, OUTPUT);

  // Setup our footswitch pins as INPUT_PULLUP
  pinMode(EFFECT_FOOTSWITCH_PIN, INPUT_PULLUP);
  pinMode(BOOST_FOOTSWITCH_PIN, INPUT_PULLUP);

  // Setup our pins on the MAX4701 switch as OUTPUT
  pinMode(MAX_SWITCH_EFFECT, OUTPUT);
  pinMode(MAX_SWITCH_BOOST, OUTPUT);

  // Set our multiplexer pins appropriately (the main pin is INPUT and the address select pins are OUTPUT)
  pinMode(ANALOG_MULTIPLEX_PIN, INPUT);
  pinMode(MUX_SELECT_PIN_A, OUTPUT);
  pinMode(MUX_SELECT_PIN_B, OUTPUT);
  pinMode(MUX_SELECT_PIN_C, OUTPUT);

  // Setup our CS select pins (for SPI) and our I2C Address Select pin for the Bass digi-pot to OUTPUT
  pinMode(CS_SELECTPIN_25K, OUTPUT);
  pinMode(CS_SELECTPIN_250K, OUTPUT);
  pinMode(BASSPOT_ADSEL_PIN, OUTPUT);

  // Pull the "BASSPOT_ADSEL_PIN" HIGH so we set our I2C address appropriately
  digitalWrite(BASSPOT_ADSEL_PIN, HIGH);

  // Pull the CS pins HIGH so they're ready for SPI communication
  digitalWrite(CS_SELECTPIN_25K, HIGH);
  digitalWrite(CS_SELECTPIN_250K, HIGH);

  // start the Wire (I2C), SPI, and Serial library communications
  Wire.begin();
  SPI.begin();
  Serial.begin(9600);

  // start our timers
  presetLedBlinkTimer = millis();

  // Some of the digi-pots are volatile, so they won't hold value after a restart of the controller
  // read them all into the global variables and force them to set the digital pots as well on startup
  readAnalogPotentiometers(true);

  // always activate the effect on startup
  // NOTE: We set this here if we're using a true-bypass looper.  If we AREN'T using a true-bypass looper, we should do the opposite and DE-activate the effect on startup!
  activateEffectFootswitch(true, false, false);

  // make sure the boost is DISABLED on startup (the bypass/effect is already set with the call to activateEffectFootswitch(...) above)
  digitalWrite(MAX_SWITCH_BOOST, LOW);
  digitalWrite(BOOST_LED_PIN, LOW);

  // make sure the preset LED starts off
  digitalWrite(PRESET_LED_PIN, LOW);
  
  // setup ReceiveOnlySoftwareSerial for MIDI control
  midiSerial.begin(31250);
  delay(MIDI_WAIT_TIME); // give MIDI-device a short time to "digest" MIDI messages
}

// Read the current values from the actual analog potentiometers
  // PARAMETERS:
    // bool forceSetDigitalPotentiometers:
        // If set to true, will always update the digi-pots with the read values
        // If set to false, will only update the digi-pots if the analog value has changed since last read.
void readAnalogPotentiometers(bool forceSetDigitalPotentiometers) {
  // NOTE:  The actual analog potentiometers for Normal/Brite/Volume should be Log (A) pots, and the potentiometers for Bass/Mid/Treble should be Linear (B) pots
  // The resistance values of these DO NOT MATTER!  The "analogRead" function just returns a value between 0-1023 representing the wiper travel in the pot.
  // These values will be different for the sweep of the pot for Linear vs. Log - for example, put a Linear pot at 50%, and a Log pot at 50%, and you'll see very 
  // different values from the analogRead function.  Due to that, as long as you use the RIGHT pot (either A taper or B taper), the resistance on these doesn't matter 
  // at all.  The digi-pots, which are actually hooked up to the audio effect circuit, DO have to be the right values though, but we can just use Linear taper for all of those,
  // since we aren't "moving" them step-by-step, we're just setting them to a value representing their fraction of movement between 0-1023 on their corresponding analog pot
  int tempNormalPotReading = readPotentiometerFromMultiplexer(NORMAL_POT);
  int tempBritePotReading = readPotentiometerFromMultiplexer(BRITE_POT);
  int tempVolumePotReading = readPotentiometerFromMultiplexer(VOLUME_POT);
  int tempBassPotReading = readPotentiometerFromMultiplexer(BASS_POT);
  int tempMidPotReading = readPotentiometerFromMultiplexer(MID_POT);
  int tempTreblePotReading = readPotentiometerFromMultiplexer(TREBLE_POT);
  
  // check if any have changed since our last reading
  bool normalPotChanged = hasPotentiometerValueChanged(normalPotReading, tempNormalPotReading);
  bool britePotChanged = hasPotentiometerValueChanged(britePotReading, tempBritePotReading);
  bool volumePotChanged = hasPotentiometerValueChanged(volumePotReading, tempVolumePotReading);
  bool bassPotChanged = hasPotentiometerValueChanged(bassPotReading, tempBassPotReading);
  bool midPotChanged = hasPotentiometerValueChanged(midPotReading, tempMidPotReading);
  bool treblePotChanged = hasPotentiometerValueChanged(treblePotReading, tempTreblePotReading);

  bool anyChanges = normalPotChanged || britePotChanged || volumePotChanged || bassPotChanged || midPotChanged || treblePotChanged;

  // Now we're going to make calls to set the digital potentiometer values if any of them have changed, or if we're forcing a reset
  if (normalPotChanged || forceSetDigitalPotentiometers) {
    normalPotReading = tempNormalPotReading;
    setNormalDigitalPotentiometer(normalPotReading);
  }

  if (britePotChanged || forceSetDigitalPotentiometers) {
    britePotReading = tempBritePotReading;
    setBriteDigitalPotentiometer(britePotReading);
  }

  if (volumePotChanged || forceSetDigitalPotentiometers) {
    volumePotReading = tempVolumePotReading;
    setVolumeDigitalPotentiometer(volumePotReading);
  }

  if (bassPotChanged || forceSetDigitalPotentiometers) {
    bassPotReading = tempBassPotReading;
    setBassDigitalPotentiometer(bassPotReading);
  }

  if (midPotChanged || forceSetDigitalPotentiometers) {
    midPotReading = tempMidPotReading;
    setMidDigitalPotentiometer(midPotReading);
  }

  if (treblePotChanged || forceSetDigitalPotentiometers) {
    treblePotReading = tempTreblePotReading; 
    setTrebleDigitalPotentiometer(treblePotReading);
  }

  // FUTURE: JUSTIN: Consider in the future applying some logic where if you have a preset active, we don't care about the analog pots until you get within a certain percentage (5%?) of the preset value, 
  // in order to prevent "jumping" from the preset value of the knob to the physical location when you go to move it
  if (isPresetCurrentlyApplied && anyChanges) {
    // IMPORTANT NOTE:  If you have a preset currently applied/active, and you change one or more pots, that will "invalidate" the preset (the Preset active LED will turn off)
    // HOWEVER - Any potentiometers you have NOT changed, will still be set to their preset values, which won't really match up with the "physical location" of the analog potentiometers
    // We do this intentionally - so that if you have an active preset that you like, but just want to change one value, you can just change that one pot until you get it how you like, 
    // then you could save the preset with the same PC number to overwrite it in EEPROM
    isPresetCurrentlyApplied = false;
    digitalWrite(PRESET_LED_PIN, LOW);
  }
}

// Reads the value of a selected potentiometer from the 4051 multiplexer
  // PARAMETERS:
    // int potentiometerToRead:  The number of the potentiometer to be read
  // RETURNS:
    // The value of the selected potentiometer
  // REMARKS:
    // Adapted From: https://cityos.io/tutorial/1958/Use-multiplexer-with-Arduino
int readPotentiometerFromMultiplexer(uint8_t potentiometerToRead) {
  
  pin_A_value = bitRead(potentiometerToRead, 0);   // Take the first bit from binary value of potentiometerToRead
  pin_B_value = bitRead(potentiometerToRead, 1);   // Take the second bit from binary value of potentiometerToRead
  pin_C_value = bitRead(potentiometerToRead, 2);   // Take the third bit from binary value of potentiometerToRead

  // Use our selector pins to write the selected address for the multiplexer
  digitalWrite(MUX_SELECT_PIN_A, pin_A_value);
  digitalWrite(MUX_SELECT_PIN_B, pin_B_value);
  digitalWrite(MUX_SELECT_PIN_C, pin_C_value);

  // Read the value from the multiplexer for our selected potentiometer  
  lastReadPotValue = analogRead(ANALOG_MULTIPLEX_PIN);

  // return the value to caller
  return lastReadPotValue;
}

// Determines if an analog potentiometer value has changed since the last reading, outside of allowable tolerances
  // PARAMETERS:
    // int previous:  The previous reading for this potentiometer
    // int current:  The current reading for this potentiometer
  // RETURNS:
    // Whether or not the value has changed from the previous reading on the potentiometer
bool hasPotentiometerValueChanged(int previous, int current) {
  int diff = current - previous;
  if (diff < 0) {
    diff = diff * -1;
  }

  return diff > ALLOWABLE_POT_VARIANCE;
}

// Sets the value on the digital potentiometer for the "Normal" knob
  // PARAMETERS:
    // int normalReading:  The value to be applied to the digital potentiometer
void setNormalDigitalPotentiometer(int normalReading) {
  // Normal is a 256 tap (0-255) pot, so we need to scale the current reading
  int normalValue = map(normalReading, 0, 1023, 0, 255);
  writeToI2CPotentiometer(NORMAL_BRITE_POT_I2C_ADDR, normalValue, 0);
}

// Sets the value on the digital potentiometer for the "Brite" knob
  // PARAMETERS:
    // int briteReading:  The value to be applied to the digital potentiometer
void setBriteDigitalPotentiometer(int briteReading) {
  // Brite is a 256 tap (0-255) pot, so we need to scale the current reading
  int briteValue = map(briteReading, 0, 1023, 0, 255);
  writeToI2CPotentiometer(NORMAL_BRITE_POT_I2C_ADDR, briteValue, 1);
}

// Sets the value on the digital potentiometer for the "Volume" knob
  // PARAMETERS:
    // int volumeReading:  The value to be applied to the digital potentiometer
void setVolumeDigitalPotentiometer(int volumeReading) {
  // Volume is a 256 tap (0-255) pot, so we need to scale the current reading 
  int volumeValue = map(volumeReading, 0, 1023, 0, 255);
  writeToI2CPotentiometer(VOLUME_POT_I2C_ADDR, volumeValue, 0);
}

// Sets the value on the digital potentiometer for the "Bass" knob
  // PARAMETERS:
    // int bassReading:  The value to be applied to the digital potentiometer
void setBassDigitalPotentiometer(int bassReading) {
  // Bass is a 256 tap (0-255) pot, so we need to scale the current reading 
  int bassValue = map(bassReading, 0, 1023, 0, 255);
  writeToI2CPotentiometer(BASS_POT_I2C_ADDR, bassValue, 0);
}

// Sets the value on the digital potentiometer for the "Mid" knob
  // PARAMETERS:
    // int midReading:  The value to be applied to the digital potentiometer
void setMidDigitalPotentiometer(int midReading) {
    // Mid is a 1024 tap (0-1023) pot, so NO need to scale the current reading 
    writeToSpiPotentiometer(CS_SELECTPIN_25K, midReading);
}

// Sets the value on the digital potentiometer for the "Treble" knob
  // PARAMETERS:
    // int trebleReading:  The value to be applied to the digital potentiometer
void setTrebleDigitalPotentiometer(int trebleReading) {
    // Treble is a 1024 tap (0-1023) pot, so NO need to scale the current reading 
    writeToSpiPotentiometer(CS_SELECTPIN_250K, trebleReading);
}

// Writes a value to one of the SPI digi-pots (Mid and Treble pots use SPI)
  // PARAMETERS:
    // int spiCSSelectPin:  The CS pin to be pulled low/high for SPI communications
    // uint16_t value:  The value (between 0-1023) to be sent to the potentiometer
  // REMARKS:
    // Adapted From: https://www.arduino.cc/en/Tutorial/DigitalPotControl & http://www.grozeaion.com/electronics/arduino?amp;view=article&amp;catid=35:digital-photos&amp;id=125:gvi-dslr-rc-with-touch-shield-slide
    // Coded to work with the AD5235 series - https://www.analog.com/media/en/technical-documentation/data-sheets/AD5235.pdf
void writeToSpiPotentiometer(int spiCSSelectPin, uint16_t value) {
  SPI.beginTransaction(SPISettings(SPI_MAXIMUM_SPEED, MSBFIRST, SPI_MODE0));

  digitalWrite(spiCSSelectPin, LOW);
  delay(SPI_DELAY_TIME);

  uint8_t bytes[3];

  // Since we're ONLY ever using ONE of the two pots on the SPI chips (for 25k and 250k ohm digi-pots), we can hard-code the cmd to "0xB0", and make sure we ONLY use Wiper 1 (RDAC1 register)
  // If we did want to use the other (Wiper 2 - RDAC2 register), it should just use "0xB1" for the first byte
  bytes[0] = 0xB0;
  bytes[1] = value >> 8;
  bytes[2] = value & 0xFF;

  for (int i = 0; i < 3; i++) {
    SPI.transfer(bytes[i]);
  }
  
  digitalWrite(spiCSSelectPin, HIGH);
  SPI.endTransaction();
}

// Writes a value to one of the I2C digi-pots (Normal/Brite share one dual-channel I2C pot, Volume has it's own I2C pot, and Bass has it's own I2C pot)
  // PARAMETERS:
    // int address:  The I2C address to communicate with
    // uin8_t value:  The value (between 0-255) to be sent to the potentiometer
    // int potRegister:  The value (either 0 or 1) representing which register to write to (these are dual channel digi-pots)
  // REMARKS:
    // Adapted From: https://github.com/RobTillaart/Arduino/tree/master/libraries/AD524X
    // Coded to work with the AD524X Series - https://www.analog.com/media/en/technical-documentation/data-sheets/AD5241_5242.pdf
void writeToI2CPotentiometer(int address, uint8_t value, int potRegister) {
  Wire.beginTransmission(address);
  uint8_t cmd = (potRegister == 0) ? 0x00 : 0x80;  // RDAC1 = 0x00, RDAC2 = 0x80
  Wire.write(cmd);
  Wire.write(value);
  Wire.endTransmission();
}

// Checks either the effect/bypass or boost footswitch to see if it is engaged
  // PARAMETERS:
    // bool isEffectsFootswitch:
        // If set to true, we process the effect/bypass footswitch
        // If set to false, we process the boost footswitch
    // int switchPin:  The Atmega pin tied to the footswitch
    // bool &buttonActive: Boolean passed by reference from the caller to note if the button is currently active or not
    // bool &buttonLongPressActive: Boolean passed by reference from the caller to note if the button is currently "long press" active or not
    // unsigned long &buttonTimer: Timer (long) passed by reference to see when the button was first pressed
    // unsigned long &buttonPressDuration: Duration (long) passed by reference to see how long the button has been pressed
  // REMARKS:
     // Adapted From: https://bloggymcblogface.blog/example-arduino-code-for-debouncing-and-long-pressing-buttons/
void checkFootswitch(bool isEffectsFootswitch, int switchPin, bool &buttonActive, bool &buttonLongPressActive, unsigned long &buttonTimer, unsigned long &buttonPressDuration) {
  // if the button pin reads LOW, the button is pressed (negative/ground switch)
  if (digitalRead(switchPin) == LOW) {
    // mark the button as active, and start the timer
    if (!buttonActive) {
      buttonActive = true;
      buttonTimer = millis();
    }
    
    // calculate the button press duration by subtracting the button time from the boot time
    buttonPressDuration = millis() - buttonTimer;

    // mark the button as long-pressed if the button press duration exceeds the long press threshold
    if ((buttonPressDuration > LONG_PRESS_THRESHOLD) && (buttonLongPressActive == false)) {
      buttonLongPressActive = true;

      // We currently only process or do any action on long-press of the effect/bypass footswitch
      if (isEffectsFootswitch) {
        inPresetSaveMode = true;
        inPresetSaveModeStartTime = millis();
      }
    }
  }
  else {
    // If we're here, either the button hasn't been pressed, or was recently released
    // If the button was marked as active, it was recently pressed
    if (buttonActive == true) {
      // reset the long press active state
      if (buttonLongPressActive == true) {
        buttonLongPressActive = false;
      }
      else if (buttonPressDuration > DEBOUNCE_THRESHOLD) {
        if (isEffectsFootswitch) {
          activateEffectFootswitch(false, false, false);
        }
        else {
          activateBoostFootswitch(false, false);
        }
      }
      
      // reset the button active status
      buttonActive = false;
    }
  }
}

// Check the state of both the Effect/Bypass and Boost footswitches
void checkFootswitchesState() {
  checkFootswitch(true, EFFECT_FOOTSWITCH_PIN, effectButtonActive, effectButtonLongPressActive, effectButtonTimer, effectButtonPressDuration);
  checkFootswitch(false, BOOST_FOOTSWITCH_PIN, boostButtonActive, boostButtonLongPressActive, boostButtonTimer, boostButtonPressDuration);
}

// Activates the effect footswitch by writing to the MAX4701 analog switch IC
  // PARAMETERS:
    // bool fromMidiProgramChange: Whether or not a MIDI PC is causing the change
    // bool fromMidiControlChange: Whether or not a MIDI CC is causing the change
    // bool on:  Only used when "fromMidiControlChange" is true - defines if the MIDI CC is trying to turn the effect on or off
void activateEffectFootswitch(bool fromMidiProgramChange, bool fromMidiControlChange, bool on) {
  if (fromMidiProgramChange && !effectEnabled) {
    // if we're activating from MIDI program change, we always want the effect footswitch on
    effectEnabled = true;
    digitalWrite(MAX_SWITCH_EFFECT, HIGH);
    digitalWrite(EFFECT_LED_PIN, HIGH);
  }
  else if (fromMidiControlChange) {
    if (!effectEnabled && on) {
      effectEnabled = true;
      digitalWrite(MAX_SWITCH_EFFECT, HIGH);
      digitalWrite(EFFECT_LED_PIN, HIGH);
    }
    else if (effectEnabled && !on) {
      effectEnabled = false;
      digitalWrite(MAX_SWITCH_EFFECT, LOW);
      digitalWrite(EFFECT_LED_PIN, LOW);
    }
  }
  else {
    // just do the opposite of what it already is
    if (effectEnabled) {
      effectEnabled = false;
      digitalWrite(MAX_SWITCH_EFFECT, LOW);
      digitalWrite(EFFECT_LED_PIN, LOW);
    }
    else {
      effectEnabled = true;
      digitalWrite(MAX_SWITCH_EFFECT, HIGH);
      digitalWrite(EFFECT_LED_PIN, HIGH);
    }
  }
}

// Activates the boost footswitch by writing to the MAX4701 analog switch IC
  // PARAMETERS:
    // bool fromMidiProgramOrControlChange: Whether or not a MIDI PC or CC is causing the change
    // bool on:  Only used when "fromMidiProgramOrControlChange" is true - defines if the MIDI PC/CC is trying to turn the boost on or off
void activateBoostFootswitch(bool fromMidiProgramOrControlChange, bool on) {
  if (fromMidiProgramOrControlChange) {
    if (on && !boostEnabled) {
      // boost is NOT currently enabled, but the MIDI change needs it on
      boostEnabled = true;
      digitalWrite(MAX_SWITCH_BOOST, HIGH);
      digitalWrite(BOOST_LED_PIN, HIGH);
    }
    else if (!on && boostEnabled) {
      // boost IS currently enabled, but the MIDI change needs it off
      boostEnabled = false;
      digitalWrite(MAX_SWITCH_BOOST, LOW);
      digitalWrite(BOOST_LED_PIN, LOW);
    }
  }
  else {
    // just do the opposite of what it already is
    if (boostEnabled) {
      boostEnabled = false;
      digitalWrite(MAX_SWITCH_BOOST, LOW);
      digitalWrite(BOOST_LED_PIN, LOW);
    }
    else {
      boostEnabled = true;
      digitalWrite(MAX_SWITCH_BOOST, HIGH);
      digitalWrite(BOOST_LED_PIN, HIGH);
    }

    if (isPresetCurrentlyApplied) {
      // IMPORTANT NOTE:  If you have a preset currently applied/active, and you change the "boost" footswitch value, that will "invalidate" the preset (the Preset active LED will turn off)
      // HOWEVER - The potentiometers, which you have NOT changed at this point, will still be set to their preset values, which won't really match up with the "physical location" of the analog potentiometers
      // We do this intentionally - so that if you have an active preset that you like, but just want to change the boost value, you can just change that one value, and then save the preset with the same PC number to overwrite it in EEPROM
      isPresetCurrentlyApplied = false;
      digitalWrite(PRESET_LED_PIN, LOW);
    }
  }
}

// Reads the serial MIDI channel and acts on any messages received
void processMidiCommands() {
  if (midiSerial.available() > 0) {
    
    // read MIDI byte
    midiByte = midiSerial.read();
    
    // parse channel and command values
    midiChannel = midiByte & B00001111;
    midiCommand = midiByte & B11110000;

    // Only move forward if the channel is ours
    if (midiChannel == expectedMidiChannel) {
      
      // process PC and CC commands differently
      if (midiCommand == MIDI_PROGRAM_CHANGE) {
        
        // read the PC number byte
        midiProgramChangeNumberByte - midiSerial.read();
        
        if (midiProgramChangeNumberByte == 0) {
          // PC#0 is a special case - we force reset all digi-pots to be whatever the analog pots are currently set to
          // A user cannot save anything to PC#0, so technically they only have 127 presets to save, not 128!
          inPresetSaveMode = false;
          isPresetCurrentlyApplied = false;
          readAnalogPotentiometers(true);
          digitalWrite(PRESET_LED_PIN, LOW);
        }
        else if (midiProgramChangeNumberByte > 127) {
          // Invalid - this shouldn't be possible, since you can only send a preset up to 127 via MIDI
          inPresetSaveMode = false;
          isPresetCurrentlyApplied = false;
          digitalWrite(PRESET_LED_PIN, LOW);
        }
        else if (inPresetSaveMode) {
          inPresetSaveMode = false;
          savePresetToEEPROM(midiProgramChangeNumberByte);
          isPresetCurrentlyApplied = true;
          digitalWrite(PRESET_LED_PIN, HIGH);
        }
        else {
          isPresetCurrentlyApplied = true;
          loadAndApplyPresetFromEEPROM(midiProgramChangeNumberByte);
          digitalWrite(PRESET_LED_PIN, HIGH);
        }
      }
      else if (midiCommand == MIDI_CONTROL_CHANGE) {
        // read the CC number byte
        midiControlChangeNumberByte = midiSerial.read();
        
        // read the CC value byte
        midiControlChangeValueByte = midiSerial.read();

        switch (midiControlChangeNumberByte) {
          case MIDI_CC_EFFECT: {
            // Sending a value of 127 enages the pedal, sending a value of 0 disengages the pedal
            if (midiControlChangeValueByte == 0) {
              activateEffectFootswitch(false, true, false);
            }
            else if (midiControlChangeValueByte == 127) {
              activateEffectFootswitch(false, true, true);
            }
            
            break;
          }
          case MIDI_CC_BOOST: {
            // Sending a value of 127 enages the boost, sending a value of 0 disengages the boost
            if (midiControlChangeValueByte == 0) {
              activateBoostFootswitch(true, false);
            }
            else if (midiControlChangeValueByte == 127) {
              activateBoostFootswitch(true, true);
            }
            
            break;
          }
          case MIDI_CC_NORMALPOT: {
            int normalValue = mapMidiCCValueToLogPotValue(midiControlChangeValueByte);
            setNormalDigitalPotentiometer(normalValue);
            break;
          }
          case MIDI_CC_BRITEPOT: {
            int briteValue = mapMidiCCValueToLogPotValue(midiControlChangeValueByte);
            setBriteDigitalPotentiometer(briteValue);
            break;
          }
          case MIDI_CC_VOLUMEPOT: {
            int volumeValue = mapMidiCCValueToLogPotValue(midiControlChangeValueByte);
            setVolumeDigitalPotentiometer(volumeValue);
            break;
          }
          case MIDI_CC_BASSPOT: {
            // the value will be sent in as 0-127, but the functions expect 0-1023
            int bassValue = map(midiControlChangeValueByte, 0, 127, 0, 1023);
            setBassDigitalPotentiometer(bassValue);
            break;
          }
          case MIDI_CC_MIDPOT: {
            // the value will be sent in as 0-127, but the functions expect 0-1023
            int midValue = map(midiControlChangeValueByte, 0, 127, 0, 1023);
            setMidDigitalPotentiometer(midValue);
            break;
          }
          case MIDI_CC_TREBLEPOT: {
            // the value will be sent in as 0-127, but the functions expect 0-1023
            int trebleValue = map(midiControlChangeValueByte, 0, 127, 0, 1023);
            setTrebleDigitalPotentiometer(trebleValue);
            break;
          }
          default:{
            break;
          }
        }
      }
    }
 }
}

// Performs mapping to attempt to map a MIDI CC value to a logarithmic value that would be generated by an audio taper potentiometer
  // PARAMETERS:
    // int midiCCValue:  The MIDI CC value that was passed in by the MIDI controller
  // RETURNS:
    // A mapped value between 0-1023 that can be used to set the potentiometer value
  // REMARKS:
    // Since Normal, Brite, and Volume are Audio taper (Log taper) pots, so we want to do some special mapping to convert the 0-127 CC MIDI value
    // Otherwise, doing a simple map (from 0-127 to 0-1023) would result in a "halfway value" on the MIDI CC command (like 63) representing something
    // like 512 for the pot value - which in a Log pot would be WAY more than what "halfway" on the knob would be (50% on a Log pot is about 10% of total resistance)
    // However, audio tapers don't follow a true logarithmic curve - It's basically linear (with a small rise over run) up to 50%, then linear again with a larger
    // rise over run from 50%-100%.  We try our best to map that appropriately here
    // Formulas Adapted From This Chart: http://www.resistorguide.com/potentiometer-taper/#Audio_taper
int mapMidiCCValueToLogPotValue(int midiCCValue) {
  int calculatedResistanceStepValue = 0;
  if (midiCCValue <= 4) {
    // if it's this low we'll just set it to the min (0)
    calculatedResistanceStepValue = 0;
  }
  else if (midiCCValue > 123) {
    // if it's this high we just set to the max (1023)
    calculatedResistanceStepValue = 1023;
  }
  else if (midiCCValue <= 64) {
    // first map it to a 0-1023 range
    calculatedResistanceStepValue = map(midiCCValue, 0, 127, 0, 1023);
    // then apply the smaller linear function
    calculatedResistanceStepValue = calculatedResistanceStepValue / 5;  // y = 1/5x
  }
  else {
    // first map it to a 0-100 range (that's what the linear formula was based on)
    calculatedResistanceStepValue = map(calculatedResistanceStepValue, 0, 127, 0, 100);
    // now apply the larger linear function:
    calculatedResistanceStepValue = (calculatedResistanceStepValue * 1.8771) - 84;  //y=1.8771x−84.2857
    // lastly map it to the 0-1023 range that the pots will expect
    calculatedResistanceStepValue = map(calculatedResistanceStepValue, 0, 100, 0, 1023);
  }

  return calculatedResistanceStepValue;
}

// Loads a given preset value out of EEPROM and applies the settings
  // PARAMETERS:
    // int programChangeNumber:  The MIDI PC value that was passed in by the MIDI controller
void loadAndApplyPresetFromEEPROM(int programChangeNumber) {
  
  // We need to subtract 1 from the PC number to find our starting index in EEPROM
  int pc = programChangeNumber - 1;

  // The VERY FIRST byte [0] in EEPROM represents our MIDI channel - so we have to skip that one!
  // From the starting byte [1]
    // [1] = A byte (value between 0-255) representing the value for our Normal pot
    // [2] = A byte (value between 0-255) representing the value for our Brite pot
    // [3] = A byte (value between 0-255) representing the value for our Volume pot
    // [4] = A byte (value between 0-255) representing the value for our Bass pot  
    // [5] = A byte (value between 0-255) representing the value for our Mid pot
    // [6] = A byte (value between 0-255) representing the value for our Treble pot
    // [7] = A byte (value between 0-255) representing the boost value - 0 = boost disabled, anything else = boost engaged

  // So, for a PC of 1, we subtract 1 and get 0, then add 1 to skip the MIDI channel EEPROM byte.  1 is our starting address, and we follow the example above
  // Then, for a PC of 3, we subtract 1 and get 2, then add 1 to skip the MIDI channel EEPROM byte.  Multiply 2 times the length of a preset (7) to get 14, then add 1, and 15 is our starting address

  int startIndex = (pc * PRESET_BYTE_LENGTH) + 1;

  int normal = EEPROM[startIndex];
  int brite = EEPROM[startIndex + 1];
  int volume = EEPROM[startIndex + 2];
  int bass = EEPROM[startIndex + 3];
  int mid = EEPROM[startIndex + 4];
  int treble = EEPROM[startIndex + 5];
  int boost = EEPROM[startIndex + 6];

  // the "setPotentiometer" methods expect a value from 0-1023, not 0-255, so we have to scale our values before sending them in
  normal = map(normal, 0, 255, 0, 1023);
  brite = map(brite, 0, 255, 0, 1023);
  volume = map(volume, 0, 255, 0, 1023);
  bass = map(bass, 0, 255, 0, 1023);
  mid = map(mid, 0, 255, 0, 1023);
  treble = map(treble, 0, 255, 0, 1023);

  // set the potentiometer values
  setNormalDigitalPotentiometer(normal);
  setBriteDigitalPotentiometer(brite);
  setVolumeDigitalPotentiometer(volume);
  setBassDigitalPotentiometer(bass);
  setMidDigitalPotentiometer(mid);
  setTrebleDigitalPotentiometer(treble);

  // determine boost value
  bool boostEnabled = boost > 0;

  // activate the footswitches
  activateEffectFootswitch(true, false, false);
  activateBoostFootswitch(true, boostEnabled);
}

// Saves the current potentiomter settings and boost switch settings to EEPROM
  // PARAMETERS:
    // int programChangeNumber:  The preset number to be saved (this is what you would then pass in from your MIDI controller to recall it)
void savePresetToEEPROM(int programChangeNumber) {
  
  // We need to subtract 1 from the PC number to find our starting index in EEPROM
  int pc = programChangeNumber - 1;

  // Add 1 so we don't write over the MIDI channel byte in EEPROM
  int startIndex = (pc * PRESET_BYTE_LENGTH) + 1;

  // Our pot readings are int (2 bytes), but we want to store in EEPROM as 1 byte, so we have to scale from 0-1023 to 0-255
  int normalValue = map(normalPotReading, 0, 1023, 0, 255);
  int briteValue = map(britePotReading, 0, 1023, 0, 255);
  int volumeValue = map(volumePotReading, 0, 1023, 0, 255);
  int bassValue = map(bassPotReading, 0, 1023, 0, 255);
  int midValue = map(midPotReading, 0, 1023, 0, 255);
  int trebleValue = map(treblePotReading, 0, 1023, 0, 255);

  // figure out boost
  int boostValue = boostEnabled ? 1 : 0;

  // write all values to EEPROM
  EEPROM[startIndex] = normalValue;
  EEPROM[startIndex + 1] = briteValue;
  EEPROM[startIndex + 2] = volumeValue;
  EEPROM[startIndex + 3] = bassValue;
  EEPROM[startIndex + 4] = midValue;
  EEPROM[startIndex + 5] = trebleValue;
  EEPROM[startIndex + 6] = boostValue;
  
  blinkMainLedsAfterSuccessfulPresetSave();
}

// Blinks both the "Effect/Bypass" and "Boost" LED's several times after a preset is successfully saved
void blinkMainLedsAfterSuccessfulPresetSave() {
  // turn them both off
  digitalWrite(EFFECT_LED_PIN, LOW);
  digitalWrite(BOOST_LED_PIN, LOW);

  // blink 4 times (will take approximately 1.6 seconds)
  for (int i = 0; i < 4; i++) {
    digitalWrite(EFFECT_LED_PIN, HIGH);
    digitalWrite(BOOST_LED_PIN, HIGH);
    delay(200);

    digitalWrite(EFFECT_LED_PIN, LOW);
    digitalWrite(BOOST_LED_PIN, LOW);
    delay(200);
  }

  // at this point, both LED's will be off, so we need to turn them back on if applicable
  if (effectEnabled) {
    digitalWrite(EFFECT_LED_PIN, HIGH);
  }

  if (boostEnabled) {
    digitalWrite(BOOST_LED_PIN, HIGH);
  }
}

// Blinks the preset LED while we are in "Preset Save" mode
void blinkPresetLED() {
  if ((millis() - presetLedBlinkTimer) > PRESET_LED_BLINK_TIME) {
    presetLedBlinkTimer = millis();
    digitalWrite(PRESET_LED_PIN, !digitalRead(PRESET_LED_PIN));
  }
}

void seedEEPROMIfNeeded() {
  byte savedMidiChannel = EEPROM.read(0);
  byte midiChannelToSet;
  if (savedMidiChannel == 255) {
    // If the first byte is 255, that means it's never been written to, and we can assume that the entire EEPROM is empty
    // First set the default MIDI channel
    EEPROM[0] = DEFAULT_MIDI_CHANNEL;
    midiChannelToSet = DEFAULT_MIDI_CHANNEL;
    
    // Now, set every other value in EEPROM to 0 - by default the EEPROM will return 255 if never written to, 
    // which would actually be MAX value on the potentiometers!  So if you accidentally recalled a preset that you 
    // had never actually saved, it would MAX ALL VALUES!  And we don't want that!
    for (int i = i; i < 1024; i++) {
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
  
  // At this point, we know whatever is in "midiChannelToSet" is our valid MIDI channel, either because we fixed it above, or it was already set prior
  // Subtract 1 from it (MIDI Controller sends in 1-16, but the byte is zero-indexed so will be 0-15),
  // and set our local variable so we know what to listen for
  expectedMidiChannel = midiChannelToSet - 1;
}

// Standard program loop
void loop() {
  if (inPresetSaveMode) {
    blinkPresetLED();
    
    // How long have we been in preset save mode?
    if ((millis() - inPresetSaveModeStartTime) > PRESET_MODE_TIMEOUT) {
       // waited too long, give up on trying to save preset
       inPresetSaveMode = false;
       isPresetCurrentlyApplied = false;
       digitalWrite(PRESET_LED_PIN, LOW);
    }
  }
  else {
    // Check both the "Effect" and "Boost" footswitches, and use the analog switcher to enable/disable as needed
    checkFootswitchesState();

    // check potentiometer values and if they have changed, send the changes to the digital potentiometers  
    readAnalogPotentiometers(false);
  }

  // always process MIDI commands
  processMidiCommands();
}

// Logic remaining to test on Metro (test on larger Metro device before Atmega328p device)
  // The Rest - FILL THIS IN

// Logic already tested on Metro
  // Footswitch reading/behavior
  // LED Blinking/Behavior
  // MAX4701 Switching behavior
  // 25k SPI Digi-pot, by itself (need to test with it daisy chained to the 250k SPI digi-pot)
