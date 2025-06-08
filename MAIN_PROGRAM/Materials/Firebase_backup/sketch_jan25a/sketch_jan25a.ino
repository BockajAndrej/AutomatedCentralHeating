
#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "time.h"

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID "Boky_2G"
#define WIFI_PASSWORD "gameover2"

// Insert Firebase project API Key
#define API_KEY "AIzaSyAkXu2A9Ank8M8ISkFQzLbjylPZ2WpxrNU"

// Insert Authorized Email and Corresponding Password
#define USER_EMAIL "bockajandrej@gmail.com"
#define USER_PASSWORD "8kmfgaq4"

// Insert RTDB URLefine the RTDB URL
#define DATABASE_URL "https://smartstovecontrol-default-rtdb.europe-west1.firebasedatabase.app/"

// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variable to save USER UID
String uid;

// Database main path (to be updated in setup with the user UID)
String databasePath;
String databasePathListener;

// Database child nodes
String TemperaturePath1 = "/Temperature1";
String TemperaturePath2 = "/Temperature2";
String HumidityPath1 = "/Humidity1";
String HumidityPath2 = "/Humidity2";
String RequiredT_Path1 = "/RequiredT1";
String RequiredT_Path2 = "/RequiredT2";
String PositiveTolerance_Path1 = "/PositiveTolerance1";
String PositiveTolerance_Path2 = "/PositiveTolerance2";
String NegativeTolerance_Path1 = "/NegativeTolerance1";
String NegativeTolerance_Path2 = "/NegativeTolerance2";
String State_Path1 = "/State1";
String State_Path2 = "/State2";
String Mode_Path1 = "/Mode1";
String Mode_Path2 = "/Mode2";

String timePath = "/timestamp";

// Parent Node (to be updated in every loop)
String parentPath;

int timestamp;
FirebaseJson json;

const char* ntpServer = "pool.ntp.org";

// BME280 sensor
Adafruit_BME280 bme;  // I2C
float temperature;
float humidity;
float pressure;

// Timer variables (send new readings every three minutes)
unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 20000;

bool signupSuccess = false;

// Initialize WiFi
void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  Serial.println();
}

// Callback function that runs on database changes
void streamCallback(FirebaseStream data) {
  Serial.printf("stream path, %s\nevent path, %s\ndata type, %s\nevent type, %s\n\n",
                data.streamPath().c_str(),
                data.dataPath().c_str(),
                data.dataType().c_str(),
                data.eventType().c_str());
  printResult(data);  //see addons/RTDBHelper.h
  Serial.println();

  // Get the path that triggered the function
  String streamPath = String(data.dataPath());

  // if the data returned is an integer, there was a change on the GPIO state on the following path /{gpio_number}
  if (data.dataTypeEnum() == fb_esp_rtdb_data_type_integer) {
    String gpio = streamPath.substring(1);
    int state = data.intData();
    Serial.print("GPIO: ");
    Serial.println(gpio);
    Serial.print("STATE: ");
    Serial.println(state);
  }

  /* When it first runs, it is triggered on the root (/) path and returns a JSON with all keys
  and values of that path. So, we can get all values from the database and updated the GPIO states*/
  /*
  if (data.dataTypeEnum() == fb_esp_rtdb_data_type_json){
    FirebaseJson json = data.to<FirebaseJson>();

    // To iterate all values in Json object
    size_t count = json.iteratorBegin();
    Serial.println("\n---------");
    for (size_t i = 0; i < count; i++){
        FirebaseJson::IteratorValue value = json.valueAt(i);
        int gpio = value.key.toInt();
        int state = value.value.toInt();
        Serial.print("STATE: ");
        Serial.println(state);
        Serial.print("GPIO:");
        Serial.println(gpio);
        Serial.printf("Name: %s, Value: %s, Type: %s\n", value.key.c_str(), value.value.c_str(), value.type == FirebaseJson::JSON_OBJECT ? "object" : "array");
    }
    Serial.println();
    json.iteratorEnd(); // required for free the used memory in iteration (node data collection)
  }
  */
  //This is the size of stream payload received (current and max value)
  //Max payload size is the payload size under the stream path since the stream connected
  //and read once and will not update until stream reconnection takes place.
  //This max value will be zero as no payload received in case of ESP8266 which
  //BearSSL reserved Rx buffer size is less than the actual stream payload.
  //Serial.printf("Received stream payload size: %d (Max. %d)\n\n", data.payloadLength(), data.maxPayloadLength());
}

