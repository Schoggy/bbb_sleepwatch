# bbb_sleepwatch

A project to build a small system logging various parameters while you sleep!

This is a project within the scope of the course "Embedded Linux" at the Univerity of Applied Science Augsburg.

Currently it's planned to log the following values:

 - room temperature
 - humidity of the room
 - noise level
 - air quality
 - light level

**Hardware:**

 - Beaglebone Black
 - AM2302 temperature and humidity sensor
 - MQ2 Gas sensor
 - some cheap microphone bought on Amazon (has this string on it: CZN-15E)
 - GL5528 light sensitive resistor
 
**Software:**

 - Currently working with the Debian image provided by beaglebone.org (I uninstalled X though)
 - gcc & gdb
 - clang-format
 - cppcheck, valgrind, flawfinder
 - sqlite3
 - lighttpd
 - some code written by Tony DiCola for Adafruit (https://github.com/adafruit/Adafruit_Python_DHT)
   to read the AM2302

**The idea:**

Log all these parameters constantly (depending on the sensor) writing the averages to the sqlite3 database every 10 or so seconds.
Supply a web interface to view charts of the gathered data, see trends or spikes (like a loud noise, a flash of light, etc.).
This will help understand problems with the sleeping environment and help optimize it.

**Progress/Milestones:**

X = done, F = relatively tested - kind of stable code, B = testing/debugging, -> = wip, - = todo, + = optional/possible feature

 - X  aquire hardware
 - X  connect hardware to BBB
 - X  get adc working
 - X  read from AM2302
 - X  database design
 - F  interface for the database
 - B  reading & buffering data from the sensors
 - B  "scheduler" and threads
 - -> module to analyze the data
 - -  create style for the website
 - -  web interface
 - -  lots of testing & debugging
 - +  alarm module
 - +  display
 - +  wifi module
 - +  case
