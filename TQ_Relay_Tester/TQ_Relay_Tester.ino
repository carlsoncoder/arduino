// How it works
  // When we start up, in "reset" or "startup" mode, COM and NC (2/3 and 9/8 - the left of COM) have continuity, and COM and NO (2/4, 8/7 - the right of COM) do NOT have continuity
  // The first switch changes, so 2/4 and 8/7 (right of COM) has continuity and 2/3 and 9/8 (left of COM) does NOT
  // Then it just flip-flops between NO/NC 9 more times

#define TQRELAY_POSITIVE_PIN 8
#define TQRELAY_NEGATIVE_PIN 9
#define LED_PIN 5

#define PULSE_TIME 3

bool isRelayInResetMode = false;

void setup() {
  pinMode(TQRELAY_POSITIVE_PIN, OUTPUT);
  pinMode(TQRELAY_NEGATIVE_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  // we always start in reset mode
  isRelayInResetMode = true;
}

void loop() {
  // blink LED five times to show we're starting
  blinkLED(5);
  testSingleCoilRelay();
}

void testSingleCoilRelay() {
  //wait 15 seconds before starting
  delay(15000);

  for (int i = 0; i < 10; i++) {
    // blink LED to show we're sending single coil pulse
    blinkLED(2);

    // figure out which pin to activate
    int pinToPulse = 0;
    if (isRelayInResetMode) {
      pinToPulse = TQRELAY_NEGATIVE_PIN;
    }
    else {
      pinToPulse = TQRELAY_POSITIVE_PIN;
    }

    // send the pulse to the appropriate pin
    digitalWrite(pinToPulse, HIGH);
    delay(PULSE_TIME);
    digitalWrite(pinToPulse, LOW);

    // set our state to the new value
    isRelayInResetMode = !isRelayInResetMode;

    // wait 15 seconds before the next round through the loop
    delay(15000);
    
  }

  // blink five times when we're done
  blinkLED(5);
}

void blinkLED(int numTimes) {
  for (int i = 0; i < numTimes; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(500);
    digitalWrite(LED_PIN, LOW);
    delay(500); 
  }
}
