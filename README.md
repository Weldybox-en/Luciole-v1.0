# Luciole-v1.0

Luciole is an RGB controller controlled though WIFI. No need extra applications because the board embeds a Webserver. Luciole has multiple operating mode in addition to basics functionalities that you can find in a classic RGB controller. The goal with this board is to provide and very simple way to control RGB LED for a very low price.

The Luciole-v1.0 code can be decomposed in two parts. The Arduino and the HTML/CSS/JS code. The webserver is stored on the chip and communicate though a websocket with the Arduino code.

# Contents

- [Webserver quick view](#webserver-quick-view)
- [Goals](#goals)
- [Implementation](#implementation)
  - [Librairy](#librairy)
- [Flashing ESP8266](#flashing-esp8266)
  - [Platformio](#platformio)
  - [Arduino](#arduino)
- [Boards schematics](#boards-schematics)
- [My networks](#my-networks)
  
  
# Webserver quick view

The webserver looked like this : 

<p align="center">
  <img src="https://imgur.com/XMOOdEB.jpg" width="300">
  <img src="https://imgur.com/52pnw5x.jpg" width="300">
</p>


The frist page is the most important because all the functionalities can be reach from here while the second is simply the alarm settings.

# Goals

All the goals that i want to be implemented/is implemented on luciole's board.

- [x] Any color selection **w/Brightness**
- [x] Color storing **(up to 5)**
- [x] Smart light **/w colors settings**
- [x] Alarm light
- [ ] Animations
- [ ] Pairing between multiple modules
- [ ] RF remote control

# Implementation

## Librairy

In order to make luciole working there is some things to do. Firstly, luciole use some librairy that i have manually modified to fit perfectly with the app.

Here is the links of the two modified librairy that you need to make it work:
- ![WiFiManagerByjulfi](https://github.com/Weldybox-en/WiFiManagerByjulfi)
- ![NTPClientByjulfi](https://github.com/Weldybox-en/NTPClientByjulfi)
<p align="center"><i>You will find above a documentation with the installation informations</i><p>
  
 Here is the librairy list that you have to had: 
 
```cpp
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <ArduinoOTA.h>
#include <SPI.h>
#include <WebSocketsServer.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <WiFiUdp.h>
#include <NTPClientByjulfi.h>
#include <WiFiManagerByWeldy.h> 
```

## Flashing ESP8266

### Platformio

Because the webserver is stored in the SPIFFS memory it is necessary to flash both original sktech and spiffs sketch. In platformio you have to run those two commands:

1. pio run -t upload
2. pio run -t uploadfs

<p align="center"><i>For more informations about flashing methods you can go read the <a href="https://docs.platformio.org/en/latest/platforms/espressif8266.html" target="_blank">platformio documentation</a></i></p>

### Arduino


------
<p align="center"><i><b>If you are using arduino IDE, you just need the data/ directory (that contain the webserver) and the main.cpp file located in the src/ directory.</i></b></p>

------

I bet you know how to flash default sktech on esp8266 with arduino IDE. So bellow are the steps to flash the esp8266 spiffs memory! If you haven't install the necessary things to do it I higly recommand you to read the "![Using ESP8266 SPIFFS](https://www.instructables.com/id/Using-ESP8266-SPIFFS/)" tuto on instructable.

Else, find bellow the necessary steps to archieve that:

1.  Ensure you have a sub-directory within your sketch directory named 'data'
2. Place the files you wish to upload into the 'data' directory.
3. From 'Tools' menu, select the correct ESP8266 device and choose the 'Flash Size' with the SPIFFS you require.
4. Ensure the serial monitor dialogue box is closed.
5. From Tools menu select 'ESP8266 Sketch Data Upload
6. Once upload is complete. Arduino IDE message window will show 100% upload. 

<p align="center">
  <img src="https://imgur.com/dhy3P5X.jpg" width="500">
</p>

## Boards schematics

### Luciole dev board

You can find all the informations about the components that I use though the BOM csv file in the *schematics/dev_board folder*. Same for the gerber file.

You also can check the project on my [EasyEDA account](https://easyeda.com/weldybox/luciolev2-0-microusb-3-0). You will find the same informations

 ## My networks
 
 - Twitter: https://twitter.com/julfiofficial
 - Blog : http://weldybox.com/
 - Youtube channel : https://www.youtube.com/channel/UCTNOaG1svq1xgBzBOvLj6vw?view_as=subscriber
