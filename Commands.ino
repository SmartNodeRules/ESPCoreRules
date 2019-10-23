//*********************************************************************************************
// Execute commands
//*********************************************************************************************

#define INPUT_COMMAND_SIZE          80
void ExecuteCommand(const char *Line)
{
  #if FEATURE_PLUGINS
     // first check the plugins
    String cmd = Line;
    String params = "";
    int parampos = getParamStartPos(cmd, 2);
    if (parampos != -1) {
      params = cmd.substring(parampos);
      cmd = cmd.substring(0, parampos - 1);
    }
    if (PluginCall(PLUGIN_WRITE, cmd, params)) {
      logger->println("OK");
      return;
    }
  #endif
  
  boolean success = false;
  char TmpStr1[80];
  TmpStr1[0] = 0;
  char Command[80];
  Command[0] = 0;
  int Par1 = 0;
  int Par2 = 0;
  int Par3 = 0;
  int Par4 = 0;

  GetArgv(Line, Command, 1);
  if (GetArgv(Line, TmpStr1, 2)) Par1 = str2int(TmpStr1);
  if (GetArgv(Line, TmpStr1, 3)) Par2 = str2int(TmpStr1);
  if (GetArgv(Line, TmpStr1, 4)) Par3 = str2int(TmpStr1);
  if (GetArgv(Line, TmpStr1, 5)) Par4 = str2int(TmpStr1);


  //********************************************************************************
  // Some Debug commands, unofficial stuff goes here
  //********************************************************************************

  #if FEATURE_DEBUG_CMD
  if (strcasecmp_P(Command, PSTR("WifiOff")) == 0)
  {
    success = true;
    WiFi.mode( WIFI_OFF );
    #if defined(ESP8266)
      WiFi.forceSleepBegin();
    #endif
    delay(1);
  }
  if (strcasecmp_P(Command, PSTR("WifiClear")) == 0)
  {
    success = true;
    #if defined(ESP8266)
      ESP.eraseConfig();
      ESP.reset();
    #endif
  }
  if (strcasecmp_P(Command, PSTR("WifiConnectPersistent")) == 0)
  {
    success = true;
    String strLine = Line;
    String ssid = parseString(strLine, 2);
    String key = parseString(strLine, 3);
    WiFi.persistent(true);
    WiFi.begin(ssid.c_str(),key.c_str());
  }
  if (strcasecmp_P(Command, PSTR("WifiDisConnect")) == 0)
  {
    success = true;
    WiFi.disconnect();
  }
  if (strcasecmp_P(Command, PSTR("WifiStatus")) == 0)
  {
    success = true;
    Serial.println(WiFi.status());
  }
  if (strcasecmp_P(Command, PSTR("WifiAPMode")) == 0)
  {
    success = true;
    WifiAPMode(true);
    Settings.ForceAPMode = (Par1 == 1);
  }
  
  #if SERIALDEBUG
    if (strcasecmp_P(Command, PSTR("debugLevel")) == 0)
    {
      success = true;
      debugLevel = Par1;
    }
  #endif
  #endif
    
  //********************************************************************************
  // Config commands
  //********************************************************************************

  if (strcasecmp_P(Command, PSTR("Config")) == 0)
  {
    success = true;
    String strLine = Line;
    String setting = parseString(strLine, 2);
    String strP1 = parseString(strLine, 3);
    String strP2 = parseString(strLine, 4);
    String strP3 = parseString(strLine, 5);
    String strP4 = parseString(strLine, 6);
    String strP5 = parseString(strLine, 7);

    if (setting.equalsIgnoreCase(F("AutoConnect"))){
      Settings.AutoConnect = (Par2 == 1);
    }

    if (setting.equalsIgnoreCase(F("Baudrate"))){
      if (Par2){
        Settings.BaudRate = Par2;
        Settings.UseSerial = 1;
        Serial.begin(Settings.BaudRate);
      }
      else
      {
        Settings.BaudRate = Par2;
        Settings.UseSerial = 0;
        pinMode(1, INPUT);
        pinMode(3, INPUT);
      }
    }

    if (setting.equalsIgnoreCase(F("DST"))){
      Settings.DST = (Par2 == 1);
    }

    if (setting.equalsIgnoreCase(F("Name"))){
      strcpy(Settings.Name, strP1.c_str());
    }

    if (setting.equalsIgnoreCase(F("Group"))){
      strcpy(Settings.Group, strP1.c_str());
    }

    if (setting.equalsIgnoreCase(F("Network"))){
      char tmpString[26];
      strP1.toCharArray(tmpString, 26);
      str2ip(tmpString, Settings.IP);
      strP2.toCharArray(tmpString, 26);
      str2ip(tmpString, Settings.Subnet);
      strP3.toCharArray(tmpString, 26);
      str2ip(tmpString, Settings.Gateway);
      strP4.toCharArray(tmpString, 26);
      str2ip(tmpString, Settings.DNS);
      IPAddress ip = Settings.IP;
      IPAddress gw = Settings.Gateway;
      IPAddress subnet = Settings.Subnet;
      IPAddress dns = Settings.DNS;
//      WiFi.config(ip, gw, subnet, dns);
    }

    if (setting.equalsIgnoreCase(F("sendARP"))){
      Settings.UseGratuitousARP = (Par2 == 1);
    }
    if (setting.equalsIgnoreCase(F("Port"))){
      Settings.Port = Par2;
    }

    if (setting.equalsIgnoreCase(F("Time"))){
      Settings.UseTime = (Par2 == 1);
    }

    if (setting.equalsIgnoreCase(F("Timezone"))){
      Settings.TimeZone = Par2;
    }

    if (setting.equalsIgnoreCase(F("WifiSSID")))
      strcpy(SecuritySettings.WifiSSID, strP1.c_str());

    if (setting.equalsIgnoreCase(F("WifiKey")))
      strcpy(SecuritySettings.WifiKey, strP1.c_str());

    if (setting.equalsIgnoreCase(F("WifiSSID2")))
      strcpy(SecuritySettings.WifiSSID2, strP1.c_str());

    if (setting.equalsIgnoreCase(F("WifiKey2")))
      strcpy(SecuritySettings.WifiKey2, strP1.c_str());

    if (setting.equalsIgnoreCase(F("WifiAPKey")))
      strcpy(SecuritySettings.WifiAPKey, strP1.c_str());

    if (setting.equalsIgnoreCase(F("MSGBUS"))){
      if(strP1.equalsIgnoreCase(F("UDP")))
        Settings.UseMSGBusUDP = (Par3 == 1);
      if(strP1.equalsIgnoreCase(F("MQTT")))
        Settings.UseMSGBusMQTT = (Par3 == 1);
    }

    #if FEATURE_MQTT
      if (setting.equalsIgnoreCase(F("MQTT"))){
        char tmpString[26];
        strP1.toCharArray(tmpString, 26);
        str2ip(tmpString, Settings.Controller_IP);
        Settings.ControllerPort = strP2.toInt();
        strcpy(SecuritySettings.ControllerUser, strP3.c_str());
        strcpy(SecuritySettings.ControllerPassword, strP4.c_str());
        MQTTConnect();
      }
    #endif

    if (setting.equalsIgnoreCase(F("Rules"))){
      if(strP1.equalsIgnoreCase(F("Clock")))
        Settings.RulesClock = (Par3 == 1);
      if(strP1.equalsIgnoreCase(F("Serial")))
        Settings.RulesSerial = (Par3 == 1);
    }

    if (setting.equalsIgnoreCase(F("LogEvents"))){
      Settings.LogEvents = (Par2 == 1);
    }

  }


  //********************************************************************************
  // Operational commands
  //********************************************************************************

  if (strcasecmp_P(Command, PSTR("DeepSleep")) == 0)
  {
    success = true;
    #if defined(ESP8266)
      ESP.deepSleep(Par1 * 1000000, WAKE_RF_DEFAULT); // Sleep for set delay
    #endif
  }

  if (strcasecmp_P(Command, PSTR("Delay")) == 0)
  {
    success = true;
    delay(Par1);
  }
  
  #if FEATURE_ESPNOW
  if (strcasecmp_P(Command, PSTR("espnowConfig")) == 0)
  {
    success = true;
    String strLine = Line;
    String kok = parseString(strLine, 2);
    String key = parseString(strLine, 3);
    String macStr = parseString(strLine, 4);
    String mode = parseString(strLine, 5);
    byte mac[6];
    parseBytes(macStr.c_str(), ':', mac, 6, 16);
    if (mode.equalsIgnoreCase(F("Sender"))){
      espnowSender(kok.c_str(), key.c_str(), mac);
    }else{
      espnowReceiver(kok.c_str(), key.c_str(), mac);
    }
  }

  if (strcasecmp_P(Command, PSTR("espnowAddPeer")) == 0)
  {
    success = true;
    String strLine = Line;
    String key = parseString(strLine, 2);
    String macStr = parseString(strLine, 3);
    byte role = parseString(strLine, 4).toInt();
    byte mac[6];
    parseBytes(macStr.c_str(), ':', mac, 6, 16);
    espnowAddPeer(key.c_str(), mac, role);
  }  

  if (strcasecmp_P(Command, PSTR("espnowsend")) == 0)
  {
    success = true;
    String msg = Line;
    msg = msg.substring(11);
    espnowSend(msg);
  }
  #endif

  #if FEATURE_I2C
  if (strcasecmp_P(Command, PSTR("I2C")) == 0)
  {
    success = true;
    byte error, address;
    for (address = 1; address <= 127; address++) {
      Wire.beginTransmission(address);
      error = Wire.endTransmission();
      if (error == 0) {
        String log = F("I2C  : Found 0x");
        log += String(address, HEX);
        logger->println(log);
      }
    }
  }
  #endif
  
  #if FEATURE_MSGBUS
    if (strcasecmp_P(Command, PSTR("MSGBus")) == 0)
    {
      success = true;
      String msg = Line;
      msg = msg.substring(7);
    
      if(Settings.UseMSGBusUDP){ 
        if (msg[0] == '>') {
          for (byte x = 0; x < CONFIRM_QUEUE_MAX ; x++) {
            if (confirmQueue[x].State == 0) {
              confirmQueue[x].Name = msg;
              confirmQueue[x].Attempts = 9;
              confirmQueue[x].State = 1;
              confirmQueue[x].TimerTicks = 3;
              UDPSend(msg);
              break;
            }
          }
        }
        else
          UDPSend(msg);
      }
    
      #if FEATURE_MQTT
        if(Settings.UseMSGBusMQTT){ 
          String topic = F(MSGBUS_TOPIC);
          String payload = "";
          int pos = msg.indexOf('=');
          if(pos != -1){
            topic += msg.substring(0,pos);
            payload = msg.substring(pos+1);
          }
          MQTTclient.publish(topic.c_str(), payload.c_str(),Settings.MQTTRetainFlag);
        }
      #endif
    }
  #endif

  #if FEATURE_TIME
    if (strcasecmp_P(Command, PSTR("NTP")) == 0)
    {
      success = true;
      getNtpTime();
      logger->println(getTimeString(':'));
    }
  #endif

  if (strcasecmp_P(Command, PSTR("Reboot")) == 0)
  {
    success = true;
    pinMode(0, INPUT);
    pinMode(2, INPUT);
    pinMode(15, INPUT);
    reboot();
  }

  if (strcasecmp_P(Command, PSTR("Reset")) == 0)
  {
    success = true;
    SPIFFS.end();
    logger->println(F("RESET: formatting..."));
    SPIFFS.format();
    logger->println(F("RESET: formatting done..."));
    if (SPIFFS.begin()){
      initFiles();
    }
    else{
      logger->println(F("RESET: FORMAT SPIFFS FAILED!"));
    }
  }

  if (strcasecmp_P(Command, PSTR("SendToUDP")) == 0)
  {
    success = true;
    String strLine = Line;
    String ip = parseString(strLine, 2);
    String port = parseString(strLine, 3);
    int msgpos = getParamStartPos(strLine, 4);
    String message = strLine.substring(msgpos);
    byte ipaddress[4];
    str2ip((char*)ip.c_str(), ipaddress);
    IPAddress UDP_IP(ipaddress[0], ipaddress[1], ipaddress[2], ipaddress[3]);
    portUDP.beginPacket(UDP_IP, port.toInt());
    #if defined(ESP8266)
      portUDP.write(message.c_str(), message.length());
    #endif
    #if defined(ESP32)
      portUDP.write((uint8_t*)message.c_str(), message.length());
    #endif
    portUDP.endPacket();
  }

  if (strcasecmp_P(Command, PSTR("Serial")) == 0)
  {
    success = true;
    Serial.begin(115200);
  }

  if (strcasecmp_P(Command, PSTR("SerialFloat")) == 0)
  {
    success = true;
    pinMode(1, INPUT);
    pinMode(3, INPUT);
  }

  if (strcasecmp_P(Command, PSTR("SerialSend")) == 0)
  {
    success = true;
    String tmpString = Line;
    tmpString = tmpString.substring(11);
    Serial.println(tmpString);
  }

  if (strcasecmp_P(Command, PSTR("SerialSendFile")) == 0)
  {
    success = true;
    String tmpString = Line;
    String fileName = parseString(tmpString, 2);
    fs::File f = SPIFFS.open(fileName, "r");
    while (f.available()){
      byte data = f.read();
      Serial.write(data);
      if(data == '\n')
        delay(100);
      delay(10);
    }
  }

  if (strcasecmp_P(Command, PSTR("SerialTelnet")) == 0)
  {
    success = true;
    Settings.SerialTelnet = Par1;
  }

  if (strcasecmp_P(Command, PSTR("Settings")) == 0)
  {
    success = true;
    char str[20];
    logger->println();

    logger->println(F("System Info"));
    IPAddress ip = WiFi.localIP();
    sprintf_P(str, PSTR("%u.%u.%u.%u"), ip[0], ip[1], ip[2], ip[3]);
    logger->print(F("  IP Address    : ")); logger->println(str);
    logger->print(F("  Build         : ")); logger->println((int)BUILD);
    String core = getSystemLibraryString();
    logger->print(F("  Core          : ")); logger->println(core);
    logger->print(F("  WifiSSID      : ")); logger->println(SecuritySettings.WifiSSID);
    logger->print(F("  WifiKey       : ")); logger->println(SecuritySettings.WifiKey);
    logger->print(F("  Wifi          : ")); logger->println(Settings.Wifi);
  }

  #if FEATURE_MQTT
  if (strcasecmp_P(Command, PSTR("SubScribe")) == 0)
  {
    success = true;
    MQTTclient.subscribe(&Line[10]);
  }
  #endif

#if FEATURE_RULES
  if (strcasecmp_P(Command, PSTR("TimerSet")) == 0)
  {
    if (GetArgv(Line, TmpStr1, 2)) {
      String varName = TmpStr1;
      success = true;
      if (Par2)
        setTimer(varName, millis() + (1000 * Par2));
      else
        setTimer(varName, 0L);
    }
  }

  if (strcasecmp_P(Command, PSTR("Event")) == 0)
  {
    success = true;
    String event = Line;
    event = event.substring(6);
    rulesProcessing(FILE_RULES, event);
  }

  if (strcasecmp_P(Command, PSTR("ValueSet")) == 0)
  {
    success = true;
    if (GetArgv(Line, TmpStr1, 3))
    {
      float result = 0;
      Calculate(TmpStr1, &result);
      if (GetArgv(Line, TmpStr1, 2)) {
        String varName = TmpStr1;
        if (GetArgv(Line, TmpStr1, 4))
          setNvar(varName, result, Par3);
        else
          setNvar(varName, result);
      }
    }
  }

  if (strcasecmp_P(Command, PSTR("StringSet")) == 0)
  {
    success = true;
    String sline = Line;
    int pos = getParamStartPos(sline, 3);
    if (pos != -1) {
      if (GetArgv(Line, TmpStr1, 2)) {
        String varName = TmpStr1;
        setSvar(varName, sline.substring(pos));
      }
    }
  }

  if (strcasecmp_P(Command, PSTR("Syslog")) == 0)
  {
    success = true;
    String log = Line;
    log = log.substring(7);
    syslog(log);
  }

  #if FEATURE_ADC_VCC
    if (strcasecmp_P(Command, PSTR("VCCRead")) == 0)
    {
      success = true;
      String strLine = Line;
      String varName = parseString(strLine, 2);
      setNvar(varName, ESP.getVcc() / 1000.0);
    }
  #endif

  if (strcasecmp_P(Command, PSTR("webPrint")) == 0)
  {
    success = true;
    String wprint = Line;
    if (wprint.length() == 8)
      printWebString = "";
    else
      printWebString += wprint.substring(9);
  }

  if (strcasecmp_P(Command, PSTR("webButton")) == 0)
  {
    success = true;
    String params = Line;
    printWebString += F("<a class=\"");
    printWebString += parseString(params, 2,';');
    printWebString += F("\" href=\"");
    printWebString += parseString(params, 3,';');
    printWebString += F("\">");
    printWebString += parseString(params, 4,';');
    printWebString += F("</a>");
  }
  
#endif

  if (strcasecmp_P(Command, PSTR("WifiConnect")) == 0)
  {
    success = true;
    String strLine = Line;
    String ssid = parseString(strLine, 2);
    String key = parseString(strLine, 3);
    WiFi.begin(ssid.c_str(),key.c_str());
  }

  if (strcasecmp_P(Command, PSTR("WifiInit")) == 0)
  {
    success = true;
    WifiInit();
  }
  
  if (success)
    logger->println("OK");
  else
    logger->println("?");
}


//*********************************************************************************************
// Find positional parameter in a char string
//*********************************************************************************************

boolean GetArgv(const char *string, char *argv, int argc)
{
  int string_pos = 0, argv_pos = 0, argc_pos = 0;
  char c, d;

  while (string_pos < strlen(string))
  {
    c = string[string_pos];
    d = string[string_pos + 1];

    if       (c == ' ' && d == ' ') {}
    else if  (c == ' ' && d == ',') {}
    else if  (c == ',' && d == ' ') {}
    else if  (c == ' ' && d >= 33 && d <= 126) {}
    else if  (c == ',' && d >= 33 && d <= 126) {}
    else
    {
      argv[argv_pos++] = c;
      argv[argv_pos] = 0;

      if (d == ' ' || d == ',' || d == 0)
      {
        argv[argv_pos] = 0;
        argc_pos++;

        if (argc_pos == argc)
        {
          return true;
        }

        argv[0] = 0;
        argv_pos = 0;
        string_pos++;
      }
    }
    string_pos++;
  }
  return false;
}