void streamTimeoutCallback(bool timeout) {

  if (timeout)
    Serial.println("stream timeout, resuming...\n");
  if (!fbdo.httpConnected())
    Serial.printf("error code: %d, reason: %s\n\n", fbdo.httpCode(), fbdo.errorReason().c_str());
}

// Function that gets current epoch time
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

void setup() {
  Serial.begin(115200);

  // Initialize BME280 sensor
  initWiFi();
  configTime(0, 0, ntpServer);

  // Assign the api key (required)
  config.api_key = API_KEY;

  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;

  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);

  Serial.print("Sign up new user... ");
  if (Firebase.signUp(&config, &auth, USER_EMAIL, USER_PASSWORD)) {
    Serial.println("ok");
    signupSuccess = true;
  }
  else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  // Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback;  //see addons/TokenHelper.h

  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  //if (!Firebase.RTDB.beginStream(&fbdo, databasePathListener.c_str()));

  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);

  // Update database path
  databasePath = "/UsersData/" + uid + "/readings";
  databasePathListener = "/UsersData/" + uid + "/writing";

  // Streaming (whenever data changes on a path)
  // Begin stream on a database path --> board1/outputs/digital
  //if (!Firebase.RTDB.beginStream(&fbdo, databasePathListener.c_str()))
  //Serial.printf("stream begin error, %s\n\n", fbdo.errorReason().c_str());

  // Assign a calback function to run when it detects changes on the database
  //Firebase.RTDB.setStreamCallback(&fbdo, streamCallback, streamTimeoutCallback);

  delay(2000);
}

void loop() {
  int aaa = 0;

  // Send new readings to database
  if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();

    if (Firebase.RTDB.getInt(&fbdo, databasePathListener + "/RequiredT2")){
      if (fbdo.dataType() == "int"){
        int read_data = fbdo.intData();
        Serial.print("Int Data2 received: ");
        Serial.println(read_data);  //print the data received from the Firebase database
      }
      else{
        int read_dataa = fbdo.stringData().toInt();
        Serial.print("String Data2 received: ");
        Serial.println(read_dataa);
      }
    }
    else{
      Serial.println(fbdo.errorReason());  //print he error (if any)
    }

    if (Firebase.RTDB.getInt(&fbdo, databasePathListener + "/RequiredT1")){
      if (fbdo.dataType() == "int"){
        int read_data = fbdo.intData();
        Serial.print("Int Data1 received: ");
        Serial.println(read_data);  //print the data received from the Firebase database
      }
      else{
        int read_dataa = fbdo.stringData().toInt();
        Serial.print("String Data1 received: ");
        Serial.println(read_dataa);
      }
    }
    else{
      Serial.println(fbdo.errorReason());  //print he error (if any)
    }

    //Get current timestamp
    timestamp = getTime();
    Serial.print("time: ");
    Serial.println(timestamp);

    parentPath = databasePath + "/" + String(timestamp);

    if (random(2) == 1) aaa = 26;

    json.set(State_Path1.c_str(), String(aaa));
    json.set(TemperaturePath1.c_str(), String(random(10, 30)));
    json.set(HumidityPath1.c_str(), String(random(10, 70)));
    json.set(RequiredT_Path1.c_str(), String(25));
    json.set(PositiveTolerance_Path1.c_str(), String(26));
    json.set(NegativeTolerance_Path1.c_str(), String(24));

    json.set(State_Path2.c_str(), String(aaa));
    json.set(TemperaturePath2.c_str(), String(random(10, 30)));
    json.set(HumidityPath2.c_str(), String(random(10, 70)));
    json.set(RequiredT_Path2.c_str(), String(20));
    json.set(PositiveTolerance_Path2.c_str(), String(22));
    json.set(NegativeTolerance_Path2.c_str(), String(17));

    json.set(timePath, String(timestamp));
    Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());

    delay(500);
  } else if (Firebase.isTokenExpired()) {
    Firebase.refreshToken(&config);
    Serial.println("Refresh token");
  }
}