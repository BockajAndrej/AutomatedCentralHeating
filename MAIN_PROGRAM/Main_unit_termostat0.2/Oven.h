
class Oven {
private:
  Time* real_time;
  Flash* flash;

  uint8_t ID;

  struct control {
    bool NO_contact;
    struct PARAMETERS {
      bool Enable_heating;
      double Required_T;
      double Positive_tolerance;
      double Negative_tolerance;
      int Heating_interval;
      struct TOLERANCE {
        double RequiredT_UP;
        double RequiredT_LOW;
        double Positive_tolerance_UP;
        double Positive_tolerance_LOW;
        double Negative_tolerance_UP;
        double Negative_tolerance_LOW;
      };
      TOLERANCE Tolerance;
    };
    PARAMETERS Parameters;
    struct PARAMETERS_OFF {
      bool Enable_heating;
      double Required_T;
      int Positive_tolerance;
      int Negative_tolerance;
      struct TOLERANCE {
        double RequiredT_UP;
        double RequiredT_LOW;
        int Positive_tolerance_UP;
        int Positive_tolerance_LOW;
        int Negative_tolerance_UP;
        int Negative_tolerance_LOW;
      };
      TOLERANCE Tolerance;
    };
    PARAMETERS_OFF Parameters_off;
    enum MODE {
      OFF = 0,
      ON = 1,
      Auto = 2
    };
    MODE Mode;
    struct PID_struct {
      double Proportional_ERROR;  //Odchylka
      double Proportional_ERROR_prev;
      double Proportional_constant;
      double Integral_ERROR;
      double Integral_constant;  //Rýchlosť zmeny veľkosti integračnej zložky v čase je daná veľkosťou konštanty
      double Integral_offset;    //Zacne reagovat len ak bude 1K od PT
      double Derivate_ERROR;
      int Derivate_time_ERROR;
      unsigned int Derivate_time;
      unsigned int Derivate_time_prev;
      int Derivete_constant;
      int Result;
      TON falling_timer = TON(60000);
      TON rising_timer = TON(60000);
      enum PID_MODE {
        off = 0,
        Low = 1,
        Medium = 2,
        High = 3
      };
      PID_MODE PID_Mode;
    };
    PID_struct PID;
  };
  control Control;

  struct state {
    double Temperature;
    double Humidity;
    bool Heating;
    bool Heating_prev;
    struct OVERSHOT {
      TON Checking_timer = TON(1000);
      unsigned int Heating;  //Time
      unsigned int Cooling;
      unsigned int Heating_raw;  //Time
      unsigned int Cooling_raw;
    };
    OVERSHOT Overshot;
  };
  state State;

  struct Pins_struct {
    uint8_t Rele;
  };
  Pins_struct Pins;

