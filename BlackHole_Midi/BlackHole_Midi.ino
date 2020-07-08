#include <SPI.h>
#include <Wire.h>
#include <EEPROM.h>
#include <ReceiveOnlySoftwareSerial.h>  //http://gammon.com.au/Arduino/ReceiveOnlySoftwareSerial.zip

// Possible MIDI CC commands that could be sent
#define MIDI_CC_EFFECT              1           // CC to turn the effect On or Off - Sending a value of 127 enages the pedal, sending a value of 0 disengages the pedal
#define MIDI_CC_BOOST               2           // CC to turn the boost On or Off - Sending a value of 127 enages the boost, sending a value of 0 disengages the boost
#define MIDI_CC_NORMALPOT           3           // CC to change the value of the "Normal" pot - 0 is fully CCW (Min value), 127 is fully CW (Max value)
#define MIDI_CC_BRITEPOT            4           // CC to change the value of the "Brite" pot - 0 is fully CCW (Min value), 127 is fully CW (Max value)
#define MIDI_CC_VOLUMEPOT           5           // CC to change the value of the "Volume" pot - 0 is fully CCW (Min value), 127 is fully CW (Max value)
#define MIDI_CC_BASSPOT             6           // CC to change the value of the "Bass" pot - 0 is fully CCW (Min value), 127 is fully CW (Max value)
#define MIDI_CC_MIDPOT              7           // CC to change the value of the "Mid" pot - 0 is fully CCW (Min value), 127 is fully CW (Max value)
#define MIDI_CC_TREBLEPOT           8           // CC to change the value of the "Treble" pot - 0 is fully CCW (Min value), 127 is fully CW (Max value)

// MIDI constants
#define MIDI_LISTEN_CHANNEL         7           // MIDI channel to listen on - We listen on MIDI Channel 8, but is zero-indexed so we look for 7.  MIDI controller should send commands on channel 8!
#define MIDI_PROGRAM_CHANGE         192         // The number that represents a MIDI PC (Program Change) command
#define MIDI_CONTROL_CHANGE         176         // The number that represents a MIDI CC (Control Change) command

// Arduino PIN definitions
#define MIDI_RECEIVE_PIN            0           // Used for the Arduino to receive data from the MIDI-IN port
#define EFFECT_FOOTSWITCH_PIN       1           // Used to read the state of the footswitch for turning the effect on/off
#define BOOST_FOOTSWITCH_PIN        2           // Used to read the state of the footswitch for enabling/disabling the BOOST
#define MAX_SWITCH_EFFECT           3           // Used to switch the effect on/off - Goes to the IN1/IN2 Pin on MAX4701
#define MAX_SWITCH_BOOST            4           // Used to switch the boost on/off - Goes to the IN3/IN4 Pin on MAX4701
#define ANALOG_POT_NORMAL           5           // Used to read in the value of the analog potentiometer for the "Normal" pot
#define ANALOG_POT_BRITE            6           // Used to read in the value of the analog potentiometer for the "Brite" pot
#define ANALOG_POT_VOLUME           7           // Used to read in the value of the analog potentiometer for the "Volume" pot
#define ANALOG_POT_BASS             8           // Used to read in the value of the analog potentiometer for the "Bass" pot
#define ANALOG_POT_MID              9           // Used to read in the value of the analog potentiometer for the "Mid" pot
#define ANALOG_POT_TREBLE           10          // Used to read in the value of the analog potentiometer for the "Treble" pot
#define EFFECT_LED_PIN              11          // Used to indicate if the effect is on or off
#define BOOST_LED_PIN               12          // Used to indicate if the boost is enabled or not
#define PRESET_LED_PIN              13          // Used to indicate the state of the currently selected preset, if any
#define CS_SELECTPIN_25K            14          // Used as the slave select pin for the 25k ohm Digi-pot connected over SPI (Mids)
#define CS_SELECTPIN_250K           15          // Used as the slave select pin for the 250k ohm Digi-pot connected over SPI (Treble)
#define BASSPOT_ADSEL_PIN           16          // Used to set the address select pin on ONE of the 1M ohm Digi-pots (the one used for ONLY the Bass control). This should be hooked up to AD0 on the digi-pot IC

