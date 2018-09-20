# ESP8266 Publish Temperature / Humidity Sensor Data to MQTT

This was developed on an Adafruit Huzzah ESP8266 Wifi.

  - Periodically check Temperature and Humidity Settings
  - Maintain a Wifi Connection to your network
  - Maintain MQTT Connection and Publish using your designated queues

## Hardware

  - [DHT22 Temperature and Humidity Sensor](https://www.adafruit.com/products/385)
  - [Adafruit Huzzah ESP8266 Wifi](https://www.adafruit.com/products/2821)

This project assumes you already have an MQTT server up and running on your network. Testing was done with [Mosquitto MQTT](https://mosquitto.org/) running on Ubuntu. 

I currently use this with [OpenHAB](https://www.openhab.org/) open-source home automation server. OpenHAB can be configured to subscribe to a Mosquitto message queue and use rrdtool to log the data over time. I can move this sensor around the house, and track the heating / cooling. This was useful to figure out how early to have the digital thermostat in our bedroom turn on in order to hit a target temperature when we wake up. Devices like the Nest will do this automatically for you, but this is a lot cheaper. :)

You may recognize this chart style if you're familiar with RRDTool. That's what OpenHAB is using under the hood to store this.
![OpenHAB Chart](chart.png?raw=true "Title")
