# LED METAR Map 
  ![C++](https://img.shields.io/badge/c++-%2300599C.svg?style=for-the-badge&logo=c%2B%2B&logoColor=white)
  ![Arduino](https://img.shields.io/badge/-Arduino-00979D?style=for-the-badge&logo=Arduino&logoColor=white)
  ![Homebridge](https://img.shields.io/badge/homebridge-%23491F59.svg?style=for-the-badge&logo=homebridge&logoColor=white)
  ![Mosquitto](https://img.shields.io/badge/mosquitto-%233C5280.svg?style=for-the-badge&logo=eclipsemosquitto&logoColor=white)

  ## Description 
  This project is for my LED METAR map that I used to check the weather at nearby airports before I go flying. For those who don't know, METAR stands for METeorological Aerodrome Report and tells pilots what the weather is at certain airports with reporting stations. Depending on the color of a circle over the airport, that shows the flight conditions at a glance:
  
  * Green - VFR (Visual Flight Rules)
  * Blue - MVFR (Marginal Visual Flight Rules)
  * Red - IFR (Instrument Flight Rules)
  * Purple - LIFR (Low Instrument Flight Rules)

  I also added a feature to flicker an LED white if there is lightning detected at that airport, and to slowly pulse an LED on and off if winds exceed a set threshold. Lastly, I implemented MQTT and [Homebridge](https://github.com/homebridge/homebridge) to allow for HomeKit control of the brightness from my iPhone, mainly just so I can have the map shut off at night or when I am not home. The project runs off of the ESP8266 and uses WS2812b LEDS to display the status.
  

  ## Table of Contents
  * [Hardware](#hardware)

  ## Hardware 
  This project relies on the ESP8266 to fetch the weather data from [AWC](https://www.aviationweather.gov), do some regex, and output to the string of WS2812b LEDS. The hardware is not that complicated in this project, but lining up the LEDs with the airport marker is the difficult part.
  
  README is in progress!
  
 
