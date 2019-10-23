#define BUILD                               5

#define CMD_REBOOT                         89
#define UNIT_MAX                           32
#define PLUGIN_MAX                         32
#define RULES_MAX_SIZE                   2048
#define RULES_BUFFER_SIZE                  64
#define RULES_MAX_NESTING_LEVEL             3
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

#if FEATURE_I2C
  #include <Wire.h>
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
    #if FEATURE_ESPNOW
      #include <espnow.h>
    #endif
  }
  #define PIN_D_MAX        16
  #if FEATURE_ADC_VCC
    ADC_MODE(ADC_VCC);
  #endif
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
  boolean       Wifi = true;
  boolean       AutoConnect = true;
  boolean       AutoAPOnFailure = true;
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
  boolean       UseTime = true;
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
  boolean        LogEvents = false;
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

Print* logger;
