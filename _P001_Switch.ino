//#######################################################################################################
//#################################### Plugin 001: Input Switch #########################################
//#######################################################################################################

#define PLUGIN_001
#define PLUGIN_ID_001         1

boolean Plugin_001(byte function, String& cmd, String& params)
{
  boolean success = false;

  static int8_t PinMonitor[PIN_D_MAX];
  static int8_t PinMonitorState[PIN_D_MAX];

  switch (function)
  {
    case PLUGIN_WRITE:
      {
        if (cmd.equalsIgnoreCase(F("gpio")))
        {
          success = true;
          byte pin = parseString(params,1).toInt();
          byte state = parseString(params,2).toInt();
          if (pin >= 0 && pin <= 16)
          {
            pinMode(pin, OUTPUT);
            digitalWrite(pin, state);
          }
        }
        
        if (cmd.equalsIgnoreCase(F("gpioRead")))
        {
          success = true;
          String varName = parseString(params,1);
          byte pin = parseString(params,2).toInt();
          pinMode(pin, INPUT);
          byte state = digitalRead(pin);
          setNvar(varName, state);
        }

        if (cmd.equalsIgnoreCase(F("gpioState")))
        {
          success = true;
          String varName = parseString(params,1);
          byte pin = parseString(params,2).toInt();
          byte state = digitalRead(pin);
          setNvar(varName, state);
        }
        
        if (cmd.equalsIgnoreCase(F("monitor")))
        {
          String param1 = parseString(params, 1);
          if (param1.equalsIgnoreCase(F("gpio")))
          {
            byte pin = parseString(params,2).toInt();
            pinMode(pin,INPUT_PULLUP);
            PinMonitor[pin] = 1;
            success = true;
          }
        }
        
        break;
      }
      
    case PLUGIN_UNCONDITIONAL_POLL:
      {
        // port monitoring, on request by rule command
        for (byte x=0; x < PIN_D_MAX; x++)
           if (PinMonitor[x] != 0){
             byte state = digitalRead(x);
             if (PinMonitorState[x] != state){
               String eventString = F("GPIO#");
               eventString += x;
               eventString += F("=");
               eventString += state;
               rulesProcessing(FILE_RULES, eventString);
               PinMonitorState[x] = state;
             }
           }
        break;
      }
     
  }
  return success;
}
