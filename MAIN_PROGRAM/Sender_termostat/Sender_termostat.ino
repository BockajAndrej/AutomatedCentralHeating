#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>

#include "Language.h"
#include "TON_timer.h"
#include "Trigger.h"
#include "Flash.h"
#include "State.h"
#include "Oven.h"
#include "Temperature_sensor.h"
#include "Battery.h"
#include "ESP_NOW.h"
#include "HMI.h"
#include "Main_logic.h"
#include "Hardware.h"

//Oznacenie pre okruh 1 -> ID = 2 + Deklarovat = Temperature_sensor *Temp = new Temperature_sensor(oven_1, flash);
//Oznacenie pre okruh 2 -> ID = 3 + Deklarovat = Temperature_sensor *Temp = new Temperature_sensor(oven_2, flash);

//E8:31:CD:90:43:10 - Main_Termostat

//e8:31:cd:90:27:90 - 1st_Termostat
//e8:31:cd:91:2c:90 - 2st_Termostat

#define ID 3

Flash *flash = new Flash(200);
State *state = new State();
Oven *oven_1 = new Oven(1, flash);
Oven *oven_2 = new Oven(2, flash);
Temperature_sensor *Temp = new Temperature_sensor(oven_2, flash);
//Temperature_sensor *Temp = new Temperature_sensor(oven_1, flash);
Battery *battery = new Battery();
ESP_NOW *esp_now = new ESP_NOW(ID, 0xE8, 0x31, 0xCD, 0x90, 0x43, 0x10, oven_1, oven_2, state);
HMI *hmi = new HMI(oven_1, oven_2, Temp, state, esp_now, battery, flash);
Main_logic main_logic = Main_logic(hmi, oven_1, oven_2, Temp, esp_now, battery, flash);
Hardware hardware = Hardware(hmi, oven_1, oven_2, Temp, battery);

bool Wakeup_reason_variable = false;

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);

  esp_now->Setup();

  flash->Setup();

  Wakeup_reason_variable = main_logic.Setup();
  
  hardware.Setup(Wakeup_reason_variable); 

  esp_now->Sending(false, 0);
}

void loop() {  
  hardware.Inputs();
  
  main_logic.fcMain();
  
  hardware.Outputs();
}
