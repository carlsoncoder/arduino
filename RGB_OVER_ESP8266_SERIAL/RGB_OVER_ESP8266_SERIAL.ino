#define RedPin 9
#define GreenPin 10
#define BluePin 11

String serialInput;
int r, g, b;

void setup()
{
  Serial.begin(115200);
  
  pinMode(RedPin, OUTPUT);
  pinMode(GreenPin, OUTPUT);
  pinMode(BluePin, OUTPUT);
}

void loop()
{ 
  if (Serial.available() > 0) {
    serialInput = Serial.readStringUntil('\n');
    if (serialInput.startsWith("RGB:")) {
      String rgbSubstring = serialInput.substring(4);
      char rgbCharArray[rgbSubstring.length() + 1];
      strcpy(rgbCharArray, rgbSubstring.c_str());
      
      char *p = rgbCharArray;
      char *color;
      int counter = 0;
      while ((color = strtok_r(p, ",", &p)) != NULL) {
        counter++;
        if (counter == 1) {
          r = atoi(color);
        }
        else if (counter == 2) {
          g = atoi(color);
        }
        else if (counter == 3) {
          b = atoi(color);
        }
        else {
          break;
        }
      }

      setColors(r, g, b);

    }
  }
  
  delay(100);
}

void setColors(int red, int green, int blue) {
  analogWrite(RedPin, red);
  analogWrite(GreenPin, green);
  analogWrite(BluePin, blue);
}
