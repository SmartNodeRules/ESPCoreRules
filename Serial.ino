/********************************************************************************************\
 Get data from Serial Interface
\*********************************************************************************************/
void serial()
{
  while (Serial.available())
  {
    delay(0);
    SerialInByte = Serial.read();
    if (SerialInByte == 255) // binary data...
    {
      Serial.flush();
      return;
    }

    if (isprint(SerialInByte))
    {
      if (SerialInByteCounter < INPUT_BUFFER_SIZE) // add char to string if it still fits
        InputBuffer_Serial[SerialInByteCounter++] = SerialInByte;
    }

    if (SerialInByte == '\n')
    {
      InputBuffer_Serial[SerialInByteCounter] = 0; // serial data completed
      //Serial.write('>');
      //Serial.println(InputBuffer_Serial);
      if(Settings.SerialTelnet && ser2netClient.connected())
      {
        ser2netClient.write((const uint8_t*)InputBuffer_Serial, strlen(InputBuffer_Serial));
        ser2netClient.write(10);
        ser2netClient.write(13);
        ser2netClient.flush();
      }

      String action = InputBuffer_Serial;

      if (Settings.RulesSerial){
        String event = "";
        if(action.substring(0,3) == F("20;")){
          action = action.substring(6); // RFLink, strip "20;xx;" from incoming message
          event = "RFLink#" + action;
        }
        else
          event = "Serial#" + action;
        rulesProcessing(FILE_RULES, event);
      }

      PluginCall(PLUGIN_SERIAL_IN, action, dummyString);
      ExecuteCommand(InputBuffer_Serial);
      SerialInByteCounter = 0;
      InputBuffer_Serial[0] = 0; // serial data processed, clear buffer
    }
  }
}

