class Battery {
private:
  struct Capacite_struct {
    int value;
    int Max_value;
    int Min_value;
  };
  Capacite_struct Capacite;

  enum Charging_state_enum {
    CHARGING = 0,
    FULL = 1,
    NOCHARGING = 10
  };
  Charging_state_enum Charging_state;

  struct Pins_struct {
    int Capacite;
    int Charging_chrg;
    int Charging_stdby;
  };
  Pins_struct Pins;

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
  Battery() {
    Capacite.value = 0;
    Capacite.Max_value = 1000;
    Capacite.Min_value = 2000;

    Charging_state = NOCHARGING;

    Time.Update_time = 3000;
    Time.Max_Update_time = 7200000; //2h 
    Time.Min_Update_time = 300000;
    
    Alarm.activated = false;
  }
  void Setup(int Capacite, int Charging_chrg, int Charging_stdby) {
    Pins.Capacite = Capacite;
    Pins.Charging_chrg = Charging_chrg;
    Pins.Charging_stdby = Charging_stdby;

    pinMode(Pins.Capacite, INPUT);
    pinMode(Pins.Charging_chrg, INPUT);
    pinMode(Pins.Charging_stdby, INPUT);
  }
  void set_Battery() {
    bool value_chrg = digitalRead(Pins.Charging_chrg);
    bool value_stdby = digitalRead(Pins.Charging_stdby);
    int value_capacite = 0;
    if(value_chrg && value_stdby){
      Charging_state = NOCHARGING;
      Alarm.activated = false;
      //value_capacite = analogRead(Pins.Capacite);
      //map(Capacite.value,Capacite.Min_value, Capacite.Max_value, 0, 100);
    }
    else if((value_chrg == false) && (value_stdby == false)){
      Alarm.activated = true;
      Alarm.message->set_Message("Battery.h/set_Battery: Chyba = Nezapojena Bateria", "Battery.h/set_Battery: Error = Battery is disconnect");
    }
    else if ((value_chrg == false) && value_stdby) {
      Charging_state = CHARGING;
      Alarm.activated = false;
    } 
    else if((value_stdby == false) && value_chrg) {
      Charging_state = FULL;
      Alarm.activated = false;
    }
  }
  void set_Update_time(unsigned long value){
    if((Time.Min_Update_time <= value) && (Time.Max_Update_time >= value)) Time.Update_time = value;
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
  int get_State() {
    return (int)Charging_state;
  }
};