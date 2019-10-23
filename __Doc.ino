/* List of core commands
Config,AutoConnect,<0|1>                    Enable/Disable Auto connect as Wifi client during boot
Config,AutoAPOnFailure,<0|1>                Enable/Disable Auto launch AP on Wifi client connect failure
Config,DST,<0|1>                            Enable/Disable DST
Config,Baudrate,<speed>                     Enable/Disable Serial and speed (speed 0=disable)
Config,LogEvents,<0|1>                      Enable/Disable logging msbus events on telnet session
Config,MSGBUS,<UDP|MQTT>,<0|1>              Select protocol to use for the messagebus
Config,Name                                 Set the device name
Config,Network,<ip>,<mask>,<gateway>,<DNS>  Set static IP config
Config,Time,<0|1>                           Enable/Disable NTP time service client
Config,Port,<portnr>                        MessageBus UPD port number
Config,Rules,Clock,<0|1>                    Enable/Disable rules processing on clock events (each minute)
Config,Rules,Serial,<0|1>                   Enable/Disable rules processing on serial input
Config,SendARP,<0|1>                        Enable/Disable grat ARP broadcast
Config,Timezone,<offset>                    Timezone offset in minutes (can also be negative)
Config,WifiSSID,<ssid>                      Set the Wifi SSID
Config,WifiKey,<key>                        Set the Wifi WPA key
Config,WifiAPKey,<key>                      Set the Wifi AP key
DeepSleep <seconds)                         Turn off to deepsleep mode for <seconds> (max 4294 seconds). Only RTC will be active. Connect GPIO16 to Reset pin
Delay <milliseconds>                        Stop processing for <milliseconds> (blocking command!, use only for small delays)
ESPNowConfig <kok>,<key>,<mac>,<mode>       Start ESPNOW sender/receiver (do not use Wifi at the same time)
ESPNowAddPeer <key>,<mac>,<role>            Add an ESPNOW peer with encryption key and mac address
ESPNowSend <msg>                            Send a message using ESPNOW protocol to peered receiver
Event <event>                               Generate event
MSGBus <msg>                                Send a message on the messagebus
Reboot                                      Reboot device
SerialFloat                                 Makes the serial pins float
Serial                                      Enables serial (after using float)
SerialSend <string>                         Sends a text to serial port
SerialTelnet <0|1>                          Enable/Disable Serial/Telnet bridge
SendToUDP <ip>,<port>,<msg>                 Sends a text string to give IP and port on UDP protocol
Settings                                    Outputs some basic settings to serial port
StringSet <name>,<text>                     Set a variable by name to string value
Syslog <string>                             Sends a text string to syslog using broadcast
TimerSet <name>,<seconds>                   Set a timer by name to x seconds countdown
ValueSet <name>,<value>                     Set a variable by name to float value
VCCRead <variable>                          Read VCC voltage (when feature for ADC to read VCC is enabled)
WebPrint <html>                             Print html to the main webpage (no pararameter clears the buffer)
WebButton <class>;<href>;<caption>          Adds a button to the main webpage

Plugins commands are explained in the plugin sections

System variables, to be used in rules:
%sysname%                                  System name that has been configured
%systime%                                  Time in HH:MM
%millis%                                   Uptime in milliseconds, returns internal millis() counter, overflows in 49 days
*/

