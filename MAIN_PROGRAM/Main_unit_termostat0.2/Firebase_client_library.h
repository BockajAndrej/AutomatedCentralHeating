class Firebase_class {
private:
  Oven* oven_1;
  Oven* oven_2;
  Time* real_time;
  // Define Firebase objects
  FirebaseData fbdo;
  FirebaseAuth auth;
  FirebaseConfig config;

  // Variable to save USER UID
  String uid;

  FirebaseJson json;

  int timestamp1;

  struct Paths_struct {
    // Variables to save database paths
    String databasePath_oven1;
    String databasePath_oven2;

    // Database child nodes
    String TemperaturePath1;
    String TemperaturePath2;
    String HumidityPath1;
    String HumidityPath2;
    String RequiredT_Path1;
    String RequiredT_Path2;
    String PositiveTolerance_Path1;
    String PositiveTolerance_Path2;
    String NegativeTolerance_Path1;
    String NegativeTolerance_Path2;
    String State_Path1;
    String State_Path2;
    String Mode_Path1;
    String Mode_Path2;

    String timePath;

    // Parent Node (to be updated in every loop)
    String parentPath1;
  };
  Paths_struct Path;

  struct ConectionKey_struct {
    String WIFI_SSID;
    String WIFI_PASSWORD;
    String API_KEY;
    String USER_EMAIL;
    String USER_PASSWORD;
    String DATABASE_URL;
  };
  ConectionKey_struct ConnectionKey;

public:
  Firebase_class(Oven* oven_1, Oven* oven_2, Time* real_time, String WIFI_SSID, String WIFI_PASSWORD, String API_KEY, String USER_EMAIL, String USER_PASSWORD, String DATABASE_URL) {
    this->oven_1 = oven_1;
    this->oven_2 = oven_2;
    this->real_time = real_time;

    ConnectionKey.WIFI_SSID = WIFI_SSID;
    ConnectionKey.WIFI_PASSWORD = WIFI_PASSWORD;
    ConnectionKey.API_KEY = API_KEY;
    ConnectionKey.USER_EMAIL = USER_EMAIL;
    ConnectionKey.USER_PASSWORD = USER_PASSWORD;
    ConnectionKey.DATABASE_URL = DATABASE_URL;

    Path.TemperaturePath1 = "/Temperature1";
    Path.TemperaturePath2 = "/Temperature2";
    Path.HumidityPath1 = "/Humidity1";
    Path.HumidityPath2 = "/Humidity2";
    Path.RequiredT_Path1 = "/RequiredT1";
    Path.RequiredT_Path2 = "/RequiredT2";
    Path.PositiveTolerance_Path1 = "/PositiveTolerance1";
    Path.PositiveTolerance_Path2 = "/PositiveTolerance2";
    Path.NegativeTolerance_Path1 = "/NegativeTolerance1";
    Path.NegativeTolerance_Path2 = "/NegativeTolerance2";
    Path.State_Path1 = "/State1";
    Path.State_Path2 = "/State2";
    Path.Mode_Path1 = "/Mode1";
    Path.Mode_Path2 = "/Mode2";

    Path.timePath = "/timestamp";

    timestamp1 = 0;
  }
  void Setup() {
    // Assign the api key (required)
    config.api_key = ConnectionKey.API_KEY;

    // Assign the user sign in credentials
    auth.user.email = ConnectionKey.USER_EMAIL;
    auth.user.password = ConnectionKey.USER_PASSWORD;

    // Assign the RTDB URL (required)
    config.database_url = ConnectionKey.DATABASE_URL;

    Firebase.reconnectWiFi(true);
    fbdo.setResponseSize(4096);

    Serial.print("Sign up new user... ");
    if (Firebase.signUp(&config, &auth, ConnectionKey.USER_EMAIL, ConnectionKey.USER_PASSWORD)) {
      Serial.println("ok");
    } else {
      Serial.printf("%s\n", config.signer.signupError.message.c_str());
    }

    // Assign the callback function for the long running token generation task */
    config.token_status_callback = tokenStatusCallback;  //see addons/TokenHelper.h

    // Assign the maximum retry of token generation
    config.max_token_generation_retry = 5;

    // Initialize the library with the Firebase authen and config
    Firebase.begin(&config, &auth);

    // Getting the user UID might take a few seconds
    Serial.println("Getting User UID");

    int auth_token_counter = 0;

    while ((auth.token.uid) == "") {
      Serial.print('.');
      delay(1000);
      auth_token_counter++;
      if(auth_token_counter >= 6){
        Firebase.begin(&config, &auth);
        Serial.println("Getting User UID");
        auth_token_counter = 0;
      }
    }

    // Print user UID
    uid = auth.token.uid.c_str();
    Serial.print("User UID: ");
    Serial.println(uid);

    // Update database path
    Path.databasePath_oven1 = "/UsersData/" + uid + "/oven1";

  }
  void set_Data(bool value, int option) {  //(option == 1) => new, other update
    if (Firebase.ready()) {
      if (option == 1) {
        timestamp1 = real_time->getTime();
        Path.parentPath1 = Path.databasePath_oven1 + "/" + String(timestamp1);
        if (oven_1->get_Heating()) json.set(Path.State_Path1.c_str(), String(((oven_1->get_Required_T() + oven_1->get_Positive_tolerance()) + 5)));
        else json.set(Path.State_Path1.c_str(), String(0));
        if(oven_1->get_Temperature() < 90) json.set(Path.TemperaturePath1.c_str(), String(oven_1->get_Temperature()));
        else json.set(Path.TemperaturePath1.c_str(), String(0.0));
        json.set(Path.HumidityPath1.c_str(), String(oven_1->get_Humidity()));
        json.set(Path.Mode_Path1.c_str(), String(oven_1->get_Mode()));
        json.set(Path.RequiredT_Path1.c_str(), String(oven_1->get_Required_T()));
        json.set(Path.PositiveTolerance_Path1.c_str(), String((oven_1->get_Required_T() + oven_1->get_Positive_tolerance())));
        json.set(Path.NegativeTolerance_Path1.c_str(), String((oven_1->get_Required_T() - oven_1->get_Negative_tolerance())));

        json.set(Path.timePath, String(timestamp1));

        if (oven_2->get_Heating()) json.set(Path.State_Path2.c_str(), String(((oven_2->get_Required_T() + oven_2->get_Positive_tolerance()) + 5)));
        else json.set(Path.State_Path2.c_str(), String(0));
        if(oven_2->get_Temperature() < 90) json.set(Path.TemperaturePath2.c_str(), String(oven_2->get_Temperature()));
        else json.set(Path.TemperaturePath2.c_str(), String(0.0));
        json.set(Path.HumidityPath2.c_str(), String(oven_2->get_Humidity()));
        json.set(Path.Mode_Path2.c_str(), String(oven_2->get_Mode()));
        json.set(Path.RequiredT_Path2.c_str(), String(oven_2->get_Required_T()));
        json.set(Path.PositiveTolerance_Path2.c_str(), String((oven_2->get_Required_T() + oven_2->get_Positive_tolerance())));
        json.set(Path.NegativeTolerance_Path2.c_str(), String((oven_2->get_Required_T() - oven_2->get_Negative_tolerance())));

        Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, Path.parentPath1.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
      }
      else if(option == 20){
        Path.parentPath1 = Path.databasePath_oven1 + "/" + String(timestamp1);
        if (oven_1->get_Heating()) json.set(Path.State_Path1.c_str(), String(((oven_1->get_Required_T() + oven_1->get_Positive_tolerance()) + 5)));
        else json.set(Path.State_Path1.c_str(), String(0));
        if(oven_1->get_Temperature() < 90) json.set(Path.TemperaturePath1.c_str(), String(oven_1->get_Temperature()));
        else json.set(Path.TemperaturePath1.c_str(), String(0.0));
        json.set(Path.HumidityPath1.c_str(), String(oven_1->get_Humidity()));
        json.set(Path.Mode_Path1.c_str(), String(oven_1->get_Mode()));
        json.set(Path.RequiredT_Path1.c_str(), String(oven_1->get_Required_T()));
        json.set(Path.PositiveTolerance_Path1.c_str(), String((oven_1->get_Required_T() + oven_1->get_Positive_tolerance())));
        json.set(Path.NegativeTolerance_Path1.c_str(), String((oven_1->get_Required_T() - oven_1->get_Negative_tolerance())));

        json.set(Path.timePath, String(timestamp1));

        if (oven_2->get_Heating()) json.set(Path.State_Path2.c_str(), String(((oven_2->get_Required_T() + oven_2->get_Positive_tolerance()) + 5)));
        else json.set(Path.State_Path2.c_str(), String(0));
        if(oven_2->get_Temperature() < 90) json.set(Path.TemperaturePath2.c_str(), String(oven_2->get_Temperature()));
        else json.set(Path.TemperaturePath2.c_str(), String(0.0));
        json.set(Path.HumidityPath2.c_str(), String(oven_2->get_Humidity()));
        json.set(Path.Mode_Path2.c_str(), String(oven_2->get_Mode()));
        json.set(Path.RequiredT_Path2.c_str(), String(oven_2->get_Required_T()));
        json.set(Path.PositiveTolerance_Path2.c_str(), String((oven_2->get_Required_T() + oven_2->get_Positive_tolerance())));
        json.set(Path.NegativeTolerance_Path2.c_str(), String((oven_2->get_Required_T() - oven_2->get_Negative_tolerance())));

        Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, Path.parentPath1.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
      } 
      else if (value) {
        Path.parentPath1 = Path.databasePath_oven1 + "/" + String(timestamp1);
        json.set(Path.Mode_Path1.c_str(), String(oven_1->get_Mode()));
        json.set(Path.RequiredT_Path1.c_str(), String(oven_1->get_Required_T()));
        json.set(Path.PositiveTolerance_Path1.c_str(), String((oven_1->get_Required_T() + oven_1->get_Positive_tolerance())));
        json.set(Path.NegativeTolerance_Path1.c_str(), String((oven_1->get_Required_T() - oven_1->get_Negative_tolerance())));
        json.set(Path.timePath, String(timestamp1));
        Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, Path.parentPath1.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
      } else {
        Path.parentPath1 = Path.databasePath_oven1 + "/" + String(timestamp1);
        json.set(Path.Mode_Path2.c_str(), String(oven_2->get_Mode()));
        json.set(Path.RequiredT_Path2.c_str(), String(oven_2->get_Required_T()));
        json.set(Path.PositiveTolerance_Path2.c_str(), String((oven_2->get_Required_T() + oven_2->get_Positive_tolerance())));
        json.set(Path.NegativeTolerance_Path2.c_str(), String((oven_2->get_Required_T() - oven_2->get_Negative_tolerance())));
        json.set(Path.timePath, String(timestamp1));
        Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, Path.parentPath1.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
      }
    } else {
      Serial.println("ERROR to send data on Firebase!!");
      if (Firebase.isTokenExpired()) {
        Firebase.refreshToken(&config);
        Serial.println("Refresh token");
      } else {
        ESP.restart();
      }
    }
  }

  void get_Data() {
    if (Firebase.ready()) {
      for (int i = 0; i < 4; i++) {
        String local_path = "";

        if (i == 0) local_path = Path.parentPath1 + Path.RequiredT_Path1;
        else if (i == 1) local_path = Path.parentPath1 + Path.RequiredT_Path2;
        else if (i == 2) local_path = Path.parentPath1 + Path.Mode_Path1;
        else if (i == 3) local_path = Path.parentPath1 + Path.Mode_Path2;

        if (Firebase.RTDB.getInt(&fbdo, local_path)) {
          if (fbdo.dataType() == "int") {
            if (i == 0) oven_1->set_Required_T(fbdo.intData());
            else if (i == 1) oven_2->set_Required_T(fbdo.intData());
            else if (i == 2) oven_1->set_Mode(fbdo.intData());
            else if (i == 3) oven_2->set_Mode(fbdo.intData());
          } else {
            if (i == 0) oven_1->set_Required_T(fbdo.stringData().toDouble());
            else if (i == 1) oven_2->set_Required_T(fbdo.stringData().toDouble());
            else if (i == 2) oven_1->set_Mode(fbdo.stringData().toInt());
            else if (i == 3) oven_2->set_Mode(fbdo.stringData().toInt());
          }
        } else {
          Serial.println(fbdo.errorReason());  //print he error (if any)
        }
      }
    }
  }
};