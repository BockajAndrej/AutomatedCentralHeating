typedef struct struct_message {
  uint8_t ID;
  float Temperature;
  float Humidity;
  double Positive_tolerance[2];
  double Negative_tolerance[2];
  double Required_T[2];
  int Mode[2];
  bool Enable_heating[2];
  int Current_mode;
  bool Heating[2];  //ci je rele aktivne alebo nie
  bool PID_state[2];
  bool Oven_Interval_1[2];
  bool Oven_Interval_2[2];
  bool Oven_Interval_3[2];
  uint8_t Sleep_time;

  //Time
  uint16_t Year_;
  uint8_t Month_;
  uint8_t Day_;
  uint8_t Hour_;
  uint8_t Minute_;
  uint8_t Second_;
  bool Time_state;

  uint32_t Turn_on_time[2][5];
  uint32_t Turn_off_time[2][5];
  bool State_interval[2][6];  //Binary
  bool State_interval_Days[2][7];
  //6 - enable for receive

  uint8_t Ultrasonic_Capacite;
  bool Ultrasonic_state;

  bool ERROR;
} struct_message;

struct_message SendData;
struct_message ReceiveData;

class ESP_NOW {
private:
  Oven* oven_1;
  Oven* oven_2;
  Time* real_time;
  Ultrasonic* ultrasonic;

  TON time_for_control_Comunication1 = TON(60000);
  TON time_for_control_Comunication2 = TON(60000);
  bool Comunication1;
  bool Comunication2;

  uint8_t BroadcastAddress_Sender_1[6];
  uint8_t BroadcastAddress_Sender_2[6];
  int prev_ID[2];
  int cas_spanku[2];
  int cas_spanku_tolerance;
  bool Synchronization;


  struct Time_struct {
    unsigned long Update_time;
    unsigned long Max_Update_time;
    unsigned long Min_Update_time;
  };
  Time_struct Time__;

  enum current_mode {
    Nominal_run__ = 1,
    Ground_position = 2,
  };
  current_mode Current_Mode;

  static void OnDataSent(const uint8_t* mac_addr, esp_now_send_status_t status) {
    if (status == ESP_NOW_SEND_SUCCESS) {
      Serial.println("ODOSLALO");
    } else {
      Serial.println("FALSE_odoslanie");
    }
  }

  // callback function that will be executed when data is received
  static void OnDataRecv(const uint8_t* mac_addr, const uint8_t* incomingData, int len) {
    static char macStr[18];
    Serial.print("Packet received from: ");
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
             mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    memcpy(&ReceiveData, incomingData, sizeof(ReceiveData));
    Serial.println(ReceiveData.ID);
    Serial.println(ReceiveData.Temperature);
    Serial.println("Size of = " + String(sizeof(ReceiveData)));
    Serial.println("Receive sleep time in function = " + String(ReceiveData.Sleep_time));
  }

public:
  ESP_NOW(uint8_t Number,
          uint8_t mac_addr_0, uint8_t mac_addr_1, uint8_t mac_addr_2, uint8_t mac_addr_3, uint8_t mac_addr_4, uint8_t mac_addr_5,
          uint8_t mac_addr_02, uint8_t mac_addr_12, uint8_t mac_addr_22, uint8_t mac_addr_32, uint8_t mac_addr_42, uint8_t mac_addr_52,
          Oven* oven_1_l, Oven* oven_2_l, Time* real_time_l, Ultrasonic* ultrasonic_l) {

    SendData.ID = Number;

    BroadcastAddress_Sender_1[0] = mac_addr_0;
    BroadcastAddress_Sender_1[1] = mac_addr_1;
    BroadcastAddress_Sender_1[2] = mac_addr_2;
    BroadcastAddress_Sender_1[3] = mac_addr_3;
    BroadcastAddress_Sender_1[4] = mac_addr_4;
    BroadcastAddress_Sender_1[5] = mac_addr_5;

    BroadcastAddress_Sender_2[0] = mac_addr_02;
    BroadcastAddress_Sender_2[1] = mac_addr_12;
    BroadcastAddress_Sender_2[2] = mac_addr_22;
    BroadcastAddress_Sender_2[3] = mac_addr_32;
    BroadcastAddress_Sender_2[4] = mac_addr_42;
    BroadcastAddress_Sender_2[5] = mac_addr_52;

    Comunication1 = false;
    Comunication2 = false;

    oven_1 = oven_1_l;
    oven_2 = oven_2_l;
    real_time = real_time_l;
    ultrasonic = ultrasonic_l;

    prev_ID[0] = 0;
    prev_ID[1] = 0;

    Current_Mode = Ground_position;

    Synchronization = false;

    cas_spanku[0] = 100000;
    cas_spanku[1] = 100000;
    cas_spanku_tolerance = 15;  //In percent
  }
  void Setup() {
    // Init ESP-NOW
    if (esp_now_init() != ESP_OK) {
      Serial.println("Error initializing ESP-NOW");
    } else Serial.println("Initialized ESP-NOW");

    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataRecv);

