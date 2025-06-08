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
bool Comunication = false;

class ESP_NOW {
private:
  Oven* oven_1;
  Oven* oven_2;
  State* state;

  int ID;

  uint8_t BroadcastAddress[6];
  int prev_ID[2];
  int cas_spanku;
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
  current_mode Current_mode;

  struct alarm {
    bool activated;
    Language* message = new Language();
  };
  alarm Alarm;

  static void OnDataSent(const uint8_t* mac_addr, esp_now_send_status_t status) {
    if (status == ESP_NOW_SEND_SUCCESS) {
      Serial.println("SEND - SUCCESS");
      Comunication = true;
    } else {
      Serial.println("SEND - FAILED");
      Comunication = false;
    }
  }

  // callback function that will be executed when data is received
  static void OnDataRecv(const uint8_t* mac_addr, const uint8_t* incomingData, int len) {
    static char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
             mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    memcpy(&ReceiveData, incomingData, sizeof(ReceiveData));
    Serial.println("Incoming data from: " + ReceiveData.ID);
  }

  int32_t getWiFiChannel(const char* ssid) {
    if (int32_t n = WiFi.scanNetworks()) {
      for (uint8_t i = 0; i < n; i++) {
        if (!strcmp(ssid, WiFi.SSID(i).c_str())) {
          Serial.println("WIFI Channel = " + String(WiFi.channel(i)));
          return WiFi.channel(i);
        }
      }
    }
    Serial.println("WIFI Channel = 0");
    return 0;
  }

public:
  ESP_NOW(uint8_t Number, uint8_t mac_addr_0, uint8_t mac_addr_1, uint8_t mac_addr_2,
          uint8_t mac_addr_3, uint8_t mac_addr_4, uint8_t mac_addr_5, Oven* oven_1_l, Oven* oven_2_l, State* state_l) {

    SendData.ID = Number;
    ID = Number;
    BroadcastAddress[0] = mac_addr_0;
    BroadcastAddress[1] = mac_addr_1;
    BroadcastAddress[2] = mac_addr_2;
    BroadcastAddress[3] = mac_addr_3;
    BroadcastAddress[4] = mac_addr_4;
    BroadcastAddress[5] = mac_addr_5;

    oven_1 = oven_1_l;
    oven_2 = oven_2_l;
    state = state_l;

    prev_ID[0] = 0;
    prev_ID[1] = 0;

    Synchronization = false;

    cas_spanku = 100000;

    Alarm.activated = false;
  }
  void Setup() {

    int32_t channel = getWiFiChannel("Slave_1");
    //int32_t channel = WiFi.channel(0);

    Serial.print("Orig Channel: ");
    Serial.println(channel);

    WiFi.printDiag(Serial);  // Uncomment to verify channel number before
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(false);
    WiFi.printDiag(Serial);  // Uncomment to verify channel change after

    // Init ESP-NOW
    if (esp_now_init() != ESP_OK) {
      Serial.println("Error initializing ESP-NOW");
      return;
    }

    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataRecv);

