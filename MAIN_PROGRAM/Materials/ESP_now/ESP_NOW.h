typedef struct struct_message {
  byte ID;      
  float Temperature;    
  float Humidity;    
  double Positive_tolerance[2];  
  double Negative_tolerance[2];
  double Required_T[2];
  uint8_t Mode[2];           

  uint8_t Synch;
  
  //Time
  uint16_t Year_;
  uint8_t Month_;
  uint8_t Day_;
  uint8_t Hour_;
  uint8_t Minute_;
  uint8_t Second_;
    
  int Turn_on_time[2][5];   
  int Turn_off_time[2][5];  
  bool State_interval[2][6]; //Binary
  //6 - enable for receive

  bool Heating[2];  //ci je rele aktivne alebo nie 

  uint8_t kapacita_pece;

  uint16_t cas_spanku;
} struct_message;

struct_message SendData;
struct_message ReceiveData;

class ESP_NOW{
private:
  
  bool Send_result;
  uint8_t BroadcastAddress[6];

  struct Time_struct{
    unsigned long Update_time;
    unsigned long Max_Update_time;
    unsigned long Min_Update_time;
  };
  Time_struct Time;

  struct alarm{
    bool activated;
  };
  alarm Alarm;

  static void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    if(status == ESP_NOW_SEND_SUCCESS) Serial.println("ODOSLALO");
    else Serial.println("FALSE_odoslanie");
  }
  
  // callback function that will be executed when data is received
  static void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) {
    static char macStr[18];
    Serial.print("Packet received from: ");
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
             mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    memcpy(&ReceiveData, incomingData, sizeof(ReceiveData));
    Serial.println(ReceiveData.ID);
    Serial.println(ReceiveData.Temperature);
    Serial.println(ReceiveData.Humidity);
  }
  
public:
  ESP_NOW(uint8_t Number, uint8_t mac_addr_0, uint8_t mac_addr_1, uint8_t mac_addr_2, 
          uint8_t mac_addr_3, uint8_t mac_addr_4, uint8_t mac_addr_5)
          {
    
    SendData.ID = Number;
    Send_result = false;
    BroadcastAddress[0] = mac_addr_0;
    BroadcastAddress[1] = mac_addr_1;
    BroadcastAddress[2] = mac_addr_2;
    BroadcastAddress[3] = mac_addr_3;
    BroadcastAddress[4] = mac_addr_4;
    BroadcastAddress[5] = mac_addr_5;
  }
  int32_t getWiFiChannel(const char *ssid) {
    if (int32_t n = WiFi.scanNetworks()) {
        for (uint8_t i=0; i<n; i++) {
            if (!strcmp(ssid, WiFi.SSID(i).c_str())) {
                return WiFi.channel(i);
            }
        }
    }
    return 0;
  }

  void Setup(){
    /*
    int32_t channel = getWiFiChannel("ESP_7B2535");

    WiFi.printDiag(Serial); // Uncomment to verify channel number before
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(false);
    WiFi.printDiag(Serial); // Uncomment to verify channel change after
*/
     // Init ESP-NOW
    if (esp_now_init() != ESP_OK) {
      Serial.println("Error initializing ESP-NOW");
    }
    /*
    uint8_t primaryChan = 0;
    wifi_second_chan_t secondChan = WIFI_SECOND_CHAN_NONE;
    esp_wifi_get_channel(&primaryChan, &secondChan);
    ESP_LOGI(TAG, "CONFIG_ESPNOW_CHANNEL=%d; WiFi is on channel %d  ", CONFIG_ESPNOW_CHANNEL, primaryChan);
*/
    // Register peer
    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, BroadcastAddress, 6);
    peerInfo.channel = 0;  
    
    // Add peer        
    if (esp_now_add_peer(&peerInfo) != ESP_OK){
      Serial.println("Failed to add 1st peer");
    }

    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataRecv);
  }

  void Sending(){
    
    SendData.Temperature = 22.5;
    SendData.Humidity = 80.1;
    
    SendData.Positive_tolerance[0] = 2.2;  
    SendData.Negative_tolerance[0] = 3.4;
    SendData.Required_T[0] = 25.6;
    SendData.Mode[0] = 1;

     SendData.Positive_tolerance[1] = 3.2;  
    SendData.Negative_tolerance[1] = 4.4;
    SendData.Required_T[1] = 23.6;
    SendData.Mode[1] = 2;
    Serial.println("SEND");
    
    esp_now_send(BroadcastAddress, (uint8_t *) &SendData, sizeof(SendData));
  }
};
