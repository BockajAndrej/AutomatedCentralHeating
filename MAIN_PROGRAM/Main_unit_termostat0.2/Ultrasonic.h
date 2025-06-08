class Ultrasonic {
private:
  Flash* flash;
  struct Control_struct {
    bool Enable;
  };
  Control_struct Control;
  struct State_struct {
    unsigned long Duration;
    int Distance;
    int Max_distance;
    int Min_distance;
    int Capacity;
    int Capacity_in_percent;
    bool Measured;
  };
  State_struct State;

  struct Time_struct {
    unsigned long Update_time;
    unsigned long Max_Update_time;
    unsigned long Min_Update_time;
  };
  Time_struct Time;

  struct Pins_struct {
    uint8_t Trig_pin;
    uint8_t Echo_pin;
  };
  Pins_struct Pins;

  struct alarm {
    bool activated;
    Language* message = new Language();
  };
  alarm Alarm;

public:
  Ultrasonic(Flash* flash) {
    this->flash = flash;

    Control.Enable = true;

    State.Duration = 0;
    State.Distance = 0;
    State.Max_distance = 110;
    State.Min_distance = 10;
    State.Capacity = 0;
    State.Capacity_in_percent = 0;
    State.Measured = false;

    Time.Update_time = 60000;
    Time.Max_Update_time = 3600000;  //1h
    Time.Min_Update_time = 60000;    //1min - 900000

    Pins.Trig_pin = 0;
    Pins.Echo_pin = 0;

    Alarm.activated = false;
  }
  void Setup(int trigPin, int echoPin) {
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);

    Pins.Trig_pin = trigPin;
    Pins.Echo_pin = echoPin;

    Control.Enable = flash->get_Flash(50);
    State.Max_distance = flash->get_Flash(51);
    State.Min_distance = flash->get_Flash(52);
    Time.Update_time = flash->get_Flash(53)*60000;
    
  }
  bool Measuring_main() {
    if (Alarm.activated && Control.Enable) {
      Alarm.message->set_Message("Ultrazvukovy senzor neaktivny. Prosim skontrolujte obvod", "Failed to read from Ultrasonic sensor!  Please check the circuit.");
      return false;
    }
    return true;
  }
  void Reset(){
    Control.Enable = true;

    State.Duration = 0;
    State.Distance = 0;
    State.Max_distance = 110;
    State.Min_distance = 10;
    State.Capacity = 0;
    State.Capacity_in_percent = 0;
    State.Measured = false;

    Time.Update_time = 60000;
    Time.Max_Update_time = 3600000;  //1h
    Time.Min_Update_time = 60000;    //1min - 900000

    Alarm.activated = false;
  }
  
  void set_Distance() {
    if (Control.Enable) {
      State.Measured = true;

      digitalWrite(Pins.Trig_pin, LOW);
      delayMicroseconds(2);
      digitalWrite(Pins.Trig_pin, HIGH);
      delayMicroseconds(10);
      digitalWrite(Pins.Trig_pin, LOW);

      State.Duration = pulseIn(Pins.Echo_pin, HIGH);
      State.Distance = State.Duration / 29 / 2;  //It is Measured in centimetrs

      if (State.Duration == 0) {
        Alarm.activated = true;
        Control.Enable = false;
        State.Capacity_in_percent = 0;
      } else {
        Serial.println("Ultrasonic measuring = " + String(State.Distance) + "cm");
        Alarm.activated = false;
        State.Capacity = State.Max_distance - State.Min_distance;
        int value = ((State.Distance - State.Min_distance) * 100) / State.Capacity;
        State.Capacity_in_percent = 100 - value;
        if (State.Capacity_in_percent > 100) State.Capacity_in_percent = 100;
        else if (State.Capacity_in_percent < 0) State.Capacity_in_percent = 0;
      }
    } else {
      Alarm.activated = false;
      State.Capacity_in_percent = 0;
    }
  }
  void set_MinMaxDistance(bool i) {
    digitalWrite(Pins.Trig_pin, LOW);
    delayMicroseconds(2);
    digitalWrite(Pins.Trig_pin, HIGH);
    delayMicroseconds(10);
    digitalWrite(Pins.Trig_pin, LOW);

    State.Duration = pulseIn(Pins.Echo_pin, HIGH);
    State.Distance = State.Duration / 29 / 2;  //It is Measured in centimetrs

    if (State.Duration == 0) {
      Alarm.activated = true;
    } else {
      Alarm.activated = false;
      if (i) State.Max_distance = State.Distance;
      else State.Min_distance = State.Distance;
    }
  }
  void set_Enable(bool state) {
    Control.Enable = state;
    set_Distance();
  }
  void set_Update_time(unsigned long value) {
    if ((Time.Max_Update_time >= value) && (Time.Min_Update_time <= value)) Time.Update_time = value;
    else if (Time.Max_Update_time < value) Time.Update_time = Time.Max_Update_time;
    else if (Time.Min_Update_time > value) Time.Update_time = Time.Min_Update_time;
  }
  bool get_Enable() {
    return Control.Enable;
  }
  int get_Distance() {
    return State.Distance;
  }
  int get_Max_distance() {
    return State.Max_distance;
  }
  int get_Min_distance() {
    return State.Min_distance;
  }
  int get_Capacity_in_percent() {
    return State.Capacity_in_percent;
  }
  unsigned long get_Update_time() {
    return Time.Update_time;
  }
  bool get_Measured() {
    if (State.Measured == true) {
      State.Measured = false;
      return true;
    } else return false;
  }

  bool get_Alarm_activated() {
    return Alarm.activated;
  }
  String get_Alarm_message() {
    return Alarm.message->get_Message();
  }
};
