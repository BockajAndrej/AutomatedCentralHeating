class Main_logic {
private:
  HMI* hmi;
  Oven* oven_1;
  Oven* oven_2;
  Temperature_sensor* Temp;
  ESP_NOW* esp_now;
  Battery* battery;
  Flash* flash;

  TON timer_for_Flash_save = TON(5000);
  TON timer_for_ESPNOW = TON(60000);
  TON timer_for_ReconectionESPNOW = TON(5000);
  TON timer_for_reset_during_ERROR = TON(5000);

  Trigger R_Trig_Sleep_send = Trigger(true);

  Language* Message = new Language();

  bool Esp_now_Receive;

  float prev_Temperature, prev_Humidity;
  bool update_Display;

  enum Current_Mode {
    Nominal_run = 1,
    Groung_position = 2,
    Sleep_run = 5,
    Error = 10
  };
  Current_Mode Current_mode;
  Current_Mode prev_Current_mode;

  struct SLEEP {
    int Time_to_sleep;
    int Time_to_sleep_constant;
    int Time;
    bool Wake_up_cause;
    int Connection_attempt;
    struct fcSleep_variables_struct {
      int last_time;
      int time_constant;  //5s
    };
    fcSleep_variables_struct fcSleep_variables;
  };
  SLEEP Sleep;

public:
  Main_logic(HMI* hmi_l, Oven* oven_1_l, Oven* oven_2_l, Temperature_sensor* Temp_l, ESP_NOW* esp_now_l, Battery* battery, Flash* flash) {
    hmi = hmi_l;
    oven_1 = oven_1_l;
    oven_2 = oven_2_l;
    Temp = Temp_l;
    esp_now = esp_now_l;
    this->battery = battery;
    this->flash = flash;

    Current_mode = Groung_position;
    prev_Current_mode = Current_mode;

    Esp_now_Receive = false;
    update_Display = false;

    Sleep.Time_to_sleep_constant = 15;
    Sleep.Time_to_sleep = millis() + (Sleep.Time_to_sleep_constant * 1000);
    Sleep.Time = 1;
    Sleep.Wake_up_cause = true;
    Sleep.Connection_attempt = 0;

    Sleep.fcSleep_variables.last_time = millis();
    Sleep.fcSleep_variables.time_constant = 3000;  //3s

    prev_Temperature = 0;
    prev_Humidity = 0;
  }
  bool Setup() {

    Sleep.Time = (int)flash->get_Flash(11);
    esp_now->set_Sleep_time(Sleep.Time);

    //Sleep.Wake_up_cause = value;
    esp_sleep_wakeup_cause_t wakeup_reason;

    esp_sleep_enable_timer_wakeup(Sleep.Time * 60000000);  // x * 1000000 = second
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_25, 1);          //1 = High, 0 = Low

    wakeup_reason = esp_sleep_get_wakeup_cause();

    switch (wakeup_reason) {
      case ESP_SLEEP_WAKEUP_EXT0:
        Sleep.Wake_up_cause = true;
        break;
      case ESP_SLEEP_WAKEUP_TIMER:
        Sleep.Wake_up_cause = false;
        break;
      default:
        Sleep.Wake_up_cause = true;
        break;
    }

    Sleep.Time_to_sleep = millis() + (hmi->get_Sleep_time_constant() * 1000);
    return Sleep.Wake_up_cause;
  }

