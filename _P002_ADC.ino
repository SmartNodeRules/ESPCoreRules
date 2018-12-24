//#######################################################################################################
//#################################### Plugin 002: Analog ###############################################
//#######################################################################################################

#define PLUGIN_002
#define PLUGIN_ID_002         2

boolean Plugin_002(byte function, String& cmd, String& params)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_WRITE:
      {
        if (cmd.equalsIgnoreCase(F("AnalogRead")))
        {
          success = true;
          String varName = parseString(params,1);
          int value = analogRead(A0);
          setNvar(varName, value);
        }
        break;
      }
  }
  return success;
}