  struct warnings {
    double Upper_limit;
    double Lower_limit;
  };
  warnings Warnings;
public:
  Oven(uint8_t ID, Time* real_time, Flash* flash) {
    this->real_time = real_time;
    this->flash = flash;

    this->ID = ID;

    Control.NO_contact = true;
    Control.Parameters.Required_T = 22.0;
    Control.Parameters.Positive_tolerance = 0.5;
    Control.Parameters.Negative_tolerance = 0.5;
    Control.Parameters.Enable_heating = true;
    Control.Parameters.Heating_interval = 31;
    Control.Parameters.Tolerance.RequiredT_UP = 40.0;
    Control.Parameters.Tolerance.RequiredT_LOW = 10.0;
    Control.Parameters.Tolerance.Positive_tolerance_UP = 5.0;
    Control.Parameters.Tolerance.Positive_tolerance_LOW = 0.1;
    Control.Parameters.Tolerance.Negative_tolerance_UP = 5.0;
    Control.Parameters.Tolerance.Negative_tolerance_LOW = 0.1;

    Control.Parameters_off.Required_T = 15.0;
    Control.Parameters_off.Positive_tolerance = 3.0;
    Control.Parameters_off.Negative_tolerance = 3.0;
    Control.Parameters_off.Enable_heating = true;
    Control.Parameters_off.Tolerance.RequiredT_UP = 35.0;
    Control.Parameters_off.Tolerance.RequiredT_LOW = 5.0;
    Control.Parameters_off.Tolerance.Positive_tolerance_UP = 5;
    Control.Parameters_off.Tolerance.Positive_tolerance_LOW = 1;
    Control.Parameters_off.Tolerance.Negative_tolerance_UP = 5;
    Control.Parameters_off.Tolerance.Negative_tolerance_LOW = 1;

    Control.Mode = Control.Auto;

    Control.PID.Proportional_ERROR = 0;
    Control.PID.Proportional_ERROR_prev = 0;
    Control.PID.Proportional_constant = 3000;
    Control.PID.Integral_ERROR = 0;
    Control.PID.Integral_constant = 3000;
    Control.PID.Integral_offset = 1;
    Control.PID.Derivate_ERROR = 0;
    Control.PID.Derivate_time_ERROR = 0;
    Control.PID.Derivate_time = 0;
    Control.PID.Derivate_time_prev = 0;
    Control.PID.Derivete_constant = 3000;
    Control.PID.Result = 0;
    Control.PID.PID_Mode = Control.PID.Low;

    State.Overshot.Heating_raw = 0;
    State.Overshot.Cooling_raw = 0;
    State.Overshot.Heating = 0;
    State.Overshot.Cooling = 0;

    State.Temperature = 99.9;
    State.Humidity = 99.9;
    State.Heating = false;
    State.Heating_prev = false;

    Warnings.Upper_limit = 25.0;
    Warnings.Lower_limit = 19.0;
  }
  void Initialization() {
    if (Control.NO_contact) {
      if (State.Heating) digitalWrite(Pins.Rele, HIGH);
      else digitalWrite(Pins.Rele, LOW);
    } else {
      if (State.Heating == false) digitalWrite(Pins.Rele, HIGH);
      else digitalWrite(Pins.Rele, LOW);
    }
  }
  void Setup(uint8_t pin) {
    Pins.Rele = pin;
    pinMode(pin, OUTPUT);

    if (ID == 1) {

      State.Overshot.Heating = (flash->get_Flash(2) * 100) + flash->get_Flash(3);
      State.Overshot.Cooling = (flash->get_Flash(4) * 100) + flash->get_Flash(5);

      Control.Parameters.Enable_heating = flash->get_Flash(10);

      Control.Parameters.Required_T = flash->get_Flash(11);
      Control.Parameters.Required_T = Control.Parameters.Required_T + (((double)flash->get_Flash(12)) / 10);
      Control.Parameters.Positive_tolerance = flash->get_Flash(13);
      Control.Parameters.Positive_tolerance = Control.Parameters.Positive_tolerance + (((double)flash->get_Flash(14)) / 10);
      Control.Parameters.Negative_tolerance = flash->get_Flash(15);
      Control.Parameters.Negative_tolerance = Control.Parameters.Negative_tolerance + (((double)flash->get_Flash(16)) / 10);

      Control.Mode = (Oven::control::MODE)flash->get_Flash(17);
      Control.PID.PID_Mode = (Oven::control::PID_struct::PID_MODE)flash->get_Flash(19);

      Control.Parameters_off.Enable_heating = flash->get_Flash(20);
      Control.Parameters_off.Required_T = flash->get_Flash(21);
      Control.Parameters_off.Required_T =  Control.Parameters_off.Required_T + (((double)flash->get_Flash(22)) / 10);
      Control.Parameters_off.Positive_tolerance = flash->get_Flash(23);
      Control.Parameters_off.Negative_tolerance = flash->get_Flash(24);
      Control.NO_contact = flash->get_Flash(25);

      Control.Parameters.Heating_interval = flash->get_Flash(26);

      Warnings.Upper_limit = flash->get_Flash(27);
      Warnings.Lower_limit = flash->get_Flash(28);
      
    } else if (ID == 2) {
      State.Overshot.Heating = (flash->get_Flash(6) * 100) + flash->get_Flash(7);
      State.Overshot.Cooling = (flash->get_Flash(8) * 100) + flash->get_Flash(9);

      Control.Parameters.Enable_heating = flash->get_Flash(30);

      Control.Parameters.Required_T = flash->get_Flash(31);
      Control.Parameters.Required_T = Control.Parameters.Required_T + (((double)flash->get_Flash(32)) / 10);
      Control.Parameters.Positive_tolerance = flash->get_Flash(33);
      Control.Parameters.Positive_tolerance = Control.Parameters.Positive_tolerance + (((double)flash->get_Flash(34)) / 10);
      Control.Parameters.Negative_tolerance = flash->get_Flash(35);
      Control.Parameters.Negative_tolerance = Control.Parameters.Negative_tolerance + (((double)flash->get_Flash(36)) / 10);

      Control.Mode = (Oven::control::MODE)flash->get_Flash(37);
      Control.PID.PID_Mode = (Oven::control::PID_struct::PID_MODE)flash->get_Flash(39);

      Control.Parameters_off.Enable_heating = flash->get_Flash(40);
      Control.Parameters_off.Required_T = flash->get_Flash(41);
      Control.Parameters_off.Required_T =  Control.Parameters_off.Required_T + (((double)flash->get_Flash(42)) / 10);
      Control.Parameters_off.Positive_tolerance = flash->get_Flash(43);
      Control.Parameters_off.Negative_tolerance = flash->get_Flash(46);
      Control.NO_contact = flash->get_Flash(47);

      Control.Parameters.Heating_interval = flash->get_Flash(48);
  
      Warnings.Upper_limit = flash->get_Flash(29);
      Warnings.Lower_limit = flash->get_Flash(54);
    }
  }
  void Reset(){
    Control.NO_contact = true;
    Control.Parameters.Required_T = 22.0;
    Control.Parameters.Positive_tolerance = 0.5;
    Control.Parameters.Negative_tolerance = 0.5;
    Control.Parameters.Enable_heating = true;
    Control.Parameters.Heating_interval = 31;
    Control.Parameters.Tolerance.RequiredT_UP = 40.0;
    Control.Parameters.Tolerance.RequiredT_LOW = 10.0;
    Control.Parameters.Tolerance.Positive_tolerance_UP = 5.0;
    Control.Parameters.Tolerance.Positive_tolerance_LOW = 0.1;
    Control.Parameters.Tolerance.Negative_tolerance_UP = 5.0;
    Control.Parameters.Tolerance.Negative_tolerance_LOW = 0.1;

    Control.Parameters_off.Required_T = 15.0;
    Control.Parameters_off.Positive_tolerance = 3.0;
    Control.Parameters_off.Negative_tolerance = 3.0;
    Control.Parameters_off.Enable_heating = true;
    Control.Parameters_off.Tolerance.RequiredT_UP = 35.0;
    Control.Parameters_off.Tolerance.RequiredT_LOW = 5.0;
    Control.Parameters_off.Tolerance.Positive_tolerance_UP = 5;
    Control.Parameters_off.Tolerance.Positive_tolerance_LOW = 1;
    Control.Parameters_off.Tolerance.Negative_tolerance_UP = 5;
    Control.Parameters_off.Tolerance.Negative_tolerance_LOW = 1;

    Control.Mode = Control.Auto;

    Control.PID.Proportional_ERROR = 0;
    Control.PID.Proportional_ERROR_prev = 0;
    Control.PID.Proportional_constant = 3000;
    Control.PID.Integral_ERROR = 0;
    Control.PID.Integral_constant = 3000;
    Control.PID.Integral_offset = 1;
    Control.PID.Derivate_ERROR = 0;
    Control.PID.Derivate_time_ERROR = 0;
    Control.PID.Derivate_time = 0;
    Control.PID.Derivate_time_prev = 0;
    Control.PID.Derivete_constant = 3000;
    Control.PID.Result = 0;
    Control.PID.PID_Mode = Control.PID.Low;

    State.Overshot.Heating_raw = 0;
    State.Overshot.Cooling_raw = 0;
    State.Overshot.Heating = 0;
    State.Overshot.Cooling = 0;

    State.Heating = false;
    State.Heating_prev = false;

    Warnings.Upper_limit = 25.0;
    Warnings.Lower_limit = 19.0;
  }