public:
  void fcMain() {
    //Set language
    Message->Set_language(hmi->get_Language_option());
    R_Trig_Sleep_send.Check(false);

    Sleep.Time = hmi->get_Sleep_time();

    if (esp_now->get_ID() == 3) {
      if ((prev_Temperature != oven_2->get_Temperature()) || (prev_Humidity != oven_2->get_Humidity())) {
        update_Display = true;
        prev_Temperature = oven_2->get_Temperature();
        prev_Humidity = oven_2->get_Humidity();
      } else update_Display = false;
    } else {
      if ((prev_Temperature != oven_1->get_Temperature()) || (prev_Humidity != oven_1->get_Humidity())) {
        update_Display = true;
        prev_Temperature = oven_1->get_Temperature();
        prev_Humidity = oven_1->get_Humidity();
      } else update_Display = false;
    }

    //Set mode by main unit
    if (esp_now->Nominal_run()) {
      Esp_now_Receive = true;
      if (esp_now->get_Current_mode()) {
        Current_mode = Nominal_run;
        hmi->set_Current_mode(1);
      } else {
        Current_mode = Groung_position;
        hmi->set_Current_mode(2);
      }
    } else Esp_now_Receive = false;

    //Set mode by user
    if (hmi->get_Current_mode() == 1) {
      Current_mode = Nominal_run;
      esp_now->set_Current_mode(true);
    } else if (hmi->get_Current_mode() == 2) {
      Current_mode = Groung_position;
      esp_now->set_Current_mode(false);
    }

    if ((Sleep.Wake_up_cause) || (Current_mode == Error)) {
      hmi->Refresh(Esp_now_Receive || update_Display);
      if (hmi->get_Pressed()) Sleep.Time_to_sleep = millis() + (hmi->get_Sleep_time_constant() * 1000);
    } else Current_mode = Sleep_run;

    if (hmi->get_Alarm_activated()) Current_mode = Error;
    if (esp_now->get_Alarm_activated() && Sleep.Wake_up_cause) Current_mode = Error;
    if (Temp->get_Alarm_activated()) Current_mode = Error;

    if ((Sleep.Time_to_sleep < millis()) && (Current_mode != Error) && (hmi->get_ALLWAIS_ON() == false)) Current_mode = Sleep_run;

    //if (prev_Current_mode != Current_mode) Serial.println("Main logic -> Current mode = " + String((int)Current_mode));

    switch (Current_mode) {
      case Nominal_run:
        Message->set_Message("Automat", "Automat");
        prev_Current_mode = Current_mode;
        fcNominal_run();
        break;
      case Groung_position:
        Message->set_Message("Zakl. poloha", "Gnd. position");
        prev_Current_mode = Current_mode;
        fcGround_position();
        break;
      case Sleep_run:
        Message->set_Message("Sleep run", "Sleep run");
        if (Sleep.Wake_up_cause) fcSleep(true);
        else fcSleep(false);
      case Error:
        Message->set_Message("ERROR", "ERROR");
        fcError();
        break;
    }

    bool language_option = false;

    if (timer_for_Flash_save.Timer(true)) {

      if (hmi->get_Language_option() == "EN") language_option = true;
      else language_option = false;
      if (flash->get_Flash(1) != language_option) flash->set_Flash(1, language_option);

      if (flash->get_Flash(10) != hmi->get_Sleep_time_constant()) flash->set_Flash(10, hmi->get_Sleep_time_constant());
      if (flash->get_Flash(11) != Sleep.Time) flash->set_Flash(11, Sleep.Time);

      if (flash->get_Flash(20) != Temp->get_Update_time() / 1000) flash->set_Flash(20, (Temp->get_Update_time() / 1000));
      if (flash->get_Flash(21) != Temp->get_Mode()) flash->set_Flash(21, Temp->get_Mode());

      if (flash->get_Flash(25) != oven_1->get_Upper_limit()) flash->set_Flash(25, (int)oven_1->get_Upper_limit());
      if (flash->get_Flash(26) != oven_1->get_Lower_limit()) flash->set_Flash(26, (int)oven_1->get_Lower_limit());
      if (flash->get_Flash(27) != oven_2->get_Upper_limit()) flash->set_Flash(27, (int)oven_2->get_Upper_limit());
      if (flash->get_Flash(28) != oven_2->get_Lower_limit()) flash->set_Flash(28, (int)oven_2->get_Lower_limit());

      //if(flash->get_Flash(30) != hmi->get_Rotation()) flash->set_Flash(30, hmi->get_Rotation());
      //31 Touch REPEAT_CAL
    }

    if (hmi->get_ALLWAIS_ON()) {
      timer_for_ESPNOW.set_Time_LOOP_constant(Sleep.Time * 60000);
      if (timer_for_ESPNOW.Timer(true)) esp_now->Sending(false, 0);
    }
  }
private:
  void fcNominal_run() {
  }
  void fcGround_position() {
  }
  void fcError() {
    fcGround_position();

    hmi->set_Current_mode(10);

    if (hmi->get_Alarm_activated()) Serial.println(hmi->get_Alarm_message());
    if (esp_now->get_Alarm_activated()){
      Serial.println(esp_now->get_Alarm_message());
      if (hmi->get_Pressed()) timer_for_reset_during_ERROR.Timer(false);
      else if(timer_for_reset_during_ERROR.Timer(true)) ESP.restart();
    }
    if (Temp->get_Alarm_activated()) Serial.println(Temp->get_Alarm_message());

    if ((hmi->get_Alarm_activated() == false) && (esp_now->get_Alarm_activated() == false) && (Temp->get_Alarm_activated() == false)) {
      Serial.println("MAIN LOGIC -> ERROR = 0, Current = " + String((int)Current_mode) + ", PrevMod = " + String((int)prev_Current_mode));
      Current_mode = prev_Current_mode;
      hmi->set_Current_mode(2);
      timer_for_reset_during_ERROR.Timer(false);
    }
  }
  void fcSleep(bool state) {
    if (Esp_now_Receive) {
      hmi->set_Display(false);
      Serial.println("It is Going to SLEEP");
      esp_deep_sleep_start();
    } else if (Sleep.Connection_attempt >= 3) {
      ESP.restart();
    } else {
      if(R_Trig_Sleep_send.Check(true)) esp_now->Sending(false, 0);
      if (timer_for_ReconectionESPNOW.Timer(true)) {
        Sleep.Connection_attempt += 1;
        Sleep.fcSleep_variables.last_time = millis();
        //if (state) esp_now->Sending(false, 1000);
        //else esp_now->Sending(false, 0);
        esp_now->Sending(false, 0);
      }
    }
  }
};