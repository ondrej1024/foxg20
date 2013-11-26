
Description:

  DHT Temperature & Humidity Sensor library for use on 
  FoxG20 embedded Linux board (by ACME Systems).

  Author: Ondrej Wisniewski
  
  Features:
  - Support for DHT11 and DHT22/AM2302/RHT03
  - Auto detect sensor model

  Based on code for Arduino from Mark Ruys, mark@paracas.nl
  http://www.github.com/markruys/arduino-DHT

 
Installation:
  
  Get source code form Github:
  git clone ...

  Build library:
  cd dhtlib
  make

  Install library:
  make install


Usage:

  You find an example application using the library in example/dhtsensor.c


Known issues:

  This library runs in user space and uses GPIO sysfs to access the IO pin 
  used for communication with the DHT sensor. Since we need to detect pulse
  length of around 50us there are timing issues with this approach which means
  in some occasions the sensor reading will fail (timeout or checksum error).
  However it is enough to retry the read operation which will most likely 
  succeed this time.
  A proper solution to this problem would be to write a Kernel driver, like
  for the DS28 1-wire temperature sensor.

