#include <TimeLib.h>
#include <DS1307RTC.h>

#include "time.h"

class Time {
private:
  Flash* flash;

  const char* ntpServer = "pool.ntp.org";
  const long gmtOffset_sec = 3600;
  const int daylightOffset_sec = 3600;

  tmElements_t TIME;

  uint32_t Real_time;  //everything in seconds
  uint32_t prev_Real_time;

  uint8_t number_of_day;

  struct control {
    bool WIFI_enable;
  };
  control Control;

  struct state {
    int Years;
    uint8_t Months;
    uint8_t Days;
    uint8_t Hours;
    uint8_t Minutes;
    uint8_t Seconds;
    uint8_t WeekDay;
  };
  state State;

  struct time_for_heating {
    uint16_t Time_constant;  //for open mode
    uint16_t Required_time;
  };
  time_for_heating Time_for_heating;

  struct interval {
    uint32_t zap[5] = { 0, 0, 0, 0, 0 };
    uint32_t vyp[5] = { 0, 0, 0, 0, 0 };
    bool enable[5] = { false, false, false, false, false };
    bool Days[7] = { false, false, false, false, false, false, false };
  };
  interval Interval[5];

  struct alarm {
    bool activated;
    Language* message = new Language();
  };
  alarm Alarm;

public:
  Time(Flash* flash) {
    this->flash = flash;

    number_of_day = 1;  //1=Pon - 7=Ne

    Real_time = 0;
    prev_Real_time = 0;

    Control.WIFI_enable = true;

    Alarm.activated = false;

    Time_for_heating.Time_constant = 10800;  //3h

    State.Years = 0;
    State.Months = 0;
    State.Days = 0;
    State.Hours = 0;
    State.Minutes = 0;
    State.Seconds = 0;
    State.WeekDay = 0;
  }
  void Setup() {
    State.WeekDay = flash->get_Flash(60);

    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    int Number = 70;
    bool Switch = true;

    for (int j = 0; j < 5; j++) {
      for (int i = 0; i < 5; i++) {
        set_Interval_heating(j, true, i, ((flash->get_Flash(Number) * 60) + flash->get_Flash(Number + 1)));
        set_Interval_heating(j, false, i, ((flash->get_Flash(Number + 2) * 60) + flash->get_Flash(Number + 3)));
        Number += 4;
      }
    }

    //Serial.println("Setup Number after 1.st = " + String(Number));

    for (int j = 0; j < 5; j++) {
      for (int i = 0; i < 5; i++) {
        set_Interval_enable(j, i, flash->get_Flash(Number));
        Number++;
      }
    }

    //Serial.println("Setup Number after 2.st = " + String(Number));

    for (int j = 0; j < 5; j++) {
      for (int i = 0; i < 7; i++) {
        set_Interval_Days(j, i, flash->get_Flash(Number));
        Number++;
      }
    }

    //Serial.println("Setup Number after 3.st = " + String(Number));
  }
  bool Nominal_run() {
    if (Control.WIFI_enable) {
      //NTP WIFI time
      struct tm timeinfo;
      if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        Alarm.activated = true;
      }
      Alarm.activated = false;

      State.Years = timeinfo.tm_year + 1900;
      State.Months = timeinfo.tm_mon;
      State.Days = timeinfo.tm_yday;
      State.Hours = timeinfo.tm_hour;
      State.Minutes = timeinfo.tm_min;
      State.Seconds = timeinfo.tm_sec;
      if (timeinfo.tm_wday == 0) State.WeekDay = 7;
      else State.WeekDay = timeinfo.tm_wday;

      Real_time = (((State.Hours * 60) + State.Minutes) * 60) + State.Seconds;

    } else {
      if (RTC.read(TIME)) {
        Alarm.activated = false;
        Real_time = 0;
        Real_time = (((TIME.Hour * 60) + TIME.Minute) * 60) + TIME.Second;
        State.Years = tmYearToCalendar(TIME.Year);
        State.Months = TIME.Month;
        State.Days = TIME.Day;
        State.Hours = TIME.Hour;
        State.Minutes = TIME.Minute;
        State.Seconds = TIME.Second;
        if (prev_Real_time > Real_time) {
          prev_Real_time = Real_time;
          if (State.WeekDay > 6) State.WeekDay = 1;
          else State.WeekDay++;
        } else prev_Real_time = Real_time;
      } else {
        Alarm.activated = true;
        if (RTC.chipPresent()) {
          Alarm.message->set_Message("Nahrajte prosim setTime. Cas nefunkcny", "Failed to read from RTC sensor!  Please upload setTime.");
        } else {
          Alarm.message->set_Message("RTC neaktivny. Prosim skontrolujte obvod", "Failed to read from RTC sensor!  Please check the circuit.");
        }
        return false;
      }
    }
    return true;
  }
  void Reset() {
    for (int j = 0; j < 5; j++) {
      for (int i = 0; i < 5; i++) {
        Interval[j].zap[i] = 0;
        Interval[j].vyp[i] = 0;
        Interval[j].Days[i] = false;
        Interval[j].enable[i] = false;
      }
      Interval[j].Days[5] = false;
      Interval[j].Days[6] = false;
    }
  }

  unsigned long getTime() {
    time_t now;
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
      //Serial.println("Failed to obtain time");
      return (0);
    }
    time(&now);
    return now;
  }
  void set_for_time_open_mode(bool state) {
    if (state) Time_for_heating.Required_time = Real_time + Time_for_heating.Time_constant;
    else Time_for_heating.Required_time = 0;
  }
  void set_Interval_heating(int i, bool ON_OFF, int i_2, int32_t value) {
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
  void set_Interval_enable(int i, int i_2, bool value) {
    Interval[i].enable[i_2] = value;
  }
  void set_Interval_Days(int i, int i_2, bool value) {
    Interval[i].Days[i_2] = value;
  }
  void set_Number_of_day(uint8_t value) {
    State.WeekDay = value;
  }
  void set_WIFI_enable(bool value) {
    Control.WIFI_enable = value;
  }

  bool get_Alarm_activated() {
    return Alarm.activated;
  }
  String get_Alarm_message() {
    return Alarm.message->get_Message();
  }
  unsigned int get_Real_time() {
    return Real_time;
  }
  short get_Real_hours() {
    return State.Hours;
  }
  short get_Real_minutes() {
    return State.Minutes;
  }
  short get_Real_seconds() {
    return State.Seconds;
  }
  short get_Real_years() {
    return State.Years;
  }
  short get_Real_months() {
    return State.Months;
  }
  short get_Real_days() {
    return State.Days;
  }
  bool get_Permission_Heating(int value) {
    if (value - 16 >= 0) {
      value = value - 16;
      for (int i = 0; i < 5; i++) {
        if ((((Real_time / 60) >= Interval[4].zap[i]) && ((Real_time / 60) < Interval[4].vyp[i])) && (Interval[4].Days[State.WeekDay - 1]) && Interval[4].enable[i]) return true;
      }
    }
    if (value - 8 >= 0) {
      value = value - 8;
      for (int i = 0; i < 5; i++) {
        if ((((Real_time / 60) >= Interval[3].zap[i]) && ((Real_time / 60) < Interval[3].vyp[i])) && (Interval[3].Days[State.WeekDay - 1]) && Interval[3].enable[i]) return true;
      }
    }
    if (value - 4 >= 0) {
      value = value - 4;
      for (int i = 0; i < 5; i++) {
        if ((((Real_time / 60) >= Interval[2].zap[i]) && ((Real_time / 60) < Interval[2].vyp[i])) && (Interval[2].Days[State.WeekDay - 1]) && Interval[2].enable[i]) return true;
      }
    }
    if (value - 2 >= 0) {
      value = value - 2;
      for (int i = 0; i < 5; i++) {
        if ((((Real_time / 60) >= Interval[1].zap[i]) && ((Real_time / 60) < Interval[1].vyp[i])) && (Interval[1].Days[State.WeekDay - 1]) && Interval[1].enable[i]) return true;
      }
    }
    if (value - 1 >= 0) {
      value = value - 1;
      for (int i = 0; i < 5; i++) {
        if ((((Real_time / 60) >= Interval[0].zap[i]) && ((Real_time / 60) < Interval[0].vyp[i])) && (Interval[0].Days[State.WeekDay - 1]) && Interval[0].enable[i]) return true;
      }
    }

    /*
    for (int j = 0; j < 5; j++) {
      for (int i = 0; i < 5; i++) {
        if ((((Real_time / 60) >= Interval[j].zap[i]) && ((Real_time / 60) < Interval[j].vyp[i])) && (Interval[j].Days[State.WeekDay - 1]) && Interval[j].enable[i]) return true;
      }
    }
    */
    return false;
  }
  bool get_Permission_for_time_open_mode() {
    return true;
  }
  uint32_t get_Interval_heating(int i, bool ON_OFF, uint8_t i_2) {
    if (ON_OFF) {
      return Interval[i].zap[i_2];
    } else {
      return Interval[i].vyp[i_2];
    }
  }
  bool get_Interval_enable(int i, uint8_t i_2) {
    return Interval[i].enable[i_2];
  }
  bool get_Interval_Day(int i, int i_2) {
    return Interval[i].Days[i_2];
  }
  uint8_t get_Number_of_day() {
    return State.WeekDay;
  }
  bool get_WIFI_enable() {
    return Control.WIFI_enable;
  }
};