// I2C addresses
#define NORMAL_BRITE_POT_I2C_ADDR   44          // The I2C address to communicate with the digi-pot for the Normal/Brite settings (should have AD0 and AD1 pulled low (default/ground) so no pins needed from arduino to set address)
#define BASS_POT_I2C_ADDR           45          // The I2C address to communicate with the digi-pot for Bass settings (should have AD0 pulled HIGH at program startup by an Arduino GPIO pin)
#define VOLUME_POT_I2C_ADDR         42          // The I2C address to communicate with the digi-pot for Volume settings

// Constants used elsewhere
#define MIDI_WAIT_TIME              250         // Delay time to use when starting up the MIDI SoftwareSerial channel
#define PRESET_BYTE_LENGTH          25          // The amount of bytes used when saving a preset
#define PRESET_LED_BLINK_TIME       150         // The time in milliseconds between blinking the Preset LED in "Preset Save" mode
#define MIDI_CHECK_FREQUENCY_TIME   50          // The time in milliseconds to wait between checking the MIDI serial channel for new messages
#define POT_CHECK_FREQUENCY_TIME    15          // The time in milliseconds to check to see if the analog pot values have changed
#define ALLOWABLE_POT_VARIANCE      15          // The amount of variation in the analog pot values (from 0-1023) that we'll allow without considering it a "change" (due to minor discrepancies during analogRead operations)
#define DEBOUNCE_THRESHOLD          50          // The time in milliseconds for a button press to be considered valid (and not noise)
#define LONG_PRESS_THRESHOLD        1500        // The time in milliseconds for a button press to be considered a "long press" (to go into "Preset Save" mode)
#define PRESET_MODE_TIMEOUT         15000       // The amount of time in milliseconds we will wait in "Preset Save" mode before giving up
#define SPI_DELAY_TIME              50          // The time in milliseconds to delay operations after pulling an SPI CS pin HIGH

// state management
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
bool boostButtonLongPressActiveUnused = false;

// timers
unsigned long presetLedBlinkTimer;
unsigned long midiReadTimer;
unsigned long potReadTimer;
unsigned long effectButtonTimer;
unsigned long boostButtonTimer;

// Incoming MIDI values
byte midiByte;
byte midiChannel;
byte midiCommand;
byte midiProgramOrControlChangeByte;
byte midiControlChangeValueByte;

// Setup the MIDI serial connection
ReceiveOnlySoftwareSerial midiSerial(MIDI_RECEIVE_PIN);

