//********************************************************************************
// Init Wifi station
//********************************************************************************
void WifiInit() {
  String event = "";
  
  #if SERIALDEBUG
    Serial.println("Start Wificonnect");
  #endif
    
  WifiAPconfig();
  #if defined(ESP8266)
    WiFi.forceSleepWake();
    delay(1);
  #endif
  WiFi.mode(WIFI_STA);
  WifiConnect(3);

  #if SERIALDEBUG
    Serial.println("End Wificonnect");
  #endif

  #if SERIALDEBUG
    Serial.println("Start WebServer");
  #endif
  WebServerInit();

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
    if(Settings.UseTime)
      initTime();
  #endif

  #ifdef FEATURE_ARDUINO_OTA
    #if SERIALDEBUG
      Serial.println("Start OTA Server");
    #endif
    ArduinoOTAInit();
  #endif

  #if FEATURE_MSGBUS
    // Get others to announce themselves so i can update my node list
    if(Settings.UseMSGBusUDP){
      event = F("MSGBUS/Refresh");
      UDPSend(event);
    }
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

  // When all things are setup, we send a Wifi#Connected event when Wifi STA is connected
  #if FEATURE_RULES
  if(Settings.Wifi){
    if(WiFi.status() == WL_CONNECTED){
      String event = F("WiFi#Connected");
      rulesProcessing(FILE_RULES, event);
    }
  }
  #endif
}

boolean WifiClient(){
 if(Settings.Wifi && WiFi.status() == WL_CONNECTED)
   return true;
 return false;
}

//********************************************************************************
// Send ARP update
//********************************************************************************
void sendGratuitousARP() {
#ifndef ESP32
  netif *n = netif_list;
  while (n) {
    etharp_gratuitous(n);
    n = n->next;
  }
#endif
}


//********************************************************************************
// Set Wifi AP Mode config
//********************************************************************************
void WifiAPconfig()
{
  char ap_ssid[40];
  ap_ssid[0] = 0;
  strcpy(ap_ssid, "ESP_");
  sprintf_P(ap_ssid, PSTR("%s%s"), ap_ssid, Settings.Name);
  WiFi.softAP(ap_ssid, SecuritySettings.WifiAPKey,6,true);
}


//********************************************************************************
// Set Wifi AP Mode
//********************************************************************************
void WifiAPMode(boolean state)
{
  if(Settings.ForceAPMode){
    if(!AP_Mode){
      #if SERIALDEBUG
        Serial.println("Wifi AP-STA forced");
      #endif
      AP_Mode = true;
      WiFi.mode(WIFI_AP_STA);
    }
    return;
  }
  
  if (state)
  {
    if(!AP_Mode){
      #if SERIALDEBUG
        Serial.println("Wifi AP-STA");
      #endif
      AP_Mode = true;
      WiFi.mode(WIFI_AP_STA);
    }
  }
  else
  {
    if(AP_Mode){
      #if SERIALDEBUG
        Serial.println("Wifi STA");
      #endif
      AP_Mode = false;
      WiFi.mode(WIFI_STA);
    }
  }
}


//********************************************************************************
// Connect to Wifi AP
//********************************************************************************
boolean WifiConnect(byte connectAttempts)
{
  String log = "";

  if (WiFi.status() != WL_CONNECTED)
  {
    if (Settings.IP[0] != 0 && Settings.IP[0] != 255)
    {
      char str[20];
      sprintf_P(str, PSTR("%u.%u.%u.%u"), Settings.IP[0], Settings.IP[1], Settings.IP[2], Settings.IP[3]);
      IPAddress ip = Settings.IP;
      IPAddress gw = Settings.Gateway;
      IPAddress subnet = Settings.Subnet;
      IPAddress dns = Settings.DNS;
      WiFi.config(ip, gw, subnet, dns);
    }

    if ((SecuritySettings.WifiSSID[0] != 0)  && (strcasecmp(SecuritySettings.WifiSSID, "ssid") != 0))
    {
      for (byte tryConnect = 1; tryConnect <= connectAttempts; tryConnect++)
      {
        #if SERIALDEBUG
          Serial.print("WifiStatus:");
          Serial.println(WiFi.status());
          Serial.print("Wificonnect:");
          Serial.println(tryConnect);
        #endif

        if (tryConnect == 1)
          WiFi.begin(SecuritySettings.WifiSSID, SecuritySettings.WifiKey);
        else
          WiFi.begin();

        for (byte x = 0; x < 20; x++)
        {
          if (WiFi.status() != WL_CONNECTED)
          {
            delay(500);
          }
          else
            break;
        }
        if (WiFi.status() == WL_CONNECTED)
        {
          break;
        }
        else
        {
          #if defined(ESP8266)
            ETS_UART_INTR_DISABLE();
            wifi_station_disconnect();
            ETS_UART_INTR_ENABLE();
          #endif
          delay(1000);
        }
      }
    }
    else
    {
      WifiAPMode(true);
    }
  }
}