  bool get_Enable_heating() {
    return Control.Parameters.Enable_heating;
  }
  double get_Required_T() {
    return Control.Parameters.Required_T;
  }
  double get_Positive_tolerance() {
    return Control.Parameters.Positive_tolerance;
  }
  double get_Negative_tolerance() {
    return Control.Parameters.Negative_tolerance;
  }
  bool get_Enable_heating_off() {
    return Control.Parameters_off.Enable_heating;
  }
  double get_Required_T_off() {
    return Control.Parameters_off.Required_T;
  }
  int get_Positive_tolerance_off() {
    return Control.Parameters_off.Positive_tolerance;
  }
  int get_Negative_tolerance_off() {
    return Control.Parameters_off.Negative_tolerance;
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
  short get_Mode() {
    return Control.Mode;
  }
  double get_Upper_limit() {
    return Warnings.Upper_limit;
  }
  double get_Lower_limit() {
    return Warnings.Lower_limit;
  }
  short get_PID_Mode() {
    return Control.PID.PID_Mode;
  }
  int get_Heating_Interval() {
    return Control.Parameters.Heating_interval;
  }
  bool get_NO_contact() {
    return Control.NO_contact;
  }
  unsigned int get_Overshot_Cooling_time() {
    return State.Overshot.Cooling;
  }
  unsigned int get_Overshot_Heating_time() {
    return State.Overshot.Heating;
  }

  void set_Enable_heating(bool value) {
    Control.Parameters.Enable_heating = value;
  }
  void set_Enable_heating_off(bool value) {
    Control.Parameters_off.Enable_heating = value;
  }
  void set_Mode(uint8_t i) {
    if (i == 0) Control.Mode = Control.OFF;
    else if (i == 1) Control.Mode = Control.ON;
    else if (i == 2) Control.Mode = Control.Auto;
  }
  void set_Required_T(double value) {
    if ((value <= Control.Parameters.Tolerance.RequiredT_UP) && (value >= Control.Parameters.Tolerance.RequiredT_LOW)) Control.Parameters.Required_T = value;
    else if (value > Control.Parameters.Tolerance.RequiredT_UP) Control.Parameters.Required_T = Control.Parameters.Tolerance.RequiredT_UP;
    else if (value > Control.Parameters.Tolerance.RequiredT_LOW) Control.Parameters.Required_T = Control.Parameters.Tolerance.RequiredT_LOW;
  }
  void set_Positive_tolerance(double value) {
    if ((value <= Control.Parameters.Tolerance.Positive_tolerance_UP) && (value >= Control.Parameters.Tolerance.Positive_tolerance_LOW)) Control.Parameters.Positive_tolerance = value;
    else if (value > Control.Parameters.Tolerance.Positive_tolerance_UP) Control.Parameters.Positive_tolerance = Control.Parameters.Tolerance.Positive_tolerance_UP;
    else if (value < Control.Parameters.Tolerance.Positive_tolerance_LOW) Control.Parameters.Positive_tolerance = Control.Parameters.Tolerance.Positive_tolerance_LOW;
  }
  void set_Negative_tolerance(double value) {
    if ((value <= Control.Parameters.Tolerance.Negative_tolerance_UP) && (value >= Control.Parameters.Tolerance.Negative_tolerance_LOW)) Control.Parameters.Negative_tolerance = value;
    else if (value > Control.Parameters.Tolerance.Negative_tolerance_UP) Control.Parameters.Negative_tolerance = Control.Parameters.Tolerance.Negative_tolerance_UP;
    else if (value < Control.Parameters.Tolerance.Negative_tolerance_LOW) Control.Parameters.Negative_tolerance = Control.Parameters.Tolerance.Negative_tolerance_LOW;
  }
  void set_Required_T_off(double value) {
    if ((value <= Control.Parameters_off.Tolerance.RequiredT_UP) && (value >= Control.Parameters_off.Tolerance.RequiredT_LOW)) Control.Parameters_off.Required_T = value;
    else if (value > Control.Parameters_off.Tolerance.RequiredT_UP) Control.Parameters_off.Required_T = Control.Parameters_off.Tolerance.RequiredT_UP;
    else if (value > Control.Parameters_off.Tolerance.RequiredT_LOW) Control.Parameters_off.Required_T = Control.Parameters_off.Tolerance.RequiredT_LOW;
  }
  void set_Positive_tolerance_off(double value) {
    if ((value <= Control.Parameters_off.Tolerance.Positive_tolerance_UP) && (value >= Control.Parameters_off.Tolerance.Positive_tolerance_LOW)) Control.Parameters_off.Positive_tolerance = value;
    else if (value > Control.Parameters_off.Tolerance.Positive_tolerance_UP) Control.Parameters_off.Positive_tolerance = Control.Parameters_off.Tolerance.Positive_tolerance_UP;
    else if (value < Control.Parameters_off.Tolerance.Positive_tolerance_LOW) Control.Parameters_off.Positive_tolerance = Control.Parameters_off.Tolerance.Positive_tolerance_LOW;
  }
  void set_Negative_tolerance_off(double value) {
    if ((value <= Control.Parameters_off.Tolerance.Negative_tolerance_UP) && (value >= Control.Parameters_off.Tolerance.Negative_tolerance_LOW)) Control.Parameters_off.Negative_tolerance = value;
    else if (value > Control.Parameters_off.Tolerance.Negative_tolerance_UP) Control.Parameters_off.Negative_tolerance = Control.Parameters_off.Tolerance.Negative_tolerance_UP;
    else if (value < Control.Parameters_off.Tolerance.Negative_tolerance_LOW) Control.Parameters_off.Negative_tolerance = Control.Parameters_off.Tolerance.Negative_tolerance_LOW;
  }
  void set_Temp_and_Hum(double T, double H) {
    State.Temperature = T;
    State.Humidity = H;

    if ((Control.PID.Result == 0)) {
      //*10 - for avoid double type of number 
      Control.PID.Proportional_ERROR = (int)((Control.Parameters.Required_T - State.Temperature) * 10);
      Control.PID.Proportional_ERROR_prev = Control.PID.Proportional_ERROR;
      Control.PID.Derivate_time = real_time->get_Real_time();
      Control.PID.Derivate_time_prev = real_time->get_Real_time();
    } else if (Control.PID.Result == 2) {
      Control.PID.Proportional_ERROR = (int)(((Control.Parameters.Required_T + Control.Parameters.Positive_tolerance) - State.Temperature) * 10);
    } else if (Control.PID.Result == 1) {
      Control.PID.Proportional_ERROR = (int)(((Control.Parameters.Required_T - Control.Parameters.Negative_tolerance) - State.Temperature) * 10);
    }

    if (Control.PID.Proportional_ERROR_prev != Control.PID.Proportional_ERROR) {
      Control.PID.Derivate_ERROR = Control.PID.Proportional_ERROR_prev - Control.PID.Proportional_ERROR;  //Odchylka oproti minulej odchylke
      Serial.println("Derivate Error = " + String(Control.PID.Derivate_ERROR));
      Control.PID.Proportional_ERROR_prev = Control.PID.Proportional_ERROR;
      Control.PID.Derivate_time = real_time->get_Real_time();
      if (Control.PID.Derivate_time_prev != Control.PID.Derivate_time) {
        Control.PID.Derivate_time_ERROR = Control.PID.Derivate_time - Control.PID.Derivate_time_prev;
        Serial.println("Derivate time Error = " + String(Control.PID.Derivate_time_ERROR));
        Control.PID.Derivate_time_prev = Control.PID.Derivate_time;
      }
    }

    //if (((Control.PID.Integral_offset * 10 >= Control.PID.Proportional_ERROR) && (Control.PID.Proportional_ERROR >= 0)) || ((-Control.PID.Integral_offset * 10 <= Control.PID.Proportional_ERROR) && (Control.PID.Proportional_ERROR < 0))) Control.PID.Integral_ERROR += Control.PID.Proportional_ERROR;
    //else Control.PID.Integral_ERROR = 0;
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
    if (i == 0) Control.PID.PID_Mode = Control.PID.off;
    else if (i == 1) Control.PID.PID_Mode = Control.PID.Low;
    else if (i == 2) Control.PID.PID_Mode = Control.PID.Medium;
    else if (i == 3) Control.PID.PID_Mode = Control.PID.High;
  }
  void set_Heating(bool value) {
    State.Heating = value;
  }
  void set_Heating_Interval(int value) {
    if ((value >= 1) && (value <= 31)) Control.Parameters.Heating_interval = value;
    else if (value < 1) Control.Parameters.Heating_interval = 31;
    else if (value > 31) Control.Parameters.Heating_interval = 1;
  }
  void set_NO_contact(bool value) {
    Control.NO_contact = value;
  }
  void set_Overshot_Cooling_time(int value) {
    State.Overshot.Cooling = value;
  }
  void set_Overshot_Heating_time(int value) {
    State.Overshot.Heating = value;
  }

public:
  void Oven_Start() {
    if (Control.Parameters.Enable_heating) {
      if (Control.Mode == Control.OFF) {  //STOPED
        State.Heating = false;
      } else if (Control.Mode == Control.ON) {  //OPEN
        if (real_time->get_Permission_for_time_open_mode()) {
          if (Control.PID.PID_Mode != Control.PID.off) Oven_PID(true);
          else if (State.Temperature >= Control.Parameters.Required_T + Control.Parameters.Positive_tolerance) State.Heating = false;
          else if (State.Temperature <= Control.Parameters.Required_T - Control.Parameters.Negative_tolerance) State.Heating = true;
        } else State.Heating = false;
      } else if (Control.Mode == Control.Auto) {  //AUTO
        if (real_time->get_Permission_Heating(Control.Parameters.Heating_interval)) {
          if (Control.PID.PID_Mode != Control.PID.off) Oven_PID(true);
          else if (State.Temperature >= Control.Parameters.Required_T + Control.Parameters.Positive_tolerance) State.Heating = false;
          else if (State.Temperature <= Control.Parameters.Required_T - Control.Parameters.Negative_tolerance) State.Heating = true;
        } else if (Control.Parameters_off.Enable_heating) {
          if (Control.PID.PID_Mode != Control.PID.off) Oven_PID(false);
          else if (State.Temperature >= Control.Parameters_off.Required_T + Control.Parameters_off.Positive_tolerance) State.Heating = false;
          else if (State.Temperature <= Control.Parameters_off.Required_T - Control.Parameters_off.Negative_tolerance) State.Heating = true;
        } else State.Heating = false;
      } else State.Heating = false;
    }else{
      State.Heating = false;
    }

    if (State.Heating_prev != State.Heating) {
      if (State.Heating) {
        if (State.Temperature <= (Control.Parameters.Required_T - Control.Parameters.Negative_tolerance)) {
          if (State.Overshot.Checking_timer.Timer(true)) State.Overshot.Heating_raw += 1;
        } else {
          State.Overshot.Checking_timer.Timer(false);
          if ((State.Overshot.Heating_raw != State.Overshot.Heating) && (State.Temperature > (Control.Parameters.Required_T - Control.Parameters.Negative_tolerance))) {
            Serial.println("Heating time = " + String(State.Overshot.Heating_raw));
            State.Overshot.Heating = State.Overshot.Heating_raw;
            State.Overshot.Heating_raw = 0;
          }
        }
      } else {
        if (State.Temperature >= (Control.Parameters.Required_T + Control.Parameters.Positive_tolerance)) {
          if (State.Overshot.Checking_timer.Timer(true)) State.Overshot.Cooling_raw += 1;
        } else {
          State.Overshot.Checking_timer.Timer(false);
          if ((State.Overshot.Cooling_raw != State.Overshot.Cooling) && (State.Temperature < (Control.Parameters.Required_T + Control.Parameters.Positive_tolerance)) && (State.Overshot.Heating > 0)) {
            Serial.println("Cooling time = " + String(State.Overshot.Cooling_raw));
            State.Overshot.Cooling = State.Overshot.Cooling_raw;
            State.Overshot.Cooling_raw = 0;
          }
        }
      }
      State.Heating_prev = State.Heating;
    }
  }
  void Oven_PID(bool state) {
    if (state) {
      if (Control.PID.Result == 0) {
        Control.PID.falling_timer.Timer(false);
        Control.PID.rising_timer.Timer(false);
        if (State.Temperature >= Control.Parameters.Required_T + Control.Parameters.Positive_tolerance) {
          State.Heating = false;
          Control.PID.Result = 1;
        } else if (State.Temperature <= Control.Parameters.Required_T - Control.Parameters.Negative_tolerance) {
          State.Heating = true;
          Control.PID.Result = 2;
        }
      } else if (Control.PID.Result == 2) {
        //Serial.println("Heating time = " + String(State.Overshot.Cooling));
        Control.PID.rising_timer.Timer(false);
        if ((State.Temperature * 10 + Control.PID.Derivate_ERROR) >= (Control.Parameters.Required_T * 10 + Control.Parameters.Positive_tolerance * 10)) {
          if (State.Overshot.Cooling > 0) {
            if ((((Control.PID.Derivate_time_ERROR / Control.PID.Derivate_ERROR) * ((Control.Parameters.Required_T * 10 + Control.Parameters.Positive_tolerance * 10) - State.Temperature * 10)) - (State.Overshot.Cooling / 2)) > 0) {
              Serial.println("Time to OFF = " + String((((Control.PID.Derivate_time_ERROR / Control.PID.Derivate_ERROR) * ((Control.Parameters.Required_T * 10 + Control.Parameters.Positive_tolerance * 10) - State.Temperature * 10)) - (State.Overshot.Cooling / 2)) * 1000));
              Control.PID.falling_timer.set_Time_LOOP_constant((((Control.PID.Derivate_time_ERROR / Control.PID.Derivate_ERROR) * ((Control.Parameters.Required_T * 10 + Control.Parameters.Positive_tolerance * 10) - State.Temperature * 10)) - (State.Overshot.Cooling / 2)) * 1000);
            } else Control.PID.Result = 1;
            if (Control.PID.falling_timer.Timer(true)) Control.PID.Result = 1;
          } else {
            if (State.Temperature >= Control.Parameters.Required_T + Control.Parameters.Positive_tolerance) {
              Control.PID.Result = 1;
            } else if (State.Temperature <= Control.Parameters.Required_T - Control.Parameters.Negative_tolerance) {
              State.Heating = true;
            }
          }
        } else State.Heating = true;
      } else if (Control.PID.Result == 1) {
        Control.PID.falling_timer.Timer(false);
        if ((State.Temperature * 10 + Control.PID.Derivate_ERROR) <= (Control.Parameters.Required_T * 10 - Control.Parameters.Negative_tolerance * 10)) {
          if (State.Overshot.Heating > 0) {
            if ((((Control.PID.Derivate_time_ERROR / Control.PID.Derivate_ERROR) * ((Control.Parameters.Required_T * 10 - Control.Parameters.Negative_tolerance * 10) - State.Temperature * 10)) - (State.Overshot.Heating / 2)) > 0) {
              Serial.println("Time to ON = " + String((((Control.PID.Derivate_time_ERROR / Control.PID.Derivate_ERROR) * ((Control.Parameters.Required_T * 10 - Control.Parameters.Negative_tolerance * 10) - State.Temperature * 10)) - (State.Overshot.Heating / 2)) * 1000));
              Control.PID.rising_timer.set_Time_LOOP_constant((((Control.PID.Derivate_time_ERROR / Control.PID.Derivate_ERROR) * ((Control.Parameters.Required_T * 10 - Control.Parameters.Negative_tolerance * 10) - State.Temperature * 10)) - (State.Overshot.Heating / 2)) * 1000);
            } else Control.PID.Result = 2;
            if (Control.PID.rising_timer.Timer(true)) Control.PID.Result = 2;
          } else {
            if (State.Temperature >= Control.Parameters.Required_T + Control.Parameters.Positive_tolerance) {
              State.Heating = false;
            } else if (State.Temperature <= Control.Parameters.Required_T - Control.Parameters.Negative_tolerance) {
              Control.PID.Result = 2;
            }
          }
        } else State.Heating = false;
      }
    } else {
      if (Control.PID.Result == 0) {
        Control.PID.falling_timer.Timer(false);
        Control.PID.rising_timer.Timer(false);
        if (State.Temperature >= Control.Parameters_off.Required_T + Control.Parameters_off.Positive_tolerance) {
          State.Heating = false;
          Control.PID.Result = 1;
        } else if (State.Temperature <= Control.Parameters_off.Required_T - Control.Parameters_off.Negative_tolerance) {
          State.Heating = true;
          Control.PID.Result = 2;
        }
      } else if (Control.PID.Result == 2) {
        //Serial.println("Heating time = " + String(State.Overshot.Cooling));
        Control.PID.rising_timer.Timer(false);
        if ((State.Temperature * 10 + Control.PID.Derivate_ERROR) >= (Control.Parameters_off.Required_T * 10 + Control.Parameters_off.Positive_tolerance * 10)) {
          if (State.Overshot.Cooling > 0) {
            if (((((Control.PID.Derivate_time_ERROR / Control.PID.Derivate_ERROR) * ((Control.Parameters_off.Required_T * 10 + Control.Parameters_off.Positive_tolerance * 10) - State.Temperature * 10)) - (State.Overshot.Cooling / 2)) > 0) && (Control.PID.PID_Mode == Control.PID.Low)) {
              Control.PID.falling_timer.set_Time_LOOP_constant((((Control.PID.Derivate_time_ERROR / Control.PID.Derivate_ERROR) * ((Control.Parameters_off.Required_T * 10 + Control.Parameters_off.Positive_tolerance * 10) - State.Temperature * 10)) - (State.Overshot.Cooling / 2)) * 1000);
            } 
            else if (((((Control.PID.Derivate_time_ERROR / Control.PID.Derivate_ERROR) * ((Control.Parameters_off.Required_T * 10 + Control.Parameters_off.Positive_tolerance * 10) - State.Temperature * 10)) - (State.Overshot.Cooling / 2.5)) > 0) && (Control.PID.PID_Mode == Control.PID.Medium)) {
              Control.PID.falling_timer.set_Time_LOOP_constant((((Control.PID.Derivate_time_ERROR / Control.PID.Derivate_ERROR) * ((Control.Parameters_off.Required_T * 10 + Control.Parameters_off.Positive_tolerance * 10) - State.Temperature * 10)) - (State.Overshot.Cooling / 2.5)) * 1000);
            }
            else if (((((Control.PID.Derivate_time_ERROR / Control.PID.Derivate_ERROR) * ((Control.Parameters_off.Required_T * 10 + Control.Parameters_off.Positive_tolerance * 10) - State.Temperature * 10)) - (State.Overshot.Cooling / 3)) > 0) && (Control.PID.PID_Mode == Control.PID.High)) {
              Control.PID.falling_timer.set_Time_LOOP_constant((((Control.PID.Derivate_time_ERROR / Control.PID.Derivate_ERROR) * ((Control.Parameters_off.Required_T * 10 + Control.Parameters_off.Positive_tolerance * 10) - State.Temperature * 10)) - (State.Overshot.Cooling / 3)) * 1000);
            }
            else Control.PID.Result = 1;
            if (Control.PID.falling_timer.Timer(true)) Control.PID.Result = 1;
          } else {
            if (State.Temperature >= Control.Parameters_off.Required_T + Control.Parameters_off.Positive_tolerance) {
              Control.PID.Result = 1;
            } else if (State.Temperature <= Control.Parameters_off.Required_T - Control.Parameters_off.Negative_tolerance) {
              State.Heating = true;
            }
          }
        } else State.Heating = true;
      } else if (Control.PID.Result == 1) {
        Control.PID.falling_timer.Timer(false);
        if ((State.Temperature * 10 + Control.PID.Derivate_ERROR) <= (Control.Parameters_off.Required_T * 10 - Control.Parameters_off.Negative_tolerance * 10)) {
          if (State.Overshot.Heating > 0) {
            if (((((Control.PID.Derivate_time_ERROR / Control.PID.Derivate_ERROR) * ((Control.Parameters_off.Required_T * 10 - Control.Parameters_off.Negative_tolerance * 10) - State.Temperature * 10)) - (State.Overshot.Heating / 2)) > 0) && (Control.PID.PID_Mode == Control.PID.Low)) {
              Control.PID.rising_timer.set_Time_LOOP_constant((((Control.PID.Derivate_time_ERROR / Control.PID.Derivate_ERROR) * ((Control.Parameters_off.Required_T * 10 - Control.Parameters_off.Negative_tolerance * 10) - State.Temperature * 10)) - (State.Overshot.Heating / 2)) * 1000);
            } 
            else if (((((Control.PID.Derivate_time_ERROR / Control.PID.Derivate_ERROR) * ((Control.Parameters_off.Required_T * 10 - Control.Parameters_off.Negative_tolerance * 10) - State.Temperature * 10)) - (State.Overshot.Heating / 2.5)) > 0) && (Control.PID.PID_Mode == Control.PID.Low)) {
              Control.PID.rising_timer.set_Time_LOOP_constant((((Control.PID.Derivate_time_ERROR / Control.PID.Derivate_ERROR) * ((Control.Parameters_off.Required_T * 10 - Control.Parameters_off.Negative_tolerance * 10) - State.Temperature * 10)) - (State.Overshot.Heating / 2.5)) * 1000);
            } 
            else if (((((Control.PID.Derivate_time_ERROR / Control.PID.Derivate_ERROR) * ((Control.Parameters_off.Required_T * 10 - Control.Parameters_off.Negative_tolerance * 10) - State.Temperature * 10)) - (State.Overshot.Heating / 3)) > 0) && (Control.PID.PID_Mode == Control.PID.Low)) {
              Control.PID.rising_timer.set_Time_LOOP_constant((((Control.PID.Derivate_time_ERROR / Control.PID.Derivate_ERROR) * ((Control.Parameters_off.Required_T * 10 - Control.Parameters_off.Negative_tolerance * 10) - State.Temperature * 10)) - (State.Overshot.Heating / 3)) * 1000);
            } 
            else Control.PID.Result = 2;
            if (Control.PID.rising_timer.Timer(true)) Control.PID.Result = 2;
          } else {
            if (State.Temperature >= Control.Parameters_off.Required_T + Control.Parameters_off.Positive_tolerance) {
              State.Heating = false;
            } else if (State.Temperature <= Control.Parameters_off.Required_T - Control.Parameters_off.Negative_tolerance) {
              Control.PID.Result = 2;
            }
          }
        } else State.Heating = false;
      }
    }
  }
  void Oven_Stop() {
    State.Heating = false;
  }
  bool Oven_Reset() {
    Control.Parameters.Required_T = 22.0;
    Control.Parameters.Positive_tolerance = 1.0;
    Control.Parameters.Negative_tolerance = 1.0;
    Control.Mode = Control.Auto;

    State.Temperature = 0.0;
    State.Heating = false;
  }
};
