#include "Flash.h"

Flash flash = Flash(1);
int i = 5;

void setup() {
  Serial.begin(115200);
  pinMode(6, OUTPUT);

  flash.Setup();

  i = EEPROM.read(0);

  Serial.print("I = ");
  Serial.println(i);
}

void loop() {
  digitalWrite(6, HIGH);
  i++;
  EEPROM.write(0, i);
  EEPROM.commit();
  Serial.print("I = ");
  Serial.println(i);
  delay(1500);

}