//********************************************************************************
// Check if we are still connected to a Wifi AP
//********************************************************************************
void WifiCheck()
{
  String log = "";

  if (WiFi.status() != WL_CONNECTED)
  {
    NC_Count++;
    if (NC_Count > 2)
    {
      WifiConnect(2);
      C_Count=0;
      if (WiFi.status() != WL_CONNECTED)
        WifiAPMode(true);
      NC_Count = 0;
    }
  }
  else
  {
    C_Count++;
    NC_Count = 0;
    if (C_Count > 2) // close AP after timeout if a Wifi connection is established...
    {
      WifiAPMode(false);
    }
  }
}


#if FEATURE_ESPNOW
void espnowSender(const char* kok, const char* key, uint8_t* mac){
#if SERIALDEBUG
  Serial.println("ESP Sender");
  Serial.print("KOK: ");
  Serial.println(kok);
  Serial.print("Key: ");
  Serial.println(key);
  Serial.print("MAC: ");
  for(byte x=0; x <6;x++){
    Serial.print(mac[x],HEX);
    if(x != 5)
      Serial.print("-");
  }
  Serial.println();
#endif
  WiFi.forceSleepWake();
  delay(1);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  byte wifiChannel = 1; 
  wifi_set_macaddr(STATION_IF, mac);
  #if SERIALDEBUG
    Serial.print("This node STA mac: "); Serial.println(WiFi.macAddress());
  #endif
  if (esp_now_init() == 0) {
    #if SERIALDEBUG
      Serial.println("*** ESP_Now Sender init");
    #endif
    esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
    esp_now_set_kok((uint8_t*)kok,16);
    esp_now_register_send_cb([](uint8_t* mac, uint8_t sendStatus) {
      #if SERIALDEBUG
      Serial.printf("send_cb, send done, status = %i\n", sendStatus);
      #endif
    });
  }
}


void espnowSend(String msg){
  struct __attribute__((packed)) SENSOR_DATA {
    char msg[64];
  }sensorData;
  strcpy(sensorData.msg, msg.c_str());  
  uint8_t bs[sizeof(sensorData)];
  memcpy(bs, &sensorData, sizeof(sensorData));
  esp_now_send(NULL, bs, sizeof(sensorData)); // NULL means send to all peers
}


void espnowAddPeer(const char* key, uint8_t* mac, byte role){
#if SERIALDEBUG
  Serial.println("Add Peer");
  Serial.print("Key: ");
  Serial.println(key);
  Serial.print("MAC: ");
  for(byte x=0; x <6;x++){
    Serial.print(mac[x],HEX);
    if(x != 5)
      Serial.print("-");
  }
  Serial.println();
#endif
  byte wifiChannel = 1;
  if(role == 0){ 
    #if SERIALDEBUG
      Serial.println("PEER SLAVE");
    #endif
    esp_now_add_peer(mac, ESP_NOW_ROLE_SLAVE, wifiChannel, (uint8_t*)key, 16);
  }
  else
  {
    #if SERIALDEBUG
      Serial.println("PEER CONTROLLER");
    #endif
    esp_now_add_peer(mac, ESP_NOW_ROLE_CONTROLLER, wifiChannel, (uint8_t*)key, 16);
  }
}


void espnowReceiver(const char* kok, const char* key, uint8_t* mac){
#if SERIALDEBUG
  Serial.println("ESP Receiver");
  Serial.print("KOK: ");
  Serial.println(kok);
  Serial.print("Key: ");
  Serial.println(key);
  Serial.print("MAC: ");
  for(byte x=0; x <6;x++){
    Serial.print(mac[x],HEX);
    if(x != 5)
      Serial.print("-");
  }
  Serial.println();
#endif  

  WiFi.forceSleepWake();
  delay(1);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  byte wifiChannel = 1; 

  wifi_set_macaddr(STATION_IF, mac);
  #if SERIALDEBUG
    Serial.print("This node STA mac: "); Serial.println(WiFi.macAddress());
  #endif
    if (esp_now_init()==0) {
      #if SERIALDEBUG
        Serial.println("*** ESP_Now receiver init");
      #endif
      esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
      esp_now_set_kok((uint8_t*)kok,16);
      esp_now_register_recv_cb([](uint8_t *mac, uint8_t *data, uint8_t len) {
      struct __attribute__((packed)) SENSOR_DATA {
        char msg[64];
      }sensorData;
        if(esp_now_is_peer_exist(mac)){
          String eventString = F("Event ESPNOW#");
          eventString += mac[5];
          eventString += "=";
          //Serial.write(mac, 6); // mac address of remote ESP-Now device
          //Serial.write(len);
          memcpy(&sensorData, data, sizeof(sensorData));
          //Serial.write(data, len);
          eventString += sensorData.msg;
          Serial.println(eventString);
        }
        else
        {
          #if SERIALDEBUG
            Serial.print("Unknown MAC: ");
            for(byte x=0; x <6;x++){
              Serial.print(mac[x],HEX);
              if(x != 5)
                Serial.print("-");
            }
            Serial.println();
          #endif
        }
        
      });
    }
}
#endif
