/********************************************************************************************\
Telnet
\*********************************************************************************************/
void telnet() {
  byte telnetByte = 0;

  if (ser2netServer->hasClient())
  {
    if (ser2netClient) ser2netClient.stop();
    ser2netClient = ser2netServer->available();
  }

  if (ser2netClient.connected())
  {
    while (ser2netClient.available()) {
      telnetByte = ser2netClient.read();

      if (isprint(telnetByte))
      {
        if (TelnetByteCounter < INPUT_BUFFER_SIZE)
          TelnetBuffer[TelnetByteCounter++] = telnetByte;
      }
      if (telnetByte == '\n') {
        TelnetBuffer[TelnetByteCounter] = 0;
        if(Settings.SerialTelnet)
          if(TelnetBuffer[0] != ':')
            Serial.println(TelnetBuffer);
          else
            ExecuteCommand(TelnetBuffer + 1);
        else
          ExecuteCommand(TelnetBuffer);
        TelnetByteCounter = 0;
        TelnetBuffer[0] = 0;
      }
    }
  }
  else
  {
    if (connectionState == 1) // there was a client connected before...
    {
      connectionState = 0;
      // workaround see: https://github.com/esp8266/Arduino/issues/4497#issuecomment-373023864
      ser2netClient = WiFiClient();
    }
  }
}


/********************************************************************************************\
Telnet log
\*********************************************************************************************/
void telnetLog(String s)
{
  if(Settings.SerialTelnet) return;
  
  if (ser2netClient.connected()) {
    ser2netClient.println(s);
    ser2netClient.flush();
  }
}


/********************************************************************************************\
Telnet log flashstring
\*********************************************************************************************/
void telnetLog(const __FlashStringHelper* flashString)
{
  if(Settings.SerialTelnet) return;

  if (ser2netClient.connected()) {
    String s(flashString);
    ser2netClient.println(s);
    ser2netClient.flush();
  }
}

