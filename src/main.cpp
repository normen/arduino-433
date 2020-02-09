/**
 * arduino-433 v1.0.3
 * Use the arduino platform to control 433MHz switches
 * (c) by Normen Hansen, released under MIT license
***/

// comment this out when using Arduino IDE
#include <Arduino.h>

// input pin for RC receiver -> D2 for 8266 by default, 3 for Arduino Micro by default
#define RC_INPUT_PIN D2
// output pin for RC transmitter -> D1 for 8266 by default, 4 for Arduino Micro by default
#define RC_OUTPUT_PIN D1

// uncomment this to use CC1101 insted of simple 443 sender/receivers
//#define USE_CC1101

// uncomment this to start websocket server for homebridge -> set WLAN login creds
//#define USE_WEBSOCKET

// uncomment this to use ESPilight insted of rc-switch to decode switches -> different message format
//#define USE_ESPILIGHT

// WIFI login if using websocket
#ifdef USE_WEBSOCKET
#define WIFI_SSID "my_wifi_ssid"
#define WIFI_PASS "my_wifi_pass"
#define WIFI_HOSTNAME "arduino-433"
#endif

// size of local message buffer
#define CHAR_BUFFER_SIZE 255


/**** CODE STARTS HERE ****/
#ifdef USE_ESPILIGHT
#include <ESPiLight.h>
extern "C" {
#include "pilight/libs/pilight/core/json.h"
}
#else
#include <RCSwitch.h>
#endif

#ifdef USE_CC1101
#include <ELECHOUSE_CC1101_RCS_DRV.h>
#endif

#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#elif ESP32
#include <WiFi.h>
#include <WiFiMulti.h>
#endif

#ifdef USE_WEBSOCKET
#include <WebSocketsServer.h>
#endif

#ifdef USE_ESPILIGHT
ESPiLight rf(RC_OUTPUT_PIN);  // use -1 to disable transmitter
#else //RCSWITCH
RCSwitch mySwitch = RCSwitch();
#endif

char receivedChars[CHAR_BUFFER_SIZE]; // an array to store the received data
boolean newData = false; // was a full new string received?
String dash = "/";

#ifdef USE_WEBSOCKET
#ifdef ESP8266
ESP8266WiFiMulti WiFiMulti;
#elif ESP32
WiFiMulti WiFiMulti;
#endif
WebSocketsServer webSocket = WebSocketsServer(80);
uint8_t lastClientNum = 0;

void copyPayload(char* src, char* dst, size_t len) {
    for (int i = 0; i < len; i++) {
        *dst++ = *src++;
    }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      break;
    case WStype_CONNECTED:
        webSocket.sendTXT(num, "OK");
      break;
    case WStype_TEXT:
      lastClientNum = num;
      if(length < CHAR_BUFFER_SIZE){
        copyPayload((char*)payload, receivedChars, length);
        receivedChars[length]='\0';
        newData = true;
      }
      break;
    case WStype_BIN:
      break;
    default:
      break;
  }
}
#endif

#ifdef USE_ESPILIGHT
void espiCallback(const String &protocol, const String &message, int status,
                size_t repeats, const String &deviceID) {
  if (status == VALID) {
    String out = "{\"type\":\""+protocol+"\",\"message\":"+message+"}";
    Serial.println(out);
#ifdef USE_WEBSOCKET
    webSocket.broadcastTXT(out);
#endif
  }
}
#endif

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(100);
#ifdef USE_CC1101
//CC1101 Settings:                (Settings with "//" are optional!)
//ELECHOUSE_cc1101.setRxBW(16);     // set Receive filter bandwidth (default = 812khz) 1 = 58khz, 2 = 67khz, 3 = 81khz, 4 = 101khz, 5 = 116khz, 6 = 135khz, 7 = 162khz, 8 = 203khz, 9 = 232khz, 10 = 270khz, 11 = 325khz, 12 = 406khz, 13 = 464khz, 14 = 541khz, 15 = 650khz, 16 = 812khz.
  ELECHOUSE_cc1101.setMHZ(433.92); // Here you can set your basic frequency. The lib calculates the frequency automatically (default = 433.92).The cc1101 can: 300-348 MHZ, 387-464MHZ and 779-928MHZ. Read More info from datasheet.
  ELECHOUSE_cc1101.Init(PA10);    // must be set to initialize the cc1101! set TxPower  PA10, PA7, PA5, PA0, PA_10, PA_15, PA_20, PA_30.
  ELECHOUSE_cc1101.SetRx();  // set Recive on