// Setup function - initialize all pins, libraries, and state variables, as well as start up the MIDI serial connection
void setup() {
  // Setup all LED pins as OUTPUT
  pinMode(EFFECT_LED_PIN, OUTPUT);
  pinMode(BOOST_LED_PIN, OUTPUT);
  pinMode(PRESET_LED_PIN, OUTPUT);

  // Setup our footswitch pins as INPUT
  pinMode(EFFECT_FOOTSWITCH_PIN, INPUT);
  pinMode(BOOST_FOOTSWITCH_PIN, INPUT);

  // Setup our pins on the MAX4701 switch as OUTPUT
  pinMode(MAX_SWITCH_EFFECT, OUTPUT);
  pinMode(MAX_SWITCH_BOOST, OUTPUT);

  // Setup our CS select pins (for SPI) and our I2C Address Select pin for the Bass digi-pot to OUTPUT
  pinMode(CS_SELECTPIN_25K, OUTPUT);
  pinMode(CS_SELECTPIN_250K, OUTPUT);
  pinMode(BASSPOT_ADSEL_PIN, OUTPUT);

  // Pull the "BASSPOT_ADSEL_PIN" HIGH so we set our I2C address appropriately
  digitalWrite(BASSPOT_ADSEL_PIN, HIGH);

  // Pull the CS pins HIGH so they're ready for SPI communication
  digitalWrite(CS_SELECTPIN_25K, HIGH);
  digitalWrite(CS_SELECTPIN_250K, HIGH);

  // start the Wire (I2C) and the SPI library communications
  Wire.begin();
  SPI.begin();

  // start our timers
  presetLedBlinkTimer = millis();
  potReadTimer = millis();
  midiReadTimer = millis();

  // Some of the digipots are volatile, so they won't hold value after a restart of the controller
  // read them all into the global variables and force them to set the digital pots as well on startup
  readAnalogPotentiometers(true);

  // always activate the effect on startup
  activateEffectFootswitch(true, false, false);
  
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
  int tempNormalPotReading = analogRead(ANALOG_POT_NORMAL);
  int tempBritePotReading = analogRead(ANALOG_POT_BRITE);
  int tempVolumePotReading = analogRead(ANALOG_POT_VOLUME);
  int tempBassPotReading = analogRead(ANALOG_POT_BASS);
  int tempMidPotReading = analogRead(ANALOG_POT_MID);
  int tempTreblePotReading = analogRead(ANALOG_POT_TREBLE);

  // check if any have changed since our last reading
  bool normalPotChanged = hasPotentiometerValueChanged(normalPotReading, tempNormalPotReading);
  bool britePotChanged = hasPotentiometerValueChanged(britePotReading, tempBritePotReading);
  bool volumePotChanged = hasPotentiometerValueChanged(volumePotReading, tempVolumePotReading);
  bool bassPotChanged = hasPotentiometerValueChanged(bassPotReading, tempBassPotReading);
  bool midPotChanged = hasPotentiometerValueChanged(midPotReading, tempMidPotReading);
  bool treblePotChanged = hasPotentiometerValueChanged(treblePotReading, tempTreblePotReading);

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

  if (isPresetCurrentlyApplied && (normalPotChanged || britePotChanged || volumePotChanged || bassPotChanged || midPotChanged || treblePotChanged)) {
    // IMPORTANT NOTE:  If you have a preset currently applied/active, and you change one or more pots, that will "invalidate" the preset (the Preset active LED will turn off)
    // HOWEVER - Any potentiometers you have NOT changed, will still be set to their preset values, which won't really match up with the "physical location" of the analog potentiometers
    // We do this intentionally - so that if you have an active preset that you like, but just want to change one value, you can just change that one pot until you get it how you like, 
    // then you could save the preset with the same PC number to overwrite it in EEPROM
    isPresetCurrentlyApplied = false;
    digitalWrite(PRESET_LED_PIN, LOW);
  }
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
    int midValue = midReading;
    writeToSpiPotentiometer(CS_SELECTPIN_25K, midReading);
}

// Sets the value on the digital potentiometer for the "Treble" knob
  // PARAMETERS:
    // int trebleReading:  The value to be applied to the digital potentiometer
void setTrebleDigitalPotentiometer(int trebleReading) {
    // Treble is a 1024 tap (0-1023) pot, so NO need to scale the current reading 
    int trebleValue = trebleReading;
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
  digitalWrite(spiCSSelectPin, LOW);
  delay(SPI_DELAY_TIME);

  uint8_t bytes[3];

  // Since we're ONLY ever using ONE of the two pots on the SPI chips (for 25k and 250k ohm digipots), we can hard-code the cmd to "0xB0", and make sure we ONLY use Wiper 1 (RDAC1 register)
  // If we did want to use the other (Wiper 2 - RDAC2 register), it should just use "0xB1" for the first byte
  bytes[0] = 0xB0;
  bytes[1] = value >> 8;
  bytes[2] = value & 0xFF;

  for (int i = 0; i < 3; i++) {
    SPI.transfer(bytes[i]);
  }
  
  digitalWrite(spiCSSelectPin, HIGH);
}

// Writes a value to one of the SPI digi-pots (Mid and Treble pots use SPI)
  // PARAMETERS:
    // int address:  The I2C address to communicate with
    // int value:  The value (between 0-255) to be sent to the potentiometer
    // int potRegister:  The value (either 0 or 1) representing which register to write to (these are dual-pot IC's)
  // REMARKS:
    // Adapted From: https://github.com/RobTillaart/Arduino/tree/master/libraries/AD524X
    // Coded to work with the AD524X Series - https://www.analog.com/media/en/technical-documentation/data-sheets/AD5241_5242.pdf
