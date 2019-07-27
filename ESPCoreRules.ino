/****************************************************************************************************************************\
  Arduino project "ESP Easy" © Copyright www.letscontrolit.com
  This file incorporates work covered by the following copyright:
  Arduino project "Nodo" © Copyright 2010..2015 Paul Tonkes

  All work is covered by the following license notice:

  This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
  This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
  You received a copy of the GNU General Public License along with this program in file 'License.txt'.

  Additional information about licensing can be found at : http://www.gnu.org/licenses
  \*************************************************************************************************************************/

// This is a custom Core edition of ESP Easy, based on R120 and some Mega code

#define BUILD                               3

// Select features to include into the Core:
#define FEATURE_RULES                    true
#define FEATURE_PLUGINS                  true
#define FEATURE_MQTT                     true


// Other settings
#define ESP_PROJECT_PID           2016110801L
#if defined(ESP8266)
#define VERSION                           2
#endif
#if defined(ESP32)
#define VERSION                           3
#endif

#define CMD_REBOOT                         89
#define UNIT_MAX                           32
#define PLUGIN_MAX                         32
#define RULES_MAX_SIZE                   2048
#define RULES_BUFFER_SIZE                  64
#define RULES_MAX_NESTING_LEVEL             3
#define P020_BUFFER_SIZE 128
#define RULES_TIMER_MAX                     8
#define USER_VAR_MAX                        8
#define USER_STRING_VAR_MAX                 8
#define CONFIRM_QUEUE_MAX                   8
#define INPUT_BUFFER_SIZE                 128

#define PLUGIN_INIT                         2
#define PLUGIN_WRITE                       13
#define PLUGIN_SERIAL_IN                   16
#define PLUGIN_UNCONDITIONAL_POLL          25

#if defined(ESP32)
#define ARDUINO_OTA_PORT  3232
#else
#define ARDUINO_OTA_PORT  8266
#endif

#if defined(ESP32)
#define FEATURE_ARDUINO_OTA
#endif

#if defined(ESP32)
#define FILE_BOOT         "/boot.txt"
#define FILE_CONFIG       "/config.dat"
#define FILE_SECURITY     "/security.dat"
#define FILE_RULES        "/rules1.txt"
#include <WiFi.h>
#include <WebServer.h>
#include "SPIFFS.h"
WebServer WebServer(80);
#ifdef FEATURE_ARDUINO_OTA
  #include <ArduinoOTA.h>
  #include <ESPmDNS.h>
  bool ArduinoOTAtriggered = false;
#endif
#define PIN_D_MAX        16
#endif

#if defined(ESP8266)
#define FILE_BOOT         "boot.txt"
#define FILE_CONFIG       "config.dat"
#define FILE_SECURITY     "security.dat"
#define FILE_NOTIFICATION "notification.dat"
#define FILE_RULES        "rules1.txt"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
ESP8266HTTPUpdateServer httpUpdater(false);
ESP8266WebServer WebServer(80);
extern "C" {
#include "user_interface.h"
}
#define PIN_D_MAX        16
#endif

WiFiServer *ser2netServer;
WiFiClient ser2netClient;
byte connectionState = 0;

#include <WiFiUdp.h>
#include <FS.h>
using namespace fs;
WiFiUDP portUDP;

#if FEATURE_MQTT
  #include <PubSubClient.h>
  WiFiClient mqtt;
  PubSubClient MQTTclient(mqtt);
  unsigned long connectionFailures;
  #define MSGBUS_TOPIC "BROADCAST/"
#endif

struct SecurityStruct
{
  char          WifiSSID[32];
  char          WifiKey[64];
  char          WifiSSID2[32];
  char          WifiKey2[64];
  char          WifiAPKey[64];
  char          ControllerUser[64];
  char          ControllerPassword[64];
} SecuritySettings;

struct SettingsStruct
{
  byte          IP[4];
  byte          Gateway[4];
  byte          Subnet[4];
  byte          DNS[4];
  unsigned int  Port;
  char          Name[26];
  char          Group[26];
  char          NTPHost[64];
  int8_t        Pin_i2c_sda;
  int8_t        Pin_i2c_scl;
  unsigned long BaudRate;
  byte          deepSleep;
  boolean       DST;
  boolean       RulesClock;
  boolean       RulesSerial;
  boolean       UseSerial;
  boolean       UseNTP;
  int16_t       TimeZone;
  byte          SerialTelnet=0;
  byte          Controller_IP[4];
  unsigned int  ControllerPort;
  char          MQTTsubscribe[80];
  boolean       MQTTRetainFlag;
  boolean       UseMSGBusUDP = true;
  boolean       UseMSGBusMQTT;
} Settings;

struct NodeStruct
{
  byte IP[4];
  byte age;
  String nodeName;
  String group;
} Nodes[UNIT_MAX];

struct nvarStruct
{
  String Name;
  float Value;
  byte Decimals;
} nUserVar[USER_VAR_MAX];

struct svarStruct
{
  String Name;
  String Value;
} sUserVar[USER_STRING_VAR_MAX];

#if FEATURE_RULES
//  unsigned long RulesTimer[RULES_TIMER_MAX];
struct timerStruct
{
  String Name;
  unsigned long Value;
} RulesTimer[RULES_TIMER_MAX];

struct confirmQueueStruct
{
  String Name;
  byte Attempts;
  byte State;
  byte TimerTicks;
} confirmQueue[CONFIRM_QUEUE_MAX];

#endif

unsigned int NC_Count = 0;
unsigned int C_Count = 0;
boolean AP_Mode = false;
byte cmd_within_mainloop = 0;

