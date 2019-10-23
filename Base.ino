//********************************************************************************************
// Setup
//********************************************************************************************
void setup2()
{
  logger = &Serial;

  WiFi.mode(WIFI_OFF);    // Allways start in OFF mode
  #if defined(ESP8266)
    WiFi.forceSleepBegin();
    delay(1);
  #endif
  WiFi.persistent(false); // Do not use SDK storage of SSID/WPA parameters

  #if FEATURE_I2C
    Wire.begin();
  #endif

  // Run plugin init before running spiffs/config because we may need commands that are provided by plugins!
  #if FEATURE_PLUGINS
    PluginInit();
  #endif
  
  ConfigInit();
  serialInit();
  if(Settings.Wifi && Settings.AutoConnect){
    WifiInit();
  }
  
  #if FEATURE_RULES
    #if SERIALDEBUG
    Serial.println("Boot/Rules processing");
    #endif
    String event = F("System#Boot");
    rulesProcessing(FILE_BOOT, event);
    rulesProcessing(FILE_RULES, event);
  #endif

}

//*********************************************************************************************
// MAIN LOOP
//*********************************************************************************************
void loop()
{
  serial();

  if(WifiClient()){
    WebServer.handleClient();

    #if FEATURE_MSGBUS
      if(Settings.Wifi && Settings.UseMSGBusUDP)
        MSGBusReceive();
    
      #if FEATURE_MQTT
      if(Settings.UseMSGBusMQTT) 
        MQTTclient.loop();
      #endif
    #endif

    telnet();
    
    #ifdef FEATURE_ARDUINO_OTA
      ArduinoOTA.handle();
      //once OTA is triggered, only handle that and dont do other stuff. (otherwise it fails)
      while (ArduinoOTAtriggered)
      {
        yield();
        ArduinoOTA.handle();
      }
    #endif

    #if defined(ESP8266)
      tcpCleanup();
    #endif

    if (cmd_within_mainloop == CMD_REBOOT)
    reboot();

  }
  
  if (millis() > timer10ps)
    run10PerSecond();

  if (millis() > timer1)
    runEachSecond();

  if (millis() > timer60)
    runEach60Seconds();

  delay(0);
  
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
    if(WifiClient() && Settings.UseMSGBusUDP)
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
    if(WifiClient() && Settings.UseTime)
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
    if(Settings.Wifi && Settings.AutoConnect){
      if(Settings.AutoAPOnFailure)
        WifiCheck();
      if(WifiClient() && Settings.UseGratuitousARP)   
        sendGratuitousARP();
    }
    #if FEATURE_MQTT
    if(WifiClient() && Settings.UseMSGBusMQTT) 
      MQTTCheck();
    #endif

    refreshNodeList();
    
    #if FEATURE_MSGBUS
      if(WifiClient()){
        MSGBusAnnounceMe();
      }
    #endif
    loopCounterLast = loopCounter;
    loopCounter = 0;
}
