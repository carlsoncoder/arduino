#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ESP8266WiFi.h>

const char* ssid = "Verizon-SM-G900V-BBA4";
const char* password = "yudu497/";
const char* host = "www.carlsoncoder.com";

boolean startReading = false;
int stringIndex = 0;
char stringBuffer[16];

int lastR = -1;
int lastG = -1;
int lastB = -1;

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
}

void loop() {
  connectAndReadRGB();
  processRGBString();
  delay(1000);
}

void connectAndReadRGB() {
  Serial.print("connecting to ");
  Serial.println(host);
    
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("Connection failed");
    return;
  }

  // now create a URL for the request
  String url = "/admin/selectedcolor";
  
  // this will actually send the request
  client.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Connection: close\r\n\r\n");

  delay(20);
  readRGBValue();
}

void readRGBValue() {
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

void processRGBString() {
  int r = 0;
  int g = 0;
  int b = 0;

  char tempBuffer[4];
  memset(&tempBuffer, 0, 4);
  boolean previousNumberDone = false;
  int charIndexer = 0;
  int numArrayIndexer = 0;
  int numBuffer[3];

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

  for (int j=0; j < 3; j++) {
    if (j == 0) {
      r = numBuffer[j];
    }
    else if (j == 1) {
      g = numBuffer[j];
    }
    else if (j == 2) {
      b = numBuffer[j];
    }
  }

  setRGBValues(r, g, b);
}

void setRGBValues(int red, int green, int blue) {
   if (red == lastR && green == lastG && blue == lastB) {
    return;
   }

   // send data over serial as "RGB:24,150,90" string with line terminator
   Serial.print("RGB:");
   Serial.print(red);
   Serial.print(",");
   Serial.print(green);
   Serial.print(",");
   Serial.println(blue);

   lastR = red;
   lastG = green;
   lastB = blue;
}

