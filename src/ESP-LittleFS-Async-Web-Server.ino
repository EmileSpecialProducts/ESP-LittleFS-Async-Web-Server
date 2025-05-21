/*
  ESP-LittleFS-Async-Web-Server - Example of a WebServer with LittleFS backend for esp

  Copyright (c) 2015 Hristo Gochkov. All rights reserved.
  This file is part of the ESP8266WebServer library for Arduino environment.
  And is modyfied for the LittleFS

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <Arduino.h> //  https://github.com/espressif/arduino-esp32/tree/master/cores/esp32

#if defined(ESP8266)
#include <ESP8266WiFi.h>
//#include <ESP8266WebServer.h>
#include <ESPAsyncTCP.h> //https://github.com/ESP32Async/ESPAsyncTCP
#include <ESP8266mDNS.h>
#else
#include <WiFi.h>      // https://github.com/espressif/arduino-esp32/tree/master/libraries/WiFi
#include <esp_wifi.h>

#include <ESPmDNS.h>   // https://github.com/espressif/arduino-esp32/tree/master/libraries/ESPmDNS
#include <AsyncTCP.h> // https://github.com/ESP32Async/AsyncTCP
#endif

#include <ArduinoOTA.h>  // ArduinoOTA by Arduino, Juraj  https://github.com/JAndrassy/ArduinoOTA

#if defined(ESP8266)
// https://github.com/tzapu/WiFiManager/issues/1530
#define WEBSERVER_H
#endif 

#define USEWIFIMANAGER

#ifdef USEWIFIMANAGER
#include <WiFiManager.h> // WifiManager by tzapu  https://github.com/tzapu/WiFiManager
#endif

#include <ESPAsyncWebServer.h> // https://github.com/ESP32Async/ESPAsyncWebServer

#if defined(ESP8266)
#include <LittleFS.h>
#include <FS.h>
#else
#include <LittleFS.h>
#include <FS.h>
#endif

// https://github.com/tzapu/WiFiManager/issues/1530
typedef enum {
  MY_HTTP_GET = 0b00000001,
  MY_HTTP_POST = 0b00000010,
  MY_HTTP_DELETE = 0b00000100,
  MY_HTTP_PUT = 0b00001000,
  MY_HTTP_PATCH = 0b00010000,
  MY_HTTP_HEAD = 0b00100000,
  MY_HTTP_OPTIONS = 0b01000000,
  MY_HTTP_ANY = 0b01111111,
} MyWebRequestMethod;

#define DEBUG_PRINT  // manages most of the print and println debug, not all but most

#ifdef DEBUG_PRINT
#define debug_begin(...) Serial.begin(__VA_ARGS__)
#define setdebug(...) Serial.setDebugOutput(__VA_ARGS__)
#define debug(...) Serial.print(__VA_ARGS__)
#define debugln(...) Serial.println(__VA_ARGS__)
#define debugf(...) Serial.printf(__VA_ARGS__)
#else
#define debug_begin(...) ;
#define debug(...) ;
#define debugln(...) ;
#define setdebug(...) ;
#define debugf(...) ;
#endif


#ifndef USEWIFIMANAGER
const char *ssid = "ssid";
const char *password = "password";
#endif

#if defined(ESP8266)
const char *host = "ESP-LittleFS-12E";
#elif defined(CONFIG_IDF_TARGET_ESP32)
const char *host = "ESP-LittleFS-ESP";
#elif defined(CONFIG_IDF_TARGET_ESP32C2)
const char *host = "ESP-LittleFS-C2";
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
const char *host = "ESP-LittleFS-C3";
#elif defined(CONFIG_IDF_TARGET_ESP32C6)
const char *host = "ESP-LittleFS-C6";
#elif defined(CONFIG_IDF_TARGET_ESP32S2)
const char *host = "ESP-LittleFS-S2";
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
const char *host = "ESP-LittleFS-S3";
#endif

#if defined(ESP8266)
#ifndef PIN_BOOT
  #define PIN_BOOT 0
#endif
#elif defined(CONFIG_IDF_TARGET_ESP32)
#ifndef PIN_BOOT
  #define PIN_BOOT 0
#endif
#elif defined(CONFIG_IDF_TARGET_ESP32C2)
#ifndef PIN_BOOT
  #define PIN_BOOT 9
#endif
#ifndef LED_BUILTIN
  #define LED_BUILTIN 8
#endif
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
#ifndef PIN_BOOT
  #define PIN_BOOT 9
#endif
#ifndef LED_BUILTIN
  #define LED_BUILTIN 8
#endif
#elif defined(CONFIG_IDF_TARGET_ESP32C6)
#ifndef PIN_BOOT
  #define PIN_BOOT 9
#endif
#elif defined(CONFIG_IDF_TARGET_ESP32S2)
#ifndef PIN_BOOT
  #define PIN_BOOT 0
#endif
#ifndef LED_BUILTIN
#define LED_BUILTIN 15
#endif
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
#ifndef PIN_BOOT
  #define PIN_BOOT 0
#endif
#if not defined(PIN_NEOPIXEL)
#define PIN_NEOPIXEL 48
#endif
#endif

#ifdef LED_BUILTIN
#define PIN_LED LED_BUILTIN
#else
#define PIN_LED -1
#endif

AsyncWebServer server(80);

unsigned long PreviousTimeSeconds;
unsigned long PreviousTimeMinutes;
unsigned long PreviousTimeHours;
unsigned long PreviousTimeDay;
unsigned long currentTimeSeconds = 0;
unsigned long NextTime = 0;

uint16_t Config_Reset_Counter = 0;
int OTAUploadBusy = 0;

File uploadFile;


void reply(AsyncWebServerRequest *request, int code, const char *type, const uint8_t *data, size_t len)
{
    debugf("reply Len = %d code = %d Type= %s\n ", len, code, type);  
    //request->send(code, type, data, len);
        AsyncWebServerResponse *response =
            request->beginResponse(code, type, data, len);
        // response->addHeader("Content-Encoding", "gzip");
        // response->addHeader("Content-Encoding", "7zip");
        request->send(response);
}


String urlDecode(const String &text)
{
  String decoded = "";
  char temp[] = "0x00";
  unsigned int len = text.length();
  unsigned int i = 0;
  while (i < len)
  {
    char decodedChar;
    char encodedChar = text.charAt(i++);
    if ((encodedChar == '%') && (i + 1 < len))
    {
      temp[2] = text.charAt(i++);
      temp[3] = text.charAt(i++);
      decodedChar = strtol(temp, NULL, 16);
    }
    else
    {
      if (encodedChar == '+')
      {
        decodedChar = ' ';
      }
      else
      {
        decodedChar = encodedChar; // normal ascii char
      }
    }
    decoded += decodedChar;
  }
  return decoded;
}

const uint8_t Index_html[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
  <head>
    <title>LittleFS Async Editor</title>
    <script src="https://emilespecialproducts.github.io/ESP-LittleFS-Async-Web-Server/editor.js" type="text/javascript"></script> 
  </head>
  <body onload="onBodyLoad();">
    <div id="uploader"></div>
    <div id="tree" class="css-tree"></div>
    <div id="editor"></div>
    <div id="preview" style="display:none;"></div>
    <iframe id=download-frame style='display:none;'></iframe>
  </body>
</html>
)rawliteral";

const uint8_t error404_html[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
  <head>
    <title>LittleFS Async Editor</title>
  </head>
  <body>
    Page not found
  </body>
</html>
)rawliteral";

void Log(String Str)
{
  debugln(Str);
#if defined(ESP8266)
  File LogFile = LittleFS.open("/log.txt", "a"); // FILE_WRITE | O_APPEND);
#else
  File LogFile = LittleFS.open("/log.txt", FILE_APPEND);
#endif
  LogFile.println(Str);
  LogFile.close();
}

void setup(void)
{
  pinMode(PIN_BOOT, INPUT_PULLUP);
  pinMode(PIN_LED, OUTPUT);

#if defined(ESP8266) || defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32) || Serial == USBSerial
  debug_begin(115200);
#else
  debug_begin(115200, SERIAL_8N1, RX, TX);
#endif
  setdebug(true);
  debug("\n");
#ifdef USEWIFIMANAGER
  // WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wm;

  // reset settings - wipe stored credentials for testing
  // these are stored by the esp library
  //  wm.resetSettings();

  // Automatically connect using saved credentials,
  // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
  // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
  // then goes into a blocking loop awaiting configuration and will return success result

  bool res;
  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  wm.setTimeout(180);

  res = wm.autoConnect(); // auto generated AP name from chipid
  // res = wm.autoConnect(DeviceName); // anonymous ap
  // res = wm.autoConnect("AutoConnectAP","password"); // password protected ap
  if (!res)
  {
    debugln("Failed to connect Restarting");
    delay(5000);
    if (digitalRead(PIN_BOOT) == LOW)
    {
      wm.resetSettings();
    }
    ESP.restart();
  }
  else
  {
    // if you get here you have connected to the WiFi
  }
#else
  WiFi.begin(ssid, password);
  debug("Connecting to ");
  debugln(ssid);
  // Wait for connection
  uint8_t i = 0;
  while (WiFi.status() != WL_CONNECTED && i++ < 20)
  { // wait 10 seconds
    delay(500);
  }
  if (i == 21)
  {
    debug("Could not connect to");
    debugln(ssid);
    while (1)
      delay(500);
  }
#endif
  WiFi.setSleep(false);
#if not defined(ESP8266)
  esp_wifi_set_ps(WIFI_PS_NONE); // Esp32 enters the power saving mode by default,
#endif
  debug("Connected! IP address: ");
  debugln(WiFi.localIP());
  debug("Connecting to ");
  debugln(WiFi.SSID());

  if (MDNS.begin(host))
  {
    MDNS.addService("http", "tcp", 80);
    debugln("MDNS responder started");
    debug("You can now connect to http://");
    debug(host);
    debug(".local or http://");
    debugln(WiFi.localIP());
  }

  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  ArduinoOTA.setHostname(host);

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");
  ArduinoOTA.onStart([]()
                     {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {  // U_FS
      type = "filesystem";
    }
    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    OTAUploadBusy=60; // only do a update for 60 sec;
    debugln("Start updating " + type); });
  ArduinoOTA.onEnd([]()
                   {
    OTAUploadBusy=0;
    debugln("\nEnd"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                        { debug("Progress: " + String((progress / (total / 100))) + "\r"); });
  ArduinoOTA.onError([](ota_error_t error)
                     {
    OTAUploadBusy=0;
    debugln("Error: "+String( error));
    if (error == OTA_AUTH_ERROR) {
      debugln("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      debugln("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      debugln("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      debugln("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      debugln("End Failed");
    } });
  ArduinoOTA.begin();
  debug("LittleFS : "); // LittleFS.begin can take 10 seconds to start formating
#if defined(ESP8266)
  if (!LittleFS.begin())
#else
  if (!LittleFS.begin(true)) // FORMAT_LITTLEFS_IF_FAILED
#endif
  {
    debugln(" Mount Failed");
  }
  else
  {
    debugln(" Started ");
  }

server.on("/list", MY_HTTP_GET, [](AsyncWebServerRequest *request)
    {
#if defined(ESP8266)
        int cnt = 0;
        bool foundfile;
        if (!request->hasArg("dir"))
          return request->send(500, "text/plain", "BAD ARGS\r\n");
        String path = request->arg("dir");
        String output="[";
        debugln(" DIR: ");
        request->send(200, "text/json", "");
        Dir dir = LittleFS.openDir(path);
        foundfile = dir.next();
        while (foundfile)
        {
          output += String(cnt++>0?",":"")
          + "{\"name\":\"" + String(dir.fileName())+"\"" 
          + ",\"type\":\"" + String((dir.isDirectory()) ? "dir" : "file")+"\"" 
          + String((dir.isDirectory()) ?"":(",\"size\":\"" + String(dir.fileSize())+"\"")) 
          + "}";
          foundfile = dir.next();
          if (!foundfile)
            output += ']';
        }
        debugln("Dir: " + output);
        request->send(200, "text/json", output);
#else
        if (!request->hasArg("dir"))
          return request->send(500, "text/plain", "BAD ARGS\r\n"); 
        String path = request->arg("dir");
        String output="[";
        if(path != "/" && !LittleFS.exists(path)) return request->send(500, "text/plain", "BAD PATH\r\n"); 
        File dir = LittleFS.open(path);
        path = String();
        if (!dir.isDirectory())
        {
          dir.close();
          return request->send(500, "text/plain", "NOT DIR\r\n"); 
        }
        dir.rewindDirectory();
        for (int cnt = 0; true; ++cnt)
        {
          File entry = dir.openNextFile();
          if (!entry)
            break;          
          output += String(cnt++>0?",":"") +"{\"type\":\"" + String((entry.isDirectory()) ? "dir" : "file")+"\"" 
          + ",\"name\":\"" + String(entry.name())+"\"" 
          + String((entry.isDirectory()) ?"":",\"size\":\"" + String(entry.size())+"\"") 
          + "}";
          entry.close();
        }
        output+="]";
        request->send(200, "text/json", output);
        dir.close();
#endif
      });
      
server.on("/edit", MY_HTTP_PUT, [](AsyncWebServerRequest *request)
{
  debugln("Create ");
  if (request->args() == 0)
  return request->send(500, "text/plain", "BAD ARGS\r\n");  
  String path = request->arg(0);
  debugln("Create: " + path);
  if (path == "/" || LittleFS.exists(path))
  {
    request->send(500, "text/plain", "BAD PATH\r\n"+ path); 
    return;
  }
  if(path.indexOf('.') > 0){
    debugln("CreateFile: " + path);
    File file = LittleFS.open(path, "w");
    if (file)
    {
      file.write((const uint8_t *)" ", 1); // must write one char
      file.close();
    }
  }
  else
  {
    debugln("CreateDir: " + path);
    LittleFS.mkdir(path);
  }
  request->send(200, "text/plain", ""); 
});

server.on("/edit", MY_HTTP_DELETE, [](AsyncWebServerRequest *request)
{
  debugln("Delete ");
  if (request->args() == 0)
    return request->send(500, "text/plain", "BAD ARGS\r\n");  
  String path = request->arg(0);
  debugln("Delete: " + path);
  if(path.indexOf('.') > 0){
    if (path == "/" || !LittleFS.exists(path))
    {
      request->send(500, "text/plain", "BAD PATH\r\n" + path); 
      return;
    }
    debugln("Delete file "+path);
    
    LittleFS.remove(path);
    request->send(200, "text/plain", "");
  } else
  {
    debugln("Delete Dir "+path);
    LittleFS.rmdir(path);
    request->send(200, "text/plain", "");
  } 
});

server.on("/edit", MY_HTTP_POST, 
        [](AsyncWebServerRequest *request)
          {
            debugln("File upload completed " + request->url());
            //request->send(200, "text/plain; chartset=\"UTF-8\"", "File upload completed");
            //request->send(200);
            request->redirect("/"); 
          }, 
        [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
          {
            debugf("Upload[%s]: start=%u, len=%u, final=%d\n", filename.c_str(), index, len, final);
            if (!index) {
            request->_tempFile = LittleFS.open("/"+ filename, "w+");
            }
            if (len) request->_tempFile.write(data, len);
            if (final) {
            request->_tempFile.close();
            } 
          }
    );
    server.on("/", MY_HTTP_GET, [](AsyncWebServerRequest *request)
    { request->redirect("/index.html"); });
    
    server.onNotFound([](AsyncWebServerRequest *request)
    { 
        debugf("url NotFound %s , Method =%s\n", request->url().c_str(), request->methodToString());
        if (request->method() == HTTP_GET)
        {
            if (LittleFS.exists(request->url())) // exists will give a error in the error log see: https://github.com/espressif/arduino-esp32/issues/7615
            {
                request->send(LittleFS, request->url(), String(), false);
            }
            else
            {
            if (request->url() == "/index.html")
                    reply(request, 200, "text/html", Index_html, sizeof(Index_html) - 1);
            else if (request->url() == "/error404.html")
                    reply(request, 404, "text/html", error404_html, sizeof(error404_html) - 1);else 
                request->redirect("/error404.html");
            }
        }
        else if (request->method() == MY_HTTP_POST)
        {
            reply(request, 404, "text/html", error404_html, sizeof(error404_html) - 1);
        }
    });
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*"); 
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, PUT, POST, DELETE, HEAD");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "content-type");
  server.begin();  
  debugln("HTTP server started");
  String message;
  message.reserve(512);
  message = "Reboot from: ";
#if defined(ESP8266)
  message += ESP.getChipId();
#else
  message += ESP.getChipModel();
#endif
  message += "_";
  message += WiFi.macAddress();
  message += " LocalIpAddres: " + WiFi.localIP().toString();
  message += " SSID: " + String(WiFi.SSID());
  message += " Rssi: " + String(WiFi.RSSI());

#if not defined(ESP8266)

  message += " Total heap: " + String(ESP.getHeapSize() / 1024);
  message += " Free heap: " + String(ESP.getFreeHeap() / 1024);
  message += " Total PSRAM: " + String(ESP.getPsramSize() / 1024);
  message += " Free PSRAM: " + String(ESP.getFreePsram() / 1024);
  message += " bytes getFreeHeap: " +String(ESP.getFreeHeap()) ; 
  message += " byte esp_get_free_heap_size: "+ String(esp_get_free_heap_size())
  message += " byte free internal_heap_size: "+ String(esp_get_free_internal_heap_size())
  message += " byte ArduinoLoopTaskStackSize: "+ String(getArduinoLoopTaskStackSize());
  message += " byte getSketchSize: " + String(ESP.getSketchSize());
  message += " Temperature: " + String(temperatureRead()) + " °C "; // internal TemperatureSensor
  #else
  message += " FlashChipId: " + String(ESP.getFlashChipId());
  message += " FlashChipRealSize: " + String(ESP.getFlashChipRealSize());
#endif
  message += " FlashChipSize: " + String(ESP.getFlashChipSize());
  message += " FlashChipSpeed: " + String(ESP.getFlashChipSpeed());
#if not defined(ESP8266)
#if ESP_ARDUINO_VERSION != ESP_ARDUINO_VERSION_VAL(2, 0, 17)
  // [ESP::getFlashChipMode crashes on ESP32S3 boards](https://github.com/espressif/arduino-esp32/issues/9816)
  message += " FlashChipMode: ";
  switch (ESP.getFlashChipMode())
  {
  case FM_QIO:
    message += "FM_QIO";
    break;
  case FM_QOUT:
    message += "FM_QOUT";
    break;
  case FM_DIO:
    message += "FM_DIO";
    break;
  case FM_DOUT:
    message += "FM_DOUT";
    break;
  case FM_FAST_READ:
    message += "FM_FAST_READ";
    break;
  case FM_SLOW_READ:
    message += "FM_SLOW_READ";
    break;
  default:
    message += String(ESP.getFlashChipMode());
  }
#endif
#endif
#if not defined(ESP8266)
  message += " esp_idf_version: " + String(esp_get_idf_version());
  message += " arduino_version: " + String(ESP_ARDUINO_VERSION_MAJOR) + "." + String(ESP_ARDUINO_VERSION_MINOR) + "." + String(ESP_ARDUINO_VERSION_PATCH);
#endif
  message += " Build Date: " + String(__DATE__ " " __TIME__);
  Log(message);
  NextTime = millis() + 1000;
}

void loop(void)
{

  unsigned long Time = millis();
  yield();
  ArduinoOTA.handle();
  if (OTAUploadBusy == 0)
  { // Do not do things that take time when OTA is busy
    //server.handleClient();
  }

  if (PreviousTimeDay != (currentTimeSeconds / (60 * 60 * 24)))
  { // Day Loop
    PreviousTimeDay = (currentTimeSeconds / (60 * 60 * 24));
  }
  if (PreviousTimeHours != (currentTimeSeconds / (60 * 60)))
  { // Hours Loop
    PreviousTimeHours = (currentTimeSeconds / (60 * 60));
  }

  if (PreviousTimeMinutes != (currentTimeSeconds / 60))
  { // Minutes loop
    PreviousTimeMinutes = (currentTimeSeconds / 60);
    if ((WiFi.status() != WL_CONNECTED))
    { // if WiFi is down, try reconnecting
      WiFi.disconnect();
      WiFi.reconnect();
    }
  }

  if (Time >= NextTime) // This will fail after 71 days
  {
    NextTime = millis() + 1000;
    currentTimeSeconds++;
    if (digitalRead(PIN_LED) == 0)
      digitalWrite(PIN_LED, 1);
    else
      digitalWrite(PIN_LED, 0);
    if (OTAUploadBusy > 0)
      OTAUploadBusy--;
#ifdef USEWIFIMANAGER
    if (digitalRead(PIN_BOOT) == LOW)
    {
      if (++Config_Reset_Counter > 5)
      {                 // press the BOOT 5 sec to reset the WifiManager Settings
        WiFiManager wm; // WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
        delay(500);
        wm.resetSettings();
        ESP.restart();
      }
    }
    else
    {
      Config_Reset_Counter = 0;
    }
#endif
  }
}
