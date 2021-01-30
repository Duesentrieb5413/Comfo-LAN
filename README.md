# Comfo-LAN
Device to exchange information from Comfo to LAN
This gives you the possibility to read data from the Comfo ventilation and to set parameters, e.g. the ventilation level.
Connecting an air quality sensor allows to reduce the ventilation level in case of bad air quality (e.g. wood heating in the neighborhood) until the air gets better.

## Installation

* Use a Arduino Mega or Due (~35€ - for communication with a Comfo ventilation, a serial interface is needed. Only Arduino Mega and Due have two serial interfaces that allow debugging and communication to the ventilation the same time.)  - connect to Comfo ventilation using the Serial1 of the Arduino (Pins 18 TX / 19 RX for Mega) according to the following description in page 2 and 3: http://www.see-solutions.de/sonstiges/sonstiges.htm#Zehnder
* Use a air quality sensor (e.g. AMS IAQ-CORE C ~15€) - connect the air quality sensor with the I2C interface of the Arduino (Pins 20/21 for Mega)
* Use an ethernet interface to connect to LAN (e.g. ENC28J60 ~6€) - connect the ethernet interface with the Arduino (Pins 50/51/52/53 for Mega)

## Software

The following settings have to be adjusted in the config.h:
* Set an appropriate IP-address
* Adjust the MAC-address if needed to avoid having a duplicate MAC-address in the network
* Optionally define a passkey
* If no air quality sensor is connected, comment out line 62 ("#define AirQualityCheck")
* If MQTT is not used, comment out line 51 ("#define MQTTBrokerIP"), otherwise adjust the MQTT-parameters

After adapting the parameters, rename the file 'Config.h.default' to 'Config.h'.

Finally upload the program to the Arduino.
