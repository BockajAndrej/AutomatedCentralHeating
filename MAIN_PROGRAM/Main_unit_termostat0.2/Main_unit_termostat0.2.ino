//#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

#include "Language.h"
#include "TON_timer.h"
#include "Trigger.h"
#include "ButtonAB.h"
#include "Flash.h"
#include "Cooler.h"
#include "Real_Time.h"
#include "Ultrasonic.h"
#include "Oven.h"
#include "ESP_NOW.h"
#include "Firebase_client_library.h"
#include "HMI.h"
#include "Main_logic.h"
#include "Hardware.h"

//#define WIFI_SSID "Boky_2G"
//#define WIFI_PASSWORD "gameover2"

#define WIFI_SSID "iPhoneAB"
#define WIFI_PASSWORD "12345678"

// Insert Firebase project API Key
#define API_KEY "AIzaSyAkXu2A9Ank8M8ISkFQzLbjylPZ2WpxrNU"

// Insert Authorized Email and Corresponding Password
#define USER_EMAIL "bockajandrej@gmail.com"
#define USER_PASSWORD "8kmfgaq4"

// Insert RTDB URLefine the RTDB URL
#define DATABASE_URL "https://smartstovecontrol-default-rtdb.europe-west1.firebasedatabase.app/"

  //34:86:5D:7B:25:34 - Main_Termostat
  //E8:31:CD:90:43:10 - Xst_Termostat

  //e8:31:cd:90:27:90 - 1st_Termostat
  //e8:31:cd:91:2c:90 - 2st_Termostat

ButtonAB* on_off_Btn = new ButtonAB(false);
Flash* flash = new Flash(300);
Cooler* cooler = new Cooler(flash, 2);
Time* real_time = new Time(flash);
Ultrasonic* ultrasonic = new Ultrasonic(flash);
Oven* oven_1 = new Oven(1, real_time, flash);
Oven* oven_2 = new Oven(2, real_time, flash);
//Sender_1 = E8:31:CD:90:43:10
ESP_NOW* esp_now = new ESP_NOW(1, 0xE8, 0x31, 0xCD, 0x90, 0x27, 0x90, 0xE8, 0x31, 0xCD, 0x91, 0x2C, 0x90, oven_1, oven_2, real_time, ultrasonic);
Firebase_class* firebase = new Firebase_class(oven_1, oven_2, real_time, WIFI_SSID, WIFI_PASSWORD, API_KEY, USER_EMAIL, USER_PASSWORD, DATABASE_URL);
HMI *hmi = new HMI(real_time, oven_1, oven_2, ultrasonic, cooler, esp_now, flash, on_off_Btn, firebase);
Main_logic main_logic = Main_logic(hmi, oven_1, oven_2, real_time, ultrasonic, cooler, esp_now, flash, firebase);
Hardware hardware = Hardware(hmi, oven_1, oven_2, ultrasonic, cooler, real_time, on_off_Btn);

//Main = 34:86:5D:7B:25:34
//ESP_NOW* esp_now = new ESP_NOW(1, 0x34, 0x86, 0x5D, 0x7B, 0x25, 0x34);

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_AP_STA);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");

  int Counter_attempts = 0;
  
  while ((WiFi.status() != WL_CONNECTED) && (main_logic.get_WiFi_mode())) {
    Serial.print('.');
    delay(1000);
    Counter_attempts += 1;
    if(Counter_attempts > 16){
      WiFi.mode(WIFI_STA);
      WiFi.disconnect();
      main_logic.set_WiFi_mode(false);
      real_time->set_WIFI_enable(false);
    }
    else if(Counter_attempts > 8){
      //ESP.restart();
      WiFi.disconnect();
      WiFi.reconnect();
    }
  }
  Serial.println(WiFi.localIP());
  
  esp_now->Setup();

  Serial.println("ESP-NOW: Setup");

  if(main_logic.get_WiFi_mode()) firebase->Setup();

  Serial.println("Firebase: Setup");

  flash->Setup();

  Serial.println("Flash: Setup");

  main_logic.Setup();

  Serial.println("Main logic: Setup");

  hardware.Setup();

  Serial.println("Hareware: Setup");

  Serial.println("----- SETUP : DONE -----");

  if(main_logic.get_WiFi_mode()) firebase->set_Data(true, 1);
}

void loop() {
  hardware.Inputs();
  
  main_logic.fcMain();
  
  hardware.Outputs(); 
} 
