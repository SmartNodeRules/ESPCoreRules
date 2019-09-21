//#######################################################################################################
//#################################### Plugin 201: TUYA #################################################
//#######################################################################################################

/*
 * Commands:
 * tuyaSend <cmd>,<len>,<value>                  Send a serial message to the 'tuya' MCU as used on LSC Smart Connect PIR and Doorsensors
 *
 * Still debugging. Official documentation does not seem to match with commands used for PIR/Doorsensor
 *   tuyasend 1,0,0	Get device info from MCU
 *   tuyasend 2,1,0     fast blink LED
 *   tuyasend 2,1,1     start AP/config mode, power stays on, slow blink LED
 *   tuyasend 2,1,2     LED blink off
 *   tuyasend 2,1,3     ??
 *   tuyasend 2,1,4     Get device status from MCU
*/

#define PLUGIN_201
#define PLUGIN_ID_201         201

boolean Plugin_201(byte function, String& cmd, String& params)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_INFO:
      {
        printWebTools += F("<TR><TD><TD>P201 - TuyaSend");
        break;
      }
          
    case PLUGIN_WRITE:
      {
        if (cmd.equalsIgnoreCase(F("tuyaSend")))
        {
          success = true;
          byte cmd = parseString(params,1).toInt();
          byte len = parseString(params,2).toInt();
          byte val = parseString(params,3).toInt();
          P201_tuyaSend(cmd,len,val);
        }
        break;
      }
  }
  return success;
}


byte P201_tuyaSend(byte cmd, byte len, byte value)
{
  while (Serial.available())
    Serial.read();

  Serial.write(0x55);
  Serial.write(0xAA);
  Serial.write(0x00);
  Serial.write(cmd);
  Serial.write(0x00);
  Serial.write(len);
  if(len == 1)
    Serial.write(value);
  byte cs = 0xff + cmd + len + value;
  Serial.write(cs);
  Serial.flush();

  // wait max 1000 mSec for reply
  for (int x = 0; x < 1000; x++){
    if (Serial.available())
      break;
    delay(1);
  }

  // prepare for logging
  String reply = "";
  reply += cmd;
  reply += "-";
  reply += value;
  reply += ":";
  byte count = 0;
  byte msgType = 0;
  byte length = 0;
  byte status = 0;
  byte statusCount = 0;
  byte event = 255;

  // Status messages from door sensor:
  // 55 aa 0 5 0 5 1 1 0 1 1 d  door closed
  // 55 aa 0 5 0 5 1 1 0 1 0 c  door open
  // 55 aa 0 5 0 5 3 4 0 1 2 13 additional message when batteries inserted
  // 55 aa 0 5 0 5 3 4 0 1 0 11 additional message when batteries inserted

  // process reply
  while (Serial.available()){
    count++;
    byte data = Serial.read();

    if(count == 4) // byte 4 contains message type
      msgType = data;
    if(count == 6) // byte 6 contains data length
      length = data;
    if(msgType == 5){ // msg type 5 is a status message
      if (count == 7){ // byte 7 contains status indicator
        status = data;
        statusCount++;
      }
      if(count == 11) // byte 11 status indicator value
        event = data;
    }

    // add to logging, msg type 1 is readable text containing device info in json format
    if(msgType == 1){
      if(count > 6 && data != 0){
        reply += (char)data;
      }
    }
    else{ // other msg types are binary, so display as HEX
      reply += String(data, HEX);
      reply += " ";
    }

    // Check if message is complete, based on msg length (header + checksum = 7 bytes)
    if(count == length + 7){

      #if FEATURE_RULES
        if(msgType == 5 && status == 1){ // msg type 5 is a status message, status 1 indicates the open/close state
          String eventString = F("TUYA#Event=");
          eventString += event;
          rulesProcessing(FILE_RULES, eventString);
        }
      #endif
               
      // add to logging string
      if(status != 0){
        reply += " status:";
        reply += status;
      }
      reply += " msg:";
      reply += msgType;
      reply += " length:";
      reply += length;
      if(event != 255){
        reply += " event:";
        reply += event;
      }
      
      // report logging and reset counters, status bytes, etc..
      syslog(reply);
      reply = "";
      count = 0;
      length = 0;
      msgType = 0;
      status = 0;
    }
    delay(2); // wait for next serial char, timing based on 9600 baud
  }

  if(statusCount > 1){
    status = 99;
  }
  return status;
}

