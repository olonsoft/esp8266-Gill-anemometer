## Anemometer for Gill instruments

This project connects Gill instrumets anemometer to an mqtt server to send wind speed and wind direction data.  
It uses Software Serial to retreive the commands sent by the instrument.

You can control how ofter the data will be sent to mqtt broker.  

### The pinout of ESP8266 is:  
|GPIO PIN|Connection|
|--------|----|
|GPIO14 D5| <- TTL input (from rs232)
|GPIO12 D6 |-> TTL output (to RS232)
|GPIO5  D1 |SCL
|GPIO4  D2 |SDA
|GPIO0  D3 |button
|GPIO13 D7 |mosfet on/off (there is a mosfet acting as a switch to turn on/off anemometer instrument)

The commands you can send to ESP8266 topic `devicename/cmd` are:  

- status
- restart
- reset
- update
- broker userid:pass@url:port
- interval 120
- format