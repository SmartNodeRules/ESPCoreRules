// This is a Core edition of ESP Easy, initially based on R120 and some Mega code
// Copyright notice on tab __Copyright!
// Command reference on tab __Doc and the plugins each have their own command reference

// This tab does not contain code other than one call to setup2
// Maintain your own version when required

// Select features to include into the Core:
#define FEATURE_RULES                    true
#define FEATURE_MSGBUS                   true
#define FEATURE_PLUGINS                  true
#define FEATURE_TIME                     true
#define FEATURE_I2C                      true

// Currently only supported on ESP8266
#if defined(ESP8266)
  #define FEATURE_ESPNOW                 true
#endif

// Some extra features, disabled by default
#define FEATURE_MQTT                     false
#define FEATURE_ADC_VCC                  false
#define FEATURE_DEBUG_CMD                false
#define SERIALDEBUG                      false

// Select a custom plugin set
//#define PLUGIN_SET_BASIC
#define PLUGIN_SET_ALL

#ifdef PLUGIN_SET_BASIC
  #define USES_P001
  #define USES_P002
#endif

#ifdef PLUGIN_SET_ALL
  #define USES_P001
  #define USES_P002
  #define USES_P004
  #define USES_P011
  #define USES_P012
  #define USES_P014
  #define USES_P200
  #define USES_P201
#endif


// End of config section, do not remove this:
// Could not move setup() to base tab, preprocessor gets confused (?)
// So we just call setup2() from here
#include "Globals.h"
void setup(){
  setup2();
}
