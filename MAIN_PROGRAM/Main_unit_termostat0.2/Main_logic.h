class Main_logic {
private:
  HMI* hmi;
  Oven* oven_1;
  Oven* oven_2;
  Ultrasonic* ultrasonic;
  Time* real_time;
  Cooler* cooler;
  ESP_NOW* esp_now;
  Flash* flash;
  Firebase_class* firebase;

  TON timer_for_Flash_save = TON(60000);
  TON timer_for_download_data = TON(30000);
  TON timer_for_send_data = TON(600000);
  TON timer_for_lost_connection = TON(20000);
  Trigger r_trig_send_data = Trigger(true);

  Language* Message = new Language();

  bool WiFi_mode;
  bool Wifi_connection;

  bool Enable_draw;
  bool Send_message;

  enum Current_Mode {
    Nominal_run = 1,
    Ground_position = 2,
    Error = 10
  };
  Current_Mode Current_mode;
  Current_Mode prev_Current_mode;

public:
  Main_logic(HMI* hmi_l, Oven* oven_1_l, Oven* oven_2_l, Time* real_time_l, Ultrasonic* ultrasonic_l, Cooler* cooler_l, ESP_NOW* esp_now_l, Flash* flash, Firebase_class* firebase) {
    hmi = hmi_l;
    oven_1 = oven_1_l;
    oven_2 = oven_2_l;
    real_time = real_time_l;
    ultrasonic = ultrasonic_l;
    cooler = cooler_l;
    esp_now = esp_now_l;
    this->flash = flash;
    this->firebase = firebase;

    WiFi_mode = true;
  }
  void Setup() {
    Current_mode = (Main_logic::Current_Mode)flash->get_Flash(0);
    prev_Current_mode = Current_mode;

    Enable_draw = false;
    Send_message = false;
  }
  void set_WiFi_mode(bool value){
    WiFi_mode = value;
  }
  bool get_WiFi_mode(){
    return WiFi_mode;
  }

public:
  void fcMain() {
    if (millis() < 0) ESP.restart();  //After 50 days millis overflow and device need to be reset

    if ((WiFi.status() != WL_CONNECTED) && WiFi_mode){
      if (timer_for_lost_connection.Timer(true)) {
        hmi->set_Display_off();
        ESP.restart();
      }
      Wifi_connection = false;
    }
    else {
      timer_for_lost_connection.Timer(false);
      Wifi_connection = true;
    }

    //Set language set in HMI
    Message->Set_language(hmi->get_Language_option());

    //Set mode
    if (esp_now->Nominal_run()) {
      Enable_draw = true;
      Send_message = true;
      if (esp_now->get_Current_mode()) {
        Current_mode = Nominal_run;
        hmi->set_Current_mode(true);
      } else {
        Current_mode = Ground_position;
        hmi->set_Current_mode(false);
      }
    } else Enable_draw = false;

    if (ultrasonic->get_Measured() == true) Enable_draw = true;

    //Set mode
    if (hmi->get_Current_mode() == 1) {
      Current_mode = Nominal_run;
      esp_now->set_Current_mode(true);
    } else if (hmi->get_Current_mode() == 2) {
      Current_mode = Ground_position;
      esp_now->set_Current_mode(false);
    }

    //Read data from database
    if (timer_for_download_data.Timer(true) && (Current_mode == Nominal_run) && WiFi_mode && Wifi_connection) {
      Serial.println("Read data from firebase");
      firebase->get_Data();
      Enable_draw = true;
    } else if (Current_mode != Nominal_run) timer_for_download_data.Timer(false);

    //Run and ERROR state
    if (real_time->Nominal_run() == false) Current_mode = Error;
    if (cooler->Cooling() == false) Current_mode = Error;
    if (ultrasonic->Measuring_main() == false) Current_mode = Error;
    if (WiFi_mode == false) {
      if (hmi->Refresh(Enable_draw, 3) == false) Current_mode = Error;
    }
    else if (Wifi_connection) {
      if (hmi->Refresh(Enable_draw, 1) == false) Current_mode = Error;
    }
    else if (Wifi_connection == false) {
      if (hmi->Refresh(Enable_draw, 2) == false) Current_mode = Error;
    }

    //Kind of mode
    switch (Current_mode) {
      case Nominal_run:
        Message->set_Message("Automat", "Automat");
        fcNominal_run();
        break;
      case Ground_position:
        Message->set_Message("Zakl. poloha", "Gnd. position");
        fcGround_position();
        break;
      case Error:
        Message->set_Message("ERROR", "ERROR");
        fcError();
        break;
    }

    //Send data to termostat
    timer_for_send_data.set_Time_LOOP_constant(hmi->get_Server_update_time() * 60000);
    if (Send_message) {
      Serial.println("Send message true");
      Send_message = false;
      esp_now->Sending(esp_now->get_Last_ID());
      //if(Current_mode == Ground_position) firebase->set_Data(esp_now->get_Last_ID(), 10);
    } else if (timer_for_send_data.Timer(true) && WiFi_mode && Wifi_connection) {
      firebase->set_Data(esp_now->get_Last_ID(), 1);
    }

    bool language_option = false;
    //Saving values into flash memory
    if (timer_for_Flash_save.Timer(true)) {
      if (flash->get_Flash(0) != Current_mode) flash->set_Flash(0, Current_mode);
      
      if (hmi->get_Language_option() == "EN") language_option = true;
      else language_option = false;
      if (flash->get_Flash(1) != language_option) flash->set_Flash(1, language_option);

      if (flash->get_Flash(2) != oven_1->get_Overshot_Cooling_time()/100) flash->set_Flash(2, oven_1->get_Overshot_Cooling_time()/100);
      if (flash->get_Flash(3) != oven_1->get_Overshot_Cooling_time()%100) flash->set_Flash(3, oven_1->get_Overshot_Cooling_time()%100);
      if (flash->get_Flash(4) != oven_1->get_Overshot_Heating_time()/100) flash->set_Flash(4, oven_1->get_Overshot_Heating_time()/100);
      if (flash->get_Flash(5) != oven_1->get_Overshot_Heating_time()%100) flash->set_Flash(5, oven_1->get_Overshot_Heating_time()%100);

      if (flash->get_Flash(6) != oven_2->get_Overshot_Cooling_time()/100) flash->set_Flash(6, oven_2->get_Overshot_Cooling_time()/100);
      if (flash->get_Flash(7) != oven_2->get_Overshot_Cooling_time()%100) flash->set_Flash(7, oven_2->get_Overshot_Cooling_time()%100);
      if (flash->get_Flash(8) != oven_2->get_Overshot_Heating_time()/100) flash->set_Flash(8, oven_2->get_Overshot_Heating_time()/100);
      if (flash->get_Flash(9) != oven_2->get_Overshot_Heating_time()%100) flash->set_Flash(9, oven_2->get_Overshot_Heating_time()%100);

      if (flash->get_Flash(10) != oven_1->get_Enable_heating()) flash->set_Flash(10, oven_1->get_Enable_heating());

      if (flash->get_Flash(11) != (int)oven_1->get_Required_T()) flash->set_Flash(11, (int)oven_1->get_Required_T());
      if (flash->get_Flash(12) != (((int)(oven_1->get_Required_T() * 10)) % 10)) flash->set_Flash(12, (((int)(oven_1->get_Required_T() * 10)) % 10));
      if (flash->get_Flash(13) != (int)oven_1->get_Positive_tolerance()) flash->set_Flash(13, (int)oven_1->get_Positive_tolerance());
      if (flash->get_Flash(14) != (((int)(oven_1->get_Positive_tolerance() * 10)) % 10)) flash->set_Flash(14, (((int)(oven_1->get_Positive_tolerance() * 10)) % 10));
      if (flash->get_Flash(15) != (int)oven_1->get_Negative_tolerance()) flash->set_Flash(15, (int)oven_1->get_Negative_tolerance());
      if (flash->get_Flash(16) != (((int)(oven_1->get_Negative_tolerance() * 10)) % 10)) flash->set_Flash(16, (((int)(oven_1->get_Negative_tolerance() * 10)) % 10));
      if (flash->get_Flash(17) != oven_1->get_Mode()) flash->set_Flash(17, oven_1->get_Mode());
      if (flash->get_Flash(19) != oven_1->get_PID_Mode()) flash->set_Flash(19, oven_1->get_PID_Mode());
      if (flash->get_Flash(20) != oven_1->get_Enable_heating_off()) flash->set_Flash(20, oven_1->get_Enable_heating_off());
      if (flash->get_Flash(21) != (int)oven_1->get_Required_T_off()) flash->set_Flash(21, (int)oven_1->get_Required_T_off());
      if (flash->get_Flash(22) != (((int)(oven_1->get_Required_T_off() * 10)) % 10)) flash->set_Flash(22, (((int)(oven_1->get_Required_T_off() * 10)) % 10));
      if (flash->get_Flash(23) != oven_1->get_Positive_tolerance_off()) flash->set_Flash(23, oven_1->get_Positive_tolerance_off());
      if (flash->get_Flash(24) != oven_1->get_Negative_tolerance_off()) flash->set_Flash(24, oven_1->get_Negative_tolerance_off());
      if (flash->get_Flash(25) != oven_1->get_NO_contact()) flash->set_Flash(25, oven_1->get_NO_contact());
      if (flash->get_Flash(26) != oven_1->get_Heating_Interval()) flash->set_Flash(26, oven_1->get_Heating_Interval());
      if (flash->get_Flash(27) != oven_1->get_Upper_limit()) flash->set_Flash(27, (int)oven_1->get_Upper_limit());
      if (flash->get_Flash(28) != oven_1->get_Lower_limit()) flash->set_Flash(28, (int)oven_1->get_Lower_limit());
      if (flash->get_Flash(29) != oven_2->get_Upper_limit()) flash->set_Flash(29, (int)oven_2->get_Upper_limit());

      if (flash->get_Flash(30) != oven_2->get_Enable_heating()) flash->set_Flash(30, oven_2->get_Enable_heating());

      if (flash->get_Flash(31) != (int)oven_2->get_Required_T()) flash->set_Flash(31, (int)oven_2->get_Required_T());
      if (flash->get_Flash(32) != (((int)(oven_2->get_Required_T() * 10)) % 10)) flash->set_Flash(32, (((int)(oven_2->get_Required_T() * 10)) % 10));
      if (flash->get_Flash(33) != (int)oven_2->get_Positive_tolerance()) flash->set_Flash(33, (int)oven_2->get_Positive_tolerance());
      if (flash->get_Flash(34) != (((int)(oven_2->get_Positive_tolerance() * 10)) % 10)) flash->set_Flash(34, (((int)(oven_2->get_Positive_tolerance() * 10)) % 10));
      if (flash->get_Flash(35) != (int)oven_2->get_Negative_tolerance()) flash->set_Flash(35, (int)oven_2->get_Negative_tolerance());
      if (flash->get_Flash(36) != (((int)(oven_2->get_Negative_tolerance() * 10)) % 10)) flash->set_Flash(36, (((int)(oven_2->get_Negative_tolerance() * 10)) % 10));
      if (flash->get_Flash(37) != oven_2->get_Mode()) flash->set_Flash(37, oven_2->get_Mode());
      if (flash->get_Flash(39) != oven_2->get_PID_Mode()) flash->set_Flash(39, oven_2->get_PID_Mode());
      if (flash->get_Flash(40) != oven_2->get_Enable_heating_off()) flash->set_Flash(40, oven_2->get_Enable_heating_off());
      if (flash->get_Flash(41) != (int)oven_2->get_Required_T_off()) flash->set_Flash(41, (int)oven_2->get_Required_T_off());
      if (flash->get_Flash(42) != (((int)(oven_2->get_Required_T_off() * 10)) % 10)) flash->set_Flash(42, (((int)(oven_2->get_Required_T_off() * 10)) % 10));
      if (flash->get_Flash(43) != oven_2->get_Positive_tolerance_off()) flash->set_Flash(43, oven_2->get_Positive_tolerance_off());
      if (flash->get_Flash(46) != oven_2->get_Negative_tolerance_off()) flash->set_Flash(46, oven_2->get_Negative_tolerance_off());
      if (flash->get_Flash(47) != oven_2->get_NO_contact()) flash->set_Flash(47, oven_2->get_NO_contact());
      if (flash->get_Flash(48) != oven_2->get_Heating_Interval()) flash->set_Flash(48, oven_2->get_Heating_Interval());

      //44 and 45 are define in HMI

      if (flash->get_Flash(49) != hmi->get_Server_update_time()) flash->set_Flash(49, hmi->get_Server_update_time());

      if (flash->get_Flash(50) != ultrasonic->get_Enable()) flash->set_Flash(50, ultrasonic->get_Enable());
      if (flash->get_Flash(51) != ultrasonic->get_Max_distance()) {
        if ((flash->get_Flash(51) != 250) && (ultrasonic->get_Max_distance() > 250)) flash->set_Flash(51, 250);
        else if ((ultrasonic->get_Max_distance() <= 250)) flash->set_Flash(51, ultrasonic->get_Max_distance());
      }
      if (flash->get_Flash(52) != ultrasonic->get_Min_distance()) {
        if ((flash->get_Flash(52) != 250) && (ultrasonic->get_Min_distance() > 250)) flash->set_Flash(52, 250);
        else if ((ultrasonic->get_Min_distance() <= 250)) flash->set_Flash(52, ultrasonic->get_Min_distance());
      }
      if (flash->get_Flash(53) != ultrasonic->get_Update_time() / 60000) flash->set_Flash(53, ultrasonic->get_Update_time() / 60000);
      if (flash->get_Flash(54) != oven_2->get_Lower_limit()) flash->set_Flash(54, (int)oven_2->get_Lower_limit());
      if (flash->get_Flash(55) != cooler->get_Enable()) flash->set_Flash(55, cooler->get_Enable());
      if (flash->get_Flash(56) != cooler->get_Required_T()) flash->set_Flash(56, cooler->get_Required_T());
      if (flash->get_Flash(57) != cooler->get_Update_time() / 1000) flash->set_Flash(57, cooler->get_Update_time() / 1000);


      if (flash->get_Flash(60) != real_time->get_Number_of_day()) flash->set_Flash(60, real_time->get_Number_of_day());

      int Number = 70;
      bool Switch = false;
      int Counter = 0;

      for (int j = 0; j < 5; j++) {
        for (int i = 0; i < 5; i++) {
          if (flash->get_Flash(Number) != (real_time->get_Interval_heating(j, true, i) / 60)) flash->set_Flash(Number, (real_time->get_Interval_heating(j, true, i) / 60));
          if (flash->get_Flash(Number + 1) != (real_time->get_Interval_heating(j, true, i) % 60)) flash->set_Flash(Number + 1, (real_time->get_Interval_heating(j, true, i) % 60));
          if (flash->get_Flash(Number + 2) != (real_time->get_Interval_heating(j, false, i) / 60)) flash->set_Flash(Number + 2, (real_time->get_Interval_heating(j, false, i) / 60));
          if (flash->get_Flash(Number + 3) != (real_time->get_Interval_heating(j, false, i) % 60)) flash->set_Flash(Number + 3, (real_time->get_Interval_heating(j, false, i) % 60));
          Number += 4;
        }
      }
      //Serial.println("Number after 1.st = " + String(Number));

      for (int j = 0; j < 5; j++) {
        for (int i = 0; i < 5; i++) {
          if (flash->get_Flash(Number) != real_time->get_Interval_enable(j, i)) flash->set_Flash(Number, real_time->get_Interval_enable(j, i));
          Number++;
        }
      }
      //Serial.println("Number after 2.st = " + String(Number));

      for (int j = 0; j < 5; j++) {
        for (int i = 0; i < 7; i++) {
          if (flash->get_Flash(Number) != real_time->get_Interval_Day(j, i)) flash->set_Flash(Number, real_time->get_Interval_Day(j, i));
          Number++;
        }
      }
      //Serial.println("Number after 3.st = " + String(Number));
    }
  }
private:
  void fcNominal_run() {
    if (esp_now->get_Comunication(true)) oven_1->Oven_Start();
    else {
      oven_1->Oven_Stop();
      oven_1->set_Temp_and_Hum(99.9, 99.9);
    }
    if (esp_now->get_Comunication(false)) oven_2->Oven_Start();
    else {
      oven_2->Oven_Stop();
      oven_2->set_Temp_and_Hum(99.9, 99.9);
    }

    if((prev_Current_mode != Current_mode) && (Current_mode == Nominal_run) && WiFi_mode && Wifi_connection) firebase->set_Data(true, 20);
    
    prev_Current_mode = Current_mode;
  }
  void fcGround_position() {
    oven_1->Oven_Stop();
    oven_2->Oven_Stop();

    prev_Current_mode = Current_mode;
  }
  void fcError() {
    oven_1->Oven_Stop();
    oven_2->Oven_Stop();
    if (hmi->get_Alarm_activated()) Serial.println(hmi->get_Alarm_message());
    if (cooler->get_Alarm_activated()) Serial.println(cooler->get_Alarm_message());
    if (real_time->get_Alarm_activated()) Serial.println(real_time->get_Alarm_message());
    if (ultrasonic->get_Alarm_activated()) Serial.println(ultrasonic->get_Alarm_message());

    if ((hmi->get_Alarm_activated() == false) && (cooler->get_Alarm_activated() == false) && (real_time->get_Alarm_activated() == false) && ultrasonic->get_Alarm_activated() == false) Current_mode = prev_Current_mode;
  }
};
