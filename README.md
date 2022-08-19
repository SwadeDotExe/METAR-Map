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
  * [Building](#building)

  ## Hardware 
  This project relies on the ESP8266 to fetch the weather data from [AWC](https://www.aviationweather.gov), do some regex, and output to the string of WS2812b LEDS. The hardware is not that complicated in this project, but lining up the LEDs with the airport marker is the difficult part. In my map, I used 14 LEDs, because that is simply how many airports report weather around me. Other than that, no additional hardware is needed!
  
  ## Building
  Now here is where the fun begins. To start off, I needed to choose a picture frame that had a glass covering but also was fairly deep to house the electronics behind the frame. I found the frames from Walmart to be exactly what I needed, and I ended up buying [this one](https://www.walmart.com/ip/Mainstays-11x15-Front-Loading-Picture-Frames-Black/381283143). Now for the actual map that is going to be put in the frame, I happened to have a bunch laying around that were left over from flight school. If you don't have one, expired maps can easily be obtained from a local flight school for free. 
  
  After obtaining an areonautical chart for the area you want to frame, simply use the cardboard backing of the picture frame to trace a square on the map and cut it out. I then repeated the process with some scrap paper, which will be used to mark where to drill holes in the cardboard without cutting into the chart. After the chart and paper are both cut to size, I overlayed the paper above the chart so I could see the airports under the paper. I then drew circles on the paper to mark where the airports are, being careful to not move anything in the process. 
  
  Here is a picture of the map with the paper covering: ![alt text](https://github.com/SwadeDotExe/METAR-Map/blob/main/Images/MapwithPaper.jpg "Map With Paper")
  
  After having the paper with the airports marked, I layed it in the picture frame over the cardboard backing and drilled holes wherever I had a mark. This will allow the LEDs to look like they are embedded in the map without having to cut holes in the chart itself. The next step is putting the LEDs wherever there is a hole, and this is very tedious since I had to measure, cut, and solder every one.
  
  Here is a picture of what the back of the frame looks like after this step: ![alt text](https://github.com/SwadeDotExe/METAR-Map/blob/main/Images/Back.jpg "Back of Map")
  
  Now we are in the home stretch! Put everything back together, making sure the cardboard lines up with the paper map on top and put the glass back on. If everything is wired correctly, the LEDs should light up and run through a color test. If you did this project on a rainy day like I did, you should see some neat colors already: 
  
  <img src="https://github.com/SwadeDotExe/METAR-Map/blob/main/Images/Final.jpg" width=50% height=50%>
