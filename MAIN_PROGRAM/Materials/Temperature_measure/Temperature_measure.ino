/*************************************************** 
  This is an example for the SHT31-D Humidity & Temp Sensor

  Designed specifically to work with the SHT31-D sensor from Adafruit
  ----> https://www.adafruit.com/products/2857

  These sensors use I2C to communicate, 2 pins are required to  
  interface
 ****************************************************/
 
#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_SHT31.h"
#include "Adafruit_HTU21DF.h"

//I2C device found at address 0x40
//I2C device found at address 0x44


bool enableHeater = false;
uint8_t loopCnt = 0;

Adafruit_SHT31 sht31 = Adafruit_SHT31();
Adafruit_HTU21DF htu = Adafruit_HTU21DF();


void setup() {
  Serial.begin(115200);

  pinMode(26, OUTPUT);
  digitalWrite(26, HIGH);

  while (!Serial)
    delay(10);     // will pause Zero, Leonardo, etc until serial console opens

  Serial.println("SHT31 test");
  if (! sht31.begin(0x44)) {   // Set to 0x45 for alternate i2c addr
    Serial.println("Couldn't find SHT31");
    while (1) delay(1);
  }

  if (!htu.begin()) {
    Serial.println("Couldn't find HTU21 sensor!");
    while (1);
  }

}


void loop() {
  digitalWrite(26, HIGH);
  float temp = htu.readTemperature();
  float rel_hum = htu.readHumidity();
  Serial.print("HTU Temp: "); Serial.print(temp); Serial.print(" C");
  Serial.print("\t\t");
  Serial.print("HTU Humidity: "); Serial.print(rel_hum); Serial.println(" \%");
  digitalWrite(26, LOW);
  delay(1000);
  digitalWrite(26, HIGH);
  float t = sht31.readTemperature();
  float h = sht31.readHumidity();
  if (! isnan(t)) {  // check if 'is not a number'
    Serial.print("Temp *C = "); Serial.print(t); Serial.print("\t\t");
  } else { 
    Serial.println("Failed to read temperature");
  }
  
  if (! isnan(h)) {  // check if 'is not a number'
    Serial.print("Hum. % = "); Serial.println(h);
  } else { 
    Serial.println("Failed to read humidity");
  }
  digitalWrite(26, LOW);

  delay(1000);
}
