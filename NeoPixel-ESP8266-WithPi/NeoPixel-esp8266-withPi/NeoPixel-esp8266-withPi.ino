#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ESP8266WiFi.h>
#include <Adafruit_NeoPixel.h>

#define NEOPIXEL_PIN 14
#define NUMPIXELS 3

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, NEOPIXEL_PIN);

const char* ssid = "CARLSONWIFI";
const char* password = "W280N8295JJH";
const char* host = "192.168.1.58";

boolean startReading = false;
int stringIndex = 0;
char stringBuffer[16];

int lastR = -1;
int lastG = -1;
int lastB = -1;
int lastBrightness = -1;

WiFiClient client;

void setup() {
  Serial.begin(115200);

  // connect to the WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.print(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  
  Serial.println("");
  Serial.println("WiFi connected!");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // default brightness/color values
  setBrightnessValues(64);
  setColorValues(0, 0, 255);
}

void loop() {
  connectAndReadValues();
  processValues();
  delay(10000);
}

void connectAndReadValues() {
  const int httpPort = 3000;
  
  Serial.print("connecting to ");
  Serial.print(host);
  Serial.print(" at port ");
  Serial.println(httpPort);
    
  if (!client.connect(host, httpPort)) {
    Serial.println("Connection failed");
    return;
  }

  // now create a URL for the request
  String url = "/settings";
  
  // this will actually send the request
  client.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Connection: close\r\n\r\n");

  delay(20);
  readValues();
}

void readValues() {
  //reset our string index and our buffer
  stringIndex = 0;
  memset(&stringBuffer, 0, 16);

  while (true) {
    if (client.available()) {
      char c = client.read();
      if (c == '<') {
        startReading = true;
      }
      else if (startReading) {
        if (c != '>') {
          stringBuffer[stringIndex] = c;
          stringIndex++;
        }
        else {
          startReading = false;
          client.stop();
          client.flush();
          Serial.println("Closing connection");
          return;
        }
      }      
    }
  }
}

void processValues() {
  int r = 0;
  int g = 0;
  int b = 0;
  int brightness = 0;

  char tempBuffer[4];
  memset(&tempBuffer, 0, 4);
  boolean previousNumberDone = false;
  int charIndexer = 0;
  int numArrayIndexer = 0;
  int numBuffer[4];

  for (int i = 0; i < 16; i++) {
    char c = stringBuffer[i];
    if (previousNumberDone) {
      previousNumberDone = false;
      charIndexer = 0;
      numBuffer[numArrayIndexer] = atoi(tempBuffer);
      numArrayIndexer++;
      memset(&tempBuffer, 0, 4);
    }

    if (c == '\0') {
      // NULL terminator, end of string, break
      break;
    }
    else if (c == ',') {
      previousNumberDone = true;
    }
    else {
      tempBuffer[charIndexer] = c;
    }

    charIndexer++;
  }

  numBuffer[numArrayIndexer] = atoi(tempBuffer);

  for (int j=0; j < 4; j++) {
    if (j == 0) {
      r = numBuffer[j];
    }
    else if (j == 1) {
      g = numBuffer[j];
    }
    else if (j == 2) {
      b = numBuffer[j];
    }
    else if (j == 3) {
      brightness = numBuffer[j]; 
    }
  }

  setValues(r, g, b, brightness);
}

void setValues(int red, int green, int blue, int brightness) {
   Serial.print("Found the following values from web service call: ");
   Serial.print(" R: ");
   Serial.print(red);
   Serial.print(" G: ");
   Serial.print(green);
   Serial.print(" B: ");
   Serial.print(blue);
   Serial.print(" Brightness: ");
   Serial.println(brightness);
   
   if (brightness != lastBrightness) {
       Serial.print("Updating brightness to ");
       Serial.println(brightness);

       setBrightnessValues(brightness);
        
       lastBrightness = brightness;
   }
   
   if (red != lastR || green != lastG || blue != lastB) {
       Serial.print("Updating R to ");
       Serial.println(red); 
       Serial.print("Updating G to ");
       Serial.println(green); 
       Serial.print("Updating B to ");
       Serial.println(blue); 
       
       setColorValues(red, green, blue);

       lastR = red;
       lastG = green;
       lastB = blue;
   }
}

void setColorValues(int red, int green, int blue) {
  for (int i = 0; i < NUMPIXELS; i++) {
    strip.setPixelColor(i, red, green, blue);  
  }
  
  strip.show();
  delay(100);
}

void setBrightnessValues(int brightness) {
  strip.setBrightness(brightness);
  strip.show();
  delay(100);
}
