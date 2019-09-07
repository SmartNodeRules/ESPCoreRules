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
  WiFi.softAP(ap_ssid, SecuritySettings.WifiAPKey);
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

