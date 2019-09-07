#if FEATURE_MSGBUS
//********************************************************************************************
// UDP receive message bus packets
//********************************************************************************************
void MSGBusReceive() {
  int packetSize = portUDP.parsePacket();

  if (packetSize > 0)
  {
    IPAddress remoteIP = portUDP.remoteIP();
    char packetBuffer[packetSize + 1];
    int len = portUDP.read(packetBuffer, packetSize);

    // check if this is a plain text message, do not process other messages
    if (packetBuffer[0] > 127)
      return;
      
    packetBuffer[len] = 0;
    String msg = &packetBuffer[0];

    // First process messages that request confirmation
    // These messages start with '>' and must be addressed to my node name
    String mustConfirm = String(">") + Settings.Name + String("/");
    if (msg.startsWith(mustConfirm)) {
      String reply = "<" + msg.substring(1);
      UDPSend(reply);
    }
    if (msg[0] == '>'){
     msg = msg.substring(1); // Strip the '>' request token from the message
    }

    // Process confirmation messages
    if (msg[0] == '<'){
      for (byte x = 0; x < CONFIRM_QUEUE_MAX ; x++) {
        if (confirmQueue[x].Name.substring(1) == msg.substring(1)) {
          confirmQueue[x].State = 0;
          break;
        }
      }

      if(Settings.LogEvents)
        telnetLog(String("UDP: ") + msg);
      return; // This message needs no further processing, so return.
    }

    // Special MSGBus system events
    if (msg.substring(0, 7) == F("MSGBUS/")) {
      String sysMSG = msg.substring(7);
      if (sysMSG.substring(0, 9) == F("Hostname=")) {
        String params = sysMSG.substring(9);
        String hostName = parseString(params, 1);
        //String ip = parseString(params, 2); we just take the remote ip here
        String group = parseString(params, 3);
        nodelist(remoteIP, hostName, group);
      }
      if (sysMSG.substring(0, 7) == F("Refresh")) {
        MSGBusAnnounceMe();
      }
    }

    #if FEATURE_RULES
      rulesProcessing(FILE_RULES, msg);
    #endif

    if(Settings.LogEvents)
      telnetLog(String("UDP: ") + msg);
  }
}


//********************************************************************************************
// UDP Send message bus packet
//********************************************************************************************
void UDPSend(String message)
{
  IPAddress broadcastIP(255, 255, 255, 255);
  portUDP.beginPacket(broadcastIP, Settings.Port);
  portUDP.print(message);
  portUDP.endPacket();
  delay(0);
}


//********************************************************************************************
// UDP Send message bus hostname announcement
//********************************************************************************************
void MSGBusAnnounceMe() {
  if(Settings.UseMSGBusUDP){
    String msg = F("MSGBUS/Hostname=");
    msg += Settings.Name;
    msg += ",0.0.0.0,";
    msg += Settings.Group;
    UDPSend(msg);
  }

  #if FEATURE_MQTT
    if(Settings.UseMSGBusMQTT){
      String topic = F(MSGBUS_TOPIC);
      topic += F("MSGBUS/Hostname");
      String msg = Settings.Name;
      char str[20];
      IPAddress ip = WiFi.localIP();
      sprintf_P(str, PSTR("%u.%u.%u.%u"), ip[0], ip[1], ip[2], ip[3]);
      msg += ",";
      msg += str;
      msg += ",";
      msg += Settings.Group;
      MQTTclient.publish(topic.c_str(), msg.c_str(),Settings.MQTTRetainFlag);
    }
  #endif
}


//********************************************************************************************
// Check MessageBus queue
//********************************************************************************************
void MSGBusQueue() {
  for (byte x = 0; x < CONFIRM_QUEUE_MAX ; x++) {
    if (confirmQueue[x].State == 1){
      if(confirmQueue[x].Attempts !=0){
        confirmQueue[x].TimerTicks--;
        if(confirmQueue[x].TimerTicks == 0){
          confirmQueue[x].TimerTicks = 3;
          confirmQueue[x].Attempts--;
          UDPSend(confirmQueue[x].Name);
        }
      }
      else{
        telnetLog(F("Confirmation Timeout"));
        confirmQueue[x].State = 0;
      }
    }
  }
}

