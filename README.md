# Indoor temperature & humidity sensor.

Software for amateur build of wifi-enabled temperature and humidity sensor.

Application periodically reads data from sensor and sends it to HTTP server in local network.
Between reads it puts uC to sleep.

Keywords: DHT-11, ESP8266, ESP8266_RTOS_SDK, Wi-fi



## Hardware

The application was written for ESP8266 microcontroller. ESP-01 module was chosen for it low cost and low footprint.
However, this board does not have WAKE signal exposed. For a deep sleep functionality to work correctly, modification to
ESP-01 board must be made  which allows to connect WAKE with RST (or just use different board e.g. D1 mini or custom design).

Temperature and humidity sensor is DHT-11. Another low cost, widely available device. 
DHT-22 can be substitued as a sensor, it has the same interface but has wider range and appears to be less prone to incorrect
readings.

See DHT_DATA_PIN defined in humtemp.c file to see/modify which GPIO is connected to DHT data pin.

## Build

This software project is based on ESP8266_RTOS_SDK. You must install it in your system, together with all dependencies
such as xtensa gcc toolchain and python modules. You can check what is needed and verify that SDK is operational by
trying to compile hello-world example from ESP8266_RTOS_SDK.

Eclipse project is included. If you wish to use Eclipse IDE, you will need Eclipse with CDT plugins
 (Eclipse for C/C++ development) and cross compiler support plugin.

First build needs to be done from command line. Ensure that you have enviroment variables set correctly

 * IDF_PATH points to  ESP8266_RTOS_SDK installation directory
 * PATH contains xtensa toolchain bin folder
 
And run "make" command.

Subsequent builds can be done from Eclipse.

## Deploy

To flash connect your ESP-01 board to flasher and run "make flash"




