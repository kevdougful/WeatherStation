WeatherStation
==============

My home weather station

This code runs my DIY home weather station.  This gives me up-to-date weather data for my immediate area.

I plan to gather and log many data points:
  1.  Temperature
  2.  Relative humidity
  3.  Barometric pressure
  4.  Luminosity
  5.  UV
  6.  Wind speed & direction
      a.  Instantaneous
      b.  2 minute average
      c.  Daily average
  7.  Rain
      a.  Last hour
      b.  Daily total
      c.  YTD total
      
The weather station will utilize an Adafruit CC3000 WiFi shield to communicate with a Rasperry Pi web server.
It will also periodically query an NTP time server to ensure that an RTC module has the correct time.
In between time synchronizations, a timestamp will be immediately attached to all measurements by the RTC module.
This will ensure all measurements have an accurate timestamp regardless of the weather station's power status or connectivity.

Power will be provided to the weather station via a solar-charged 3.7V LiPo cell.


Directories in this repository:

Weather
  Arduino code that runs immediately at the data collection source.
  This code is meant to be light weight and fast.
  This will eventually employ the Arduino's watchdog timer and interrupts to manage power consumption
  
BaseStation
  This will be the code that runs on the Raspberry Pi server.
  It will parse and validate all the data from the weather station Arduino.
  It will also upload to the Wunderground API.
