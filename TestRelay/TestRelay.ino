#define LEFT_BUTTON_PIN 4
#define RIGHT_BUTTON_PIN 5
#define RELAY_ONE_PIN 8 // left button controls relay one, not sure which one it is (left or right?)
#define RELAY_TWO_PIN 9 // right button controls relay two, not sure which one it is (left or right?)

// state
int currentLeftButtonState = 0;
int currentRightButtonState = 0;
int lastLeftButtonState = 0;
int lastRightButtonState = 0;

void setup() {
  // setup pin modes
  pinMode(LEFT_BUTTON_PIN, INPUT);
  pinMode(RIGHT_BUTTON_PIN, INPUT);
  pinMode(RELAY_ONE_PIN, OUTPUT);
  pinMode(RELAY_TWO_PIN, OUTPUT);

  // set both relays to LOW 
  digitalWrite(RELAY_ONE_PIN, LOW);
  digitalWrite(RELAY_TWO_PIN, LOW);

  
  Serial.begin(9600);
  Serial.println();
  Serial.println("Test Relay Connection Active!");
}

void loop() {
  //currentLeftButtonState = digitalRead(LEFT_BUTTON_PIN);
  //currentRightButtonState = digitalRead(RIGHT_BUTTON_PIN);

  //if (currentLeftButtonState != lastLeftButtonState) {
    //Serial.println("left button has been pressed");
    //digitalWrite(RELAY_ONE_PIN, !digitalRead(RELAY_ONE_PIN));
    //lastLeftButtonState = currentLeftButtonState;
  //}
  //else if (currentRightButtonState != lastRightButtonState) {
    //Serial.println("right button has been pressed");
    //digitalWrite(RELAY_TWO_PIN, !digitalRead(RELAY_TWO_PIN));
    //lastRightButtonState = currentRightButtonState;
  //}

  Serial.println("Activate relay one, then wait 15 seconds");
  digitalWrite(RELAY_ONE_PIN, HIGH);
  delay(15000);
  Serial.println("Deactivate relay one, then wait 15 seconds");
  digitalWrite(RELAY_ONE_PIN, LOW);
  delay(15000);
}
