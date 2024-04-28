/**The MIT License (MIT)

Copyright (c) 2024 by Greg Cunningham, CuriousTech

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

// Build with Arduino IDE 1.8.12, ESP32

//uncomment to enable Arduino IDE Over The Air update code
#define OTA_ENABLE

#include <EEPROM.h>
#include "WiFiManager.h"
#include <ESPAsyncWebServer.h> // https://github.com/me-no-dev/ESPAsyncWebServer
#include <ESPmDNS.h>
#include <TimeLib.h> // http://www.pjrc.com/teensy/td_libs_Time.html
#include <UdpTime.h> // https://github.com/CuriousTech/ESP07_WiFiGarageDoor
#include "eeMem.h"
#include <JsonParse.h> // https://github.com/CuriousTech/ESP8266-HVAC/tree/master/Libraries/JsonParse
#include "jsonstring.h"
#ifdef OTA_ENABLE
#include <FS.h>
#include <ArduinoOTA.h>
#endif
#include "pages.h"
#include <ld2410.h> // from Library Manager in Arduino IDE

#define MONITOR_SERIAL Serial
#define RADAR_SERIAL Serial1
#define RADAR_RX_PIN 32
#define RADAR_TX_PIN 33
#define DIGITAL_PRESENCE 25

ld2410 radar;
bool engineeringMode = false;

const char hostName[] ="LD2410Cfg";

int serverPort = 80;

WiFiManager wifi;  // AP page:  192.168.4.1
AsyncWebServer server( serverPort );
AsyncWebSocket ws("/ws"); // access at ws://[esp ip]/ws

void jsonCallback(int16_t iName, int iValue, char *psValue);
JsonParse jsonParse(jsonCallback);
UdpTime udptime;
eeMem ee;

uint32_t lastReading = 0;
bool radarConnected = false;
bool bSendSettings;

String settingsJson()
{
  if(!radar.requestCurrentConfiguration()) // refesh data to update page
    Serial.println("requestConfig failed");
  jsonString js("settings");
  js.Var("tz", ee.tz);
  js.Var("r", ee.rate);
  js.Var("gates", radar.max_gate);
  js.Var("mmg", radar.max_moving_gate);
  js.Var("msg", radar.max_stationary_gate);
  js.Var("idle", radar.sensor_idle_time);
  js.Array("ma", radar.motion_sensitivity, radar.max_gate + 1);
  js.Array("sa", radar.stationary_sensitivity, radar.max_gate + 1);
  js.Var("em", engineeringMode);
  return js.Close();
}

const char *jsonList1[] = { // WebSocket commands
  "TZ",
  "rate",
  "mmgates",
  "msgates",
  "idletime",
  "gate",
  "ms",
  "ss",
  "restart",
  "em",
  "factres", // 10
  NULL
};

void sendAlert(String s)
{
  jsonString js("alert");
  js.Var("text", s);
  WsSend(js.Close() );
}

void setEngineeringMode(bool bEnable)
{
  engineeringMode = bEnable;

  bool b;
  if(bEnable)
    b = radar.requestStartEngineeringMode();
  else
    b = radar.requestEndEngineeringMode();
  if(!b)
    sendAlert("EM switch failed");
}

void jsonCallback(int16_t iName, int iValue, char *psValue) // handle WebSocket commands
{
  static uint8_t gateNum = 0;
  uint8_t newVal;

  switch(iName)
  {
    case 0: // TZ
      ee.tz = iValue;
      break;
    case 1: // rate
      ee.rate = constrain(iValue, 30, 1000);
      break;

    case 2: // mmgates
      newVal = constrain(iValue, 1, 8);
      if(radar.setMaxValues(newVal, radar.max_stationary_gate, radar.sensor_idle_time) )
        radar.requestRestart();
      else
        sendAlert("setMaxValues error");
      break;

    case 3: // msgates
      newVal = constrain(iValue, 1, 8);
      if(radar.setMaxValues(radar.max_moving_gate, newVal, radar.sensor_idle_time) )
        radar.requestRestart();
      else
        sendAlert("setMaxValues error");
      break;

    case 4: // idletime
      newVal =  constrain(iValue, 1, 200);
      if(radar.setMaxValues(radar.max_moving_gate, radar.max_stationary_gate, newVal) )
        radar.requestRestart();
      else
        sendAlert("setMaxValues error");
      break;

    case 5: // gate
      gateNum = constrain(iValue, 1, 8);
      break;
    case 6: // ms
      newVal = constrain(iValue, 0, 100);
      if(radar.setGateSensitivityThreshold(gateNum, newVal, radar.stationary_sensitivity[gateNum]) )
        radar.requestRestart();
      else
        sendAlert("setGateSensitivityThreshold error");
      break;
    case 7: // ss
      newVal = constrain(iValue, 0, 100);
      if(radar.setGateSensitivityThreshold(gateNum, radar.motion_sensitivity[gateNum], newVal) )
        radar.requestRestart();
      else
        sendAlert("setGateSensitivityThreshold error");
      break;
    case 8: // restart
      if(!radar.requestRestart())
        sendAlert("requestRestart error");
      break;
    case 9: // em
      setEngineeringMode(iValue ? true:false);
      break;
    case 10: // factres
      if(!radar.requestFactoryReset())
        sendAlert("requestFactorReset error");
      break;
  }
  bSendSettings = true; // send everything
}

void WsSend(String s) // send packat directly (preformatted)
{
  ws.textAll(s);  
}

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len)
{  //Handle WebSocket event

  switch(type)
  {
    case WS_EVT_CONNECT:      //client connected
      client->keepAlivePeriod(50);
      client->text(settingsJson());
      client->ping();
      break;
    case WS_EVT_DISCONNECT:    //client disconnected
      break;
    case WS_EVT_ERROR:    //error was received from the other end
      break;
    case WS_EVT_PONG:    //pong message was received (in response to a ping request maybe)
      break;
    case WS_EVT_DATA:  //data packet
      AwsFrameInfo * info = (AwsFrameInfo*)arg;
      if(info->final && info->index == 0 && info->len == len){
        //the whole message is in a single frame and we got all of it's data
        if(info->opcode == WS_TEXT){
          data[len] = 0;
          jsonParse.process((char*)data);
        }
      }
      break;
  }
}

void setup()
{
  Serial.begin(115200);

  WiFi.hostname(hostName);
  wifi.autoConnect(hostName, "password");

  // attach AsyncWebSocket
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  server.on( "/", HTTP_GET | HTTP_POST, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/html", page1);
  });

  server.on( "/s", HTTP_GET | HTTP_POST, [](AsyncWebServerRequest *request){
    request->send( 200, "text/html", wifi.page() );
  });

  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });

  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
    AsyncWebServerResponse *response = request->beginResponse_P(200, "image/x-icon", favicon, sizeof(favicon));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  server.onNotFound([](AsyncWebServerRequest *request){
    request->send(404);
  });

  server.onFileUpload([](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
  });
  server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
  });

  server.begin();

#ifdef OTA_ENABLE
  ArduinoOTA.setHostname(hostName);
  ArduinoOTA.begin();
  ArduinoOTA.onStart([]() {
    ee.update();
    jsonString js("alert");
    js.Var("text", "OTA Update Started");
    ws.textAll(js.Close());
    ws.closeAll(); // page has faster response with this close
  });
#endif

  jsonParse.setList(jsonList1);

  udptime.start();

  RADAR_SERIAL.begin(256000, SERIAL_8N1, RADAR_RX_PIN, RADAR_TX_PIN); //UART for monitoring the radar

  if(radar.begin(RADAR_SERIAL))
  {
    if(!radar.requestCurrentConfiguration())
      Serial.println("requestConfig failed");
  }
  else
    Serial.println("radar.begin failed");

  pinMode(DIGITAL_PRESENCE, INPUT);
}

void loop()
{
  static uint8_t hour_save, sec_save, lastS;
  static uint8_t cnt;
  time_t nw;

#ifdef OTA_ENABLE
  ArduinoOTA.handle();
#endif
  udptime.check(ee.tz);

  wifi.service();
  if(wifi.connectNew())
  {
    if(!MDNS.begin( hostName ) )
      Serial.println("Error setting up mDNS");
    MDNS.addService("iot", "tcp", 8080);
  }
  nw = now();
  int s = second(nw);

  radar.read();
  if(radar.isConnected() && millis() - lastReading > ee.rate)  //Report every 1000ms
  {
    lastReading = millis();

    uint32_t nw = now();
  
    jsonString js("state");
    js.Var("t", nw - ( (ee.tz + udptime.getDST() ) * 3600));
    bool bPresence = radar.presenceDetected();
    bool bStationary = false;
    bool bMoving = false;

    js.Var("pres", bPresence );
    if(bPresence)
    {
      bStationary = radar.stationaryTargetDetected();
      bMoving = radar.movingTargetDetected();
    }
    
    js.Var("stat", bStationary);
    js.Var("mov", bMoving);

    uint16_t nDistance;
    uint8_t nEnergy;

    if(bStationary)
    {
        nDistance = radar.stationaryTargetDistance();
        nEnergy = radar.stationaryTargetEnergy();
    }
    if(bMoving)
    {
        nDistance = radar.movingTargetDistance();
        nEnergy = radar.movingTargetEnergy();
    }
    js.Var("distance", nDistance);
    js.Var("energy", nEnergy);
    js.Var("DIG", digitalRead(DIGITAL_PRESENCE) );
    ws.textAll( js.Close() );
  }

  if(bSendSettings)
  {
    bSendSettings = false;
    WsSend(settingsJson());
  }

  if(sec_save != s) // only do stuff once per second
  {
    sec_save = s;
    if(s == 0 && hour_save != hour(nw))
    {
      hour_save = hour(nw);
      if(hour_save == 2)
      {
        udptime.start(); // update time daily at DST change
      }
      ee.update(); // update EEPROM if needed while we're at it (give user time to make many adjustments)
    }
  }
}
