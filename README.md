# ESP8266 Publish Temp / Hum Sensor Data to MQTT

This was developed on an Adafruit Huzzah ESP8266 Wifi.

  - Periodically check Temperature and Humidity Settings
  - Maintain a Wifi Connection to your network
  - Maintain MQTT Connection and Publish using your designated queues

## Hardware

  - [DHT22 Temperature and Humidity Sensor](https://www.adafruit.com/products/385)
  - [Adafruit Huzzah ESP8266 Wifi](https://www.adafruit.com/products/2821)

This project assumes you already have an MQTT server up and running on your network. Testing was done with [Mosquitto MQTT](https://mosquitto.org/) running on Ubuntu. 