unsigned long timer60 = 0;
unsigned long timer1 = 0;
unsigned long timer10ps = 0;
unsigned long uptime = 0;

// Serial support global vars
byte SerialInByte;
int SerialInByteCounter = 0;
char InputBuffer_Serial[INPUT_BUFFER_SIZE + 2];

// Telnet support global vars
int TelnetByteCounter = 0;
char TelnetBuffer[INPUT_BUFFER_SIZE + 2];
uint8_t net_buf[P020_BUFFER_SIZE];

String printWebString = "";

void setNvar(String varName, float value, int decimals = -1);
String parseString(String& string, byte indexFind, char separator = ',');

#if FEATURE_PLUGINS
boolean (*Plugin_ptr[PLUGIN_MAX])(byte, String&, String&);
byte Plugin_id[PLUGIN_MAX];
String dummyString = "";
#endif

/*********************************************************************************************\
  SETUP
  \*********************************************************************************************/
void setup()
{
  Serial.begin(115200);
  Serial.println("");
  
  String event = "";

  if (SPIFFS.begin())
  {
    event = "System#Config";
    if (SPIFFS.exists(FILE_BOOT)){
      rulesProcessing(FILE_BOOT, event);
    }
    else{
      if (SPIFFS.exists(FILE_SECURITY)){
        LoadSettings(); // use security.dat
      }
      else
      {
        SecuritySettings.WifiSSID[0] = 0;
        SecuritySettings.WifiKey[0] = 0;
        strcpy(SecuritySettings.WifiAPKey, "configesp");
      }
      fs::File f = SPIFFS.open(FILE_BOOT, "w");
      f.println(F("on System#Config do"));
      f.print(F("  Config,WifiSSID,"));
      f.println(SecuritySettings.WifiSSID);
      f.print(F("  Config,WifiKey,"));
      f.println(SecuritySettings.WifiKey);
      f.println(F("endon"));
    }
    
    rulesProcessing(FILE_RULES, event);

    WiFi.persistent(false); // Do not use SDK storage of SSID/WPA parameters
    WifiConnect(3);
    WebServerInit();
    WiFi.mode(WIFI_STA);
  }
  else
  {
    Serial.println("SPIFFS?");
    delay(1);
  }

  if (!Settings.UseSerial) {
    pinMode(1, INPUT);
    pinMode(3, INPUT);
  }

  // Add myself to the node list
  IPAddress IP = WiFi.localIP();
  for (byte x = 0; x < 4; x++)
    Nodes[0].IP[x] = IP[x];
  Nodes[0].age = 0;
  Nodes[0].nodeName = Settings.Name;
  Nodes[0].group = Settings.Group;

  ser2netServer = new WiFiServer(23);
  ser2netServer->begin();

  if(Settings.UseMSGBusUDP)
    portUDP.begin(Settings.Port);

  initTime();

#ifdef FEATURE_ARDUINO_OTA
  ArduinoOTAInit();
#endif

#if FEATURE_PLUGINS
  PluginInit();
#endif

  // Get others to announce themselves so i can update my node list
  if(Settings.UseMSGBusUDP){
    event = F("MSGBUS/Refresh");
    UDPSend(event);
  }

  #if FEATURE_RULES
    event = F("System#Boot");
    rulesProcessing(FILE_BOOT, event);
    rulesProcessing(FILE_RULES, event);
  #endif

  #if FEATURE_MQTT
    // Get others to announce themselves so i can update my node list
    // for MQTT, the connection needs to be made during System#Boot so we do this afterwards
    if(Settings.UseMSGBusMQTT){ 
      String topic = F(MSGBUS_TOPIC);
      topic += F("MSGBUS/Refresh");
      String payload = "";
      MQTTclient.publish(topic.c_str(), payload.c_str(),Settings.MQTTRetainFlag);
    }
  #endif
}


//*********************************************************************************************
// MAIN LOOP
//*********************************************************************************************
void loop()
{
  if (cmd_within_mainloop == CMD_REBOOT)
    reboot();

  serial();

  WebServer.handleClient();

  if(Settings.UseMSGBusUDP)
    MSGBusReceive();
    
  #if FEATURE_MQTT
  if(Settings.UseMSGBusMQTT) 
    MQTTclient.loop();
  #endif

  telnet();

  if (millis() > timer10ps)
    run10PerSecond();

  if (millis() > timer1)
    runEachSecond();

  if (millis() > timer60)
    runEach60Seconds();

#ifdef FEATURE_ARDUINO_OTA
  ArduinoOTA.handle();
  //once OTA is triggered, only handle that and dont do other stuff. (otherwise it fails)
  while (ArduinoOTAtriggered)
  {
    yield();
    ArduinoOTA.handle();
  }
#endif

  delay(0);

#if defined(ESP8266)
  tcpCleanup();
#endif
}

/********************************************************************************************\
  10 times per second
\*********************************************************************************************/
void run10PerSecond(){
  timer10ps = millis() + 100;
  PluginCall(PLUGIN_UNCONDITIONAL_POLL, dummyString, dummyString);
  if(Settings.UseMSGBusUDP) 
    MSGBusQueue();
}
/********************************************************************************************\
  Each second
\*********************************************************************************************/
void runEachSecond() {
  timer1 = millis() + 1000;
  checkTime();
#if FEATURE_RULES
    rulesTimers();
#endif
  }

  /********************************************************************************************\
    Each 60 seconds
    \*********************************************************************************************/
  void runEach60Seconds() {
    timer60 = millis() + 60000;
    uptime++;
    WifiCheck();
    #if FEATURE_MQTT
    if(Settings.UseMSGBusMQTT) 
      MQTTCheck();
    #endif
    refreshNodeList();
    MSGBusAnnounceMe();
  }

