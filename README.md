# bbb_sleepwatch

A project to build a small system logging various parameters while you sleep!

Currently it's planned to log the following values:

 - room temperature
 - humidity of the room
 - noise level
 - air quality
 - light level

Hardware:

 - Beaglebone Black
 - AM2302 temperature and humidity sensor
 - MQ2 Gas sensor
 - some cheap microphone bought on Amazon (has this string on it: CZN-15E)
 - GL5528 light sensitive resistor
 
Software:

 - Currently working with the Debian image provided by beaglebone.org (I uninstalled X though)
 - gcc & gdb
 - clang-format
 - cppcheck, valgrind, flawfinder
 - sqlite3
 - lighttpd
 - some code written by Tony DiCola for Adafruit (https://github.com/adafruit/Adafruit_Python_DHT)
   to read the AM2302

The idea:

Log all these parameters constantly (depending on the sensor) writing the averages to the sqlite3 database every 10 or so seconds.
Supply a web interface to view charts of the gathered data, see trends or spikes (like a loud noise, a flash of light, etc.).
This will help understand problems with the sleeping environment and help optimize it.

Progress/Milestones:

x = done, f = relatively tested/kind of stable code, b = testing/debugging,
-> = wip, - = todo, + = optional/possible feature

 x  aquire hardware
 x  connect hardware to BBB
 x  get adc working
 x  read from AM2302
 x  database design
 f interface for the database
 b  reading & buffering data from the sensors
 -> "scheduler"
 -  module to analyze the code
 -  create style for the website
 -  web interface
 -  lots of testing & debugging
 +  alarm module
 +  display
 +  wifi module
 +  case
