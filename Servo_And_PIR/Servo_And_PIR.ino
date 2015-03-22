#include <Servo.h>

int servoPin = 9;
int pirPin = 12;

Servo servo;

int angle = 0;
boolean goingUp = true;

void setup()
{
  servo.attach(servoPin);
  pinMode(pirPin, INPUT);
  Serial.begin(9600);
}

void loop()
{
  long now = millis();
  if (digitalRead(pirPin) == HIGH)
  {
    Serial.println("MOVEMENT");
    if (goingUp)
    {
      if (angle >= 179)
      {
        goingUp = false;
        angle--;
      }
      else
      {
        angle++;
      }
    }
    else
    {
      if (angle <= 1)
      {
        goingUp = true;
        angle++;
      }
      else
      {
        angle--;
      }
    }
    
    servo.write(angle);
  }
  else
  {
    Serial.println("NOTHING");
  }
  
  delay(15);
  

  
  //for (angle = 180; angle > 0; angle--)
  //{
    //servo.write(angle);
    //delay(15);
  //}
}
