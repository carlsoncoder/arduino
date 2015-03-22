/* Adafruit Arduino - Lesson 3 - RGB LED */

int redPin = 11;
int greenPin = 10;
int bluePin = 9;

int redPotPin = 1;
int greenPotPin = 2;
int bluePotPin = 3;

int delayLength = 1250;

#define COMMON_ANODE

void setup()
{
  Serial.begin(9600);
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
}

void loop()
{
  Serial.println("entering loop now!");
  
  int redVal = analogRead(redPotPin) / 4;
  int greenVal = analogRead(greenPotPin) / 4;
  int blueVal = analogRead(bluePotPin) / 4;

  setColor(redVal, greenVal, blueVal);
  delay(100);
}

void colorRotator()
{
  setColor(255, 0, 0); //red
  delay(delayLength);
  setColor(0x4B, 0x0, 0x82); //indigo, in hex
  delay(delayLength);
  setColor(0, 255, 0); //green
  delay(delayLength);
  setColor(0, 0, 255); //blue
  delay(delayLength);
  setColor(255, 255, 0); //yellow
  delay(delayLength);
  setColor(80, 0, 80); //purple
  delay(delayLength);
  setColor(0, 255, 255); //aqua
  delay(delayLength);
}

void setColor(int red, int green, int blue)
{
  #ifdef COMMON_ANODE
    red = 255 - red;
    green = 255 - green;
    blue = 255 - blue;
  #endif
  
  analogWrite(redPin, red);
  analogWrite(greenPin, green);
  analogWrite(bluePin, blue);
}
