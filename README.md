# Comfo-LAN
Device to exchange information from Comfo to LAN.\
This gives you the possibility to read data from the Comfo ventilation and to set parameters, e.g. the ventilation level.\
Connecting an air quality sensor allows to reduce the ventilation level in case of bad air quality (e.g. wood heating in the neighborhood) until the air gets better.

Tested ventilation systems:
* Zehnder ComfoD 350

<a rel="license" href="http://creativecommons.org/licenses/by-nc-sa/4.0/"><img alt="Creative Commons License" style="border-width:0" src="https://i.creativecommons.org/l/by-nc-sa/4.0/88x31.png" /></a><br />This work is licensed under a <a rel="license" href="http://creativecommons.org/licenses/by-nc-sa/4.0/">Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License</a>.

## Installation

* Use a Arduino Mega or Due (~35€ - for communication with a Comfo ventilation, a serial interface is needed. Only Arduino Mega and Due have two serial interfaces that allow debugging and communication to the ventilation the same time.)\
Connect to Comfo ventilation using the Serial1 of the Arduino (Pins 18 TX / 19 RX for Mega) according to the following description in page 2 and 3: http://www.see-solutions.de/sonstiges/sonstiges.htm#Zehnder
* Use a air quality sensor (e.g. AMS IAQ-CORE C ~15€)\
Connect the air quality sensor with the I2C interface of the Arduino (Pins 20/21 for Mega) and put the sensor in the air supply tube between the ventilation and the rooms
* Use an ethernet interface to connect to LAN (e.g. ENC28J60 ~6€)\
Connect the ethernet interface with the Arduino (Pins 50/51/52/53 for Mega)

## Software

The following settings have to be adjusted in the config.h:
* Set an appropriate IP-address
* Adjust the MAC-address if needed to avoid having a duplicate MAC-address in the network
* Optionally define a passkey
* If no air quality sensor is connected, comment out line 62 ("#define AirQualityCheck")
* If MQTT is not used, comment out line 51 ("#define MQTTBrokerIP"), otherwise adjust the MQTT-parameters

After adapting the parameters, rename the file 'Config.h.default' to 'Config.h'.

Finally load the program to the Arduino.

## Contributions

* Gero Schumacher (gero.schumacher@gmail.com) (basic implementation based on BSB LAN https://github.com/fredlcore/bsb_lan)
* Frederik Holst (bsb@code-it.de) (basic implementation based on BSB LAN https://github.com/fredlcore/bsb_lan)
* Protocol implementation based on http://www.see-solutions.de/sonstiges/sonstiges.htm#Zehnder
* Air quality sensor implementation based on https://forum.arduino.cc/index.php?topic=350712.0