    // Register peer
    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, BroadcastAddress, 6);
    //peerInfo.channel = 0;
    peerInfo.channel = channel;

    // Add peer
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      Serial.println("Failed to add 1st peer");
      return;
    }
  }
  void set_Current_mode(bool value) {
    if (value) Current_mode = Nominal_run__;
    else Current_mode = Ground_position;
  }
  void set_Sleep_time(int value){
    cas_spanku = value;
  }
  bool get_Current_mode() {
    if (Current_mode == Nominal_run__) return true;
    else return false;
    return false;
  }
  bool get_Comunication() {
    return Comunication;
  }
  int get_ID() {
    return ID;
  }
  bool Nominal_run() {
    if (Comunication == false) {  // Set to 0x45 for alternate i2c addr
      Alarm.activated = true;
      Alarm.message->set_Message("ESP-NOW: Comunikacia ERROR - Velka vzdialenost", "ESP-NOW: Comunication ERROR - LONG Distance");
    } else Alarm.activated = false;

    if (0 != ReceiveData.ID) {
      prev_ID[0] = ReceiveData.ID;

      if (ReceiveData.Current_mode == 1) Current_mode = Nominal_run__;
      else Current_mode = Ground_position;

      if (ID == 3) oven_1->set_Temp_and_Hum(ReceiveData.Temperature, ReceiveData.Humidity);
      else oven_2->set_Temp_and_Hum(ReceiveData.Temperature, ReceiveData.Humidity);

      state->set_Time(ReceiveData.Time_state, ReceiveData.Year_, ReceiveData.Month_, ReceiveData.Day_, ReceiveData.Hour_, ReceiveData.Minute_, ReceiveData.Second_);

      state->set_Ultrasonic(ReceiveData.Ultrasonic_state, ReceiveData.Ultrasonic_Capacite);

      oven_1->set_Enable_heating(ReceiveData.Enable_heating[0]);
      oven_2->set_Enable_heating(ReceiveData.Enable_heating[1]);

      oven_1->set_Heating(ReceiveData.Heating[0]);
      oven_2->set_Heating(ReceiveData.Heating[1]);

      oven_1->set_Required_T(ReceiveData.Required_T[0]);
      oven_2->set_Required_T(ReceiveData.Required_T[1]);

      oven_1->set_Positive_tolerance(ReceiveData.Positive_tolerance[0]);
      oven_2->set_Positive_tolerance(ReceiveData.Positive_tolerance[1]);

      oven_1->set_Negative_tolerance(ReceiveData.Negative_tolerance[0]);
      oven_2->set_Negative_tolerance(ReceiveData.Negative_tolerance[1]);

      oven_1->set_Mode(ReceiveData.Mode[0]);
      oven_2->set_Mode(ReceiveData.Mode[1]);

      if(ReceiveData.PID_state[0]) oven_1->set_PID_Mode(1);
      else oven_1->set_PID_Mode(0);
      if(ReceiveData.PID_state[1]) oven_2->set_PID_Mode(1);
      else oven_2->set_PID_Mode(0);

      if(ReceiveData.Oven_Interval_1[0]) oven_1->set_Heating_Interval(1);
      else if(ReceiveData.Oven_Interval_2[0]) oven_1->set_Heating_Interval(2);
      else if(ReceiveData.Oven_Interval_3[0]) oven_1->set_Heating_Interval(3);
      else oven_1->set_Heating_Interval(0);

      if(ReceiveData.Oven_Interval_1[1]) oven_2->set_Heating_Interval(1);
      else if(ReceiveData.Oven_Interval_2[1]) oven_2->set_Heating_Interval(2);
      else if(ReceiveData.Oven_Interval_3[1]) oven_2->set_Heating_Interval(3);
      else oven_2->set_Heating_Interval(0);

      state->set_Interval_heating(0, true, 0, ReceiveData.Turn_on_time[0][0]);
      state->set_Interval_heating(0, true, 1, ReceiveData.Turn_on_time[0][1]);
      state->set_Interval_heating(0, true, 2, ReceiveData.Turn_on_time[0][2]);
      state->set_Interval_heating(0, true, 3, ReceiveData.Turn_on_time[0][3]);
      state->set_Interval_heating(0, true, 4, ReceiveData.Turn_on_time[0][4]);

      state->set_Interval_heating(0, false, 0, ReceiveData.Turn_off_time[0][0]);
      state->set_Interval_heating(0, false, 1, ReceiveData.Turn_off_time[0][1]);
      state->set_Interval_heating(0, false, 2, ReceiveData.Turn_off_time[0][2]);
      state->set_Interval_heating(0, false, 3, ReceiveData.Turn_off_time[0][3]);
      state->set_Interval_heating(0, false, 4, ReceiveData.Turn_off_time[0][4]);

      state->set_Interval_heating(1, true, 0, ReceiveData.Turn_on_time[1][0]);
      state->set_Interval_heating(1, true, 1, ReceiveData.Turn_on_time[1][1]);
      state->set_Interval_heating(1, true, 2, ReceiveData.Turn_on_time[1][2]);
      state->set_Interval_heating(1, true, 3, ReceiveData.Turn_on_time[1][3]);
      state->set_Interval_heating(1, true, 4, ReceiveData.Turn_on_time[1][4]);

      state->set_Interval_heating(1, false, 0, ReceiveData.Turn_off_time[1][0]);
      state->set_Interval_heating(1, false, 1, ReceiveData.Turn_off_time[1][1]);
      state->set_Interval_heating(1, false, 2, ReceiveData.Turn_off_time[1][2]);
      state->set_Interval_heating(1, false, 3, ReceiveData.Turn_off_time[1][3]);
      state->set_Interval_heating(1, false, 4, ReceiveData.Turn_off_time[1][4]);

      state->set_Interval_enable(0, 0, ReceiveData.State_interval[0][0]);
      state->set_Interval_enable(0, 1, ReceiveData.State_interval[0][1]);
      state->set_Interval_enable(0, 2, ReceiveData.State_interval[0][2]);
      state->set_Interval_enable(0, 3, ReceiveData.State_interval[0][3]);
      state->set_Interval_enable(0, 4, ReceiveData.State_interval[0][4]);

      state->set_Interval_enable(1, 0, ReceiveData.State_interval[1][0]);
      state->set_Interval_enable(1, 1, ReceiveData.State_interval[1][1]);
      state->set_Interval_enable(1, 2, ReceiveData.State_interval[1][2]);
      state->set_Interval_enable(1, 3, ReceiveData.State_interval[1][3]);
      state->set_Interval_enable(1, 4, ReceiveData.State_interval[1][4]);

      state->set_Interval_Days(0, 0, ReceiveData.State_interval_Days[0][0]);
      state->set_Interval_Days(0, 1, ReceiveData.State_interval_Days[0][1]);
      state->set_Interval_Days(0, 2, ReceiveData.State_interval_Days[0][2]);
      state->set_Interval_Days(0, 3, ReceiveData.State_interval_Days[0][3]);
      state->set_Interval_Days(0, 4, ReceiveData.State_interval_Days[0][4]);
      state->set_Interval_Days(0, 5, ReceiveData.State_interval_Days[0][5]);
      state->set_Interval_Days(0, 6, ReceiveData.State_interval_Days[0][6]);

      state->set_Interval_Days(1, 0, ReceiveData.State_interval_Days[1][0]);
      state->set_Interval_Days(1, 1, ReceiveData.State_interval_Days[1][1]);
      state->set_Interval_Days(1, 2, ReceiveData.State_interval_Days[1][2]);
      state->set_Interval_Days(1, 3, ReceiveData.State_interval_Days[1][3]);
      state->set_Interval_Days(1, 4, ReceiveData.State_interval_Days[1][4]);
      state->set_Interval_Days(1, 5, ReceiveData.State_interval_Days[1][5]);
      state->set_Interval_Days(1, 6, ReceiveData.State_interval_Days[1][6]);

      ReceiveData.ID = 0;

      return true;
    }
    return false;
  }

  void Sending(bool which_1, int which_2) {
    Serial.println("Sending data");
    if (ID == 3) {
      SendData.Temperature = oven_2->get_Temperature();
      SendData.Humidity = oven_2->get_Humidity();
      SendData.Sleep_time = cas_spanku;
    } else {
      SendData.Temperature = oven_1->get_Temperature();
      SendData.Humidity = oven_1->get_Humidity();
      SendData.Sleep_time = cas_spanku;
    }

    SendData.Enable_heating[0] = oven_1->get_Enable_heating();
    SendData.Enable_heating[1] = oven_2->get_Enable_heating();

    SendData.Sleep_time = cas_spanku;
    Serial.println("Cas spanku = " + String(SendData.Sleep_time));

    SendData.Current_mode = -1;

    SendData.Positive_tolerance[0] = -1;
    SendData.Negative_tolerance[0] = -1;
    SendData.Required_T[0] = -1;
    SendData.Mode[0] = -1;

    SendData.Positive_tolerance[1] = -1;
    SendData.Negative_tolerance[1] = -1;
    SendData.Required_T[1] = -1;
    SendData.Mode[1] = -1;

    SendData.State_interval[1][5] = false;
    SendData.State_interval[1][5] = false;

    switch (which_2) {
      case 1:
        if (which_1) SendData.Required_T[0] = oven_1->get_Required_T();
        else SendData.Required_T[1] = oven_2->get_Required_T();
        break;
      case 2:
        if (which_1) SendData.Positive_tolerance[0] = oven_1->get_Positive_tolerance();
        else SendData.Positive_tolerance[1] = oven_2->get_Positive_tolerance();
        break;
      case 3:
        if (which_1) SendData.Negative_tolerance[0] = oven_1->get_Negative_tolerance();
        else SendData.Negative_tolerance[1] = oven_2->get_Negative_tolerance();
        break;
      case 4:
        if (which_1) SendData.Mode[0] = oven_1->get_Mode();
        else SendData.Mode[1] = oven_2->get_Mode();
        break;
      case 500:
        if (Current_mode == Nominal_run__) SendData.Current_mode = 1;
        else SendData.Current_mode = 2;
        break;
      case 1000:
        SendData.Positive_tolerance[0] = oven_1->get_Positive_tolerance();
        SendData.Negative_tolerance[0] = oven_1->get_Negative_tolerance();
        SendData.Required_T[0] = oven_1->get_Required_T();
        SendData.Mode[0] = oven_1->get_Mode();

        SendData.Positive_tolerance[1] = oven_2->get_Positive_tolerance();
        SendData.Negative_tolerance[1] = oven_2->get_Negative_tolerance();
        SendData.Required_T[1] = oven_2->get_Required_T();
        SendData.Mode[1] = oven_2->get_Mode();

        if(oven_1->get_PID_Mode() == 0) SendData.PID_state[0] = false;
        else SendData.PID_state[0] = true;
        if(oven_2->get_PID_Mode() == 0) SendData.PID_state[1] = false;
        else SendData.PID_state[1] = true;

        if(oven_1->get_Heating_Interval() == 3){
          SendData.Oven_Interval_1[0] = false;
          SendData.Oven_Interval_2[0] = false;
          SendData.Oven_Interval_3[0] = true;
        }else if(oven_1->get_Heating_Interval() == 2){
          SendData.Oven_Interval_1[0] = false;
          SendData.Oven_Interval_2[0] = true;
          SendData.Oven_Interval_3[0] = false;
        }
        else if(oven_1->get_Heating_Interval() == 1){
          SendData.Oven_Interval_1[0] = true;
          SendData.Oven_Interval_2[0] = false;
          SendData.Oven_Interval_3[0] = false;
        }
        else {
          SendData.Oven_Interval_1[0] = false;
          SendData.Oven_Interval_2[0] = false;
          SendData.Oven_Interval_3[0] = false;
        }

        if(oven_2->get_Heating_Interval() == 3){
          SendData.Oven_Interval_1[1] = false;
          SendData.Oven_Interval_2[1] = false;
          SendData.Oven_Interval_3[1] = true;
        }else if(oven_2->get_Heating_Interval() == 2){
          SendData.Oven_Interval_1[1] = false;
          SendData.Oven_Interval_2[1] = true;
          SendData.Oven_Interval_3[1] = false;
        }
        else if(oven_2->get_Heating_Interval() == 1){
          SendData.Oven_Interval_1[1] = true;
          SendData.Oven_Interval_2[1] = false;
          SendData.Oven_Interval_3[1] = false;
        }
        else{
          SendData.Oven_Interval_1[1] = false;
          SendData.Oven_Interval_2[1] = false;
          SendData.Oven_Interval_3[1] = false;
        }


        SendData.Turn_on_time[0][0] = state->get_Interval_heating(0, true, 0);
        SendData.Turn_on_time[0][1] = state->get_Interval_heating(0, true, 1);
        SendData.Turn_on_time[0][2] = state->get_Interval_heating(0, true, 2);
        SendData.Turn_on_time[0][3] = state->get_Interval_heating(0, true, 3);
        SendData.Turn_on_time[0][4] = state->get_Interval_heating(0, true, 4);

        SendData.Turn_off_time[0][0] = state->get_Interval_heating(0, false, 0);
        SendData.Turn_off_time[0][1] = state->get_Interval_heating(0, false, 1);
        SendData.Turn_off_time[0][2] = state->get_Interval_heating(0, false, 2);
        SendData.Turn_off_time[0][3] = state->get_Interval_heating(0, false, 3);
        SendData.Turn_off_time[0][4] = state->get_Interval_heating(0, false, 4);

        SendData.Turn_on_time[1][0] = state->get_Interval_heating(1, true, 0);
        SendData.Turn_on_time[1][1] = state->get_Interval_heating(1, true, 1);
        SendData.Turn_on_time[1][2] = state->get_Interval_heating(1, true, 2);
        SendData.Turn_on_time[1][3] = state->get_Interval_heating(1, true, 3);
        SendData.Turn_on_time[1][4] = state->get_Interval_heating(1, true, 4);

        SendData.Turn_off_time[1][0] = state->get_Interval_heating(1, false, 0);
        SendData.Turn_off_time[1][1] = state->get_Interval_heating(1, false, 1);
        SendData.Turn_off_time[1][2] = state->get_Interval_heating(1, false, 2);
        SendData.Turn_off_time[1][3] = state->get_Interval_heating(1, false, 3);
        SendData.Turn_off_time[1][4] = state->get_Interval_heating(1, false, 4);

        SendData.State_interval[0][0] = state->get_Interval_enable(0, 0);
        SendData.State_interval[0][1] = state->get_Interval_enable(0, 1);
        SendData.State_interval[0][2] = state->get_Interval_enable(0, 2);
        SendData.State_interval[0][3] = state->get_Interval_enable(0, 3);
        SendData.State_interval[0][4] = state->get_Interval_enable(0, 4);
        SendData.State_interval[0][5] = true;

        SendData.State_interval[1][0] = state->get_Interval_enable(1, 0);
        SendData.State_interval[1][1] = state->get_Interval_enable(1, 1);
        SendData.State_interval[1][2] = state->get_Interval_enable(1, 2);
        SendData.State_interval[1][3] = state->get_Interval_enable(1, 3);
        SendData.State_interval[1][4] = state->get_Interval_enable(1, 4);
        SendData.State_interval[1][5] = true;

        SendData.State_interval_Days[0][0] = state->get_Interval_Day(0, 0);
        SendData.State_interval_Days[0][1] = state->get_Interval_Day(0, 1);
        SendData.State_interval_Days[0][2] = state->get_Interval_Day(0, 2);
        SendData.State_interval_Days[0][3] = state->get_Interval_Day(0, 3);
        SendData.State_interval_Days[0][4] = state->get_Interval_Day(0, 4);
        SendData.State_interval_Days[0][5] = state->get_Interval_Day(0, 5);
        SendData.State_interval_Days[0][6] = state->get_Interval_Day(0, 6);

        SendData.State_interval_Days[1][0] = state->get_Interval_Day(1, 0);
        SendData.State_interval_Days[1][1] = state->get_Interval_Day(1, 1);
        SendData.State_interval_Days[1][2] = state->get_Interval_Day(1, 2);
        SendData.State_interval_Days[1][3] = state->get_Interval_Day(1, 3);
        SendData.State_interval_Days[1][4] = state->get_Interval_Day(1, 4);
        SendData.State_interval_Days[1][5] = state->get_Interval_Day(1, 5);
        SendData.State_interval_Days[1][6] = state->get_Interval_Day(1, 6);

        break;
      default:
        break;
    }
    SendData.Sleep_time = cas_spanku;
    Serial.println("Size of data structure = " + String(sizeof(SendData)));
    esp_now_send(BroadcastAddress, (uint8_t*)&SendData, sizeof(SendData));
  }
  bool get_Alarm_activated() {
    return Alarm.activated;
  }
  String get_Alarm_message() {
    return Alarm.message->get_Message();
  }
};