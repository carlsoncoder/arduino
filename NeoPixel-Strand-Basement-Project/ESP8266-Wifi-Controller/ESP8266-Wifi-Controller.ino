#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ESP8266WiFi.h>

const char* ssid = "CARLSONWIFI";
const char* password = "W280N8295JJH";
const char* host = "192.168.1.64";

boolean startReading = false;
int stringIndex = 0;
char stringBuffer[16];

WiFiClient client;

void setup() {
  Serial.begin(9600);

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
  connectAndReadValues();
  processValues();
  delay(2000);
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
   // send data over serial as "RGB:24,150,90,200" string with line terminator (values in order are RED,GREEN,BLUE,BRIGHTNESS)
   Serial.print("RGB:");
   Serial.print(red);
   Serial.print(",");
   Serial.print(green);
   Serial.print(",");
   Serial.print(blue);
   Serial.print(",");
   Serial.println(brightness);   
}
