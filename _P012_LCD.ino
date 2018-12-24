//#######################################################################################################
//#################################### Plugin 012: LCD ##################################################
//#######################################################################################################

#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C *lcd;

#define PLUGIN_012
#define PLUGIN_ID_012         12

boolean Plugin_012(byte function, String& cmd, String& params)
{
  boolean success = false;

  switch (function)
  {

    case PLUGIN_INIT:
      {
        if (!lcd)
        {
          byte row = 2;
          byte col = 16;
          lcd = new LiquidCrystal_I2C(0x27, col, row);
        }
        // Setup LCD display
        lcd->init();                      // initialize the lcd
        lcd->backlight();
        lcd->print("ESP Easy");
        success = true;
        break;
      }

    case PLUGIN_WRITE:
      {
        if (cmd.equalsIgnoreCase(F("LCD")))
        {
          success = true;
          byte row = parseString(params,1).toInt();
          byte col = parseString(params,2).toInt();
          String msg = parseString(params,3);
          lcd->setCursor(col-1, row-1);
          lcd->print(msg.c_str());
        }
        if (cmd.equalsIgnoreCase(F("LCDCMD")))
        {
          success = true;
          String subcmd = parseString(params,1);
          if (subcmd.equalsIgnoreCase(F("Off")))
            lcd->noBacklight();
          else if (subcmd.equalsIgnoreCase(F("On")))
            lcd->backlight();
          else if (subcmd.equalsIgnoreCase(F("Clear")))
            lcd->clear();
        }
        break;
      }
  }
  return success;
}
