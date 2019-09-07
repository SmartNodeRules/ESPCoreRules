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

/* List of core commands
Config,DST <0|1>			            	        Enable/Disable DST
Config,Baudrate <speed>		                  Enable/Disable Serial and speed (speed 0=disable)
Config,LogEvents <0|1>              				Enable/Disable logging msbus events on telnet session
Config,MSGBUS,<UDP|MQTT>			              Select protocol to use for the messagebus
Config,Name					                        Set the device name
Config,Network <ip>,<mask>,<gateway>,<DNS>	Set static IP config
Config,NTP <0|1>		                    		Enable/Disable NTP time service client
Config,Port <portnr>                				MessageBus UPD port number
Config,Rules,Clock <0|1>	              		Enable/Disable rules processing on clock events (each minute)
Config,Rules,Serial <0|1>             			Enable/Disable rules processing on serial input
Config,SendARP <0|1>                				Enable/Disable grat ARP broadcast
Config,Timezone <offset>              			Timezone offset in minutes (can also be negative)
Config,WifiSSID <ssid>              				Set the Wifi SSID
Config,WifiKey <key>                				Set the Wifi WPA key
Config,WifiAPKey                    				Set the Wifi AP key
Event <event>                       				Generate event
Reboot                          						Reboot device
SerialFloat                       					Makes the serial pins float
Serial                          						Enables serial (after using float)
SerialSend <string>                 				Sends a text to serial port
SerialTelnet <0|1>                  				Enable/Disable Serial/Telnet bridge
SendToUDP <ip>,<port>,<msg>           			Sends a text string to give IP and port on UDP protocol
Settings                          					Outputs some basic settings to serial port
Syslog <string>                   					Sends a text string to syslog using broadcast
MSGBus <msg>                      					Send a message on the messagebus
TimerSet <name>,<seconds>             			Set a timer by name to x seconds countdown
ValueSet <name>,<value>             				Set a variable by name to float value
StringSet <name>,<text>             				Set a variable by name to string value
WebPrint <html>                   					Print html to the main webpage (no pararameter clears the buffer)
WebButton <class>;<href>;<caption> 					Adds a button to the main webpage

Plugins commands are explained in the plugin sections

*/

// Select features to include into the Core:
#define FEATURE_RULES                    true
#define FEATURE_MSGBUS                   true
#define FEATURE_PLUGINS                  true
#define FEATURE_TIME                     true
#define FEATURE_MQTT                     false


// Other settings
#define SERIALDEBUG                      true
#define BUILD                               4

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
#define PLUGIN_ONCE_A_SECOND                4
#define PLUGIN_TEN_PER_SECOND               5
#define PLUGIN_WRITE                       13
#define PLUGIN_SERIAL_IN                   16
#define PLUGIN_UNCONDITIONAL_POLL          25
#define PLUGIN_INFO                       255

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
#define PIN_D_MAX        40
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
#include <lwip/netif.h>
#include <lwip/etharp.h>
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
  boolean       UseSerial = true;
  boolean       UseNTP;
  int16_t       TimeZone;
  byte          SerialTelnet=0;
  byte          Controller_IP[4];
  unsigned int  ControllerPort;
  char          MQTTsubscribe[80];
  boolean       MQTTRetainFlag;
  boolean       UseMSGBusUDP = true;
  boolean       UseMSGBusMQTT;
  boolean       UseGratuitousARP = false;
  boolean	      LogEvents = false;
  boolean       ForceAPMode = false;
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
struct timerStruct
{
  String Name;
  unsigned long Value;
} RulesTimer[RULES_TIMER_MAX];
#endif

struct confirmQueueStruct
{
  String Name;
  byte Attempts;
  byte State;
  byte TimerTicks;
} confirmQueue[CONFIRM_QUEUE_MAX];

unsigned int NC_Count = 0;
unsigned int C_Count = 0;
boolean AP_Mode = false;
byte cmd_within_mainloop = 0;
boolean bootConfig = false;
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
String printWebTools = "";

void setNvar(String varName, float value, int decimals = -1);
String parseString(String& string, byte indexFind, char separator = ',');

unsigned long loopCounter = 0;
unsigned long loopCounterLast=0;

#if FEATURE_PLUGINS
boolean (*Plugin_ptr[PLUGIN_MAX])(byte, String&, String&);
byte Plugin_id[PLUGIN_MAX];
String dummyString = "";
#endif

#if SERIALDEBUG
  byte debugLevel = 0;
#endif

/*********************************************************************************************\
  SETUP
  \*********************************************************************************************/