#if FEATURE_MQTT
/*********************************************************************************************\
 * Handle incoming MQTT messages
\*********************************************************************************************/
// handle MQTT messages
void callback(char* c_topic, byte* b_payload, unsigned int length) {
  char c_payload[length];
  strncpy(c_payload,(char*)b_payload,length);
  c_payload[length] = 0;

  String topic = c_topic;
  String msg = c_payload;
  msg.replace("\r","");
  msg.replace("\n","");

  if (topic.substring(0, 10) == F(MSGBUS_TOPIC))
      topic = topic.substring(10);

  // Special MSGBus system events
  if (topic.substring(0, 7) == F("MSGBUS/")) {
    String sysMSG = topic.substring(7);
    if (sysMSG.substring(0, 8) == F("Hostname")){
      String hostName = parseString(msg,1);
      String strIP = parseString(msg,2);
      String group = parseString(msg,3);
      char tmpString[26];
      strIP.toCharArray(tmpString, 26);
      byte IP[4];
      str2ip(tmpString, IP);
      IPAddress remoteIP;
      for(byte x=0; x<4;x++)
        remoteIP[x]=IP[x];
      nodelist(remoteIP, hostName, group);
    }
    if (sysMSG.substring(0, 7) == F("Refresh")) {
      MSGBusAnnounceMe();
    }
  }

  #if FEATURE_RULES
    String event = topic;
    event += "=";
    event += msg; 
    rulesProcessing(FILE_RULES, event);
  #endif
}


/*********************************************************************************************\
 * Connect to MQTT message broker
\*********************************************************************************************/
void MQTTConnect()
{
  IPAddress MQTTBrokerIP(Settings.Controller_IP);
  MQTTclient.setServer(MQTTBrokerIP, Settings.ControllerPort);
  MQTTclient.setCallback(callback);

  // MQTT needs a unique clientname to subscribe to broker
  String clientid = F("ESPClient");
  clientid += Settings.Name;
  String subscribeTo = F(MSGBUS_TOPIC);
  subscribeTo += "#";

  String LWTTopic = F(MSGBUS_TOPIC); //Settings.MQTTsubscribe;
  String LWTMsg = F("Connection lost ");
  LWTMsg +=Settings.Name;
  
  for (byte x = 1; x < 3; x++)
  {
    String log = "";
    boolean MQTTresult = false;

    //boolean connect(const char* id);
    //boolean connect(const char* id, const char* user, const char* pass);
    //boolean connect(const char* id, const char* willTopic, uint8_t willQos, boolean willRetain, const char* willMessage);
    //boolean connect(const char* id, const char* user, const char* pass, const char* willTopic, uint8_t willQos, boolean willRetain, const char* willMessage);

    if ((SecuritySettings.ControllerUser[0] != 0) && (SecuritySettings.ControllerPassword[0] != 0))
      MQTTresult = MQTTclient.connect(clientid.c_str(), SecuritySettings.ControllerUser, SecuritySettings.ControllerPassword, LWTTopic.c_str(), 0, 0, LWTMsg.c_str());
    else
      MQTTresult = MQTTclient.connect(clientid.c_str(), LWTTopic.c_str(), 0, 0, LWTMsg.c_str());

    if (MQTTresult)
    {
      log = F("MQTT : Connected to broker");
      telnetLog(log);
      MQTTclient.subscribe(subscribeTo.c_str());
      break; // end loop if succesfull
    }
    else
    {
      log = F("MQTT : Failed to connected to broker");
      telnetLog(log);
    }

    delay(500);
  }
}


/*********************************************************************************************\
 * Check connection MQTT message broker
\*********************************************************************************************/
void MQTTCheck()
{
  if (Settings.UseMSGBusMQTT)
    if (!MQTTclient.connected())
    {
      String log = F("MQTT : Connection lost");
      telnetLog(log);
      connectionFailures += 2;
      MQTTclient.disconnect();
      delay(1000);
      MQTTConnect();
    }
    else if (connectionFailures)
      connectionFailures--;
}
#endif

#endif // MSGBUS

