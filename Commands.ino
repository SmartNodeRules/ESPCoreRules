#define INPUT_COMMAND_SIZE          80
void ExecuteCommand(const char *Line)
{
  // first check the plugins
  String cmd = Line;
  String params = "";
  int parampos = getParamStartPos(cmd, 2);
  if (parampos != -1) {
    params = cmd.substring(parampos);
    cmd = cmd.substring(0, parampos - 1);
  }
  if (PluginCall(PLUGIN_WRITE, cmd, params)) {
    telnetLog("OK");
    return;
  }

  boolean success = false;
  char TmpStr1[80];
  TmpStr1[0] = 0;
  char Command[80];
  Command[0] = 0;
  int Par1 = 0;
  int Par2 = 0;
  int Par3 = 0;

  GetArgv(Line, Command, 1);
  if (GetArgv(Line, TmpStr1, 2)) Par1 = str2int(TmpStr1);
  if (GetArgv(Line, TmpStr1, 3)) Par2 = str2int(TmpStr1);
  if (GetArgv(Line, TmpStr1, 4)) Par3 = str2int(TmpStr1);


  // Config commands

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
      WiFi.config(ip, gw, subnet, dns);
    }

    if (setting.equalsIgnoreCase(F("Port"))){
      Settings.Port = Par2;
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

  }


  // operational commands
  #if FEATURE_MQTT
  if (strcasecmp_P(Command, PSTR("SubScribe")) == 0)
  {
    success = true;
    MQTTclient.subscribe(&Line[10]);
  }
  #endif
  
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

  if (strcasecmp_P(Command, PSTR("NTP")) == 0)
  {
    success = true;
    getNtpTime();
    telnetLog(getTimeString(':'));
  }

  if (strcasecmp_P(Command, PSTR("Reboot")) == 0)
  {
    success = true;
    pinMode(0, INPUT);
    pinMode(2, INPUT);
    pinMode(15, INPUT);
    reboot();
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

  if (success)
    telnetLog("OK");
  else
    telnetLog("???");
}


/********************************************************************************************\
  Find positional parameter in a char string
  \*********************************************************************************************/
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

