#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_SHT31.h"
#include "Adafruit_HTU21DF.h"

//I2C device found at address 0x40
//I2C device found at address 0x44

class Temperature_sensor{
private:
  Oven* oven;
  Flash* flash;

  uint8_t Pin;

  double Temperature;
  double Humidity;
  
  enum Mode_enum{
    SHT31_only = 1, 
    HTU21_only = 2, 
    Both = 3
  };
  Mode_enum Mode;
  
  struct SHT31_struct{
    Adafruit_SHT31 Sensor = Adafruit_SHT31();
    float Temperature;
    float Humidity;
  };
  SHT31_struct SHT31;
  
  struct HTU21_struct{
    Adafruit_HTU21DF Sensor = Adafruit_HTU21DF();
    float Temperature;
    float Humidity;
  };
  HTU21_struct HTU21;

  struct Time_struct{
    unsigned long Update_time;
    unsigned long Max_Update_time;
    unsigned long Min_Update_time;
  };
  Time_struct Time;

  struct alarm{
    bool activated;
    Language* message = new Language();
  };
  alarm Alarm;
public:
  Temperature_sensor(Oven* oven, Flash* flash){
    this->oven = oven;
    this->flash = flash;

    Pin = 0;

    Temperature = 0.0;
    Humidity = 0.0;

    Mode = Both;
    
    SHT31.Temperature = 0.0f;
    SHT31.Humidity = 0.0f;
    
    HTU21.Temperature = 0.0f;
    HTU21.Humidity = 0.0f;

    Time.Update_time = 30000;
    Time.Max_Update_time = 60000; //2h 
    Time.Min_Update_time = 5000;
    
    Alarm.activated = false;
  }
  void Setup(uint8_t pin){
    Pin = pin;

    Time.Update_time = (int)flash->get_Flash(20)*1000;
    Mode = (Mode_enum)flash->get_Flash(21);
    
    pinMode(Pin, OUTPUT);
    digitalWrite(Pin, HIGH);
    
    if (!SHT31.Sensor.begin(0x44)) {   // Set to 0x45 for alternate i2c addr
      Alarm.activated = true;
      Alarm.message->set_Message("Temperature_Sensor.h/Setup: Chyba = SHT31 Nenajdeny", "Temperature_Sensor.h/Setup: Error = SHT31 Couldn´t find");
    }
    else Alarm.activated = false;
  
    if(!HTU21.Sensor.begin()) {
      Alarm.activated = true;
      Alarm.message->set_Message("Temperature_Sensor.h/Setup: Chyba = HTU21 Nenajdeny", "Temperature_Sensor.h/Setup: Error = HTU21 Couldn´t find");
    }
    else Alarm.activated = false;

    digitalWrite(Pin, LOW);
  }
  bool test_Temperature(){
    pinMode(Pin, OUTPUT);
    digitalWrite(Pin, HIGH);
    
    if (!SHT31.Sensor.begin(0x44)) {   // Set to 0x45 for alternate i2c addr
      Alarm.activated = true;
      Alarm.message->set_Message("Temperature_Sensor.h/test_Temperature: Chyba = SHT31 Nenajdeny", "Temperature_Sensor.h/test_Temperature: Error = SHT31 Couldn´t find");
    }
    else {
      Alarm.activated = false;
      digitalWrite(Pin, LOW);
      return false;
    }
  
    if(!HTU21.Sensor.begin()) {
      Alarm.activated = true;
      Alarm.message->set_Message("Temperature_Sensor.h/test_Temperature: Chyba = HTU21 Nenajdeny", "Temperature_Sensor.h/test_Temperature: Error = HTU21 Couldn´t find");
    }
    else {
      Alarm.activated = false;
      digitalWrite(Pin, LOW);
      return false;
    }

    digitalWrite(Pin, LOW);
    return true;
  }  
  void set_Temperature(){
    digitalWrite(Pin, HIGH);
    delay(5);
    switch(Mode){
      case 1:
        SHT31.Temperature = SHT31.Sensor.readTemperature();
        SHT31.Humidity = SHT31.Sensor.readHumidity();
        Temperature = SHT31.Temperature;
        Humidity = SHT31.Humidity;
        break;
      case 2:
        HTU21.Temperature = HTU21.Sensor.readTemperature();
        HTU21.Humidity = HTU21.Sensor.readHumidity();
        Temperature = HTU21.Temperature;
        Humidity = HTU21.Humidity;
        break;
      case 3:
        SHT31.Temperature = SHT31.Sensor.readTemperature();
        SHT31.Humidity = SHT31.Sensor.readHumidity();
        delay(5);
        HTU21.Temperature = HTU21.Sensor.readTemperature();
        HTU21.Humidity = HTU21.Sensor.readHumidity();

        Temperature = (SHT31.Temperature+HTU21.Temperature)/2;
        Humidity = (SHT31.Humidity+HTU21.Humidity)/2;
        break;
    }
    if((Temperature >= -5) && (Temperature <= 60)) Alarm.activated = false;
    else {
      Alarm.activated = true;
      Alarm.message->set_Message("Temperature_Sensor.h/set_Temperature: Chyba = Teplomer Nenajdeny", "Temperature_Sensor.h/set_Temperature: Error = Sensor Couldn´t find");
    }
    oven->set_Temp_and_Hum(Temperature, Humidity);
    digitalWrite(Pin, LOW);
  }

  void set_Update_time(unsigned long value){
    if((Time.Min_Update_time <= value) && (Time.Max_Update_time >= value)) Time.Update_time = value;
    else if(Time.Max_Update_time < value) Time.Update_time = Time.Max_Update_time;
    else Time.Update_time = Time.Min_Update_time;
  }
  void set_Mode(int value){
    if(value == 1) Mode = SHT31_only;
    else if(value == 2) Mode = HTU21_only;
    else if(value == 3) Mode = Both;
  }
  
  float get_Temperature(uint8_t value){
    switch(value){
      case 1:
        return SHT31.Temperature;
        break;
      case 2:
        return HTU21.Temperature;
        break;
      case 3:
        return Temperature;
        break;
      default:
        return 99.9;
        break;
    }
  }
  float get_Humidity(uint8_t value){
    switch(value){
      case 1:
        return SHT31.Humidity;
        break;
      case 2:
        return HTU21.Humidity;
        break;
      case 3:
        return Humidity;
        break;
      default:
        return 99.9;
        break;
    }
  }
  unsigned long get_Update_time(){
    return Time.Update_time;
  }
  bool get_Alarm_activated(){
    return Alarm.activated;
  }
  String get_Alarm_message(){
    return Alarm.message->get_Message();
  }
  int get_Mode(){
    return Mode; 
  }
};