#endif

#ifdef USE_ESPILIGHT
  rf.setCallback(espiCallback);
  rf.initReceiver(RC_INPUT_PIN);
#else
  mySwitch.enableReceive(digitalPinToInterrupt(RC_INPUT_PIN));
  mySwitch.enableTransmit(RC_OUTPUT_PIN);
  mySwitch.setRepeatTransmit(8);
#endif

#ifdef ESP8266
  WiFi.mode(WIFI_STA); // prevent ESP from creating access point by default
#elif ESP32
  WiFi.mode(WIFI_MODE_STA);
#endif
#ifdef USE_WEBSOCKET
  WiFi.hostname(WIFI_HOSTNAME);
  WiFiMulti.addAP(WIFI_SSID, WIFI_PASS);
  while(WiFiMulti.run() != WL_CONNECTED) {
    delay(100);
  }
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);  
#endif
}

// gets the values from a string formatted like 123456/123
String getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;
  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
      found++;
      strIndex[0] = strIndex[1]+1;
      strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }
  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void receiveSerialData() {
  static byte ndx = 0;
  char endMarker = '\n';
  char rc;   
  if (Serial.available() > 0) {
    rc = Serial.read();
    if (rc != endMarker) {
      receivedChars[ndx] = rc;
      ndx++;
      if (ndx >= CHAR_BUFFER_SIZE) {
        ndx = CHAR_BUFFER_SIZE - 1;
      }
    }
    else {
      receivedChars[ndx] = '\0'; // terminate the string
      ndx = 0;
      newData = true;
    }
  }
}

void sendRcData() {
  if (newData == true) {
#ifdef USE_CC1101
    ELECHOUSE_cc1101.SetTx();
#endif
#ifdef USE_ESPILIGHT
    if (json_validate(receivedChars)) {
      JsonNode *fullJson = json_decode(receivedChars);
      JsonNode *typeJson = json_find_member(fullJson, "type");
      JsonNode *messageJson = json_find_member(fullJson, "message");
      if(typeJson != NULL && messageJson != NULL){
        char *message = json_encode(messageJson);
        rf.send(typeJson->string_, message);
        json_free(message);
      }else{
        Serial.println("pilight(0): missing type or message in JSON");
      }
      json_delete(messageJson);
      json_delete(typeJson);
      json_delete(fullJson);
    }else{
      Serial.println("pilight(0): invalid JSON");
    }
#else
    long value = getValue(receivedChars, '/', 0).toInt();
    long pulse = getValue(receivedChars, '/', 1).toInt();
    long protocol = getValue(receivedChars, '/', 2).toInt();
    if(protocol==0) protocol = 1;
    mySwitch.setProtocol(protocol);
    mySwitch.setPulseLength(pulse);
    mySwitch.send(value, 24);
#endif
#ifdef USE_CC1101
    ELECHOUSE_cc1101.SetRx();
#endif
#ifdef USE_WEBSOCKET
    webSocket.sendTXT(lastClientNum, "OK");
#endif
    Serial.println("OK");
    newData = false;
  }
}

#ifdef USE_ESPILIGHT
#else
void receiveRcData(){
  if (mySwitch.available()) {
    long value = mySwitch.getReceivedValue();
    long pulse = mySwitch.getReceivedDelay();
    long protocol = mySwitch.getReceivedProtocol();
    if (value != 0) {
      String out = value + dash + pulse + dash + protocol;
#ifdef USE_WEBSOCKET
      webSocket.broadcastTXT(out);
#endif
      Serial.println( out );
    }
    mySwitch.resetAvailable();
  }  
}
#endif

void loop() {
#ifdef USE_WEBSOCKET
  webSocket.loop();
#endif
  receiveSerialData();
  sendRcData();
#ifdef USE_ESPILIGHT
  rf.loop();
#else
  receiveRcData();
#endif
}
