//#######################################################################################################
//#################################### Plugin 255: Debug ################################################
//#######################################################################################################

/*
 * Commands:
 * analogRead <variable>,<pin>                   Read analog value into variable
 * analogDebug <pin>                             Show analog value every second on telnet session
*/

#ifdef USES_P255
#define P255_BUILD            6
#define PLUGIN_255
#define PLUGIN_ID_255         255

boolean Plugin_255(byte function, String& cmd, String& params)
{
  boolean success = false;
  static int8_t  PinDebug = -1;

  switch (function)
  {
    case PLUGIN_INFO:
      {
        printWebTools += F("<TR><TD>P255 - Debug<TD>");
        printWebTools += P255_BUILD;
        break;
      }
          
    case PLUGIN_WRITE:
      {
        if (cmd.equalsIgnoreCase(F("Registers")))
        {
          success = true;
          for(byte x=0; x < PLUGIN_FASTCALL_MAX; x++){
            if(corePluginCall_ptr[x] != 0){
              logger->print(x);
              logger->print(":");
              logger->println((int)corePluginCall_ptr[x]);
            }
          }
        }
        if (cmd.equalsIgnoreCase(F("WifiOff")))
        {
          success = true;
          WiFi.mode( WIFI_OFF );
          #if defined(ESP8266)
            WiFi.forceSleepBegin();
          #endif
          delay(1);
        }
        if (cmd.equalsIgnoreCase(F("WifiClear")))
        {
          success = true;
          #if defined(ESP8266)
            ESP.eraseConfig();
            ESP.reset();
          #endif
        }
        if (cmd.equalsIgnoreCase(F("WifiConnectPersistent")))
        {
          success = true;
          String ssid = parseString(params, 1);
          String key = parseString(params, 2);
          WiFi.persistent(true);
          WiFi.begin(ssid.c_str(),key.c_str());
        }
        if (cmd.equalsIgnoreCase(F("WifiDisConnect")))
        {
          success = true;
          WiFi.disconnect();
        }
        if (cmd.equalsIgnoreCase(F("WifiStatus")))
        {
          success = true;
          Serial.println(WiFi.status());
        }
        if (cmd.equalsIgnoreCase(F("WifiAPMode")))
        {
          success = true;
          int Par1 = parseString(params, 1).toInt();
          WifiAPMode(true);
          Settings.ForceAPMode = (Par1 == 1);
        }
        
        #if SERIALDEBUG
        if (cmd.equalsIgnoreCase(F("debugLevel")))
        {
          success = true;
          int Par1 = parseString(params, 1).toInt();
          //debugLevel = Par1;
        }
        #endif

        break;
      }
  }
  return success;
}
#endif

