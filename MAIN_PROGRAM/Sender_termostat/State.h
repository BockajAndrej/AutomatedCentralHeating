class State{
private:
  struct Ultrasonic_struct{
    bool State;
    uint8_t Capacity_in_percent;
  };
  Ultrasonic_struct Ultrasonic;
  
  struct Time_struct{
    bool State;
    uint32_t Real_time;
    uint8_t Year_;
    uint8_t Month_;
    uint8_t Day_;
    uint8_t Hour_;
    uint8_t Minute_;
    uint8_t Second_;
  };
  Time_struct Time;

  struct interval{
    long zap[5] = {0, 0, 0, 0, 0};
    long vyp[5] = {0, 0, 0, 0, 0};
    bool enable[5] = {false, false, false, false, false};
    bool Days[7] = { false, false, false, false, false};
  };
  interval Interval[2];


  struct Termostat_struct{
    bool Comunication;
    bool Synchronization;
  };
  Termostat_struct Termostat;

public:
  State(){
    Ultrasonic.State = 0;
    Ultrasonic.Capacity_in_percent = 0;

    Time.State = 0;
    Time.Real_time = 0;
    Time.Year_ = 0;
    Time.Month_ = 0;
    Time.Day_ = 0;
    Time.Hour_ = 0;
    Time.Minute_ = 0;
    Time.Second_ = 0;

    Termostat.Comunication = false;
  }

  void set_Ultrasonic(bool i, uint8_t value){
    Ultrasonic.State = i;
    Ultrasonic.Capacity_in_percent = value;
  }
  void set_Time(bool State, uint8_t Year_, uint8_t Month_, uint8_t Day_, uint8_t Hour_, uint8_t Minute_, uint8_t Second_){
    Time.State = State;
    Time.Year_ = Year_;
    Time.Month_ = Month_;
    Time.Day_ = Day_;
    Time.Hour_ = Hour_;
    Time.Minute_ = Minute_;
    Time.Second_ = Second_;
  }
  void set_Comunication(bool value){
    Termostat.Comunication = value;
  }
  void set_Synchronization(bool value){
    Termostat.Synchronization = value;
  }
  void set_Interval_heating(int i, bool ON_OFF, int i_2, int value) {
    if (ON_OFF) {
      if (value < 0) Interval[i].zap[i_2] = 1440 + value;
      else if (value > 1439) Interval[i].zap[i_2] = Interval[i].zap[i_2] % 60;
      else Interval[i].zap[i_2] = value;

      if (Interval[i].zap[i_2] > Interval[i].vyp[i_2]) Interval[i].vyp[i_2] = Interval[i].zap[i_2];
    } else {
      if (value < 0) Interval[i].vyp[i_2] = 1440 + value;
      else if (value > 1439) Interval[i].vyp[i_2] = Interval[i].vyp[i_2] % 60;
      else Interval[i].vyp[i_2] = value;

      if (Interval[i].zap[i_2] > Interval[i].vyp[i_2]) Interval[i].zap[i_2] = Interval[i].vyp[i_2];
    }
  }
  void set_Interval_enable(int i, uint8_t i_2, bool value){
    Interval[i].enable[i_2] = value;
  }
  void set_Interval_Days(int i, int i_2, bool value) {
    Interval[i].Days[i_2] = value;
  }
  
  bool get_Ultrasonic_State(){
    return Ultrasonic.State;
  }
  uint8_t get_Ultrasonic_Capacity(){
    return Ultrasonic.Capacity_in_percent;
  }
  bool get_Time_State(){
    return Time.State;
  }
  uint8_t get_Year(){
    return Time.Year_;
  }
  uint8_t get_Month(){
    return Time.Month_;
  }
  uint8_t get_Day(){
    return Time.Day_;
  }
  uint8_t get_Hours(){
    return Time.Hour_;
  }
  uint8_t get_Minutes(){
    return Time.Minute_;
  }
  uint8_t get_Seconds(){
    return Time.Second_;
  }
  bool get_Comunication(){
    return Termostat.Comunication;
  }
  bool get_Synchronization(){
    return Termostat.Synchronization;
  }
  uint32_t get_Interval_heating(int i, bool ON_OFF, uint8_t i_2) {
    if (ON_OFF) {
      return Interval[i].zap[i_2];
    } else {
      return Interval[i].vyp[i_2];
    }
  }
  bool get_Interval_enable(int i, uint8_t i_2){
    return Interval[i].enable[i_2];
  }
  bool get_Interval_Day(int i, int i_2) {
    return Interval[i].Days[i_2];
  }
  bool get_Permission_Heating(int value) {
    Time.Real_time = (((Time.Hour_ * 60) + Time.Minute_) * 60) + Time.Second_;
    if (value - 2 >= 0) {
      value = value - 2;
      for (int i = 0; i < 5; i++) {
        if ((((Time.Real_time / 60) >= Interval[1].zap[i]) && ((Time.Real_time / 60) < Interval[1].vyp[i])) && (Interval[1].Days[Time.Day_ - 1]) && Interval[1].enable[i]) return true;
      }
    }
    if (value - 1 >= 0) {
      value = value - 1;
      for (int i = 0; i < 5; i++) {
        if ((((Time.Real_time / 60) >= Interval[0].zap[i]) && ((Time.Real_time / 60) < Interval[0].vyp[i])) && (Interval[0].Days[Time.Day_ - 1]) && Interval[0].enable[i]) return true;
      }
    }
    return false;
  }
};
