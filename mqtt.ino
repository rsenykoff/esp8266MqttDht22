#include <ESP8266WiFi.h>
#include "Adafruit_MQTT_Client.h"
#include "DHT.h" //temp sensor

/***************
   Wifi
*/
#define WIFI_SSID  "YOUR_WIFI_SSID"
#define WLAN_PASS   "YOUR_WIFI_PASSWORD"

/***************
 * Temp Sensor
 */
#define DHTPIN 14 //input pin on the Huzzah
#define DHTTYPE DHT22 //sensor type - also support DHT11 and DHT21
DHT dht(DHTPIN, DHTTYPE); //initialize sensor
long dhtReadIntervalMillis =          60 * 1000; //can't poll more often than 2 seconds
unsigned long previousDhtReadMillis = 0;
float currentTemperature = 0;
float currentHumidity = 0;

/***************
   MQTT
*/
#define MQTT_SERVER      "192.168.1.250"
#define MQTT_PORT         1883
#define MQTT_USER        "setUserHereIfNeeded"
#define MQTT_PASS        "setPassHereIfNeeded"

/***************
   Blinkenlights
*/

/******
   Red
   Blinks when MQTT Transmission completes
   Solid when disconnected from MQTT
*/
const int redLedPin =     0;
int redLedState =         HIGH;
long redBlinkInterval =   10000;
long redBlinkDuration =   30;
unsigned long previousRedBlinkedMillis =  0;

/******
   Blue
   Blinks while wifi connected
   Solid when disconnected from wifi
*/
const int blueLedPin =    2;
int blueLedState =        HIGH;
long blueBlinkInterval =  30 * 1000;
long blueBlinkDuration =  30;
unsigned long previousBlueBlinkedMillis = 0;

/***************
   Wifi and MQTT initialization
*/
WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, MQTT_PORT, MQTT_USER, MQTT_PASS);
Adafruit_MQTT_Publish queueTemp = Adafruit_MQTT_Publish(&mqtt, "/sensors/temp/upstairslanding");
Adafruit_MQTT_Publish queueHumidity = Adafruit_MQTT_Publish(&mqtt, "/sensors/humidity/upstairslanding");
void MQTT_connect(); //apparently necessary for some Arduino versions with ESP8266
const long publishingInterval =       60 * 1000;
unsigned long previousPublishMillis = 0;

String messageToConsole;

void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.println(F("TempHum Sensor --> Wifi --> MQTT Publisher"));

  messageToConsole = "Sent temperature to MQTT at ";
  messageToConsole.concat(MQTT_SERVER);
  messageToConsole.concat(" - ");

  pinMode(redLedPin, OUTPUT);
  pinMode(blueLedPin, OUTPUT);
}

void blinkCheck() {
  unsigned long currentMillis = millis();

  if (blueLedState == HIGH) // if off
  {
    if (currentMillis - previousBlueBlinkedMillis >= blueBlinkInterval) { // if time elapsed is greater than interval
      previousBlueBlinkedMillis = currentMillis;
      blueLedState = LOW;
      digitalWrite(blueLedPin, blueLedState);
    }
  }
  else if (currentMillis - previousBlueBlinkedMillis >= blueBlinkDuration) // if it has been on longer than duration
  {
    previousBlueBlinkedMillis = currentMillis;
    blueLedState = HIGH;
    digitalWrite(blueLedPin, blueLedState);
  }
}

float tempReadTest;
float humReadTest;

void pollDHT22() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousDhtReadMillis >= dhtReadIntervalMillis) {
    previousDhtReadMillis = currentMillis;
    tempReadTest = dht.readTemperature(true);
    humReadTest = dht.readHumidity();
    if (isnan(tempReadTest) || isnan(humReadTest)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }
    else {
      currentTemperature = tempReadTest;
      currentHumidity = humReadTest;
    }
  }
}

boolean firstMessageSent = false; //use this to send first message without wait

void publishToMqtt() {
  unsigned long currentMillis = millis();
  if ((currentMillis - previousPublishMillis >= publishingInterval) || !firstMessageSent) {
    previousPublishMillis = currentMillis;
    digitalWrite(redLedPin, LOW);
    //float dasTemp = getTemp();
    if (! queueTemp.publish(currentTemperature) || ! queueHumidity.publish(currentHumidity)) {
      Serial.println("MQTT Publish Failed");
    } else {
      Serial.print(messageToConsole);
      Serial.print(currentTemperature);
      Serial.print("F - ");
      Serial.print(currentHumidity);
      Serial.println(" Humidity - ");
      firstMessageSent = true;
    }
    delay(30); //keep led lit for a bit
    digitalWrite(redLedPin, HIGH);
  }
}

boolean wifiCheck() {
  if (WiFi.status() != WL_CONNECTED) {
    digitalWrite(blueLedPin, LOW);
    Serial.println("*************");
    Serial.print("Connecting to ");
    Serial.println(WIFI_SSID);
    WiFi.begin(WIFI_SSID, WLAN_PASS);
    int i = 0;
    while (WiFi.status() != WL_CONNECTED) {
      delay(200);
      Serial.print(".");
      if (i++ > 10)
      {
        Serial.println();
        return false;
      }
    }
    Serial.println();
    Serial.println("WiFi connected");
    Serial.println("IP address: "); Serial.println(WiFi.localIP());
    Serial.println("*************");
    digitalWrite(blueLedPin, HIGH);
  }
  return true;
}

boolean mqttCheck() {
  if (mqtt.connected())
    return true;

  Serial.print("Connecting to MQTT... ");
  int errorMessage;
  int retries = 5;
  while ((errorMessage = mqtt.connect()) != 0) { // return value of 0 will exit the loop
    digitalWrite(redLedPin, LOW);
    Serial.println(mqtt.connectErrorString(errorMessage));
    Serial.println("Establishing MQTT Connection");
    mqtt.disconnect();
    delay(3000);
    retries--;
    if (retries == 0) {
      return false;
    }
  }
  digitalWrite(redLedPin, HIGH);
  Serial.println("MQTT Connection Established");
  return true;
}

boolean connectivityCheck() {
  if (wifiCheck())
    if (mqttCheck())
      return true;
}

void loop() {
  blinkCheck();
  if (connectivityCheck())
  {
    pollDHT22();
    if (currentTemperature == 0) //did not read yet
      return;
    publishToMqtt();
  }
}

