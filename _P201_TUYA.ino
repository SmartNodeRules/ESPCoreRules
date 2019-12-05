//#######################################################################################################
//#################################### Plugin 201: TUYA #################################################
//#######################################################################################################

/*
 * Commands:
 * tuyaCheck                                            Checks tuya device and creates events
 * tuyaSend <cmd>,<len>,<value>                         Send a serial message to the 'tuya' MCU as used on LSC Smart Connect PIR and Doorsensors
 * tuyaCheckProbe <ssid>,<channel>,<dev-id>,<dev-type>  Checks tuya device and uses Wifi probe 'hack' to send state
 * 
 * Still debugging. Official documentation does not seem to match with commands used for PIR/Doorsensor
 *   tuyasend 1,0,0	Get device info from MCU
 *   tuyasend 2,1,0     fast blink LED
 *   tuyasend 2,1,1     start AP/config mode, power stays on, slow blink LED
 *   tuyasend 2,1,2     LED blink off
 *   tuyasend 2,1,3     ??
 *   tuyasend 2,1,4     Get device status from MCU
*/

#ifdef USES_P201
#define P201_BUILD            6
#define PLUGIN_201
#define PLUGIN_ID_201         201

boolean Plugin_201(byte function, String& cmd, String& params)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_INFO:
      {
        printWebTools += F("<TR><TD>P201 - TuyaSend<TD>");
        printWebTools += P201_BUILD;
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
        if (cmd.equalsIgnoreCase(F("tuyaCheck")))
        {
          success = true;
          Serial.begin(9600);
          int8_t status1 = -1;
          int8_t status2 = -1;
          int8_t status3 = -1;
          int8_t status4 = -1;
          status1 = P201_tuyaSend(1,0,0);
          status2 = P201_tuyaSend(2,1,2);
          status3 = P201_tuyaSend(2,1,3);
          if(status2 < 64){ // only continue when the device was not started using the device 'connect' button
            status4 = P201_tuyaSend(2,1,4);
            if(status4 & 2){
              #if FEATURE_RULES
                String eventString = F("TUYA#Event=");
                eventString += ((status4 & 4) >> 2);
                rulesProcessing(FILE_RULES, eventString);
              #endif
            }
            if(status4 & 32){
              #if FEATURE_RULES
                String eventString = F("TUYA#BatteryLow=1");
                rulesProcessing(FILE_RULES, eventString);
              #endif
            }
          }
          #if FEATURE_RULES
            String eventString = F("TUYA#Debug=");
            eventString += status1;
            eventString += ",";
            eventString += status2;
            eventString += ",";
            eventString += status3;
            eventString += ",";
            eventString += status4;
            eventString += ",";
            eventString += millis();
            rulesProcessing(FILE_RULES, eventString);
          #endif
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

  // wait max 500 mSec for reply
  for (int x = 0; x < 500; x++){
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
  
  byte count = 0;       // count number of bytes received within a message
  byte msgType = 0;     // stores message type (byte 4)
  byte length = 0;      // length of entire message as provided by MCU
  byte devState = 0;    // subtype ?
  byte event = 255;     // event state of device, like door open/close
  byte status = 0;      // return value
    //bit 0   0 = no reply (all other bits should be zero too), 1 = reply received
    //bit 1   0 = no event, 1 = event (like open/close, pir move)
    //bit 2   event state, 0 = door open or PIR detect, 1 = door closed
    //bit 3   reserved
    //bit 4   reserved
    //bit 5   battery LOW, not in use, need to investigate protocol
    //bit 6   device button pressed to start device
    //bit 7   not used
     
    // MCU replies
    // 55 aa 0 2 0 0 1  basic confirm
    // 55 aa 0 3 0 0 2  this is received only when the sensor was activated using 5 seconds button press, additional reply after tuyasend 2,1,2

    // Status messages from door sensor:
    // 55 aa 0 5 0 5 1 1 0 1 1 d  door open
    // 55 aa 0 5 0 5 1 1 0 1 0 c  door closed
    // 55 aa 0 5 0 5 3 4 0 1 2 13 battery OK
    // 55 aa 0 5 0 5 3 4 0 1 0 11 battery low (around 2.5 Volts, DoorSensor keeps working until 2.3 Volts)

    // 55 aa 0 5 0 5 1 4 0 1 0 f  PIR
  
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
        devState = data;
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
      
      // We have a complete message, start calculating return value
      
      status |= 1; //set bit 1 to confirm valid response
      
      if(msgType == 3){
        status |= 64; // set bit 6, sensor button pressed for 5 seconds
      }

      if(msgType == 5 && devState == 1){ // msg type 5 is a status message, devState 1 indicates the open/close state
        status |= 2; // set bit 2, event received
        if(event)
          status |= 4; //set event state bit 3
      }

      if(msgType == 5 && devState == 3){ // msg type 5 is a status message, devState 3 battery info
        if(event == 0)
          status |= 32; //set battery low bit 5
      }

               
      // add to logging string
      if(devState != 0){
        reply += " devState:";
        reply += devState;
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
//    syslog(reply);
      reply = "";
      count = 0;
      length = 0;
      msgType = 0;
      devState = 0;
    }
    if(!Serial.available())
      delay(2); // wait for next serial char, timing based on 9600 baud
  }

  return status;

}
#endif

