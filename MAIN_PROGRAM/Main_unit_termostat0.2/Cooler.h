#include "DHT.h"

#define DHTTYPE DHT22

class Cooler {
private:
  Flash* flash;

  struct Property_struct {
    int PWMfreq;
    int PWMChannel;
    int PWMresolution;
    int Max_temperature;
  };
  Property_struct Property;
  struct Control_struct {
    bool Enable;
    uint16_t Required_T;
  };
  Control_struct Control;
  struct State_struct {
    int DutyCycle;
    float Temperature;
    float Humidity;
  };
  State_struct State;
  struct Time_struct {
    unsigned long Update_time;
    unsigned long Max_Update_time;
    unsigned long Min_Update_time;
  };
  Time_struct Time;
  struct Pins_struct {
    uint8_t Temperature_pin;
    uint8_t Cooler_pin;
  };
  Pins_struct Pins;

  DHT* dht;

  struct alarm {
    bool activated;
    Language* message = new Language();
  };
  alarm Alarm;

public:
  Cooler(Flash* flash, int Chanel) {
    this->flash = flash;

    Property.PWMfreq = 20;
    Property.PWMChannel = Chanel;
    Property.PWMresolution = 8;
    Property.Max_temperature = 36;

    Control.Enable = true;

    Control.Required_T = 20;

    State.DutyCycle = 0;
    State.Temperature = 0;
    State.Humidity = 0;

    Time.Update_time = 5000;
    Time.Max_Update_time = 30000;
    Time.Min_Update_time = 5000;

    Pins.Temperature_pin = 0;
    Pins.Cooler_pin = 0;
  }
  void Initialization() {
    if (Control.Enable) ledcWrite(Property.PWMChannel, State.DutyCycle);
    else ledcWrite(Property.PWMChannel, 0);
  }
  void Setup(int temperature_pin, int cooler_pin) {
    dht = new DHT(temperature_pin, DHTTYPE);

    Pins.Temperature_pin = temperature_pin;
    Pins.Cooler_pin = cooler_pin;

    dht->begin();

    // configure LED PWM functionalitites
    ledcSetup(Property.PWMChannel, Property.PWMfreq, Property.PWMresolution);

    // attach the channel to the GPIO to be controlled
    ledcAttachPin(cooler_pin, Property.PWMChannel);

    Control.Enable = flash->get_Flash(55);
    Control.Required_T = flash->get_Flash(56);
    Time.Update_time = flash->get_Flash(57);
    Time.Update_time = Time.Update_time*1000;
  }
  bool Cooling() {
    if (Control.Enable) {
      if (Alarm.activated) {
        Alarm.message->set_Message("Senzor DHT neaktivny. Prosim skontrolujte obvod", "Failed to read from DHT sensor!  Please check the circuit.");
        return false;
      }
      if (State.Temperature >= Property.Max_temperature) {
        State.DutyCycle = 255;
      } else if (State.Temperature > Control.Required_T) {
        if(State.Temperature <= Control.Required_T) State.DutyCycle = 0;
        else State.DutyCycle = map(State.Temperature, Control.Required_T, Property.Max_temperature, 150, 255);
      } else {
        State.DutyCycle = 0;
      }
    } else {
      State.DutyCycle = 0;
      if(State.Temperature >= Property.Max_temperature - 5) Control.Enable = true;
    }
    return true;
  }
  void Reset(){
    Property.PWMfreq = 20;
    Property.PWMresolution = 8;
    Property.Max_temperature = 36;

    Control.Enable = true;

    Control.Required_T = 20;

    State.DutyCycle = 0;
    State.Temperature = 0;
    State.Humidity = 0;

    Time.Update_time = 10000;
    Time.Max_Update_time = 30000;
    Time.Min_Update_time = 10000;
  }

  void set_Temperature() {
    State.Temperature = dht->readTemperature();
    State.Humidity = dht->readHumidity();
    if (isnan(State.Humidity) || isnan(State.Temperature)) {
      Alarm.activated = true;
    } else Alarm.activated = false;
  }
  void set_Enable(bool state) {
    Control.Enable = state;
    if(Control.Enable == false) State.DutyCycle = 0;
  }
  void set_Required_T(int i) {
    if ((i >= 1) && (i <= 30)) Control.Required_T = i;
  }
  void set_Update_time(unsigned long value) {
    if ((Time.Max_Update_time >= value) && (Time.Min_Update_time <= value))  Time.Update_time = value;
    else if(Time.Max_Update_time < value) Time.Update_time = Time.Max_Update_time;
    else if(Time.Min_Update_time > value) Time.Update_time = Time.Min_Update_time;
  }
  void set_DutyCycle(int value) {
    State.DutyCycle = value;
  }
  bool get_Enable() {
    return Control.Enable;
  }
  int get_Required_T() {
    return Control.Required_T;
  }
  int get_DutyCycle() {
    if(Control.Enable == false) return 0;
    return State.DutyCycle;
  }
  float get_Temperature() {
    return State.Temperature;
  }
  float get_Humidity() {
    return State.Humidity;
  }
  bool get_Alarm_activated() {
    return Alarm.activated;
  }
  String get_Alarm_message() {
    return Alarm.message->get_Message();
  }
  unsigned long get_Update_time() {
    return Time.Update_time;
  }
};