    // Register peer
    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, BroadcastAddress_Sender_1, 6);
    peerInfo.channel = 0;

    // Add peer
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      Serial.println("Failed to add 1st peer");
    } else Serial.println("Added 1st peer");

    memcpy(peerInfo.peer_addr, BroadcastAddress_Sender_2, 6);
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      Serial.println("Failed to add 2cond peer");
    } else Serial.println("Added 2st peer");
  }
  void set_Current_mode(bool value) {
    if (value) Current_Mode = Nominal_run__;
    else Current_Mode = Ground_position;
  }
  void Reset_ID() {
    ReceiveData.ID = 0;
  }
  bool get_Current_mode() {
    if (Current_Mode == Nominal_run__) return true;
    else return false;
    return false;
  }
  bool get_Last_ID() {
    if ((prev_ID[0] == 2) || (prev_ID[0] == 20)) return true;
    return false;
  }
  bool get_Comunication(bool value) {
    if (value) return Comunication1;
    else return Comunication2;
  }

  bool Nominal_run() {
    time_for_control_Comunication1.set_Time_LOOP_constant((cas_spanku[0] * 60000 * (100 + cas_spanku_tolerance)) / 100);
    if (time_for_control_Comunication1.Timer(true)) Comunication1 = false;
    time_for_control_Comunication2.set_Time_LOOP_constant((cas_spanku[1] * 60000 * (100 + cas_spanku_tolerance)) / 100);
    if (time_for_control_Comunication2.Timer(true)) Comunication2 = false;

    if (0 != ReceiveData.ID) {
      prev_ID[0] = ReceiveData.ID;

      if (true) {
        if (ReceiveData.Current_mode != -1) {
          if (ReceiveData.Current_mode == 1) Current_Mode = Nominal_run__;
          else Current_Mode = Ground_position;
        }

        oven_1->set_Enable_heating(ReceiveData.Enable_heating[0]);
        oven_2->set_Enable_heating(ReceiveData.Enable_heating[1]);

        if (ReceiveData.Required_T[0] > -1) {
          oven_1->set_Required_T(ReceiveData.Required_T[0]);
        }
        if (ReceiveData.Positive_tolerance[0] > -1) {
          oven_1->set_Positive_tolerance(ReceiveData.Positive_tolerance[0]);
        }
        if (ReceiveData.Negative_tolerance[0] > -1) {
          oven_1->set_Negative_tolerance(ReceiveData.Negative_tolerance[0]);
        }
        if (ReceiveData.Mode[0] != -1) {
          oven_1->set_Mode(ReceiveData.Mode[0]);

          int value_LL = oven_1->get_Heating_Interval();
          if (value_LL - 16 >= 0) value_LL = value_LL - 16;
          if (value_LL - 8 >= 0) value_LL = value_LL - 8;
          if (value_LL - 4 >= 0) value_LL = value_LL - 4;
          if (value_LL - 2 >= 0) {
            value_LL = value_LL - 2;
            if ((ReceiveData.Oven_Interval_2[0] == false) && (ReceiveData.Oven_Interval_3[0] == false)) oven_1->set_Heating_Interval(oven_1->get_Heating_Interval() - 2);
          } else if ((ReceiveData.Oven_Interval_2[0]) || (ReceiveData.Oven_Interval_3[0])) oven_1->set_Heating_Interval(oven_1->get_Heating_Interval() + 2);
          if (value_LL - 1 >= 0) {
            if ((ReceiveData.Oven_Interval_1[0] == false) && (ReceiveData.Oven_Interval_3[0] == false)) oven_1->set_Heating_Interval(oven_1->get_Heating_Interval() - 1);
          } else if ((ReceiveData.Oven_Interval_1[0]) || (ReceiveData.Oven_Interval_3[0])) oven_1->set_Heating_Interval(oven_1->get_Heating_Interval() + 1);

          value_LL = oven_2->get_Heating_Interval();
          if (value_LL - 16 >= 0) value_LL = value_LL - 16;
          if (value_LL - 8 >= 0) value_LL = value_LL - 8;
          if (value_LL - 4 >= 0) value_LL = value_LL - 4;
          if (value_LL - 2 >= 0) {
            value_LL = value_LL - 2;
            if ((ReceiveData.Oven_Interval_2[1] == false) && (ReceiveData.Oven_Interval_3[1] == false)) oven_2->set_Heating_Interval(oven_2->get_Heating_Interval() - 2);
          } else if ((ReceiveData.Oven_Interval_2[1]) || (ReceiveData.Oven_Interval_3[1])) oven_2->set_Heating_Interval(oven_2->get_Heating_Interval() + 2);
          if (value_LL - 1 >= 0) {
            if ((ReceiveData.Oven_Interval_1[1] == false) && (ReceiveData.Oven_Interval_3[1] == false)) oven_2->set_Heating_Interval(oven_2->get_Heating_Interval() - 1);
          } else if ((ReceiveData.Oven_Interval_1[1]) || (ReceiveData.Oven_Interval_3[1])) oven_2->set_Heating_Interval(oven_2->get_Heating_Interval() + 1);

          if (ReceiveData.PID_state[0]) oven_1->set_PID_Mode(1);
          else oven_1->set_PID_Mode(0);
          if (ReceiveData.PID_state[1]) oven_2->set_PID_Mode(1);
          else oven_2->set_PID_Mode(0);
        }

        if (ReceiveData.Required_T[1] > -1) {
          oven_2->set_Required_T(ReceiveData.Required_T[1]);
        }
        if (ReceiveData.Positive_tolerance[1] > -1) {
          oven_2->set_Positive_tolerance(ReceiveData.Positive_tolerance[1]);
        }
        if (ReceiveData.Negative_tolerance[1] > -1) {
          oven_2->set_Negative_tolerance(ReceiveData.Negative_tolerance[1]);
        }
        if (ReceiveData.Mode[1] != -1) {
          oven_2->set_Mode(ReceiveData.Mode[1]);
        }

        if (ReceiveData.State_interval[0][5]) {
          real_time->set_Interval_heating(0, true, 0, ReceiveData.Turn_on_time[0][0]);
          real_time->set_Interval_heating(0, true, 1, ReceiveData.Turn_on_time[0][1]);
          real_time->set_Interval_heating(0, true, 2, ReceiveData.Turn_on_time[0][2]);
          real_time->set_Interval_heating(0, true, 3, ReceiveData.Turn_on_time[0][3]);
          real_time->set_Interval_heating(0, true, 4, ReceiveData.Turn_on_time[0][4]);

          real_time->set_Interval_heating(0, false, 0, ReceiveData.Turn_off_time[0][0]);
          real_time->set_Interval_heating(0, false, 1, ReceiveData.Turn_off_time[0][1]);
          real_time->set_Interval_heating(0, false, 2, ReceiveData.Turn_off_time[0][2]);
          real_time->set_Interval_heating(0, false, 3, ReceiveData.Turn_off_time[0][3]);
          real_time->set_Interval_heating(0, false, 4, ReceiveData.Turn_off_time[0][4]);

          real_time->set_Interval_enable(0, 0, ReceiveData.State_interval[0][0]);
          real_time->set_Interval_enable(0, 1, ReceiveData.State_interval[0][1]);
          real_time->set_Interval_enable(0, 2, ReceiveData.State_interval[0][2]);
          real_time->set_Interval_enable(0, 3, ReceiveData.State_interval[0][3]);
          real_time->set_Interval_enable(0, 4, ReceiveData.State_interval[0][4]);

          real_time->set_Interval_heating(1, true, 0, ReceiveData.Turn_on_time[1][0]);
          real_time->set_Interval_heating(1, true, 1, ReceiveData.Turn_on_time[1][1]);
          real_time->set_Interval_heating(1, true, 2, ReceiveData.Turn_on_time[1][2]);
          real_time->set_Interval_heating(1, true, 3, ReceiveData.Turn_on_time[1][3]);
          real_time->set_Interval_heating(1, true, 4, ReceiveData.Turn_on_time[1][4]);

          real_time->set_Interval_heating(1, false, 0, ReceiveData.Turn_off_time[1][0]);
          real_time->set_Interval_heating(1, false, 1, ReceiveData.Turn_off_time[1][1]);
          real_time->set_Interval_heating(1, false, 2, ReceiveData.Turn_off_time[1][2]);
          real_time->set_Interval_heating(1, false, 3, ReceiveData.Turn_off_time[1][3]);
          real_time->set_Interval_heating(1, false, 4, ReceiveData.Turn_off_time[1][4]);

          real_time->set_Interval_enable(1, 0, ReceiveData.State_interval[1][0]);
          real_time->set_Interval_enable(1, 1, ReceiveData.State_interval[1][1]);
          real_time->set_Interval_enable(1, 2, ReceiveData.State_interval[1][2]);
          real_time->set_Interval_enable(1, 3, ReceiveData.State_interval[1][3]);
          real_time->set_Interval_enable(1, 4, ReceiveData.State_interval[1][4]);

          real_time->set_Interval_Days(0, 0, ReceiveData.State_interval_Days[0][0]);
          real_time->set_Interval_Days(0, 1, ReceiveData.State_interval_Days[0][1]);
          real_time->set_Interval_Days(0, 2, ReceiveData.State_interval_Days[0][2]);
          real_time->set_Interval_Days(0, 3, ReceiveData.State_interval_Days[0][3]);
          real_time->set_Interval_Days(0, 4, ReceiveData.State_interval_Days[0][4]);
          real_time->set_Interval_Days(0, 5, ReceiveData.State_interval_Days[0][5]);
          real_time->set_Interval_Days(0, 6, ReceiveData.State_interval_Days[0][6]);

          real_time->set_Interval_Days(1, 0, ReceiveData.State_interval_Days[1][0]);
          real_time->set_Interval_Days(1, 1, ReceiveData.State_interval_Days[1][1]);
          real_time->set_Interval_Days(1, 2, ReceiveData.State_interval_Days[1][2]);
          real_time->set_Interval_Days(1, 3, ReceiveData.State_interval_Days[1][3]);
          real_time->set_Interval_Days(1, 4, ReceiveData.State_interval_Days[1][4]);
          real_time->set_Interval_Days(1, 5, ReceiveData.State_interval_Days[1][5]);
          real_time->set_Interval_Days(1, 6, ReceiveData.State_interval_Days[1][6]);
        }
      }

      if (ReceiveData.ID == 2 || ReceiveData.ID == 20) {
        oven_1->set_Temp_and_Hum(ReceiveData.Temperature, ReceiveData.Humidity);
        cas_spanku[0] = ReceiveData.Sleep_time;
        Serial.println("Receive sleep time = " + String(ReceiveData.Sleep_time));
        Comunication1 = true;
        time_for_control_Comunication1.Timer(false);
        //Sending(true);
      } else if (ReceiveData.ID == 3 || ReceiveData.ID == 30) {
        oven_2->set_Temp_and_Hum(ReceiveData.Temperature, ReceiveData.Humidity);
        cas_spanku[1] = ReceiveData.Sleep_time;
        Serial.println("Receive sleep time = " + String(ReceiveData.Sleep_time));
        Comunication2 = true;
        time_for_control_Comunication2.Timer(false);
        //Sending(false);
      }

      ReceiveData.ID = 0;
      return true;
    }
    return false;
  }

