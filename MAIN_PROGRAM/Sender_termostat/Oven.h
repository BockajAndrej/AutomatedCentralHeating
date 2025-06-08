
class Oven {
private:
  int ID;
  Flash *flash;
  struct control {
    double Required_T;
    double Positive_tolerance;
    double Negative_tolerance;
    bool PID_;
    int Heating_interval;
    enum PID_MODE {
      off = 0,
      Low = 1,
      Medium = 2,
      High = 3
    };
    PID_MODE PID_Mode;
    enum MODE {
      OFF = 0,
      ON = 1,
      Auto = 2
    };
    MODE Mode;
    bool Enable_heating;
  };
  control Control;

  struct state {
    double Temperature;
    double Humidity;
    bool Heating;
  };
  state State;

  struct warnings {
    double Upper_limit;
    double Lower_limit;
  };
  warnings Warnings;

  struct Pins_struct {
    uint8_t Rele;
  };
  Pins_struct Pins;

public:
  Oven(int value, Flash *flash) {
    ID = value;
    this->flash = flash;

    Control.Required_T = 22.0;
    Control.Positive_tolerance = 1.0;
    Control.Negative_tolerance = 1.0;
    Control.Mode = Control.Auto;
    Control.Enable_heating = true;
    Control.PID_Mode = Control.Low;
    Control.Heating_interval = 3;

    State.Temperature = 0.0;
    State.Humidity = 0.0;
    State.Heating = false;

    Warnings.Upper_limit = 25.0;
    Warnings.Lower_limit = 19.0;
  }
  void Setup(){
    if(ID == 1){
      Warnings.Upper_limit = flash->get_Flash(25);
      Warnings.Lower_limit = flash->get_Flash(26);
    }else if(ID == 2){
      Warnings.Upper_limit = flash->get_Flash(27);
      Warnings.Lower_limit = flash->get_Flash(28);
    }
  }

  double get_Required_T() {
    return Control.Required_T;
  }
  double get_Positive_tolerance() {
    return Control.Positive_tolerance;
  }
  double get_Negative_tolerance() {
    return Control.Negative_tolerance;
  }
  double get_Temperature() {
    return State.Temperature;
  }
  double get_Humidity() {
    return State.Humidity;
  }
  bool get_Heating() {
    return State.Heating;
  }
  bool get_Enable_heating() {
    return Control.Enable_heating;
  }
  short get_Mode() {
    return Control.Mode;
  }
  bool get_PID() {
    return Control.PID_;
  }
  double get_Upper_limit() {
    return Warnings.Upper_limit;
  }
  double get_Lower_limit() {
    return Warnings.Lower_limit;
  }
  short get_PID_Mode() {
    return Control.PID_Mode;
  }
  int get_Heating_Interval() {
    return Control.Heating_interval;
  }

  void set_Enable_heating(bool state) {
    Control.Enable_heating = state;
  }
  void set_Mode(uint8_t i) {
    if (i == 0) Control.Mode = Control.OFF;
    else if (i == 1) Control.Mode = Control.ON;
    else if (i == 2) Control.Mode = Control.Auto;
  }
  void set_Required_T(double value) {
    if ((value <= 35) && (value >= 5)) Control.Required_T = value;
  }
  void set_Positive_tolerance(double value) {
    if ((value <= 5) && (value >= 0.1)) Control.Positive_tolerance = value;
  }
  void set_Negative_tolerance(double value) {
    if ((value <= 5) && (value >= 0.1)) Control.Negative_tolerance = value;
  }
  void set_PID(bool value) {
    Control.PID_ = value;
  }
  void set_Temp_and_Hum(double T, double H) {
    State.Temperature = T;
    State.Humidity = H;
  }
  void set_Upper_limit(double value) {
    if ((value <= 50) && (value >= 10)) Warnings.Upper_limit = value;
    else if(value > 50) Warnings.Upper_limit = 50;
    else if(value < 10) Warnings.Upper_limit = 10;
  }
  void set_Lower_limit(double value) {
    if ((value <= 30) && (value >= 0)) Warnings.Lower_limit = value;
    else if(value > 30) Warnings.Lower_limit = 30;
    else if(value < 0) Warnings.Lower_limit = 0;
  }
  void set_PID_Mode(short i) {
    if (i == 0) Control.PID_Mode = Control.off;
    else if (i == 1) Control.PID_Mode = Control.Low;
    else if (i == 2) Control.PID_Mode = Control.Medium;
    else if (i == 3) Control.PID_Mode = Control.High;
  }
  void set_Heating(bool value) {
    State.Heating = value;
  }
  void set_Heating_Interval(int value) {
    if ((value >= 0) && (value <= 3)) Control.Heating_interval = value;
    else if (value < 0) Control.Heating_interval = 3;
    else if (value > 3) Control.Heating_interval = 0;
  }
};
