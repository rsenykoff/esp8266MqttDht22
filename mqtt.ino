#include <ESP8266WiFi.h>
#include "Adafruit_MQTT_Client.h"
#include "DHT.h" //temp sensor

/***************
   Wifi
*/
//#define WIFI_SSID  ""
//#define WLAN_PASS   ""
const char* WIFI_SSID = "your_ssid";
const char* WLAN_PASS = "your_wifi_password";
// WHAT NOT TO DO (Hardcode credentials!) ^^

/***************
 * Temp Sensor
 */
#define DHTPIN 13 //input pin on the Huzzah
#define DHTTYPE DHT22 //sensor type - also support DHT11 and DHT21
DHT dht(DHTPIN, DHTTYPE); //initialize sensor
long dhtReadIntervalMillis =          2 * 1000; //can't poll more often than 2 seconds
unsigned long previousDhtReadMillis = 0;
float currentTemperature = 0;
float currentHumidity = 0;

/***************
   MQTT
*/
#define MQTT_SERVER      "192.168.1.123"
#define MQTT_PORT         1883
#define MQTT_USER        ""
#define MQTT_PASS        ""

/***************
   Blinkenlights
*/

boolean ledsEnabled = false; //false to save power

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
const long publishingInterval =       5 * 1000;
unsigned long previousPublishMillis = 0;

String messageToConsole;

void setup() {
  Serial.begin(115200);
  delay(30);
  
  //Serial.println(F("TempHum Sensor --> Wifi --> MQTT Publisher"));
  dht.begin();
  
 // messageToConsole = "Sent temperature to MQTT at ";
 // messageToConsole.concat(MQTT_SERVER);
 // messageToConsole.concat(" - ");

  pinMode(redLedPin, OUTPUT);
  pinMode(blueLedPin, OUTPUT);
  digitalWrite(redLedPin, HIGH);
  digitalWrite(blueLedPin, HIGH);
}

void blinkCheck() {
  if (ledsEnabled)
  {
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
unsigned long firstMessageMillis = 0;

void publishToMqtt() {
  unsigned long currentMillis = millis();
  if ((currentMillis - previousPublishMillis >= publishingInterval) || !firstMessageSent) {
    previousPublishMillis = currentMillis;
    if (ledsEnabled) digitalWrite(redLedPin, LOW);
    //float dasTemp = getTemp();
    boolean tempPublished = queueTemp.publish(currentTemperature);
    boolean humPublished = queueHumidity.publish(currentHumidity);

      if (!tempPublished || !humPublished) {
//    if (! queueTemp.publish(currentTemperature) || ! queueHumidity.publish(currentHumidity)) {
      Serial.println("MQTT Publish Failed");
    } else {
      Serial.print(messageToConsole);
      Serial.print(currentTemperature);
      Serial.print("F - ");
      Serial.print(currentHumidity);
      Serial.println(" Humidity - ");
      firstMessageSent = true;
      firstMessageMillis = millis();
      //ESP.deepSleep(50e6);
    }
    if (ledsEnabled) digitalWrite(redLedPin, HIGH);
  }
}

boolean wifiCheck() {
  if (WiFi.status() != WL_CONNECTED) {
    if (ledsEnabled) digitalWrite(blueLedPin, LOW);
    Serial.println("*************");
    Serial.print("Connecting to ");
    Serial.println(WIFI_SSID);
    WiFi.begin(WIFI_SSID, WLAN_PASS);
    int i = 0;
    while (WiFi.status() != WL_CONNECTED) {
      delay(200);
      Serial.print(".");
      if (i++ > 50)
      {
        Serial.println();
        return false;
      }
    }
    Serial.println();
    Serial.println("WiFi connected");
    Serial.println("IP address: "); Serial.println(WiFi.localIP());
    Serial.println("*************");
    if (ledsEnabled) digitalWrite(blueLedPin, HIGH);
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
    if (ledsEnabled) digitalWrite(redLedPin, LOW);
    Serial.println(mqtt.connectErrorString(errorMessage));
    Serial.println("Establishing MQTT Connection");
    mqtt.disconnect();
    delay(3000);
    retries--;
    if (retries == 0) {
      return false;
    }
  }
  if (ledsEnabled) digitalWrite(redLedPin, HIGH);
  Serial.println("MQTT Connection Established");
  return true;
}

boolean connectivityCheck() {
  if (wifiCheck())
    if (mqttCheck())
      return true;
}

// without this approach the deep sleep 
// may prevent the mqtt messages from being
// sent successfully
unsigned long waitBeforeSleepDuration = 500;

void loop() {
  blinkCheck();
  if (firstMessageSent)
  {
    unsigned long currentMillis = millis();
    if (currentMillis - firstMessageMillis >= waitBeforeSleepDuration) 
    {
      Serial.println("Going to sleep...");
      ESP.deepSleep(280e6);
    }
  } 
  else if (connectivityCheck())
  {
    pollDHT22();
    if (currentTemperature == 0) //did not read yet
      return;
    publishToMqtt();
  }
}
