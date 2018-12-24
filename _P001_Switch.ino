//#######################################################################################################
//#################################### Plugin 001: Input Switch #########################################
//#######################################################################################################

#define PLUGIN_001
#define PLUGIN_ID_001         1

boolean Plugin_001(byte function, String& cmd, String& params)
{
  boolean success = false;
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

        break;
      }
  }
  return success;
}
