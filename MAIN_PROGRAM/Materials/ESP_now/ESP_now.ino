#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>

#include "ESP_NOW.h"

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

//Sender_1 = e8:31:cd:90:27:90
ESP_NOW* esp_now = new ESP_NOW(1, 0xE8, 0x31, 0xCD, 0x90, 0x27, 0x90);

//Main = 34:86:5D:7B:25:34
//ESP_NOW* esp_now = new ESP_NOW(2, 0x34, 0x86, 0x5D, 0x7B, 0x25, 0x34);

FirebaseJson json;

// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variable to save USER UID
String uid;

// Variables to save database paths
String databasePath;
String parentPath;

String Temperature_1 = "/Temperature_1"; 
String  Humidity_1 = "/Humidity_1"; 
String  Temperature_2 = "/Temperature_2"; 
String  Humidity_2 = "/Humidity_2"; 
String Time_path = "/timestamp";

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
void streamCallback(FirebaseStream data){
  Serial.printf("stream path, %s\nevent path, %s\ndata type, %s\nevent type, %s\n\n",
                data.streamPath().c_str(),
                data.dataPath().c_str(),
                data.dataType().c_str(),
                data.eventType().c_str());
  printResult(data); //see addons/RTDBHelper.h
  Serial.println();

  // Get the path that triggered the function
  String streamPath = String(data.dataPath());

  // if the data returned is an integer, there was a change on the GPIO state on the following path /{gpio_number}
  if (data.dataTypeEnum() == fb_esp_rtdb_data_type_integer){
    String gpio = streamPath.substring(1);
    int state = data.intData();
    Serial.print("GPIO: ");
    Serial.println(gpio);
    Serial.print("STATE: ");
    Serial.println(state);
    digitalWrite(gpio.toInt(), state);
  }

  /* When it first runs, it is triggered on the root (/) path and returns a JSON with all keys
  and values of that path. So, we can get all values from the database and updated the GPIO states*/
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
        digitalWrite(gpio, state);
        Serial.printf("Name: %s, Value: %s, Type: %s\n", value.key.c_str(), value.value.c_str(), value.type == FirebaseJson::JSON_OBJECT ? "object" : "array");
    }
    Serial.println();
    json.iteratorEnd(); // required for free the used memory in iteration (node data collection)
  }
  
  //This is the size of stream payload received (current and max value)
  //Max payload size is the payload size under the stream path since the stream connected
  //and read once and will not update until stream reconnection takes place.
  //This max value will be zero as no payload received in case of ESP8266 which
  //BearSSL reserved Rx buffer size is less than the actual stream payload.
  Serial.printf("Received stream payload size: %d (Max. %d)\n\n", data.payloadLength(), data.maxPayloadLength());
}

void streamTimeoutCallback(bool timeout){
  if (timeout)
    Serial.println("stream timeout, resuming...\n");
  if (!stream.httpConnected())
    Serial.printf("error code: %d, reason: %s\n\n", stream.httpCode(), stream.errorReason().c_str());
}

void sendFloat(String path, float value){
  if (Firebase.RTDB.setFloat(&fbdo, path.c_str(), value)){
    Serial.print("Writing value: ");
    Serial.print (value);
    Serial.print(" on the following path: ");
    Serial.println(path);
    Serial.println("PASSED");
    Serial.println("PATH: " + fbdo.dataPath());
    Serial.println("TYPE: " + fbdo.dataType());
  }
  else {
    Serial.println("FAILED");
    Serial.println("REASON: " + fbdo.errorReason());
  }
}

float i = 0;
int Time = 0;
void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_AP_STA);
  //WiFi.mode(WIFI_STA);

  initWiFi();

  esp_now->Setup();


  config.api_key = API_KEY;

  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;

  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);

  // Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

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
}

void loop() {
  i = i + 1.1;
  if(i > 100.0) i = 0.0;
  Time++;
  if(Time > 500) Time = 0;

  esp_now->Sending();
  delay(5000);
  /*
  if(Firebase.ready()){
     // Send readings to database:
    sendFloat(Temperature_1, i);
    sendFloat(Humidity_1, i);
    sendFloat(Temperature_2, i);
    sendFloat(Humidity_2, i);
  }
*/
  if(Firebase.ready()){
    parentPath= databasePath + "/" + String(Time);

    json.set(Temperature_1.c_str(), String(i));
    json.set(Humidity_1.c_str(), String(i));
    json.set(Temperature_2.c_str(), String(i));
    json.set(Humidity_2.c_str(), String(i));
    json.set(Time_path, String(Time));
    Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
  }
  delay(5000);
} 
