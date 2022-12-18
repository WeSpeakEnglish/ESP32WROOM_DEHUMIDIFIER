// nRF24L01 přijímač

// připojení knihoven
#include "esp_system.h"
#include <WiFi.h>
#include <Wire.h>
#include "Adafruit_HTU21DF.h"
#include <OneWire.h>
#include <DallasTemperature.h>

#define pinFAN 15
#define dewPointThUp 1
#define dewPointThDown 4

// GPIO where the DS18B20 is connected to
const int oneWireBus = 4;     

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

Adafruit_HTU21DF htu = Adafruit_HTU21DF();

// Number of temperature devices found
int numberOfDevices;

// We'll use this variable to store a found device address
DeviceAddress tempDeviceAddress[20];

#include <string.h>
#include "EspMQTTClient.h"

EspMQTTClient client(
  "HUAWEI P10",
  "betku63U",
  "192.168.1.100",  // MQTT Broker server ip
  "MQTTUsername",   // Can be omitted if not needed
  "MQTTPassword",   // Can be omitted if not needed
  "TestClient"      // Client name that uniquely identify your device
);


void onConnectionEstablished() {

  client.subscribe("mytopic/test", [] (const String &payload)  {
    Serial.println(payload);
  });

  client.publish("mytopic/test", "This is a message");
}

void setup() {
  // komunikace přes sériovou linku rychlostí 9600 baud
  Serial.begin(9600);
  // zapnutí komunikace nRF modulu
  Wire.begin(); // pins 21 + 22
  sensors.begin();

  pinMode(pinFAN , OUTPUT);  

  // Grab a count of devices on the wire
  numberOfDevices = sensors.getDeviceCount();
  
  // locate devices on the bus
  Serial.print("Locating devices...");
  Serial.print("Found ");
  Serial.print(numberOfDevices, DEC);
  Serial.println(" devices.");

  // Loop through each device, print out address
  for(int i=0;i<numberOfDevices; i++){
    // Search the wire for address
    if(sensors.getAddress(tempDeviceAddress[i], i)){
      Serial.print("Found device ");
      Serial.print(i, DEC);
      Serial.print(" with address: ");
      printAddress(tempDeviceAddress[i]);
      Serial.println();
    } else {
      Serial.print("Found ghost device at ");
      Serial.print(i, DEC);
      Serial.print(" but could not detect address. Check power and cabling");
    }
  }
  digitalWrite(pinFAN , LOW);  

 if (!htu.begin()) {
    Serial.println("Check circuit. HTU21D not found!");
    while (1);
  }
  delay(100);

}

char ReveivedCMD;
char fanSW = 0;

 
void loop() {
  // proměnné pro příjem a odezvu
 float tempOutside = htu.readTemperature();
 float humOutside = htu.readHumidity();
 float dewPoint = calc_dewpoint(humOutside,tempOutside);
 
 sensors.requestTemperatures(); 
 //float temperatureC = sensors.getTempCByIndex(0);
      // Print the data
  float tempCold = sensors.getTempC(tempDeviceAddress[0]);
  float tempHot = sensors.getTempC(tempDeviceAddress[1]);
  Serial.print("Temp Cold C: ");
  Serial.print(tempCold);
  Serial.print(" Dew point:");
  Serial.print(dewPoint);
  Serial.print(" Temp Hot C: ");
  Serial.print(tempHot);

  Serial.print(" Temp:");
  Serial.print(tempOutside);
  Serial.print(" Humidity:");
  Serial.print(humOutside); 


  if (dewPoint > tempCold + dewPointThDown){
      digitalWrite(pinFAN , HIGH), fanSW = 1;
      }
  else
    if (dewPoint < tempCold + dewPointThUp){
      digitalWrite(pinFAN , LOW), fanSW = 0;  
        
    }
  if(!fanSW)
    Serial.println(" Fan switched OFF");
  else
    Serial.println(" Fan switched ON");

    
  client.loop();  
  delay(10000);


}
// function to print a device address
void printAddress(DeviceAddress deviceAddress) {
  for (uint8_t i = 0; i < 8; i++){
    if (deviceAddress[i] < 16) Serial.print("0");
      Serial.print(deviceAddress[i], HEX);
  }
}

float calc_dewpoint(float h,float t)
//--------------------------------------------------------------------
// calculates dew point
// input: humidity [%RH], temperature [°C]
// output: dew point [°C]
{ float k,dew_point ;
k = (log10(h)-2)/0.4343 + (17.62*t)/(243.12+t);
dew_point = 243.12*k/(17.62-k);
return dew_point;
}