void writeToI2CPotentiometer(int address, int value, int potRegister) {
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
    // int switchPin:  The arduino pin tied to the footswitch
    // bool &buttonActive: Boolean passed by reference from the caller to note if the button is currently active or not
    // bool &buttonLongPressActive: Boolean passed by reference from the caller to note if the button is currently "long press" active or not
    // unsigned long &buttonTimer: Timer (long) passed by reference to see when the button was first pressed
    // unsigned long &buttonPressDuration: Duration (long) passed by reference to see how long the button has been pressed
  // REMARKS:
     // Adapted From: https://bloggymcblogface.blog/example-arduino-code-for-debouncing-and-long-pressing-buttons/
void checkFootswitch(bool isEffectsFootswitch, int switchPin, bool &buttonActive, bool &buttonLongPressActive, unsigned long &buttonTimer, unsigned long &buttonPressDuration) {

  // TODO: JUSTIN: VERIFY IF THIS SHOULD BE HIGH OR LOW!
  // if the button pin reads LOW, the button is pressed (negative/ground switch)
  if (digitalRead(switchPin) == LOW) {
    // mark the button as active, and start the timer
    if (!buttonActive) {
      buttonActive = true;
      buttonTimer = millis();
    }
    
    // calculate the button press duration by subtracting the button time from the boot time
    buttonPressDuration = millis() - buttonTimer;

    if (isEffectsFootswitch) {
      // we only check for long-press on the effects footswitch button
      // mark the button as long-pressed if the button press duration exceeds the long press threshold
      if ((buttonPressDuration > LONG_PRESS_THRESHOLD) && (buttonLongPressActive == false)) {
        inPresetSaveMode = true;
        inPresetSaveModeStartTime = millis();
        buttonLongPressActive = true;
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
  checkFootswitch(false, BOOST_FOOTSWITCH_PIN, boostButtonActive, boostButtonLongPressActiveUnused, boostButtonTimer, boostButtonPressDuration);
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
    if (midiChannel == MIDI_LISTEN_CHANNEL) {

      // read the next byte - it will either be our PC# or CC#
      midiProgramOrControlChangeByte = midiSerial.read();
      
      // process PC and CC commands differently
      if (midiCommand == MIDI_PROGRAM_CHANGE) {
        if (midiProgramOrControlChangeByte == 99) {
          // PC#99 is a special case - we force reset all digi-pots to be whatever the analog pots are currently set to
          inPresetSaveMode = false;
          isPresetCurrentlyApplied = false;
          readAnalogPotentiometers(true);
          digitalWrite(PRESET_LED_PIN, LOW);
        }
        else if (midiProgramOrControlChangeByte > 40 || midiProgramOrControlChangeByte == 0) {
          // Invalid - we only allow program changes of 1-40 (except 99 which is a special case above)
          inPresetSaveMode = false;
          isPresetCurrentlyApplied = false;
          digitalWrite(PRESET_LED_PIN, LOW);
        }
        else if (inPresetSaveMode) {
          inPresetSaveMode = false;
          savePresetToEEPROM(midiProgramOrControlChangeByte);
          isPresetCurrentlyApplied = true;
          digitalWrite(PRESET_LED_PIN, HIGH);
        }
        else {
          isPresetCurrentlyApplied = true;
          loadAndApplyPresetFromEEPROM(midiProgramOrControlChangeByte);
          digitalWrite(PRESET_LED_PIN, HIGH);
        }
      }
      else if (midiCommand == MIDI_CONTROL_CHANGE) {
        // for a CC command, we need to read the NEXT byte, which will be our value to pass for the CC#
        midiControlChangeValueByte = midiSerial.read();

        switch (midiProgramOrControlChangeByte) {
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

  // From the starting byte [0]
    // [0-3] = Four digits representing a number from 0-1023 for the Normal pot
    // [4-7] = Four digits representing a number from 0-1023 for the Brite pot
    // [8-11] = Four digits representing a number from 0-1023 for the Volume pot
    // [12-15] = Four digits representing a number from 0-1023 for the Bass pot  
    // [16-19] = Four digits representing a number from 0-1023 for the Mid pot
    // [20-23] = Four digits representing a number from 0-1023 for the Treble pot
    // [24] = 1 digit (0 or 1) representing the boost value

    // So, for a PC of 1, we subtract 1 and get 0.  0 is our starting address, and we follow the example above
    // Then, for a PC of 3, we subtract 1 and get 2.  Multiply 2 times the length of a preset (25) to get 50, and 50 is our starting address

  // EEPROM eventually we will get a string like this:  "0000102305120768102302561"
    // In this example, 0000 = normal pot, 1023 = brite pot, 0512 = volume pot, 0768 = bass pot, 1023 = mid pot, 0256 = treble pot, 1 = boost
    // Need to remove all leading zeroes from Normal/Brite/Volume/Bass/Mid/Treble and then convert to INT (done with the "convertCharArrayToInt(...)" function)
    // Need to convert the boost value (0 or 1) to a bool

  int startIndex = pc * PRESET_BYTE_LENGTH;

  char rawNormal[4];
  rawNormal[0] = EEPROM[startIndex];
  rawNormal[1] = EEPROM[startIndex + 1];
  rawNormal[2] = EEPROM[startIndex + 2];
  rawNormal[3] = EEPROM[startIndex + 3];

  char rawBrite[4];
  rawBrite[0] = EEPROM[startIndex + 4];
  rawBrite[1] = EEPROM[startIndex + 5];
  rawBrite[2] = EEPROM[startIndex + 6];
  rawBrite[3] = EEPROM[startIndex + 7];

  char rawVolume[4];
  rawVolume[0] = EEPROM[startIndex + 8];
  rawVolume[1] = EEPROM[startIndex + 9];
  rawVolume[2] = EEPROM[startIndex + 10];
  rawVolume[3] = EEPROM[startIndex + 11];

  char rawBass[4];
  rawBass[0] = EEPROM[startIndex + 12];
  rawBass[1] = EEPROM[startIndex + 13];
  rawBass[2] = EEPROM[startIndex + 14];
  rawBass[3] = EEPROM[startIndex + 15];

  char rawMid[4];
  rawMid[0] = EEPROM[startIndex + 16];
  rawMid[1] = EEPROM[startIndex + 17];
  rawMid[2] = EEPROM[startIndex + 18];
  rawMid[3] = EEPROM[startIndex + 19];
  
  char rawTreble[4];
  rawTreble[0] = EEPROM[startIndex + 20];
  rawTreble[1] = EEPROM[startIndex + 21];
  rawTreble[2] = EEPROM[startIndex + 22];
  rawTreble[3] = EEPROM[startIndex + 23];

  char rawBoost = EEPROM[startIndex + 24];
  
  int normalValue = convertCharArrayToInt(rawNormal);
  int briteValue = convertCharArrayToInt(rawBrite);
  int volumeValue = convertCharArrayToInt(rawVolume);
  int bassValue = convertCharArrayToInt(rawBass);
  int midValue = convertCharArrayToInt(rawMid);
  int trebleValue = convertCharArrayToInt(rawTreble);
  bool boost = atoi(rawBoost) == 1;

  setNormalDigitalPotentiometer(normalValue);
  setBriteDigitalPotentiometer(briteValue);
  setVolumeDigitalPotentiometer(volumeValue);
  setBassDigitalPotentiometer(bassValue);
  setMidDigitalPotentiometer(midValue);
  setTrebleDigitalPotentiometer(trebleValue);
  
  activateEffectFootswitch(true, false, false);
  activateBoostFootswitch(true, boost);
}

// Saves the current potentiomter settings and boost switch settings to EEPROM
  // PARAMETERS:
    // int programChangeNumber:  The preset number to be saved (this is what you would then pass in from your MIDI controller to recall it)
void savePresetToEEPROM(int programChangeNumber) {
  int pc = programChangeNumber - 1;
  int startIndex = pc * PRESET_BYTE_LENGTH;

  // Normal Pot
  writePotValueToEEPROM(normalPotReading, startIndex);
  writePotValueToEEPROM(britePotReading, startIndex + 4);
  writePotValueToEEPROM(volumePotReading, startIndex + 8);
  writePotValueToEEPROM(bassPotReading, startIndex + 12);
  writePotValueToEEPROM(midPotReading, startIndex + 16);
  writePotValueToEEPROM(treblePotReading, startIndex + 20);

  char boostVal = boostEnabled ? '1' : '0';
  EEPROM[startIndex + 24] = boostVal;
  
  blinkMainLedsAfterSuccessfulPresetSave();
}

// Takes a potentiometer value (0-1023) and writes it in an appropriate manner to EEPROM
  // PARAMETERS:
    // int potValue:  The potentiometer value (0-1023) to be saved
    // int startingAddress:  The address in EEPROM to start writing at
void writePotValueToEEPROM(int potValue, int startingAddress) {
  String potValueString = String(potValue);
  
  if (potValue == 0) {
    EEPROM[startingAddress] = '0';
    EEPROM[startingAddress + 1] = '0';
    EEPROM[startingAddress + 2] = '0';
    EEPROM[startingAddress + 3] = '0';
  }
  else if (potValue < 10) {
    EEPROM[startingAddress] = '0';
    EEPROM[startingAddress + 1] = '0';
    EEPROM[startingAddress + 2] = '0';
    EEPROM[startingAddress + 3] = potValueString[0];    
  }
  else if (potValue < 100) {
    EEPROM[startingAddress] = '0';
    EEPROM[startingAddress + 1] = '0';
    EEPROM[startingAddress + 2] = potValueString[0];
    EEPROM[startingAddress + 3] = potValueString[1];   
  }
  else if (potValue < 1000) {
    EEPROM[startingAddress] = '0';
    EEPROM[startingAddress + 1] = potValueString[0];
    EEPROM[startingAddress + 2] = potValueString[1];
    EEPROM[startingAddress + 3] = potValueString[2];
  }
  else {
    EEPROM[startingAddress] = potValueString[0];
    EEPROM[startingAddress + 1] = potValueString[1];
    EEPROM[startingAddress + 2] = potValueString[2];
    EEPROM[startingAddress + 3] = potValueString[3];    
  } 
}

// Converts a char array into an integer
  // PARAMETERS:
    // char *arrayValue: A pointer to the char[] to be converted
  // RETURNS:
    // A mapped integer value between 0-1023 that can be used to set the potentiometer value
int convertCharArrayToInt(char *arrayValue) {
  char tempBuffer[5];

  // find first instance of non-zero char
  bool nonZeroFound = false;
  int tempBufferIndex = 0;
  for (int i = 0; i < 4; i++) {
    char val = arrayValue[i];
    if (val == '0' && !nonZeroFound) {
      continue;
    }
    else if (val != '0' && !nonZeroFound) {
      nonZeroFound = true;
      tempBuffer[tempBufferIndex] = val;
      tempBufferIndex++;
    }
    else {
      tempBuffer[tempBufferIndex] = val;
      tempBufferIndex++;      
    }
  }

  if (!nonZeroFound) {
    return 0;
  }
  else if (tempBufferIndex == 1) {
    // only want the first value
    char modBuffer[2];
    modBuffer[0] = tempBuffer[0];
    modBuffer[1] =  '\0';
    return atoi(modBuffer);
  }
  else if (tempBufferIndex == 2) {
    // only want the first and second value
    char modBuffer[3];
    modBuffer[0] = tempBuffer[0];
    modBuffer[1] = tempBuffer[1];
    modBuffer[2] =  '\0';
    return atoi(modBuffer);
  }
  else if (tempBufferIndex == 3) {
    // only want the first, second, and third value
    char modBuffer[4];
    modBuffer[0] = tempBuffer[0];
    modBuffer[1] = tempBuffer[1];
    modBuffer[2] = tempBuffer[2];
    modBuffer[3] =  '\0';
    return atoi(modBuffer);
  }
  else if (tempBufferIndex == 4) {
    // used all four, just set the null-terminator and convert
    tempBuffer[4] = '\0';
    return atoi(tempBuffer);
  }
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
    if ((millis() - potReadTimer) > POT_CHECK_FREQUENCY_TIME) {
      readAnalogPotentiometers(false);
      potReadTimer = millis();
    }
  }

  // always process MIDI commands
  if ((millis() - midiReadTimer) > MIDI_CHECK_FREQUENCY_TIME) {
      processMidiCommands();
      midiReadTimer = millis();
  }
}
