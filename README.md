# LED METAR Map 
  ![C++](https://img.shields.io/badge/c++-%2300599C.svg?style=for-the-badge&logo=c%2B%2B&logoColor=white)
  ![Arduino](https://img.shields.io/badge/-Arduino-00979D?style=for-the-badge&logo=Arduino&logoColor=white)

  ## Description 
  This project is for my ESP32-powered energy monitor that sends the following parameters to InfluxDB:
  * Ambient Temperature
  * Line-to-Neutral Voltage
  * Current (2 Probes)
  
  :warning: This project deals with 120 volts which can harm you. I take no responsibility of any damage as a result from this project. :warning:

  ## Table of Contents
  * [PCB Design](#pcb)  
  * [Hardware](#hardware)
  * [Installation](#installation)
  * [Calibration](#calibration)
  * [Visualization](#visualization)

  ## PCB
  I created a custom PCB for this project using the EasyEDA software, and then had the PCB manufactured through JCLPCB. I decided to go with a two-layer PCB simply because it is the same price as a one-layer, and has the added benefit of easier routing of the traces. Looking at the routing, I suspect it would be possible to make this a one-layer PCB with some careful placement of the resistors underneath the ESP, but that is for someone else to try :laughing:
  
  Something I am going to change in the next revision is making the solder pads for the CT clamps larger, as well as the holes for zip ties. When designing the PCB, it was sort of hard to visualize how big X millimeters is and it just so happened that my calipers ran out of battery. There's nothing wrong with this design, but it just made it cumbersome to solder on. Also the zip tie holes are too close to the connection points, so that needs to be adjusted in the next go-around. Something else I noticed is 120 volts are exposed in the through-hole pins on the bottom of the PCB, which requires some way of insulating if this is going to be installed in an electric panel like I did.
  
  #### Trace Layout: ![alt text](https://github.com/SwadeDotExe/ESP32-Power-Meter/blob/main/Images/TraceLayout.png "PCB Trace Layout")
  #### 3D Rendering: ![alt text](https://github.com/SwadeDotExe/ESP32-Power-Meter/blob/main/Images/PCB%203D%20Rendering.png "PCB 3D Rendering")
  #### Final PCB: ![alt text](https://github.com/SwadeDotExe/ESP32-Power-Meter/blob/main/Images/Final%20PCB.jpg "Final PCB")
  
  ## Hardware 
  This project relies on the ESP32 for reading and sending the data, and a device running [InfluxDB](https://docs.influxdata.com/influxdb/v2.0/install/) and [Grafana](https://grafana.com/docs/grafana/latest/setup-grafana/installation/debian/) for visualizing the data. You could also use something like [HomeBridge](https://github.com/homebridge/homebridge) to display these statistics in Apple HomeKit, which is what I did for the temperature reading. The HomeBridge setup is a little more convoluted because I had to relay the data through MQTT, so that is outside the scope of this project. (Feel free to add a [pull request](https://github.com/SwadeDotExe/ESP32-Power-Meter/pulls)!)
  
  Here are the other components I used:
  | Component        | Quantity      |
  | ---------------- |:-------------:|
  | 47Ω Resistor     | 2             |
  | 330Ω Resistor    | 3             |
  | 1kΩ Resistor     | 1             |
  | 180kΩ Resistor   | 6             |
  | 470Ω Resistor    | 1             |
  | 10uF Capacitor   | 3             |
  | 22uF Capacitor   | 1             |
  | 220uF Capacitor  | 1             |
  | RGB LED          | 1             |
  | [TMP36](https://learn.adafruit.com/tmp36-temperature-sensor)            | 1             |
  | [ZMPT101B](https://www.aliexpress.com/w/wholesale-zmpt101b.html)         | 1             |
  | [120 to 5V PSU](https://www.digikey.com/en/products/detail/recom-power/RAC02-05SGB/6677091) | 1 |
  | [30A CT Clamp](https://www.amazon.com/JANSANE-SCT-013-030-Non-invasive-Split-Core-Transformer/dp/B07JNDRJQ4/ref=pd_lpo_2?pd_rd_i=B07JNDRJQ4&th=1) | 2 |
  
  Some may notice that the PCB has an FTDI breakout connection. This is because I broke the USB port off of my ESP32, and since the pins are all inside the header connector, I couldn't (easily) reflash the chip without pulling it out of the board. I also wouldn't have access to the serial monitor while the ESP is inserted on the board, so I added a spot to connect my breakout board while the PCB is in use.
  
  ## Installation
  Since the CT clamps are remote from the PCB, you can install this board in many different places. I designed this project with the intentions to put this board inside the electric panel in my apartment, and that is exactly what I did. I was lucky to have spare breakers installed in my panel, so I hardwired the board to an empty breaker to provide protection from a short and also means to power off the device without taking the panel cover off. As mentioned above, the PCB is uninsulated and has exposed mains voltage on the bottom. Some barrier needs to be installed between the PCB and the electric panel to prevent a short circuit; I hope to fix this problem with a 3D printed case or something like that.
  
  #### Installed in Panel: 
  <img src="https://github.com/SwadeDotExe/ESP32-Power-Meter/blob/main/Images/InstalledinPanel.jpg" width=35% height=35%>
  
  ## Calibration
  After installing the board in the panel (or whatever you are measuring), it takes some calibration to get the measurements accurate to the actual values. I luckily have a multimeter with a current clamp, so I was able to take a reading with that and then adjust the calibration to get it to match. 
  
  ```cpp
  // Initalize Current Clamps
  emon1.current(36, 18.65);             // Current Clamp 1: input pin, calibration.
  emon2.current(39, 18.65);             // Current Clamp 2: input pin, calibration.
```
  These two lines create a new instance using the [Emon Library](https://github.com/openenergymonitor/EmonLib) for each current clamp on the board. The first value is the pin on the ESP, and the second number is the calibration factor. I don't really know what it means, so I just adjusted it multiple times until I got a reasonably close value. I would like to revisit this in the future because it is not exact. If I turn everything off, the CTs still read around 200 milliamps, which is not correct.
  
  ## Visualization
  After the calibration, the last step is seeing all the data being recorded. For this, I setup a Raspberry Pi on an old monitor that I had laying around. I had an instance of InfluxDB running on an Ubuntu server already, so I added a new database for this project. After setting up permissions and all that, I sucsessfully was getting data entered into the database from the ESP. I had a Grafana instance running on that server as well, so I made a new dashboard to show all power data from the board. Here is the finished result: 

![alt text](https://github.com/SwadeDotExe/ESP32-Power-Meter/blob/main/Images/Dashboard.jpg "Dashboard")