public:
  void Sending(bool value) {
    Serial.print("Sending to : ");

    if (Current_Mode == Nominal_run__) SendData.Current_mode = 1;
    else SendData.Current_mode = 2;

    SendData.Positive_tolerance[0] = oven_1->get_Positive_tolerance();
    SendData.Negative_tolerance[0] = oven_1->get_Negative_tolerance();
    SendData.Required_T[0] = oven_1->get_Required_T();
    SendData.Mode[0] = oven_1->get_Mode();
    SendData.Heating[0] = oven_1->get_Heating();
    SendData.Enable_heating[0] = oven_1->get_Enable_heating();

    SendData.Year_ = real_time->get_Real_years();
    SendData.Month_ = real_time->get_Real_months();
    SendData.Day_ = real_time->get_Real_days();
    SendData.Hour_ = real_time->get_Real_hours();
    SendData.Minute_ = real_time->get_Real_minutes();
    SendData.Second_ = real_time->get_Real_seconds();
    SendData.Time_state = !(real_time->get_Alarm_activated());

    SendData.Positive_tolerance[1] = oven_2->get_Positive_tolerance();
    SendData.Negative_tolerance[1] = oven_2->get_Negative_tolerance();
    SendData.Required_T[1] = oven_2->get_Required_T();
    SendData.Mode[1] = oven_2->get_Mode();
    SendData.Heating[1] = oven_2->get_Heating();
    SendData.Enable_heating[1] = oven_2->get_Enable_heating();

    if (oven_1->get_PID_Mode() == 0) SendData.PID_state[0] = false;
    else SendData.PID_state[0] = true;
    if (oven_2->get_PID_Mode() == 0) SendData.PID_state[1] = false;
    else SendData.PID_state[1] = true;

    int value_LL = oven_1->get_Heating_Interval();
    if (value_LL - 16 >= 0) value_LL = value_LL - 16;
    if (value_LL - 8 >= 0) value_LL = value_LL - 8;
    if (value_LL - 4 >= 0) value_LL = value_LL - 4;
    if (value_LL - 3 >= 0) {
      SendData.Oven_Interval_1[0] = false;
      SendData.Oven_Interval_2[0] = false;
      SendData.Oven_Interval_3[0] = true;
    } else {
      SendData.Oven_Interval_3[0] = false;
      if (value_LL - 2 >= 0) {
        SendData.Oven_Interval_2[0] = true;
        SendData.Oven_Interval_1[0] = false;
      } else if (value_LL - 1 >= 0) {
        SendData.Oven_Interval_1[0] = true;
        SendData.Oven_Interval_2[0] = false;
      }
    }

    value_LL = oven_2->get_Heating_Interval();
    if (value_LL - 16 >= 0) value_LL = value_LL - 16;
    if (value_LL - 8 >= 0) value_LL = value_LL - 8;
    if (value_LL - 4 >= 0) value_LL = value_LL - 4;
    if (value_LL - 3 >= 0) {
      SendData.Oven_Interval_1[1] = false;
      SendData.Oven_Interval_2[1] = false;
      SendData.Oven_Interval_3[1] = true;
    } else {
      SendData.Oven_Interval_3[1] = false;
      if (value_LL - 2 >= 0) {
        SendData.Oven_Interval_2[1] = true;
        SendData.Oven_Interval_1[1] = false;
      } else if (value_LL - 1 >= 0) {
        SendData.Oven_Interval_1[1] = true;
        SendData.Oven_Interval_2[1] = false;
      }
    }

    SendData.Ultrasonic_Capacite = ultrasonic->get_Capacity_in_percent();
    SendData.Ultrasonic_state = !(ultrasonic->get_Alarm_activated());

    SendData.Turn_on_time[0][0] = real_time->get_Interval_heating(0, true, 0);
    SendData.Turn_on_time[0][1] = real_time->get_Interval_heating(0, true, 1);
    SendData.Turn_on_time[0][2] = real_time->get_Interval_heating(0, true, 2);
    SendData.Turn_on_time[0][3] = real_time->get_Interval_heating(0, true, 3);
    SendData.Turn_on_time[0][4] = real_time->get_Interval_heating(0, true, 4);

    SendData.Turn_off_time[0][0] = real_time->get_Interval_heating(0, false, 0);
    SendData.Turn_off_time[0][1] = real_time->get_Interval_heating(0, false, 1);
    SendData.Turn_off_time[0][2] = real_time->get_Interval_heating(0, false, 2);
    SendData.Turn_off_time[0][3] = real_time->get_Interval_heating(0, false, 3);
    SendData.Turn_off_time[0][4] = real_time->get_Interval_heating(0, false, 4);

    SendData.Turn_on_time[1][0] = real_time->get_Interval_heating(1, true, 0);
    SendData.Turn_on_time[1][1] = real_time->get_Interval_heating(1, true, 1);
    SendData.Turn_on_time[1][2] = real_time->get_Interval_heating(1, true, 2);
    SendData.Turn_on_time[1][3] = real_time->get_Interval_heating(1, true, 3);
    SendData.Turn_on_time[1][4] = real_time->get_Interval_heating(1, true, 4);

    SendData.Turn_off_time[1][0] = real_time->get_Interval_heating(1, false, 0);
    SendData.Turn_off_time[1][1] = real_time->get_Interval_heating(1, false, 1);
    SendData.Turn_off_time[1][2] = real_time->get_Interval_heating(1, false, 2);
    SendData.Turn_off_time[1][3] = real_time->get_Interval_heating(1, false, 3);
    SendData.Turn_off_time[1][4] = real_time->get_Interval_heating(1, false, 4);

    SendData.State_interval[0][0] = real_time->get_Interval_enable(0, 0);
    SendData.State_interval[0][1] = real_time->get_Interval_enable(0, 1);
    SendData.State_interval[0][2] = real_time->get_Interval_enable(0, 2);
    SendData.State_interval[0][3] = real_time->get_Interval_enable(0, 3);
    SendData.State_interval[0][4] = real_time->get_Interval_enable(0, 4);

    SendData.State_interval[1][0] = real_time->get_Interval_enable(1, 0);
    SendData.State_interval[1][1] = real_time->get_Interval_enable(1, 1);
    SendData.State_interval[1][2] = real_time->get_Interval_enable(1, 2);
    SendData.State_interval[1][3] = real_time->get_Interval_enable(1, 3);
    SendData.State_interval[1][4] = real_time->get_Interval_enable(1, 4);

    SendData.State_interval_Days[0][0] = real_time->get_Interval_Day(0, 0);
    SendData.State_interval_Days[0][1] = real_time->get_Interval_Day(0, 1);
    SendData.State_interval_Days[0][2] = real_time->get_Interval_Day(0, 2);
    SendData.State_interval_Days[0][3] = real_time->get_Interval_Day(0, 3);
    SendData.State_interval_Days[0][4] = real_time->get_Interval_Day(0, 4);
    SendData.State_interval_Days[0][5] = real_time->get_Interval_Day(0, 5);
    SendData.State_interval_Days[0][6] = real_time->get_Interval_Day(0, 6);

    SendData.State_interval_Days[1][0] = real_time->get_Interval_Day(1, 0);
    SendData.State_interval_Days[1][1] = real_time->get_Interval_Day(1, 1);
    SendData.State_interval_Days[1][2] = real_time->get_Interval_Day(1, 2);
    SendData.State_interval_Days[1][3] = real_time->get_Interval_Day(1, 3);
    SendData.State_interval_Days[1][4] = real_time->get_Interval_Day(1, 4);
    SendData.State_interval_Days[1][5] = real_time->get_Interval_Day(1, 5);
    SendData.State_interval_Days[1][6] = real_time->get_Interval_Day(1, 6);

    if (value) {
      Serial.println("1st");
      SendData.Temperature = oven_2->get_Temperature();
      SendData.Humidity = oven_2->get_Humidity();

      esp_now_send(BroadcastAddress_Sender_1, (uint8_t*)&SendData, sizeof(SendData));

      SendData.Temperature = oven_1->get_Temperature();
      SendData.Humidity = oven_1->get_Humidity();

      esp_now_send(BroadcastAddress_Sender_2, (uint8_t*)&SendData, sizeof(SendData));
    } else {
      Serial.println("2st");

      SendData.Temperature = oven_1->get_Temperature();
      SendData.Humidity = oven_1->get_Humidity();

      esp_now_send(BroadcastAddress_Sender_2, (uint8_t*)&SendData, sizeof(SendData));

      SendData.Temperature = oven_2->get_Temperature();
      SendData.Humidity = oven_2->get_Humidity();

      esp_now_send(BroadcastAddress_Sender_1, (uint8_t*)&SendData, sizeof(SendData));
    }
  }
};
