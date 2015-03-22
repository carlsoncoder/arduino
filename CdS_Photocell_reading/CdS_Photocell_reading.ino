int photocellPin = 0;
int photocellReading;
int ledPin = 7;

void setup()
{
  Serial.begin(9600);
  pinMode(ledPin, OUTPUT);
}

void loop()
{
  photocellReading = analogRead(photocellPin);
  Serial.print("Analog reading = ");
  Serial.println(photocellReading);
  
  if (photocellReading < 440)
  {
    //turn on LEDs
    digitalWrite(ledPin, HIGH);
  }
  else
  {
    //turn off LEDs
    digitalWrite(ledPin, LOW);
  }
  
  delay(3000);
}
