# DHTlib
#### C library for reading the family of DHT sensors

### About

DHTlib is a C library that can be used to read the DHT temperature and humidity sensors on Single Board Computers running Linux 
(e.g. FoxG20, AriettaG25, RaspberryPi).  

### Features:

- Support for DHT11 and DHT22/AM2302/RHT03 sensors
- Auto detect sensor model
- Two communication modes: GPIO and SPI
- Provided as C library to be included in your own project
- Example code for library usage provided  

### Credits:

The implementation of the GPIO communication mode is based on code for Arduino from Mark Ruys, mark@paracas.nl  
http://www.github.com/markruys/arduino-DHT

The implementation of the SPI communication mode is based on the work from Daniel Perron posted on the RaspberryPi Forum.  
http://www.raspberrypi.org/forums/viewtopic.php?p=506650#p506650

### Installation:
  
* Get source code from Github:
<pre>
  git clone https://github.com/ondrej1024/foxg20
</pre>

* Build library:
<pre>
  cd dhtlib
  make
</pre>

* Install library:
<pre>
  make install
</pre>

### Usage:

  You find an example application using the library in example/dhtsensor.c


### Known issues:

This library runs in user space and when using GPIO as communication bus with the DHT sensor, the time critical detection of the sensors response pulses (with length of around 50us) is not very reliable. In some occasions the sensor reading will fail (timeout or checksum error). As a solution the reading needs to be repeated until it succeeds.  
When using SPI as communication bus, this is not an issues. The SPI mode is very robust and should be the preferred solution.