void setup()
{
  WiFi.mode(WIFI_OFF);

  #if SERIALDEBUG
    Serial.begin(115200);
    Serial.println("");
    Serial.println("Serial init");
  #endif

  #if SERIALDEBUG_EXTRA
  for(byte x=0; x < 10;x++){
    Serial.print("WifiStatus:");
    Serial.print(WiFi.status());
    #if defined(ESP8266)
      Serial.print(" - ");
      Serial.print(wifi_station_get_connect_status());
    #endif
    Serial.print(" - ");
    IPAddress ip = WiFi.localIP();
    char str[20];
    sprintf_P(str, PSTR("%u.%u.%u.%u"), ip[0], ip[1], ip[2], ip[3]);
    Serial.println(str);
    delay(1000);
  }
  #endif

  String event = "";

  if (SPIFFS.begin())
  {
    #if SERIALDEBUG
      Serial.println("SPIFFS init");
    #endif
    event = "System#Config";
    if (SPIFFS.exists(FILE_BOOT)){
      bootConfig = true;
      #if FEATURE_RULES
        rulesProcessing(FILE_BOOT, event);
      #endif
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
      f.close();
    }
    
    #if FEATURE_RULES
      rulesProcessing(FILE_RULES, event);
    #endif

    WiFi.persistent(false); // Do not use SDK storage of SSID/WPA parameters
    #if SERIALDEBUG
      Serial.println("Start Wificonnect");
    #endif
    
    WifiAPconfig();
    WiFi.mode(WIFI_STA);
    WifiConnect(3);
    #if SERIALDEBUG
      Serial.println("End Wificonnect");
    #endif
    #if SERIALDEBUG
      Serial.println("Start WebServer");
    #endif
    WebServerInit();
  }
  else
  {
    Serial.println("SPIFFS?");
    delay(1);
  }

  if (bootConfig && !Settings.UseSerial) {
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

  #if SERIALDEBUG
    Serial.println("Start TelnetServer");
  #endif
  ser2netServer = new WiFiServer(23);
  ser2netServer->begin();

  #if SERIALDEBUG
    Serial.println("Start UDP Server");
  #endif
  if(Settings.UseMSGBusUDP)
    portUDP.begin(Settings.Port);

  #if FEATURE_TIME
    initTime();
  #endif

  #ifdef FEATURE_ARDUINO_OTA
    #if SERIALDEBUG
      Serial.println("Start OTA Server");
    #endif
    ArduinoOTAInit();
  #endif

  #if FEATURE_PLUGINS
    #if SERIALDEBUG
      Serial.println("Init Plugins");
    #endif
    PluginInit();
  #endif

  #if FEATURE_MSGBUS
    // Get others to announce themselves so i can update my node list
    if(Settings.UseMSGBusUDP){
      event = F("MSGBUS/Refresh");
      UDPSend(event);
    }
  #endif

  #if FEATURE_RULES
    #if SERIALDEBUG
    Serial.println("Boot/Rules processing");
    #endif
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

  #if FEATURE_MSGBUS
    if(Settings.UseMSGBusUDP)
      MSGBusReceive();
    
    #if FEATURE_MQTT
    if(Settings.UseMSGBusMQTT) 
      MQTTclient.loop();
    #endif
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
  
  loopCounter++;
}

/********************************************************************************************\
  10 times per second
\*********************************************************************************************/
void run10PerSecond(){
  timer10ps = millis() + 100;
  #if FEATURE_PLUGINS
    PluginCall(PLUGIN_TEN_PER_SECOND, dummyString, dummyString);
  #endif
  #if FEATURE_MSGBUS
    if(Settings.UseMSGBusUDP) 
      MSGBusQueue();
  #endif
}
/********************************************************************************************\
  Each second
\*********************************************************************************************/
void runEachSecond() {
  #if SERIALDEBUG
    if(debugLevel == 2)
      Serial.println(WiFi.status());
  #endif
  timer1 = millis() + 1000;
  #if FEATURE_PLUGINS
    PluginCall(PLUGIN_ONCE_A_SECOND, dummyString, dummyString);
  #endif
  #if FEATURE_TIME
    checkTime();
  #endif
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
    if(Settings.UseGratuitousARP)   
      sendGratuitousARP();
    #if FEATURE_MQTT
    if(Settings.UseMSGBusMQTT) 
      MQTTCheck();
    #endif
    refreshNodeList();
    #if FEATURE_MSGBUS
      MSGBusAnnounceMe();
    #endif
    loopCounterLast = loopCounter;
    loopCounter = 0;
  }

