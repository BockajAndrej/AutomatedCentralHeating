#include "FS.h"
#include <TFT_eSPI.h>
#include <SPI.h>

#include "Images.h"
#include "Faceplate_Class.h"

#define CALIBRATION_FILE "/calibrationData2"

class HMI {
private:
  TFT_eSPI* tft = new TFT_eSPI();
  TFT_eSprite* img = new TFT_eSprite(tft);
  TFT_eSprite* img_icon = new TFT_eSprite(tft);
  TFT_eSprite* img_background = new TFT_eSprite(tft);

  Oven* oven_1;
  Oven* oven_2;
  Temperature_sensor* Temp;
  State* state;
  ESP_NOW* esp_now;
  Battery* battery;
  Flash* flash;

  Faceplate_Class* Faceplate = new Faceplate_Class(tft, img, img_icon, img_background);

  Language* Message = new Language();

  bool REPEAT_CAL;

  enum current_mode {
    Nominal_run = 1,
    Ground_position = 2,
    ERROR = 10
  };
  current_mode Current_mode;
  current_mode prev_Current_mode;


  struct alarm {
    bool activated;
    Language* message = new Language();
  };
  alarm Alarm;

  struct display_property {
    int Width;
    int Height;
    bool ALLWAIS_ON;
    int LOOP_PERIOD;
    short Rotation;
    int Text_color;
    unsigned long UpdateTimeHMI_LOOP;
    bool first_draw;
    unsigned long Refresh_time;
    struct Pressing_struct {
      int Enable_timer;
      unsigned long Time;
      int Count;
      int ID;
    };
    Pressing_struct Pressing;
    struct Brightness_struct {
      int Pin;
      int Pin_enable;
      int PWMfreq;
      int PWMChannel;
      int PWMresolution;
      int Value;
    };
    Brightness_struct Brightness;
    struct color {
      int Transparent[3] = { 999, 999, 999 };
      int Background[3];
      int Text[3];
      int Focused[3];
      int Red[3] = { 255, 0, 0 };
      int Green[3] = { 0, 255, 0 };
      int Lite_blue[3] = { 0, 165, 255 };
      int Blue[3] = { 0, 0, 255 };
      int Dark_blue[3] = { 0, 0, 153 };
      int Black[3] = { 0, 0, 0 };
      int White[3] = { 255, 255, 255 };
      int Yellow[3] = { 255, 255, 0 };
      int Orange[3] = { 255, 100, 0 };
    };
    color Color;
  };
  display_property Property;

  enum Page {
    Main_page = 1,
    Menu = 2,
    Settings = 3,
    Heating_circruit_ = 10,
    Ultrasonic_ = 11,
    Temperature_sensor_ = 12,
    Time_ = 13
  };
  Page page;
  Page prev_page;

  struct SLEEP {
    int Time_to_sleep_constant;
    int Time;
  };
  SLEEP Sleep;

public:
  HMI(Oven* oven_1_l, Oven* oven_2_l, Temperature_sensor* Temp_l, State* state_l, ESP_NOW* esp_now_l, Battery* battery, Flash* flash) {
    oven_1 = oven_1_l;
    oven_2 = oven_2_l;
    Temp = Temp_l;
    state = state_l;
    esp_now = esp_now_l;
    this->battery = battery;
    this->flash = flash;

    page = Main_page;
    prev_page = Menu;
    Current_mode = Ground_position;
    prev_Current_mode = Current_mode;

    Alarm.activated = false;

    Property.UpdateTimeHMI_LOOP = 0;
    Property.LOOP_PERIOD = 20;
    Property.Width = 480;
    Property.Height = 320;
    Property.ALLWAIS_ON = false;
    Property.first_draw = true;

    Property.Pressing.Time = 0;
    Property.Pressing.Enable_timer = false;

    Property.Brightness.PWMfreq = 5000;
    Property.Brightness.PWMChannel = 0;
    Property.Brightness.PWMresolution = 8;
    Property.Brightness.Value = 255;

    Property.Color.Transparent[0] = 999;
    Property.Color.Background[0] = 0;
    Property.Color.Background[1] = 0;
    Property.Color.Background[2] = 0;
    Property.Color.Text[0] = 255;
    Property.Color.Text[1] = 255;
    Property.Color.Text[2] = 255;
    Property.Color.Focused[0] = 0;
    Property.Color.Focused[1] = 255;
    Property.Color.Focused[2] = 0;

    Touch.x = 0;
    Touch.y = 0;
    Touch.raw_Pressed = false;
    Touch.UpdateTimeHMI_Touch = 0;
    Touch.Pressed = false;
    Touch.prev_Pressed = Touch.Pressed;
    Touch.Pressing_period = 80;
    Touch.Releast = false;

    Sleep.Time_to_sleep_constant = 30;
    Sleep.Time = 1;

    Settings_str.prev_language = Message->get_language();

    TopBar.Height = 30;

    Heating_circuit.Circuit = true;
    Heating_circuit.prev_Circuit = Heating_circuit.Circuit;
    Heating_circuit.Page = true;

    Time_Struct.Page_ = true;

    Temp_Struct.last_Mode = Temp->get_Mode();

    ERROR_page.last_number_of_errors = 0;
    ERROR_page.number_of_errors = 0;
  }
  void Setup(int Pin_enable, int Pin, bool value) {

    pinMode(Pin_enable, OUTPUT);
    if (value) digitalWrite(Pin_enable, HIGH);
    else digitalWrite(Pin_enable, LOW);
    Property.Brightness.Pin_enable = Pin_enable;

    // configure LED PWM functionalitites
    ledcSetup(Property.Brightness.PWMChannel, Property.Brightness.PWMfreq, Property.Brightness.PWMresolution);

    // attach the channel to the GPIO to be controlled
    ledcAttachPin(Pin, Property.Brightness.PWMChannel);
    Property.Brightness.Pin = Pin;

    //ledcWrite(Property.Brightness.PWMChannel, 0);

    tft->init();
    tft->setSwapBytes(true);
    img->setColorDepth(8);  // Set colour depth of Sprite to 8 (or 16) bits
    img_icon->setSwapBytes(true);
    //img_background->setColorDepth(8); // Set colour depth of Sprite to 8 (or 16) bits
    //img_background->setSwapBytes(true);
    Property.Rotation = flash->get_Flash(30);
    tft->setRotation(Property.Rotation);

    // Calibrate the touch screen and retrieve the scaling factors
    REPEAT_CAL = (bool)flash->get_Flash(31);
    touch_calibrate();
    REPEAT_CAL = false;
    if ((bool)flash->get_Flash(31)) flash->set_Flash(31, REPEAT_CAL);

    if ((int)flash->get_Flash(10)) Sleep.Time_to_sleep_constant = (int)flash->get_Flash(10);
    if ((int)flash->get_Flash(11)) Sleep.Time = (int)flash->get_Flash(11);

    if(flash->get_Flash(1)) Message->Set_language("EN");
    else Message->Set_language("SK");

    tft->setTextSize(1);
    tft->setTextColor(tft->color565(Property.Color.Text[0], Property.Color.Text[1], Property.Color.Text[2]), tft->color565(Property.Color.Background[0], Property.Color.Background[1], Property.Color.Background[2]));
    tft->fillScreen(tft->color565(Property.Color.Background[0], Property.Color.Background[1], Property.Color.Background[2]));
    Property.UpdateTimeHMI_LOOP = millis();
    Touch.UpdateTimeHMI_Touch = millis();

    Property.Refresh_time = millis();
  }
  void Refresh(bool Draw) {
    if (Current_mode == ERROR) digitalWrite(Property.Brightness.Pin_enable, HIGH);
    else digitalWrite(Property.Brightness.Pin_enable, LOW);

    ledcWrite(Property.Brightness.PWMChannel, Property.Brightness.Value);

    if ((battery->get_State() != 0) && (battery->get_State() != 1)) Property.ALLWAIS_ON = false;

    if (Draw) Property.first_draw = true;

    if (((Property.Pressing.Time + Property.Pressing.Count) <= millis()) && Property.Pressing.Enable_timer) {

      switch (Property.Pressing.ID) {
        case 1:
          if (Heating_circuit.Circuit) oven_1->set_Required_T(oven_1->get_Required_T() + 0.1);
          else oven_2->set_Required_T(oven_2->get_Required_T() + 0.1);
          break;
        case 2:
          if (Heating_circuit.Circuit) oven_1->set_Required_T(oven_1->get_Required_T() - 0.1);
          else oven_2->set_Required_T(oven_2->get_Required_T() - 0.1);
          break;
        case 3:
          if (Heating_circuit.Circuit) oven_1->set_Positive_tolerance(oven_1->get_Positive_tolerance() + 0.1);
          else oven_2->set_Positive_tolerance(oven_2->get_Positive_tolerance() + 0.1);
          break;
        case 4:
          if (Heating_circuit.Circuit) oven_1->set_Positive_tolerance(oven_1->get_Positive_tolerance() - 0.1);
          else oven_2->set_Positive_tolerance(oven_2->get_Positive_tolerance() - 0.1);
          break;
        case 5:
          if (Heating_circuit.Circuit) oven_1->set_Negative_tolerance(oven_1->get_Negative_tolerance() + 0.1);
          else oven_2->set_Negative_tolerance(oven_2->get_Negative_tolerance() + 0.1);
          break;
        case 6:
          if (Heating_circuit.Circuit) oven_1->set_Negative_tolerance(oven_1->get_Negative_tolerance() - 0.1);
          else oven_2->set_Negative_tolerance(oven_2->get_Negative_tolerance() - 0.1);
          break;
        case 7:
          if (Heating_circuit.Circuit) oven_1->set_Upper_limit(oven_1->get_Upper_limit() + 1);
          else oven_2->set_Upper_limit(oven_2->get_Upper_limit() + 1);
          break;
        case 8:
          if (Heating_circuit.Circuit) oven_1->set_Upper_limit(oven_1->get_Upper_limit() - 1);
          else oven_2->set_Upper_limit(oven_2->get_Upper_limit() - 1);
          break;
        case 9:
          if (Heating_circuit.Circuit) oven_1->set_Lower_limit(oven_1->get_Lower_limit() + 1);
          else oven_2->set_Lower_limit(oven_2->get_Lower_limit() + 1);
          break;
        case 10:
          if (Heating_circuit.Circuit) oven_1->set_Lower_limit(oven_1->get_Lower_limit() - 1);
          else oven_2->set_Lower_limit(oven_2->get_Lower_limit() - 1);
          break;
        case 21:
          if (Property.Pressing.Count < 250) {
            Time_Struct.Page_ = false;
            Time_Struct.ID = 0;
          }
          break;
        case 22:
          if (Property.Pressing.Count < 250) {
            Time_Struct.Page_ = false;
            Time_Struct.ID = 1;
          }
          break;
        case 23:
          if (Property.Pressing.Count < 250) {
            Time_Struct.Page_ = false;
            Time_Struct.ID = 2;
          }
          break;
        case 24:
          if (Property.Pressing.Count < 250) {
            Time_Struct.Page_ = false;
            Time_Struct.ID = 3;
          }
          break;
        case 25:
          if (Property.Pressing.Count < 250) {
            Time_Struct.Page_ = false;
            Time_Struct.ID = 4;
          }
          break;
        case 26:
          if (Property.Pressing.Count < 250) {
            Time_Struct.Page_ = false;
            Time_Struct.ID = 0;
          }
          break;
        case 27:
          if (Property.Pressing.Count < 250) {
            Time_Struct.Page_ = false;
            Time_Struct.ID = 1;
          }
          break;
        case 28:
          if (Property.Pressing.Count < 250) {
            Time_Struct.Page_ = false;
            Time_Struct.ID = 2;
          }
          break;
        case 29:
          if (Property.Pressing.Count < 250) {
            Time_Struct.Page_ = false;
            Time_Struct.ID = 3;
          }
          break;
        case 30:
          if (Property.Pressing.Count < 250) {
            Time_Struct.Page_ = false;
            Time_Struct.ID = 4;
          }
          break;
        case 31:
          state->set_Interval_heating(Time_Struct.Which, true, Time_Struct.ID, (state->get_Interval_heating(Time_Struct.Which, true, Time_Struct.ID) + 60));
          break;
        case 32:
          state->set_Interval_heating(Time_Struct.Which, true, Time_Struct.ID, (state->get_Interval_heating(Time_Struct.Which, true, Time_Struct.ID) - 60));
          break;
        case 33:
          state->set_Interval_heating(Time_Struct.Which, true, Time_Struct.ID, (state->get_Interval_heating(Time_Struct.Which, true, Time_Struct.ID) + 1));
          break;
        case 34:
          state->set_Interval_heating(Time_Struct.Which, true, Time_Struct.ID, (state->get_Interval_heating(Time_Struct.Which, true, Time_Struct.ID) - 1));
          break;
        case 35:
          state->set_Interval_heating(Time_Struct.Which, false, Time_Struct.ID, (state->get_Interval_heating(Time_Struct.Which, false, Time_Struct.ID) + 60));
          break;
        case 36:
          state->set_Interval_heating(Time_Struct.Which, false, Time_Struct.ID, (state->get_Interval_heating(Time_Struct.Which, false, Time_Struct.ID) - 60));
          break;
        case 37:
          state->set_Interval_heating(Time_Struct.Which, false, Time_Struct.ID, (state->get_Interval_heating(Time_Struct.Which, false, Time_Struct.ID) + 1));
          break;
        case 38:
          state->set_Interval_heating(Time_Struct.Which, false, Time_Struct.ID, (state->get_Interval_heating(Time_Struct.Which, false, Time_Struct.ID) - 1));
          break;
        case 40:
          Temp->set_Update_time(Temp->get_Update_time() + 1000);
          break;
        case 41:
          Temp->set_Update_time(Temp->get_Update_time() - 1000);
          break;
        case 42:
          if (Heating_circuit.Circuit) oven_1->set_Heating_Interval(oven_1->get_Heating_Interval() + 1);
          else oven_2->set_Heating_Interval(oven_2->get_Heating_Interval() + 1);
          break;
        case 43:
          if (Heating_circuit.Circuit) oven_1->set_Heating_Interval(oven_1->get_Heating_Interval() - 1);
          else oven_2->set_Heating_Interval(oven_2->get_Heating_Interval() - 1);
          break;
        case 45:
          set_Sleep_time(get_Sleep_time() + 1);
          break;
        case 46:
          set_Sleep_time(get_Sleep_time() - 1);
          break;
        case 47:
          set_Sleep_time_constant(get_Sleep_time_constant() + 1);
          break;
        case 48:
          set_Sleep_time_constant(get_Sleep_time_constant() - 1);
          break;
        default:
          break;
      }
      Property.Pressing.Time = millis();
      if (Property.Pressing.Count > 210) Property.Pressing.Count = Property.Pressing.Count / 1.5;
      else if (Property.Pressing.Count < 210 && Property.Pressing.Count > 116) Property.Pressing.Count -= 15;
    }

    Property.Pressing.ID = 0;

    if (Property.Color.Background[0] == 0) {
      Property.Color.Text[0] = 255;
      Property.Color.Text[1] = 255;
      Property.Color.Text[2] = 255;
    } else {
      Property.Color.Text[0] = 0;
      Property.Color.Text[1] = 0;
      Property.Color.Text[2] = 0;
    }

    if ((Property.UpdateTimeHMI_LOOP <= millis()) && (Current_mode != ERROR)) {
      Touch_detector();
      Property.Refresh_time = millis();
      Property.UpdateTimeHMI_LOOP = millis() + Property.LOOP_PERIOD;
      if ((page != prev_page) || (prev_Current_mode == ERROR)) {
        tft->fillScreen(tft->color565(Property.Color.Background[0], Property.Color.Background[1], Property.Color.Background[2]));
        Property.first_draw = true;
        prev_page = page;
        prev_Current_mode = Current_mode;
      }
      fcTop_bar();
      switch (page) {
        case 1:
          if (fcMain_page() == false) {
            Alarm.activated = true;
            Alarm.message->set_Message("Displej/Refresh: Error = Id out of range", "Displej/Refresh: Chyba = Id mimo rozsah");
          } else Alarm.activated = false;
          break;
        case 2:
          if (fcMenu() == false) {
            Alarm.activated = true;
            Alarm.message->set_Message("Displej/Refresh: Error = Id out of range", "Displej/Refresh: Chyba = Id mimo rozsah");
          } else Alarm.activated = false;
          break;
        case 3:
          if (fcSettings() == false) {
            Alarm.activated = true;
            Alarm.message->set_Message("Displej/Refresh: Error = Id out of range", "Displej/Refresh: Chyba = Id mimo rozsah");
          } else Alarm.activated = false;
          break;
        case 10:
          if (fcHeating_circuit() == false) {
            Alarm.activated = true;
            Alarm.message->set_Message("Displej/Refresh: Error = Id out of range", "Displej/Refresh: Chyba = Id mimo rozsah");
          } else Alarm.activated = false;
          break;
        case 11:
          if (fcSleep() == false) {
            Alarm.activated = true;
            Alarm.message->set_Message("Displej/Refresh: Error = Id out of range", "Displej/Refresh: Chyba = Id mimo rozsah");
          } else Alarm.activated = false;
          break;
        case 12:
          if (fcTemperature_sensor() == false) {
            Alarm.activated = true;
            Alarm.message->set_Message("Displej/Refresh: Error = Id out of range", "Displej/Refresh: Chyba = Id mimo rozsah");
          } else Alarm.activated = false;
          break;
        case 13:
          if (fcTime() == false) {
            Alarm.activated = true;
            Alarm.message->set_Message("Displej/Refresh: Error = Id out of range", "Displej/Refresh: Chyba = Id mimo rozsah");
          } else Alarm.activated = false;
          break;
        default:
          Alarm.activated = true;
          Alarm.message->set_Message("Displej/Refresh: Error = Page out of range", "Displej/Refresh: Chyba = Strana mimo rozsah");
          break;
      }
    } else if ((Property.UpdateTimeHMI_LOOP <= millis()) && (Current_mode == ERROR)) {
      Touch_detector();
      Property.Refresh_time = millis();
      Property.UpdateTimeHMI_LOOP = millis() + Property.LOOP_PERIOD;
      if (prev_Current_mode != Current_mode) {
        Property.first_draw = true;
        prev_Current_mode = Current_mode;
      }
      fcTop_bar();
      fcERROR_page();
    }

    //Clear
    Faceplate->TextBox_main(Property.first_draw, true, "",
                            479, 319, 1, 1, 0, 0, "",
                            Property.Color.Background, 9, "MC", 1, 1,
                            Property.Color.Background, Property.Color.Background,
                            Property.Color.Background, 0, Property.Rotation);

    Property.first_draw = false;
  }

  void set_touch() {
    Touch.raw_Pressed = tft->getTouch(&Touch.x, &Touch.y);
  }
  void set_Current_mode(int value) {
    if (value == 1) Current_mode = Nominal_run;
    else if (value == 2) Current_mode = Ground_position;
    else if (value == 10) Current_mode = ERROR;
  }
  void set_Display(bool value) {
    Property.Brightness.Value = 0;
    pinMode(Property.Brightness.Pin, OUTPUT);
    digitalWrite(Property.Brightness.Pin, LOW);
  }
  void set_Sleep_time_constant(int value) {
    if ((value >= 10) && (value <= 120)) Sleep.Time_to_sleep_constant = value;
    else if (value > 120) Sleep.Time_to_sleep_constant = 60;
    else if (value < 10) Sleep.Time_to_sleep_constant = 10;
  }
  void set_Sleep_time(int value) {
    if ((value >= 1) && (value <= 60)) Sleep.Time = value;
    else if (value > 60) Sleep.Time = 60;
    else if (value < 1) Sleep.Time = 1;

    esp_now->set_Sleep_time(Sleep.Time);
  }

  bool get_Alarm_activated() {
    return Alarm.activated;
  }
  String get_Alarm_message() {
    return Alarm.message->get_Message();
  }
  String get_Language_option() {
    return Message->get_language();
  }
  short get_Current_mode() {
    return Current_mode;
  }
  bool get_Pressed() {
    return Touch.Pressed;
  }
  int get_Sleep_time_constant() {
    return Sleep.Time_to_sleep_constant;
  }
  int get_Sleep_time() {
    return Sleep.Time;
  }
  bool get_ALLWAIS_ON() {
    return Property.ALLWAIS_ON;
  }
  int get_Rotation() {
    return Property.Rotation;
  }
  void set_Rotation(bool value) {
    Property.Rotation = value;
  }

private:
  void touch_calibrate() {
    uint16_t calData[5];
    uint8_t calDataOK = 0;
    ledcWrite(Property.Brightness.PWMChannel, 200);

    // check file system exists
    if (!SPIFFS.begin()) {
      Serial.println("Formating file system");
      SPIFFS.format();
      SPIFFS.begin();
    }

    // check if calibration file exists and size is correct
    if (SPIFFS.exists(CALIBRATION_FILE)) {
      Serial.println("Check if calibration file exists and size is correct");
      if (REPEAT_CAL) {
        // Delete if we want to re-calibrate
        SPIFFS.remove(CALIBRATION_FILE);
      } else {
        File f = SPIFFS.open(CALIBRATION_FILE, "r");
        if (f) {
          if (f.readBytes((char*)calData, 14) == 14)
            calDataOK = 1;
          f.close();
        }
      }
    }

    if (calDataOK && !REPEAT_CAL) {
      // calibration data valid
      tft->setTouch(calData);
    } else {
      // data not valid so recalibrate
      tft->fillScreen(img->color565(Property.Color.Black[0], Property.Color.Black[1], Property.Color.Black[2]));
      tft->setCursor(20, 0);
      tft->setTextFont(2);
      tft->setTextSize(1);
      tft->setTextColor(img->color565(Property.Color.Text[0], Property.Color.Text[1], Property.Color.Text[2]), img->color565(Property.Color.Black[0], Property.Color.Black[1], Property.Color.Black[2]));

      tft->println("Touch corners as indicated");

      tft->setTextFont(1);
      tft->println();

      if (REPEAT_CAL) {
        tft->setTextColor(img->color565(Property.Color.Red[0], Property.Color.Red[1], Property.Color.Red[2]), img->color565(Property.Color.Black[0], Property.Color.Black[1], Property.Color.Black[2]));
        tft->println("Set REPEAT_CAL to false to stop this running again!");
      }

      tft->calibrateTouch(calData, img->color565(Property.Color.Red[0], Property.Color.Red[1], Property.Color.Red[2]), img->color565(Property.Color.Black[0], Property.Color.Black[1], Property.Color.Black[2]), 15);

      tft->setTextColor(img->color565(Property.Color.Green[0], Property.Color.Green[1], Property.Color.Green[2]), img->color565(Property.Color.Black[0], Property.Color.Black[1], Property.Color.Black[2]));
      tft->println("Calibration complete!");

      // store data
      File f = SPIFFS.open(CALIBRATION_FILE, "w");
      if (f) {
        f.write((const unsigned char*)calData, 14);
        f.close();
      }
    }
  }
  struct Touch_struct {
    uint16_t x, y;
    bool raw_Pressed;
    unsigned long UpdateTimeHMI_Touch;
    bool Pressed, prev_Pressed;
    int Pressing_period;
    bool Releast;
  };
  Touch_struct Touch;
  void Touch_detector() {

    //Make from non-static Touch signal static
    if (Touch.raw_Pressed) Touch.UpdateTimeHMI_Touch = millis() + ((millis() - Property.Refresh_time) + 10);
    if (Touch.UpdateTimeHMI_Touch >= millis()) Touch.Pressed = true;
    else Touch.Pressed = false;

    //Waiting for releast from button
    if (Touch.prev_Pressed && Touch.Pressed == false) Touch.Releast = true;
    else Touch.Releast = false;

    Touch.prev_Pressed = Touch.Pressed;

    Faceplate->Set_datum(Touch.x, Touch.y, Touch.Pressed, Touch.Releast);

    if (Touch.Pressed && (Property.Pressing.Enable_timer == false)) {
      Property.Pressing.Enable_timer = true;
      Property.Pressing.Count = 800;
      Serial.println(Property.Pressing.Enable_timer);
    } else if (Touch.Releast) {
      Property.Pressing.Enable_timer = false;
      Property.Pressing.Count = 800;
      Property.Pressing.Time = (millis() - Property.Pressing.Count);
    }
  }

private:
  struct Top_bar_struct {
    uint8_t Height;
    TON* TON_flashing_mode = new TON(700);
    bool Current_mode_flashing;
  };
  Top_bar_struct TopBar;
  bool fcTop_bar() {

    Faceplate->Icon_main(true, 0, 0, 25, 30, 24, 24, 1, 3, thermometer, Property.Color.Blue);

    String String_convertor = "";
    if (Current_mode == ERROR) {
      Message->set_Message("ERROR", "ERROR");
      Faceplate->TextBox_main(Property.first_draw, true, Message->get_Message(),
                              25, 0, 140, TopBar.Height, 0, 0, "",
                              Property.Color.Red, 9, "ML", 5, TopBar.Height / 2,
                              Property.Color.Blue, Property.Color.Background,
                              Property.Color.Background, 0, Property.Rotation);
    } else if (page == Main_page) {
      Message->set_Message("Hlavna strana", "Main page");
      String_convertor = String(String_convertor + Message->get_Message());
      Faceplate->TextBox_main(Property.first_draw, true, String_convertor,
                              25, 0, 140, TopBar.Height, 0, 0, "",
                              Property.Color.White, 9, "ML", 5, TopBar.Height / 2,
                              Property.Color.Blue, Property.Color.Background,
                              Property.Color.Background, 0, Property.Rotation);
    } else if (page == Menu) {
      Message->set_Message("Menu", "Menu");
      String_convertor = String(String_convertor + Message->get_Message());
      Faceplate->TextBox_main(Property.first_draw, true, String_convertor,
                              25, 0, 140, TopBar.Height, 0, 0, "",
                              Property.Color.White, 9, "ML", 5, TopBar.Height / 2,
                              Property.Color.Blue, Property.Color.Background,
                              Property.Color.Background, 0, Property.Rotation);
      Faceplate->Reset_Scrools_TextBox();
    } else if (page == Settings) {
      Message->set_Message("Nastavenia", "Settings");
      String_convertor = String(String_convertor + Message->get_Message());
      Faceplate->TextBox_main(Property.first_draw, true, String_convertor,
                              25, 0, 140, TopBar.Height, 0, 0, "",
                              Property.Color.White, 9, "ML", 5, TopBar.Height / 2,
                              Property.Color.Blue, Property.Color.Background,
                              Property.Color.Background, 0, Property.Rotation);
    } else if (page == Heating_circruit_) {
      Message->set_Message("Menu", "Menu");
      String_convertor = String(String_convertor + Message->get_Message());
      String_convertor = String(String_convertor + "/");
      Message->set_Message("Vyhrevny okruh", "Heating circruit");
      String_convertor = String(String_convertor + Message->get_Message());
      Faceplate->Scrools_TextBox_main(String_convertor, 2, true, 3000,
                                      25, 0, 140, TopBar.Height, 0, -70,
                                      Property.Color.White, 9, "ML", 5, TopBar.Height / 2,
                                      Property.Color.Blue, Property.Color.Background,
                                      Property.Color.Background, 0, Property.Rotation);
    } else if (page == Ultrasonic_) {
      Message->set_Message("Menu", "Menu");
      String_convertor = String(String_convertor + Message->get_Message());
      String_convertor = String(String_convertor + "/");
      Message->set_Message("Aktualizacnie", "Actualizations");
      String_convertor = String(String_convertor + Message->get_Message());
      Faceplate->Scrools_TextBox_main(String_convertor, 2, true, 3000,
                                      25, 0, 140, TopBar.Height, 0, -70,
                                      Property.Color.White, 9, "ML", 5, TopBar.Height / 2,
                                      Property.Color.Blue, Property.Color.Background,
                                      Property.Color.Background, 0, Property.Rotation);
    } else if (page == Temperature_sensor_) {
      Message->set_Message("Menu", "Menu");
      String_convertor = String(String_convertor + Message->get_Message());
      String_convertor = String(String_convertor + "/");
      Message->set_Message("Teplomer", "Thermometer");
      String_convertor = String(String_convertor + Message->get_Message());
      Faceplate->TextBox_main(Property.first_draw, true, String_convertor,
                              25, 0, 140, TopBar.Height, 0, 0, "",
                              Property.Color.White, 9, "ML", 5, TopBar.Height / 2,
                              Property.Color.Blue, Property.Color.Background,
                              Property.Color.Background, 0, Property.Rotation);
    } else if (page == Time_) {
      Message->set_Message("Menu", "Menu");
      String_convertor = String(String_convertor + Message->get_Message());
      String_convertor = String(String_convertor + "/");
      Message->set_Message("Intervaly", "Intervals");
      String_convertor = String(String_convertor + Message->get_Message());
      Faceplate->TextBox_main(Property.first_draw, true, String_convertor,
                              25, 0, 140, TopBar.Height, 0, 0, "",
                              Property.Color.White, 9, "ML", 5, TopBar.Height / 2,
                              Property.Color.Blue, Property.Color.Background,
                              Property.Color.Background, 0, Property.Rotation);
    }

    if (TopBar.TON_flashing_mode->Timer(true)) {
      if (TopBar.Current_mode_flashing) TopBar.Current_mode_flashing = false;
      else TopBar.Current_mode_flashing = true;
    }

    if (TopBar.Current_mode_flashing) {
      if (Current_mode == Nominal_run) {
        Message->set_Message("Zapnuty", "Turn on");
        Faceplate->TextBox_main(Property.first_draw, true, Message->get_Message(),
                                165, 0, 175, TopBar.Height, 0, 0, "",
                                Property.Color.Green, 9, "Mc", 45, TopBar.Height / 2,
                                Property.Color.Blue, Property.Color.Background,
                                Property.Color.Background, 0, Property.Rotation);
      } else {
        Message->set_Message("Vypnuty", "Turn off");
        Faceplate->TextBox_main(Property.first_draw, true, Message->get_Message(),
                                165, 0, 175, TopBar.Height, 0, 0, "",
                                Property.Color.Red, 9, "Mc", 45, TopBar.Height / 2,
                                Property.Color.Blue, Property.Color.Background,
                                Property.Color.Background, 0, Property.Rotation);
      }
    }

    else {
      if (Current_mode == Nominal_run) Message->set_Message("Zapnuty", "Turn on");
      else Message->set_Message("Vypnuty", "Turn off");
      Faceplate->TextBox_main(Property.first_draw, true, Message->get_Message(),
                              165, 0, 175, TopBar.Height, 0, 0, "",
                              Property.Color.White, 9, "Mc", 45, TopBar.Height / 2,
                              Property.Color.Blue, Property.Color.Background,
                              Property.Color.Background, 0, Property.Rotation);
    }

    if (battery->get_State() == 1) Faceplate->Icon_main(true, 315, 0, 90, 30, 32, 32, 30, -1, Full_battery, Property.Color.Blue);
    else if (battery->get_State() == 0) Faceplate->Icon_main(true, 315, 0, 90, 30, 32, 32, 30, -1, Charging_battery, Property.Color.Blue);
    else if (battery->get_State() == 10) Faceplate->Icon_main(true, 315, 0, 90, 30, 32, 32, 30, -1, low_battery, Property.Color.Blue);

    String_convertor = String(state->get_Hours(), DEC);
    String_convertor = String(String_convertor + ":");
    String String_convertor_auxiliary = String(state->get_Minutes(), DEC);
    if (state->get_Minutes() < 10) String_convertor = String(String_convertor + "0");
    String_convertor = String(String_convertor + String_convertor_auxiliary);
    Faceplate->TextBox_main(Property.first_draw, true, String_convertor,
                            405, 0, 75, TopBar.Height, 0, 0, "",
                            Property.Color.White, 9, "MR", 70, TopBar.Height / 2,
                            Property.Color.Blue, Property.Color.Background,
                            Property.Color.Background, 0, Property.Rotation);
    return true;
  }

private:
  struct Main_page_struct {
    int TON_wait_for_OFF_constant = 1000;
    TON* TON_wait_for_OFF = new TON(TON_wait_for_OFF_constant);
    int TON_wait_for_ON_constant = 1000;
    TON* TON_wait_for_ON = new TON(TON_wait_for_ON_constant);
  };
  Main_page_struct MainPage;
  bool fcMain_page() {
    if (Current_mode == Nominal_run) {
      Message->set_Message("STOP", "STOP");
      if (Faceplate->Button_main(Property.first_draw, false, true, Message->get_Message(),
                                 325, 260, 150, 60, 10,
                                 Property.Color.Red, 12, "MC", 75, 30,
                                 Property.Color.Text, Property.Color.Background, Property.Color.Yellow,
                                 Property.Color.Red, 3, Property.Rotation)) {
        if (MainPage.TON_wait_for_OFF->Timer(true)) {
          Current_mode = Ground_position;
          esp_now->set_Current_mode(false);
          MainPage.TON_wait_for_OFF->Timer(false);
          MainPage.TON_wait_for_ON->Timer(false);
          esp_now->Sending(false, 500);
        }
      } else MainPage.TON_wait_for_OFF->Timer(false);
    } else {
      Message->set_Message("START", "START");
      if (Faceplate->Button_main(Property.first_draw, false, true, Message->get_Message(),
                                 325, 260, 150, 60, 10,
                                 Property.Color.Green, 12, "MC", 75, 30,
                                 Property.Color.Text, Property.Color.Background, Property.Color.Yellow,
                                 Property.Color.Green, 3, Property.Rotation)) {
        if (MainPage.TON_wait_for_ON->Timer(true)) {
          Current_mode = Nominal_run;
          esp_now->set_Current_mode(true);
          MainPage.TON_wait_for_ON->Timer(false);
          MainPage.TON_wait_for_OFF->Timer(false);
          esp_now->Sending(false, 500);
        }
      } else MainPage.TON_wait_for_ON->Timer(false);
    }
    Message->set_Message("MENU", "MENU");
    if (Current_mode == Ground_position) {
      if (Faceplate->Button_main(Property.first_draw, true, true, Message->get_Message(),
                                 165, 260, 150, 60, 10,
                                 Property.Color.Blue, 12, "MC", 75, 30,
                                 Property.Color.Text, Property.Color.Background, Property.Color.Focused,
                                 Property.Color.Blue, 3, Property.Rotation)) {
        Current_mode = Ground_position;
        page = Menu;
      }
    } else {
      if (Faceplate->Button_main(Property.first_draw, true, false, Message->get_Message(),
                                 165, 260, 150, 60, 10,
                                 Property.Color.Blue, 12, "MC", 75, 30,
                                 Property.Color.Background, Property.Color.Background, Property.Color.Focused,
                                 Property.Color.Blue, 3, Property.Rotation)) {
        Current_mode = Ground_position;
        page = Menu;
      }
    }

    Message->set_Message("NAST.", "SET.");
    if (Faceplate->Button_main(Property.first_draw, true, true, Message->get_Message(),
                               5, 260, 150, 60, 10,
                               Property.Color.Blue, 12, "MC", 75, 30,
                               Property.Color.Text, Property.Color.Background, Property.Color.Focused,
                               Property.Color.Blue, 3, Property.Rotation)) {
      page = Settings;
    }

    if (state->get_Ultrasonic_Capacity() <= 20) {
      Faceplate->Slider_main(state->get_Ultrasonic_Capacity(), true, false,
                             5, 50, 40, 190, 5,
                             Property.Color.Text, 9, "MC", 20, 95,
                             Property.Color.Background, Property.Color.Background, Property.Color.Red,
                             Property.Color.Text, 2, Property.Rotation);
    } else {
      Faceplate->Slider_main(state->get_Ultrasonic_Capacity(), true, false,
                             5, 50, 40, 190, 5,
                             Property.Color.Text, 9, "MC", 20, 95,
                             Property.Color.Background, Property.Color.Background, Property.Color.Green,
                             Property.Color.Text, 2, Property.Rotation);
    }

    String StringConvertor;
    String AuxiliaryStringConvertor;
    int temporary_X = -10;
    int temporary_X_2 = 205;
    int temporary_Y = 0;

    /////////////////////////////////////////////////////////////////////////////////////////Table: Circruit 1
    if (true) {
      Message->set_Message("Okruh 1", "Circuit 1");
      //if (esp_now->get_ID() == 3) Message->set_Message("Horny 2", "TOP 2");
      if (esp_now->get_ID() == 2) {
        Faceplate->TextBox_main(Property.first_draw, false, Message->get_Message(),
                                temporary_X + 70, temporary_Y + 40, 200, 40, 5, 5, "TOP",
                                Property.Color.Text, 12, "ML", 15, 20,
                                Property.Color.Transparent, Property.Color.Background,
                                Property.Color.Green, 2, Property.Rotation);
        //It just draw rectangle
        Faceplate->TextBox_main(Property.first_draw, false, "",
                                temporary_X + 70, temporary_Y + 78, 200, 170, 5, 5, "BOTTOM",
                                Property.Color.Text, 9, "ML", 5, 15,
                                Property.Color.Transparent, Property.Color.Background,
                                Property.Color.Green, 2, Property.Rotation);
      } else {
        Faceplate->TextBox_main(Property.first_draw, false, Message->get_Message(),
                                temporary_X + 70, temporary_Y + 40, 200, 40, 5, 5, "TOP",
                                Property.Color.Text, 12, "ML", 15, 20,
                                Property.Color.Transparent, Property.Color.Background,
                                Property.Color.Text, 2, Property.Rotation);
        //It just draw rectangle
        Faceplate->TextBox_main(Property.first_draw, false, "",
                                temporary_X + 70, temporary_Y + 78, 200, 170, 5, 5, "BOTTOM",
                                Property.Color.Text, 9, "ML", 5, 15,
                                Property.Color.Transparent, Property.Color.Background,
                                Property.Color.Text, 2, Property.Rotation);
      }

      if (oven_1->get_Heating()) {
        //tft->pushImage(temporary_X+210, temporary_Y+43, 32, 32, Fire);
        Faceplate->Icon_main(Property.first_draw, temporary_X + 210, temporary_Y + 43, 32, 32, 32, 32, 0, 0, Fire, Property.Color.Background);
      } else {
        //tft->pushImage(temporary_X+210, temporary_Y+43, 32, 32, Flake);
        Faceplate->Icon_main(Property.first_draw, temporary_X + 210, temporary_Y + 43, 32, 32, 32, 32, 0, 0, Flake, Property.Color.Background);
      }

      if (Property.first_draw) {
        if (oven_1->get_Mode() == 0) {
          //tft->pushImage(temporary_X+85, temporary_Y+135, 32, 32, switch_off);
          Faceplate->Icon_main(Property.first_draw, temporary_X + 85, temporary_Y + 133, 64, 32, 64, 64, 0, -16, switch_off, Property.Color.Background);
        } else if (oven_1->get_Mode() == 1) {
          //tft->pushImage(temporary_X+85, temporary_Y+135, 32, 32, switch_on);
          Faceplate->Icon_main(Property.first_draw, temporary_X + 85, temporary_Y + 133, 64, 32, 64, 64, 0, -16, switch_on, Property.Color.Background);
        } else if (oven_1->get_Mode() == 2) {
          //tft->pushImage(temporary_X+85, temporary_Y+135, 32, 32, AutomaticMode);
          Faceplate->Icon_main(Property.first_draw, temporary_X + 100, temporary_Y + 133, 32, 32, 32, 32, 0, 0, AutomaticMode, Property.Color.Background);
        }
      }

      StringConvertor = String(oven_1->get_Temperature(), 1);
      if (oven_1->get_Temperature() >= oven_1->get_Upper_limit()) {
        Faceplate->TextBox_main(Property.first_draw, false, StringConvertor,
                                temporary_X + 82, temporary_Y + 80, 120, 50, 0, 0, "",
                                Property.Color.Red, 24, "TR", 105, 5,
                                Property.Color.Background, Property.Color.Background,
                                Property.Color.White, 0, Property.Rotation);
      } else if (oven_1->get_Temperature() <= oven_1->get_Lower_limit()) {
        Faceplate->TextBox_main(Property.first_draw, false, StringConvertor,
                                temporary_X + 82, temporary_Y + 80, 120, 50, 0, 0, "",
                                Property.Color.Blue, 24, "TR", 105, 5,
                                Property.Color.Background, Property.Color.Background,
                                Property.Color.White, 0, Property.Rotation);
      } else {
        Faceplate->TextBox_main(Property.first_draw, false, StringConvertor,
                                temporary_X + 82, temporary_Y + 80, 120, 50, 0, 0, "",
                                Property.Color.Text, 24, "TR", 105, 5,
                                Property.Color.Background, Property.Color.Background,
                                Property.Color.White, 0, Property.Rotation);
      }

      Faceplate->degrees_Celsius(Property.first_draw,
                                 temporary_X + 202, temporary_Y + 85, 50, 45,
                                 Property.Color.Text, 24,
                                 8, Property.Rotation);

      StringConvertor = String(oven_1->get_Humidity(), 1);
      StringConvertor = String(StringConvertor + "%");
      Faceplate->TextBox_main(Property.first_draw, false, StringConvertor,
                              temporary_X + 153, temporary_Y + 130, 115, 40, 0, 0, "",
                              Property.Color.Text, 18, "MC", 57, 20,
                              Property.Color.Background, Property.Color.Background,
                              Property.Color.White, 0, Property.Rotation);

      Message->set_Message("Pozadovana T:", "Required T:");
      Faceplate->TextBox_main(Property.first_draw, false, Message->get_Message(),
                              temporary_X + 72, temporary_Y + 172, 190, 25, 0, 0, "",
                              Property.Color.Text, 9, "ML", 5, 12,
                              Property.Color.Background, Property.Color.Background,
                              Property.Color.White, 0, Property.Rotation);

      StringConvertor = String(oven_1->get_Required_T(), 1);
      Faceplate->TextBox_main(Property.first_draw, false, StringConvertor,
                              temporary_X + 72, temporary_Y + 195, 80, 50, 5, 5, "",
                              Property.Color.Text, 18, "MR", 75, 25,
                              Property.Color.Background, Property.Color.Background,
                              Property.Color.White, 0, Property.Rotation);

      Faceplate->degrees_Celsius(Property.first_draw,
                                 temporary_X + 152, temporary_Y + 200, 35, 40,
                                 Property.Color.Text, 18,
                                 6, Property.Rotation);

      StringConvertor = String(oven_1->get_Positive_tolerance(), 1);
      StringConvertor = String("+ " + StringConvertor);
      Faceplate->TextBox_main(Property.first_draw, false, StringConvertor,
                              temporary_X + 188, temporary_Y + 192, 80, 25, 5, 5, "",
                              Property.Color.Text, 12, "MC", 40, 12,
                              Property.Color.Background, Property.Color.Background,
                              Property.Color.White, 0, Property.Rotation);

      StringConvertor = String(oven_1->get_Negative_tolerance(), 1);
      StringConvertor = String("- " + StringConvertor);
      Faceplate->TextBox_main(Property.first_draw, false, StringConvertor,
                              temporary_X + 188, temporary_Y + 217, 80, 25, 5, 5, "",
                              Property.Color.Text, 12, "MC", 40, 12,
                              Property.Color.Background, Property.Color.Background,
                              Property.Color.White, 0, Property.Rotation);
    }

    /////////////////////////////////////////////////////////////////////////////////////////Table: Circruit 2
    if (true) {
      Message->set_Message("Okruh 2", "Circuit 2");
      //if (esp_now->get_ID() == 3) Message->set_Message("Dolny 1", "Bottom 1");
      if (esp_now->get_ID() == 3) {
        Faceplate->TextBox_main(Property.first_draw, false, Message->get_Message(),
                                temporary_X_2 + 70, temporary_Y + 40, 200, 40, 5, 5, "TOP",
                                Property.Color.Text, 12, "ML", 15, 20,
                                Property.Color.Transparent, Property.Color.Background,
                                Property.Color.Green, 2, Property.Rotation);
        //It just draw rectangle
        Faceplate->TextBox_main(Property.first_draw, false, "",
                                temporary_X_2 + 70, temporary_Y + 78, 200, 170, 5, 5, "BOTTOM",
                                Property.Color.Text, 9, "ML", 5, 15,
                                Property.Color.Transparent, Property.Color.Background,
                                Property.Color.Green, 2, Property.Rotation);
      } else {
        Faceplate->TextBox_main(Property.first_draw, false, Message->get_Message(),
                                temporary_X_2 + 70, temporary_Y + 40, 200, 40, 5, 5, "TOP",
                                Property.Color.Text, 12, "ML", 15, 20,
                                Property.Color.Transparent, Property.Color.Background,
                                Property.Color.Text, 2, Property.Rotation);
        //It just draw rectangle
        Faceplate->TextBox_main(Property.first_draw, false, "",
                                temporary_X_2 + 70, temporary_Y + 78, 200, 170, 5, 5, "BOTTOM",
                                Property.Color.Text, 9, "ML", 5, 15,
                                Property.Color.Transparent, Property.Color.Background,
                                Property.Color.Text, 2, Property.Rotation);
      }

      if (oven_2->get_Heating()) {
        tft->pushImage(temporary_X_2 + 210, temporary_Y + 43, 32, 32, Fire);
      } else {
        tft->pushImage(temporary_X_2 + 210, temporary_Y + 43, 32, 32, Flake);
      }

      if (Property.first_draw) {
        if (oven_2->get_Mode() == 0) {
          //tft->pushImage(temporary_X+85, temporary_Y+135, 32, 32, switch_off);
          Faceplate->Icon_main(Property.first_draw, temporary_X_2 + 85, temporary_Y + 133, 64, 32, 64, 64, 0, -16, switch_off, Property.Color.Background);
        } else if (oven_2->get_Mode() == 1) {
          //tft->pushImage(temporary_X+85, temporary_Y+135, 32, 32, switch_on);
          Faceplate->Icon_main(Property.first_draw, temporary_X_2 + 85, temporary_Y + 133, 64, 32, 64, 64, 0, -16, switch_on, Property.Color.Background);
        } else if (oven_2->get_Mode() == 2) {
          //tft->pushImage(temporary_X+85, temporary_Y+135, 32, 32, AutomaticMode);
          Faceplate->Icon_main(Property.first_draw, temporary_X_2 + 100, temporary_Y + 133, 32, 32, 32, 32, 0, 0, AutomaticMode, Property.Color.Background);
        }
      }

      StringConvertor = String(oven_2->get_Temperature(), 1);
      if (oven_2->get_Temperature() >= oven_2->get_Upper_limit()) {
        Faceplate->TextBox_main(Property.first_draw, false, StringConvertor,
                                temporary_X_2 + 82, temporary_Y + 80, 120, 50, 0, 0, "",
                                Property.Color.Red, 24, "TR", 105, 5,
                                Property.Color.Background, Property.Color.Background,
                                Property.Color.White, 0, Property.Rotation);
      } else if (oven_2->get_Temperature() <= oven_2->get_Lower_limit()) {
        Faceplate->TextBox_main(Property.first_draw, false, StringConvertor,
                                temporary_X_2 + 82, temporary_Y + 80, 120, 50, 0, 0, "",
                                Property.Color.Blue, 24, "TR", 105, 5,
                                Property.Color.Background, Property.Color.Background,
                                Property.Color.White, 0, Property.Rotation);
      } else {
        Faceplate->TextBox_main(Property.first_draw, false, StringConvertor,
                                temporary_X_2 + 82, temporary_Y + 80, 120, 50, 0, 0, "",
                                Property.Color.Text, 24, "TR", 105, 5,
                                Property.Color.Background, Property.Color.Background,
                                Property.Color.White, 0, Property.Rotation);
      }

      Faceplate->degrees_Celsius(Property.first_draw,
                                 temporary_X_2 + 202, temporary_Y + 85, 50, 45,
                                 Property.Color.Text, 24,
                                 8, Property.Rotation);

      StringConvertor = String(oven_2->get_Humidity(), 1);
      StringConvertor = String(StringConvertor + "%");
      Faceplate->TextBox_main(Property.first_draw, false, StringConvertor,
                              temporary_X_2 + 153, temporary_Y + 130, 115, 40, 0, 0, "",
                              Property.Color.Text, 18, "MC", 57, 20,
                              Property.Color.Background, Property.Color.Background,
                              Property.Color.White, 0, Property.Rotation);

      Message->set_Message("Pozadovana T:", "Required T:");
      Faceplate->TextBox_main(Property.first_draw, false, Message->get_Message(),
                              temporary_X_2 + 72, temporary_Y + 172, 190, 25, 0, 0, "",
                              Property.Color.Text, 9, "ML", 5, 12,
                              Property.Color.Background, Property.Color.Background,
                              Property.Color.White, 0, Property.Rotation);

      StringConvertor = String(oven_2->get_Required_T(), 1);
      Faceplate->TextBox_main(Property.first_draw, false, StringConvertor,
                              temporary_X_2 + 72, temporary_Y + 195, 80, 50, 5, 5, "",
                              Property.Color.Text, 18, "MR", 75, 25,
                              Property.Color.Background, Property.Color.Background,
                              Property.Color.White, 0, Property.Rotation);

      Faceplate->degrees_Celsius(Property.first_draw,
                                 temporary_X_2 + 152, temporary_Y + 200, 35, 40,
                                 Property.Color.Text, 18,
                                 6, Property.Rotation);

      StringConvertor = String(oven_2->get_Positive_tolerance(), 1);
      StringConvertor = String("+ " + StringConvertor);
      Faceplate->TextBox_main(Property.first_draw, false, StringConvertor,
                              temporary_X_2 + 188, temporary_Y + 192, 80, 25, 5, 5, "",
                              Property.Color.Text, 12, "MC", 40, 12,
                              Property.Color.Background, Property.Color.Background,
                              Property.Color.White, 0, Property.Rotation);

      StringConvertor = String(oven_2->get_Negative_tolerance(), 1);
      StringConvertor = String("- " + StringConvertor);
      Faceplate->TextBox_main(Property.first_draw, false, StringConvertor,
                              temporary_X_2 + 188, temporary_Y + 217, 80, 25, 5, 5, "",
                              Property.Color.Text, 12, "MC", 40, 12,
                              Property.Color.Background, Property.Color.Background,
                              Property.Color.White, 0, Property.Rotation);
    }

    return true;
  }

  bool fcMenu() {
    Message->set_Message("Vyhrevny okruh", "Heating circruit");
    if (Faceplate->Button_main(Property.first_draw, true, true, Message->get_Message(),
                               10, 40, 460, 50, 10,
                               Property.Color.Blue, 12, "MC", 230, 25,
                               Property.Color.Text, Property.Color.Background, Property.Color.Focused,
                               Property.Color.Blue, 3, Property.Rotation)) {
      page = Heating_circruit_;
    }
    Message->set_Message("Sleep rezim", "Sleep mode");
    if (Faceplate->Button_main(Property.first_draw, true, true, Message->get_Message(),
                               10, 100, 460, 50, 10,
                               Property.Color.Blue, 12, "MC", 230, 25,
                               Property.Color.Text, Property.Color.Background, Property.Color.Focused,
                               Property.Color.Blue, 3, Property.Rotation)) {
      page = Ultrasonic_;
    }
    Message->set_Message("Teplomer", "Temp. sensor");
    if (Faceplate->Button_main(Property.first_draw, true, true, Message->get_Message(),
                               10, 160, 460, 50, 10,
                               Property.Color.Blue, 12, "MC", 230, 25,
                               Property.Color.Text, Property.Color.Background, Property.Color.Focused,
                               Property.Color.Blue, 3, Property.Rotation)) {
      page = Temperature_sensor_;
    }
    Message->set_Message("Cas", "Time");
    if (Faceplate->Button_main(Property.first_draw, true, true, Message->get_Message(),
                               10, 220, 460, 50, 10,
                               Property.Color.Blue, 12, "MC", 230, 25,
                               Property.Color.Text, Property.Color.Background, Property.Color.Focused,
                               Property.Color.Blue, 3, Property.Rotation)) {
      page = Time_;
    }


    //Back button
    if (Faceplate->Icon_button_advance_main(Property.first_draw, true, BackBlue, BackGreen,
                                            0, 280, 480, 40, 32, 32, 224, 4, 20,
                                            Property.Color.Background, Property.Color.Background, Property.Color.Focused,
                                            Property.Color.Blue, 3, Property.Rotation)) {
      page = Main_page;
      esp_now->Sending(true, 1000);
    }
    return true;
  }

  struct Settings_struct {
    String prev_language;
  };
  Settings_struct Settings_str;
  bool fcSettings() {
    if (Settings_str.prev_language != Message->get_language()) {
      Settings_str.prev_language = Message->get_language();
      tft->fillScreen(tft->color565(Property.Color.Background[0], Property.Color.Background[1], Property.Color.Background[2]));
      Property.first_draw = true;
    }

    //Draw
    if (true) {
      Message->set_Message("Jazyk:", "Language:");
      Faceplate->TextBox_main(Property.first_draw, false, Message->get_Message(),
                              10, 40, 460, 40, 5, 5, "TOP",
                              Property.Color.Text, 9, "ML", 5, 20,
                              Property.Color.Transparent, Property.Color.Transparent,
                              Property.Color.Text, 2, Property.Rotation);

      Message->set_Message("Jas:", "Brightness:");
      Faceplate->TextBox_main(Property.first_draw, false, Message->get_Message(),
                              10, 78, 460, 60, 0, 0, "",
                              Property.Color.Text, 9, "ML", 5, 30,
                              Property.Color.Transparent, Property.Color.Transparent,
                              Property.Color.Text, 2, Property.Rotation);

      Message->set_Message("ALLWAIS ON:", "ALL WAIS ON:");
      Faceplate->TextBox_main(Property.first_draw, false, Message->get_Message(),
                              10, 136, 460, 40, 0, 0, "",
                              Property.Color.Text, 9, "ML", 5, 20,
                              Property.Color.Transparent, Property.Color.Transparent,
                              Property.Color.Text, 2, Property.Rotation);

      Message->set_Message("Rotacia:", "Rotation:");
      Faceplate->TextBox_main(Property.first_draw, false, Message->get_Message(),
                              10, 174, 460, 40, 0, 0, "",
                              Property.Color.Text, 9, "ML", 5, 20,
                              Property.Color.Transparent, Property.Color.Transparent,
                              Property.Color.Text, 2, Property.Rotation);
    }

    //Active
    if (true) {
      if (Message->get_language() == "SK") {
        Message->set_Message("SK", "SK");
        if (Faceplate->Button_main(Property.first_draw, true, true, Message->get_Message(),
                                   200, 42, 100, 36, 5,
                                   Property.Color.Green, 12, "MC", 50, 18,
                                   Property.Color.Background, Property.Color.Background, Property.Color.Green,
                                   Property.Color.Green, 3, Property.Rotation)) {
          Message->Set_language("SK");
        }
        Message->set_Message("EN", "EN");
        if (Faceplate->Button_main(Property.first_draw, true, true, Message->get_Message(),
                                   360, 42, 100, 36, 5,
                                   Property.Color.Red, 12, "MC", 50, 18,
                                   Property.Color.Background, Property.Color.Background, Property.Color.Focused,
                                   Property.Color.Red, 3, Property.Rotation)) {
          Message->Set_language("EN");
        }
      } else {
        Message->set_Message("SK", "SK");
        if (Faceplate->Button_main(Property.first_draw, true, true, Message->get_Message(),
                                   200, 42, 100, 36, 5,
                                   Property.Color.Red, 12, "MC", 50, 18,
                                   Property.Color.Background, Property.Color.Background, Property.Color.Focused,
                                   Property.Color.Red, 3, Property.Rotation)) {
          Message->Set_language("SK");
        }
        Message->set_Message("EN", "EN");
        if (Faceplate->Button_main(Property.first_draw, true, true, Message->get_Message(),
                                   360, 42, 100, 36, 5,
                                   Property.Color.Green, 12, "MC", 50, 18,
                                   Property.Color.Background, Property.Color.Background, Property.Color.Green,
                                   Property.Color.Green, 3, Property.Rotation)) {
          Message->Set_language("EN");
        }
      }

      int Brightness_Value = map(Property.Brightness.Value, 0, 255, 0, 100);
      Brightness_Value = map(Faceplate->Slider_main(Brightness_Value, false, true,
                                                    200, 80, 260, 56, 5,
                                                    Property.Color.Text, 9, "MC", 130, 28,
                                                    Property.Color.Background, Property.Color.Background, Property.Color.Green,
                                                    Property.Color.Text, 4, Property.Rotation),
                             0, 100, 1, 255);

      Property.Brightness.Value = Brightness_Value;

      bool Enable_draw;
      do {
        Enable_draw = true;
        if (Property.ALLWAIS_ON) {
          Message->set_Message("ON", "ON");
          if (Faceplate->Button_main(Property.first_draw, true, (battery->get_State() == 0) || (battery->get_State() == 1), Message->get_Message(),
                                     200, 138, 100, 36, 5,
                                     Property.Color.Green, 12, "MC", 50, 18,
                                     Property.Color.Background, Property.Color.Background, Property.Color.Green,
                                     Property.Color.Green, 3, Property.Rotation)) {
          }
          Message->set_Message("OFF", "OFF");
          if (Faceplate->Button_main(Property.first_draw, true, (battery->get_State() == 0) || (battery->get_State() == 1), Message->get_Message(),
                                     360, 138, 100, 36, 5,
                                     Property.Color.Red, 12, "MC", 50, 18,
                                     Property.Color.Background, Property.Color.Background, Property.Color.Green,
                                     Property.Color.Red, 3, Property.Rotation)) {
            Property.ALLWAIS_ON = false;
            Enable_draw = false;
          }
        } else {
          Message->set_Message("OFF", "OFF");
          if (Faceplate->Button_main(Property.first_draw, true, (battery->get_State() == 0) || (battery->get_State() == 1), Message->get_Message(),
                                     360, 138, 100, 36, 5,
                                     Property.Color.Green, 12, "MC", 50, 18,
                                     Property.Color.Background, Property.Color.Background, Property.Color.Green,
                                     Property.Color.Green, 3, Property.Rotation)) {
          }
          Message->set_Message("ON", "ON");
          if (Faceplate->Button_main(Property.first_draw, true, (battery->get_State() == 0) || (battery->get_State() == 1), Message->get_Message(),
                                     200, 138, 100, 36, 5,
                                     Property.Color.Red, 12, "MC", 50, 18,
                                     Property.Color.Background, Property.Color.Background, Property.Color.Green,
                                     Property.Color.Red, 3, Property.Rotation)) {
            Property.ALLWAIS_ON = true;
            Enable_draw = false;
          }
        }
      } while (Enable_draw == false);

      do {
        Enable_draw = true;
        if (Property.Rotation == 3) {
          Message->set_Message("UP", "UP");
          if (Faceplate->Button_main(Property.first_draw, true, true, Message->get_Message(),
                                     200, 176, 100, 36, 5,
                                     Property.Color.Green, 12, "MC", 50, 18,
                                     Property.Color.Background, Property.Color.Background, Property.Color.Green,
                                     Property.Color.Green, 3, Property.Rotation)) {
          }
          Message->set_Message("DOWN", "DOWN");
          if (Faceplate->Button_main(Property.first_draw, true, true, Message->get_Message(),
                                     360, 176, 100, 36, 5,
                                     Property.Color.Red, 12, "MC", 50, 18,
                                     Property.Color.Background, Property.Color.Background, Property.Color.Green,
                                     Property.Color.Red, 3, Property.Rotation)) {
            REPEAT_CAL = true;
            Property.Rotation = 1;
            flash->set_Flash(31, REPEAT_CAL);
            flash->set_Flash(30, Property.Rotation);
            ESP.restart();
          }
        } else {
          Message->set_Message("UP", "UP");
          if (Faceplate->Button_main(Property.first_draw, true, true, Message->get_Message(),
                                     360, 176, 100, 36, 5,
                                     Property.Color.Green, 12, "MC", 50, 18,
                                     Property.Color.Background, Property.Color.Background, Property.Color.Green,
                                     Property.Color.Green, 3, Property.Rotation)) {
          }
          Message->set_Message("DOWN", "DOWN");
          if (Faceplate->Button_main(Property.first_draw, true, true, Message->get_Message(),
                                     200, 176, 100, 36, 5,
                                     Property.Color.Red, 12, "MC", 50, 18,
                                     Property.Color.Background, Property.Color.Background, Property.Color.Green,
                                     Property.Color.Red, 3, Property.Rotation)) {
            REPEAT_CAL = true;
            Property.Rotation = 3;
            flash->set_Flash(31, REPEAT_CAL);
            flash->set_Flash(30, Property.Rotation);
            ESP.restart();
          }
        }
      } while (Enable_draw == false);
    }

    //Back button
    if (Faceplate->Icon_button_advance_main(Property.first_draw, true, BackBlue, BackGreen,
                                            0, 280, 480, 40, 32, 32, 224, 4, 20,
                                            Property.Color.Background, Property.Color.Background, Property.Color.Focused,
                                            Property.Color.Blue, 3, Property.Rotation)) {
      page = Main_page;
    }
    return true;
  }

  struct Heating_circruit_struct {
    bool Circuit;
    bool prev_Circuit;
    bool Enable_draw;
    int Page;
  };
  Heating_circruit_struct Heating_circuit;
  bool fcHeating_circuit() {
    if (Heating_circuit.prev_Circuit != Heating_circuit.Circuit) {
      Heating_circuit.prev_Circuit = Heating_circuit.Circuit;
      tft->fillScreen(tft->color565(Property.Color.Background[0], Property.Color.Background[1], Property.Color.Background[2]));
      Property.first_draw = true;
    }

    Oven* oven;

    if (Heating_circuit.Circuit) {
      oven = oven_1;
      Message->set_Message("Okruh 1", "Circuit 1");
      if (oven_1->get_Enable_heating()) {
        if (Faceplate->Button_main(Property.first_draw, true, true, Message->get_Message(),
                                   0, 30, 240, 40, 0,
                                   Property.Color.Green, 12, "MC", 120, 20,
                                   Property.Color.Black, Property.Color.Background, Property.Color.Green,
                                   Property.Color.Black, 0, Property.Rotation)) {
        }
      } else {
        if (Faceplate->Button_main(Property.first_draw, true, true, Message->get_Message(),
                                   0, 30, 240, 40, 0,
                                   Property.Color.Red, 12, "MC", 120, 20,
                                   Property.Color.Black, Property.Color.Background, Property.Color.Red,
                                   Property.Color.Black, 0, Property.Rotation)) {
        }
      }
      Message->set_Message("Okruh 2", "Circuit 2");
      if (oven_2->get_Enable_heating()) {
        if (Faceplate->Button_main(Property.first_draw, true, true, Message->get_Message(),
                                   240, 30, 240, 40, 0,
                                   Property.Color.Green, 12, "MC", 120, 20,
                                   Property.Color.Dark_blue, Property.Color.Background, Property.Color.Green,
                                   Property.Color.Dark_blue, 3, Property.Rotation)) {
          Heating_circuit.Circuit = false;
          Property.first_draw = true;
        }
      } else {
        if (Faceplate->Button_main(Property.first_draw, true, true, Message->get_Message(),
                                   240, 30, 240, 40, 0,
                                   Property.Color.Red, 12, "MC", 120, 20,
                                   Property.Color.Dark_blue, Property.Color.Background, Property.Color.Red,
                                   Property.Color.Dark_blue, 3, Property.Rotation)) {
          Heating_circuit.Circuit = false;
          Property.first_draw = true;
        }
      }
    } else {
      oven = oven_2;
      Message->set_Message("Okruh 2", "Circuit 2");
      if (oven_2->get_Enable_heating()) {
        if (Faceplate->Button_main(Property.first_draw, true, true, Message->get_Message(),
                                   240, 30, 240, 40, 0,
                                   Property.Color.Green, 12, "MC", 120, 20,
                                   Property.Color.Black, Property.Color.Background, Property.Color.Green,
                                   Property.Color.Black, 0, Property.Rotation)) {
        }
      } else {
        if (Faceplate->Button_main(Property.first_draw, true, true, Message->get_Message(),
                                   240, 30, 240, 40, 0,
                                   Property.Color.Red, 12, "MC", 120, 20,
                                   Property.Color.Black, Property.Color.Background, Property.Color.Red,
                                   Property.Color.Black, 0, Property.Rotation)) {
        }
      }
      Message->set_Message("Okruh 1", "Circuit 1");
      if (oven_1->get_Enable_heating()) {
        if (Faceplate->Button_main(Property.first_draw, true, true, Message->get_Message(),
                                   0, 30, 240, 40, 0,
                                   Property.Color.Green, 12, "MC", 120, 20,
                                   Property.Color.Dark_blue, Property.Color.Background, Property.Color.Green,
                                   Property.Color.Dark_blue, 3, Property.Rotation)) {
          Heating_circuit.Circuit = true;
          Property.first_draw = true;
        }
      } else {
        if (Faceplate->Button_main(Property.first_draw, true, true, Message->get_Message(),
                                   0, 30, 240, 40, 0,
                                   Property.Color.Red, 12, "MC", 120, 20,
                                   Property.Color.Dark_blue, Property.Color.Background, Property.Color.Red,
                                   Property.Color.Dark_blue, 3, Property.Rotation)) {
          Heating_circuit.Circuit = true;
          Property.first_draw = true;
        }
      }
    }

    if (Heating_circuit.Page == 1) {
      //Draw
      if (true) {
        Message->set_Message("Okruh:", "Circruit:");
        Faceplate->TextBox_main(Property.first_draw, false, Message->get_Message(),
                                5, 80, 470, 40, 10, 10, "TOP",
                                Property.Color.Text, 9, "ML", 5, 20,
                                Property.Color.Transparent, Property.Color.Transparent,
                                Property.Color.Text, 2, Property.Rotation);

        Message->set_Message("Rezim:", "Mode:");
        Faceplate->TextBox_main(Property.first_draw, false, Message->get_Message(),
                                5, 118, 470, 40, 0, 0, "",
                                Property.Color.Text, 9, "ML", 5, 20,
                                Property.Color.Transparent, Property.Color.Transparent,
                                Property.Color.Text, 2, Property.Rotation);

        Message->set_Message("Pozadovana teplota:", "Required temperature:");
        Faceplate->TextBox_main(Property.first_draw, false, Message->get_Message(),
                                5, 156, 470, 40, 0, 0, "",
                                Property.Color.Text, 9, "ML", 5, 20,
                                Property.Color.Transparent, Property.Color.Transparent,
                                Property.Color.Text, 2, Property.Rotation);

        Message->set_Message("Kladna tolerancia:", "Positive tolerance:");
        Faceplate->TextBox_main(Property.first_draw, false, Message->get_Message(),
                                5, 194, 470, 40, 0, 0, "",
                                Property.Color.Text, 9, "ML", 5, 20,
                                Property.Color.Transparent, Property.Color.Transparent,
                                Property.Color.Text, 2, Property.Rotation);

        Message->set_Message("Zaporna tolerancia:", "Negative tolerance:");
        Faceplate->TextBox_main(Property.first_draw, false, Message->get_Message(),
                                5, 232, 470, 40, 10, 10, "BOTTOM",
                                Property.Color.Text, 9, "ML", 5, 20,
                                Property.Color.Transparent, Property.Color.Transparent,
                                Property.Color.Text, 2, Property.Rotation);
      }
      //Active part
      if (true) {
        do {
          Heating_circuit.Enable_draw = true;
          Message->set_Message("ON", "ON");
          if (oven->get_Enable_heating()) {
            if (Faceplate->Button_main(Property.first_draw, true, true, Message->get_Message(),
                                       180, 82, 100, 36, 5,
                                       Property.Color.Green, 12, "MC", 50, 18,
                                       Property.Color.Background, Property.Color.Background, Property.Color.Green,
                                       Property.Color.Green, 3, Property.Rotation)) {
            }
            Message->set_Message("OFF", "OFF");
            if (Faceplate->Button_main(Property.first_draw, true, true, Message->get_Message(),
                                       360, 82, 100, 36, 5,
                                       Property.Color.Red, 12, "MC", 50, 18,
                                       Property.Color.Background, Property.Color.Background, Property.Color.Green,
                                       Property.Color.Red, 3, Property.Rotation)) {
              oven->set_Enable_heating(false);
              Heating_circuit.Enable_draw = false;
            }
          } else {
            Message->set_Message("OFF", "OFF");
            if (Faceplate->Button_main(Property.first_draw, true, true, Message->get_Message(),
                                       360, 82, 100, 36, 5,
                                       Property.Color.Green, 12, "MC", 50, 18,
                                       Property.Color.Background, Property.Color.Background, Property.Color.Green,
                                       Property.Color.Green, 3, Property.Rotation)) {
            }
            Message->set_Message("ON", "ON");
            if (Faceplate->Button_main(Property.first_draw, true, true, Message->get_Message(),
                                       180, 82, 100, 36, 5,
                                       Property.Color.Red, 12, "MC", 50, 18,
                                       Property.Color.Background, Property.Color.Background, Property.Color.Green,
                                       Property.Color.Red, 3, Property.Rotation)) {
              oven->set_Enable_heating(true);
              Heating_circuit.Enable_draw = false;
            }
          }

        } while (Heating_circuit.Enable_draw == false);

        do {
          Heating_circuit.Enable_draw = true;
          if (oven->get_Mode() == 0) {
            Message->set_Message("ON", "ON");
            if (Faceplate->Button_main(Property.first_draw, true, true, Message->get_Message(),
                                       180, 120, 83, 36, 5,
                                       Property.Color.Red, 12, "MC", 41, 18,
                                       Property.Color.Background, Property.Color.Background, Property.Color.Green,
                                       Property.Color.Red, 3, Property.Rotation)) {
              oven->set_Mode(1);
              Heating_circuit.Enable_draw = false;
            }
            Message->set_Message("AUTO", "AUTO");
            if (Faceplate->Button_main(Property.first_draw || Heating_circuit.Enable_draw, true, true, Message->get_Message(),
                                       278, 120, 83, 36, 5,
                                       Property.Color.Red, 12, "MC", 41, 18,
                                       Property.Color.Background, Property.Color.Background, Property.Color.Green,
                                       Property.Color.Red, 3, Property.Rotation)) {
              oven->set_Mode(2);
              Heating_circuit.Enable_draw = false;
            }
            Message->set_Message("OFF", "OFF");
            if (Faceplate->Button_main(Property.first_draw || Heating_circuit.Enable_draw, true, true, Message->get_Message(),
                                       376, 120, 83, 36, 5,
                                       Property.Color.Green, 12, "MC", 41, 18,
                                       Property.Color.Background, Property.Color.Background, Property.Color.Green,
                                       Property.Color.Green, 3, Property.Rotation)) {
            }
          } else if (oven->get_Mode() == 1) {
            Message->set_Message("ON", "ON");
            if (Faceplate->Button_main(Property.first_draw, true, true, Message->get_Message(),
                                       180, 120, 83, 36, 5,
                                       Property.Color.Green, 12, "MC", 41, 18,
                                       Property.Color.Background, Property.Color.Background, Property.Color.Green,
                                       Property.Color.Green, 3, Property.Rotation)) {
            }
            Message->set_Message("AUTO", "AUTO");
            if (Faceplate->Button_main(Property.first_draw, true, true, Message->get_Message(),
                                       278, 120, 83, 36, 5,
                                       Property.Color.Red, 12, "MC", 41, 18,
                                       Property.Color.Background, Property.Color.Background, Property.Color.Green,
                                       Property.Color.Red, 3, Property.Rotation)) {
              oven->set_Mode(2);
              Heating_circuit.Enable_draw = false;
            }
            Message->set_Message("OFF", "OFF");
            if (Faceplate->Button_main(Property.first_draw, true, true, Message->get_Message(),
                                       376, 120, 83, 36, 5,
                                       Property.Color.Red, 12, "MC", 41, 18,
                                       Property.Color.Background, Property.Color.Background, Property.Color.Green,
                                       Property.Color.Red, 3, Property.Rotation)) {
              oven->set_Mode(0);
              Heating_circuit.Enable_draw = false;
            }
          } else if (oven->get_Mode() == 2) {
            Message->set_Message("ON", "ON");
            if (Faceplate->Button_main(Property.first_draw, true, true, Message->get_Message(),
                                       180, 120, 83, 36, 5,
                                       Property.Color.Red, 12, "MC", 41, 18,
                                       Property.Color.Background, Property.Color.Background, Property.Color.Green,
                                       Property.Color.Red, 3, Property.Rotation)) {
              oven->set_Mode(1);
              Heating_circuit.Enable_draw = false;
            }
            Message->set_Message("AUTO", "AUTO");
            if (Faceplate->Button_main(Property.first_draw, true, true, Message->get_Message(),
                                       278, 120, 83, 36, 5,
                                       Property.Color.Green, 12, "MC", 41, 18,
                                       Property.Color.Background, Property.Color.Background, Property.Color.Green,
                                       Property.Color.Green, 3, Property.Rotation)) {
            }
            Message->set_Message("OFF", "OFF");
            if (Faceplate->Button_main(Property.first_draw, true, true, Message->get_Message(),
                                       376, 120, 83, 36, 5,
                                       Property.Color.Red, 12, "MC", 41, 18,
                                       Property.Color.Background, Property.Color.Background, Property.Color.Green,
                                       Property.Color.Red, 3, Property.Rotation)) {
              oven->set_Mode(0);
              Heating_circuit.Enable_draw = false;
            }
          }
        } while (Heating_circuit.Enable_draw == false);

        Faceplate->TextBox_main(Property.first_draw || Touch.Releast, false, (String)oven->get_Required_T(),
                                300, 158, 60, 36, 0, 0, "",
                                Property.Color.Text, 12, "MC", 30, 18,
                                Property.Color.Black, Property.Color.Black,
                                Property.Color.Black, 2, Property.Rotation);

        if (Faceplate->Button_main(Property.first_draw, false, true, "+",
                                   216, 158, 70, 36, 5,
                                   Property.Color.Red, 18, "MC", 35, 13,
                                   Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                   Property.Color.Red, 3, Property.Rotation)) {
          Property.Pressing.ID = 1;
          Faceplate->TextBox_main(true, true, (String)oven->get_Required_T(),
                                  300, 158, 60, 36, 0, 0, "",
                                  Property.Color.Text, 12, "MC", 30, 18,
                                  Property.Color.Black, Property.Color.Black,
                                  Property.Color.Black, 2, Property.Rotation);
        }
        if (Faceplate->Button_main(Property.first_draw, false, true, "-",
                                   386, 158, 70, 36, 5,
                                   Property.Color.Red, 18, "MC", 35, 13,
                                   Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                   Property.Color.Red, 3, Property.Rotation)) {
          Property.Pressing.ID = 2;
          Faceplate->TextBox_main(true, true, (String)oven->get_Required_T(),
                                  300, 158, 60, 36, 0, 0, "",
                                  Property.Color.Text, 12, "MC", 30, 18,
                                  Property.Color.Black, Property.Color.Black,
                                  Property.Color.Black, 2, Property.Rotation);
        }

        Faceplate->TextBox_main(Property.first_draw || Touch.Releast, false, (String)oven->get_Positive_tolerance(),
                                300, 196, 60, 36, 0, 0, "",
                                Property.Color.Text, 12, "MC", 30, 18,
                                Property.Color.Black, Property.Color.Black,
                                Property.Color.Black, 2, Property.Rotation);

        if (Faceplate->Button_main(Property.first_draw, false, true, "+",
                                   216, 196, 70, 36, 5,
                                   Property.Color.Red, 18, "MC", 35, 13,
                                   Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                   Property.Color.Red, 3, Property.Rotation)) {
          Property.Pressing.ID = 3;
          Faceplate->TextBox_main(true, true, (String)oven->get_Positive_tolerance(),
                                  300, 196, 60, 36, 0, 0, "",
                                  Property.Color.Text, 12, "MC", 30, 18,
                                  Property.Color.Black, Property.Color.Black,
                                  Property.Color.Black, 2, Property.Rotation);
        }
        if (Faceplate->Button_main(Property.first_draw, false, true, "-",
                                   386, 196, 70, 36, 5,
                                   Property.Color.Red, 18, "MC", 35, 13,
                                   Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                   Property.Color.Red, 3, Property.Rotation)) {
          Property.Pressing.ID = 4;
          Faceplate->TextBox_main(true, true, (String)oven->get_Positive_tolerance(),
                                  300, 196, 60, 36, 0, 0, "",
                                  Property.Color.Text, 12, "MC", 30, 18,
                                  Property.Color.Black, Property.Color.Black,
                                  Property.Color.Black, 2, Property.Rotation);
        }

        Faceplate->TextBox_main(Property.first_draw || Touch.Releast, false, (String)oven->get_Negative_tolerance(),
                                300, 234, 60, 36, 0, 0, "",
                                Property.Color.Text, 12, "MC", 30, 18,
                                Property.Color.Black, Property.Color.Black,
                                Property.Color.Black, 2, Property.Rotation);

        if (Faceplate->Button_main(Property.first_draw, false, true, "+",
                                   216, 234, 70, 36, 5,
                                   Property.Color.Red, 18, "MC", 35, 13,
                                   Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                   Property.Color.Red, 3, Property.Rotation)) {
          Property.Pressing.ID = 5;
          Faceplate->TextBox_main(true, true, (String)oven->get_Negative_tolerance(),
                                  300, 234, 60, 36, 0, 0, "",
                                  Property.Color.Text, 12, "MC", 30, 18,
                                  Property.Color.Black, Property.Color.Black,
                                  Property.Color.Black, 2, Property.Rotation);
        }
        if (Faceplate->Button_main(Property.first_draw, false, true, "-",
                                   386, 234, 70, 36, 5,
                                   Property.Color.Red, 18, "MC", 35, 13,
                                   Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                   Property.Color.Red, 3, Property.Rotation)) {
          Property.Pressing.ID = 6;
          Faceplate->TextBox_main(true, true, (String)oven->get_Negative_tolerance(),
                                  300, 234, 60, 36, 0, 0, "",
                                  Property.Color.Text, 12, "MC", 30, 18,
                                  Property.Color.Black, Property.Color.Black,
                                  Property.Color.Black, 2, Property.Rotation);
        }

        //Back button
        if (Faceplate->Icon_button_advance_main(Property.first_draw, true, BackBlue, BackGreen,
                                                0, 280, 235, 40, 32, 32, 100, 4, 20,
                                                Property.Color.Background, Property.Color.Background, Property.Color.Focused,
                                                Property.Color.Blue, 3, Property.Rotation)) {
          page = Menu;
        }
        if (Faceplate->Icon_button_advance_main(Property.first_draw, true, right_chevron, right_chevronGreen,
                                                245, 280, 235, 40, 32, 32, 100, 4, 20,
                                                Property.Color.Background, Property.Color.Background, Property.Color.Focused,
                                                Property.Color.Blue, 3, Property.Rotation)) {
          Heating_circuit.Page = 2;
          //for refresh
          prev_page = Main_page;

          Serial.println("Heating page = " + String(Heating_circuit.Page));
        }
      }
    } else {
      //Draw part
      if (true) {
        Message->set_Message("Upozornenie VT:", "Warning HT");
        Faceplate->TextBox_main(Property.first_draw, false, Message->get_Message(),
                                5, 80, 470, 40, 10, 10, "TOP",
                                Property.Color.Text, 9, "ML", 5, 20,
                                Property.Color.Transparent, Property.Color.Transparent,
                                Property.Color.Text, 2, Property.Rotation);

        Message->set_Message("Upozornenie NT", "Warning LT:");
        Faceplate->TextBox_main(Property.first_draw, false, Message->get_Message(),
                                5, 118, 470, 40, 0, 0, "",
                                Property.Color.Text, 9, "ML", 5, 20,
                                Property.Color.Transparent, Property.Color.Transparent,
                                Property.Color.Text, 2, Property.Rotation);

        Message->set_Message("PID:", "PID:");
        Faceplate->TextBox_main(Property.first_draw, false, Message->get_Message(),
                                5, 156, 470, 40, 0, 0, "",
                                Property.Color.Text, 9, "ML", 5, 20,
                                Property.Color.Transparent, Property.Color.Transparent,
                                Property.Color.Text, 2, Property.Rotation);

        Message->set_Message("Rezim PID:", "Mode PID:");
        Faceplate->TextBox_main(Property.first_draw, false, Message->get_Message(),
                                5, 194, 470, 40, 0, 0, "",
                                Property.Color.Text, 9, "ML", 5, 20,
                                Property.Color.Transparent, Property.Color.Transparent,
                                Property.Color.Text, 2, Property.Rotation);

        Message->set_Message("Interval:", "Interval:");
        Faceplate->TextBox_main(Property.first_draw, false, Message->get_Message(),
                                5, 232, 470, 40, 10, 10, "BOTTOM",
                                Property.Color.Text, 9, "ML", 5, 20,
                                Property.Color.Transparent, Property.Color.Transparent,
                                Property.Color.Text, 2, Property.Rotation);
      }
      //Active part
      if (true) {
        Faceplate->TextBox_main(Property.first_draw || Touch.Releast, false, (String)oven->get_Upper_limit(),
                                300, 82, 60, 36, 0, 0, "",
                                Property.Color.Text, 12, "MC", 30, 18,
                                Property.Color.Black, Property.Color.Black,
                                Property.Color.Black, 2, Property.Rotation);

        if (Faceplate->Button_main(Property.first_draw, false, true, "+",
                                   216, 82, 70, 36, 5,
                                   Property.Color.Red, 18, "MC", 35, 13,
                                   Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                   Property.Color.Red, 3, Property.Rotation)) {
          Property.Pressing.ID = 7;
          Faceplate->TextBox_main(true, true, (String)oven->get_Upper_limit(),
                                  300, 82, 60, 36, 0, 0, "",
                                  Property.Color.Text, 12, "MC", 30, 18,
                                  Property.Color.Black, Property.Color.Black,
                                  Property.Color.Black, 2, Property.Rotation);
        }
        if (Faceplate->Button_main(Property.first_draw, false, true, "-",
                                   386, 82, 70, 36, 5,
                                   Property.Color.Red, 18, "MC", 35, 13,
                                   Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                   Property.Color.Red, 3, Property.Rotation)) {
          Property.Pressing.ID = 8;
          Faceplate->TextBox_main(true, true, (String)oven->get_Upper_limit(),
                                  300, 82, 60, 36, 0, 0, "",
                                  Property.Color.Text, 12, "MC", 30, 18,
                                  Property.Color.Black, Property.Color.Black,
                                  Property.Color.Black, 2, Property.Rotation);
        }

        Faceplate->TextBox_main(Property.first_draw || Touch.Releast, false, (String)oven->get_Lower_limit(),
                                300, 120, 60, 36, 0, 0, "",
                                Property.Color.Text, 12, "MC", 30, 18,
                                Property.Color.Black, Property.Color.Black,
                                Property.Color.Black, 2, Property.Rotation);

        if (Faceplate->Button_main(Property.first_draw, false, true, "+",
                                   216, 120, 70, 36, 5,
                                   Property.Color.Red, 18, "MC", 35, 13,
                                   Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                   Property.Color.Red, 3, Property.Rotation)) {
          Property.Pressing.ID = 9;
          Faceplate->TextBox_main(true, true, (String)oven->get_Lower_limit(),
                                  300, 120, 60, 36, 0, 0, "",
                                  Property.Color.Text, 12, "MC", 30, 18,
                                  Property.Color.Black, Property.Color.Black,
                                  Property.Color.Black, 2, Property.Rotation);
        }
        if (Faceplate->Button_main(Property.first_draw, false, true, "-",
                                   386, 120, 70, 36, 5,
                                   Property.Color.Red, 18, "MC", 35, 13,
                                   Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                   Property.Color.Red, 3, Property.Rotation)) {
          Property.Pressing.ID = 10;
          Faceplate->TextBox_main(true, true, (String)oven->get_Lower_limit(),
                                  300, 120, 60, 36, 0, 0, "",
                                  Property.Color.Text, 12, "MC", 30, 18,
                                  Property.Color.Black, Property.Color.Black,
                                  Property.Color.Black, 2, Property.Rotation);
        }
        do {
          Heating_circuit.Enable_draw = true;
          Message->set_Message("ON", "ON");
          if (oven->get_PID_Mode() != 0) {
            if (Faceplate->Button_main(Property.first_draw, true, true, Message->get_Message(),
                                       180, 158, 100, 36, 5,
                                       Property.Color.Green, 12, "MC", 50, 18,
                                       Property.Color.Background, Property.Color.Background, Property.Color.Green,
                                       Property.Color.Green, 3, Property.Rotation)) {
            }
            Message->set_Message("OFF", "OFF");
            if (Faceplate->Button_main(Property.first_draw, true, true, Message->get_Message(),
                                       360, 158, 100, 36, 5,
                                       Property.Color.Red, 12, "MC", 50, 18,
                                       Property.Color.Background, Property.Color.Background, Property.Color.Green,
                                       Property.Color.Red, 3, Property.Rotation)) {
              oven->set_PID_Mode(0);
              Heating_circuit.Enable_draw = false;
              //for refresh
              prev_page = Main_page;
            }
          } else {
            Message->set_Message("OFF", "OFF");
            if (Faceplate->Button_main(Property.first_draw, true, true, Message->get_Message(),
                                       360, 158, 100, 36, 5,
                                       Property.Color.Green, 12, "MC", 50, 18,
                                       Property.Color.Background, Property.Color.Background, Property.Color.Green,
                                       Property.Color.Green, 3, Property.Rotation)) {
            }
            Message->set_Message("ON", "ON");
            if (Faceplate->Button_main(Property.first_draw, true, true, Message->get_Message(),
                                       180, 158, 100, 36, 5,
                                       Property.Color.Red, 12, "MC", 50, 18,
                                       Property.Color.Background, Property.Color.Background, Property.Color.Green,
                                       Property.Color.Red, 3, Property.Rotation)) {
              oven->set_PID_Mode(1);
              Heating_circuit.Enable_draw = false;
              //for refresh
              prev_page = Main_page;
            }
          }

        } while (Heating_circuit.Enable_draw == false);

        if (oven->get_PID_Mode() != 0) {
          do {
            Heating_circuit.Enable_draw = true;
            if (oven->get_PID_Mode() == 3) {
              Message->set_Message("LOW", "LOW");
              if (Faceplate->Button_main(Property.first_draw, true, true, Message->get_Message(),
                                         180, 196, 83, 36, 5,
                                         Property.Color.Red, 12, "MC", 41, 18,
                                         Property.Color.Background, Property.Color.Background, Property.Color.Green,
                                         Property.Color.Red, 3, Property.Rotation)) {
                oven->set_PID_Mode(1);
                Heating_circuit.Enable_draw = false;
              }
              Message->set_Message("MID", "MID");
              if (Faceplate->Button_main(Property.first_draw || Heating_circuit.Enable_draw, true, true, Message->get_Message(),
                                         278, 196, 83, 36, 5,
                                         Property.Color.Red, 12, "MC", 41, 18,
                                         Property.Color.Background, Property.Color.Background, Property.Color.Green,
                                         Property.Color.Red, 3, Property.Rotation)) {
                oven->set_PID_Mode(2);
                Heating_circuit.Enable_draw = false;
              }
              Message->set_Message("HIGH", "HIGH");
              if (Faceplate->Button_main(Property.first_draw || Heating_circuit.Enable_draw, true, true, Message->get_Message(),
                                         376, 196, 83, 36, 5,
                                         Property.Color.Green, 12, "MC", 41, 18,
                                         Property.Color.Background, Property.Color.Background, Property.Color.Green,
                                         Property.Color.Green, 3, Property.Rotation)) {
              }
            } else if (oven->get_PID_Mode() == 1) {
              Message->set_Message("LOW", "LOW");
              if (Faceplate->Button_main(Property.first_draw, true, true, Message->get_Message(),
                                         180, 196, 83, 36, 5,
                                         Property.Color.Green, 12, "MC", 41, 18,
                                         Property.Color.Background, Property.Color.Background, Property.Color.Green,
                                         Property.Color.Green, 3, Property.Rotation)) {
              }
              Message->set_Message("MID", "MID");
              if (Faceplate->Button_main(Property.first_draw, true, true, Message->get_Message(),
                                         278, 196, 83, 36, 5,
                                         Property.Color.Red, 12, "MC", 41, 18,
                                         Property.Color.Background, Property.Color.Background, Property.Color.Green,
                                         Property.Color.Red, 3, Property.Rotation)) {
                oven->set_PID_Mode(2);
                Heating_circuit.Enable_draw = false;
              }
              Message->set_Message("HIGH", "HIGH");
              if (Faceplate->Button_main(Property.first_draw, true, true, Message->get_Message(),
                                         376, 196, 83, 36, 5,
                                         Property.Color.Red, 12, "MC", 41, 18,
                                         Property.Color.Background, Property.Color.Background, Property.Color.Green,
                                         Property.Color.Red, 3, Property.Rotation)) {
                oven->set_PID_Mode(3);
                Heating_circuit.Enable_draw = false;
              }
            } else if (oven->get_PID_Mode() == 2) {
              Message->set_Message("LOW", "LOW");
              if (Faceplate->Button_main(Property.first_draw, true, true, Message->get_Message(),
                                         180, 196, 83, 36, 5,
                                         Property.Color.Red, 12, "MC", 41, 18,
                                         Property.Color.Background, Property.Color.Background, Property.Color.Green,
                                         Property.Color.Red, 3, Property.Rotation)) {
                oven->set_PID_Mode(1);
                Heating_circuit.Enable_draw = false;
              }
              Message->set_Message("MID", "MID");
              if (Faceplate->Button_main(Property.first_draw, true, true, Message->get_Message(),
                                         278, 196, 83, 36, 5,
                                         Property.Color.Green, 12, "MC", 41, 18,
                                         Property.Color.Background, Property.Color.Background, Property.Color.Green,
                                         Property.Color.Green, 3, Property.Rotation)) {
              }
              Message->set_Message("HIGH", "HIGH");
              if (Faceplate->Button_main(Property.first_draw, true, true, Message->get_Message(),
                                         376, 196, 83, 36, 5,
                                         Property.Color.Red, 12, "MC", 41, 18,
                                         Property.Color.Background, Property.Color.Background, Property.Color.Green,
                                         Property.Color.Red, 3, Property.Rotation)) {
                oven->set_PID_Mode(3);
                Heating_circuit.Enable_draw = false;
              }
            }
          } while (Heating_circuit.Enable_draw == false);
        } else {
          Message->set_Message("LOW", "LOW");
          if (Faceplate->Button_main(Property.first_draw, true, false, Message->get_Message(),
                                     180, 196, 83, 36, 5,
                                     Property.Color.Red, 12, "MC", 41, 18,
                                     Property.Color.Background, Property.Color.Background, Property.Color.Green,
                                     Property.Color.Red, 3, Property.Rotation)) {
          }
          Message->set_Message("MID", "MID");
          if (Faceplate->Button_main(Property.first_draw, true, false, Message->get_Message(),
                                     278, 196, 83, 36, 5,
                                     Property.Color.Green, 12, "MC", 41, 18,
                                     Property.Color.Background, Property.Color.Background, Property.Color.Green,
                                     Property.Color.Green, 3, Property.Rotation)) {
          }
          Message->set_Message("HIGH", "HIGH");
          if (Faceplate->Button_main(Property.first_draw, true, false, Message->get_Message(),
                                     376, 196, 83, 36, 5,
                                     Property.Color.Red, 12, "MC", 41, 18,
                                     Property.Color.Background, Property.Color.Background, Property.Color.Green,
                                     Property.Color.Red, 3, Property.Rotation)) {
          }
        }

        String String_Convertor = String(oven->get_Heating_Interval());
        if (oven->get_Heating_Interval() == 3) String_Convertor = "ALL";

        Faceplate->TextBox_main(Property.first_draw || Touch.Releast, false, String_Convertor,
                                300, 234, 60, 36, 0, 0, "",
                                Property.Color.Text, 12, "MC", 30, 18,
                                Property.Color.Black, Property.Color.Black,
                                Property.Color.Black, 2, Property.Rotation);

        if (Faceplate->Button_main(Property.first_draw, false, true, "+",
                                   216, 234, 70, 36, 5,
                                   Property.Color.Red, 18, "MC", 35, 13,
                                   Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                   Property.Color.Red, 3, Property.Rotation)) {
          Property.Pressing.ID = 42;
          Faceplate->TextBox_main(true, true, String_Convertor,
                                  300, 234, 60, 36, 0, 0, "",
                                  Property.Color.Text, 12, "MC", 30, 18,
                                  Property.Color.Black, Property.Color.Black,
                                  Property.Color.Black, 2, Property.Rotation);
        }
        if (Faceplate->Button_main(Property.first_draw, false, true, "-",
                                   386, 234, 70, 36, 5,
                                   Property.Color.Red, 18, "MC", 35, 13,
                                   Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                   Property.Color.Red, 3, Property.Rotation)) {
          Property.Pressing.ID = 43;
          Faceplate->TextBox_main(true, true, String_Convertor,
                                  300, 234, 60, 36, 0, 0, "",
                                  Property.Color.Text, 12, "MC", 30, 18,
                                  Property.Color.Black, Property.Color.Black,
                                  Property.Color.Black, 2, Property.Rotation);
        }

        if (Faceplate->Icon_button_advance_main(Property.first_draw, true, left_chevron, left_chevronGreen,
                                                0, 280, 480, 40, 32, 32, 224, 4, 20,
                                                Property.Color.Background, Property.Color.Background, Property.Color.Focused,
                                                Property.Color.Blue, 3, Property.Rotation)) {
          Heating_circuit.Page = 1;
          //for refresh
          prev_page = Main_page;
          Serial.println("Heating page = " + String(Heating_circuit.Page));
        }
      }
    }

    return true;
  }

  bool fcSleep() {

    int X_ = 200;
    int Y_ = 45;
    int Y_2 = Y_ + 100;

    //Draw
    if (true) {
      Faceplate->Icon_main(Property.first_draw, 15, 85, 128, 128, 128, 128, 0, 0, sleeping, Property.Color.Black);

      Message->set_Message("Cas spanku:", "Sleep time:");
      Faceplate->TextBox_main(Property.first_draw, false, Message->get_Message(),
                              X_, Y_, 240, 40, 10, 10, "",
                              Property.Color.Text, 12, "MC", 120, 20,
                              Property.Color.Transparent, Property.Color.Transparent,
                              Property.Color.Background, 0, Property.Rotation);

      Message->set_Message("Vypnutie po:", "Turn off after:");
      Faceplate->TextBox_main(Property.first_draw, false, Message->get_Message(),
                              X_, Y_2, 240, 40, 10, 10, "",
                              Property.Color.Text, 12, "MC", 120, 20,
                              Property.Color.Transparent, Property.Color.Transparent,
                              Property.Color.Background, 0, Property.Rotation);
    }

    String String_convertor = String(String(get_Sleep_time()) + "m");
    //Active
    if (true) {
      Faceplate->TextBox_main(Property.first_draw || Touch.Releast, false, String_convertor,
                              X_ + 84, Y_ + 50, 60, 36, 0, 0, "",
                              Property.Color.Text, 12, "MC", 30, 18,
                              Property.Color.Black, Property.Color.Black,
                              Property.Color.Black, 2, Property.Rotation);

      if (Faceplate->Button_main(Property.first_draw, false, true, "+",
                                 X_, Y_ + 50, 70, 36, 5,
                                 Property.Color.Red, 18, "MC", 35, 13,
                                 Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                 Property.Color.Red, 3, Property.Rotation)) {
        Property.Pressing.ID = 45;
        Faceplate->TextBox_main(true, true, String_convertor,
                                X_ + 84, Y_ + 50, 60, 36, 0, 0, "",
                                Property.Color.Text, 12, "MC", 30, 18,
                                Property.Color.Black, Property.Color.Black,
                                Property.Color.Black, 2, Property.Rotation);
      }
      if (Faceplate->Button_main(Property.first_draw, false, true, "-",
                                 X_ + 170, Y_ + 50, 70, 36, 5,
                                 Property.Color.Red, 18, "MC", 35, 13,
                                 Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                 Property.Color.Red, 3, Property.Rotation)) {
        Property.Pressing.ID = 46;
        Faceplate->TextBox_main(true, true, String_convertor,
                                X_ + 84, Y_ + 50, 60, 36, 0, 0, "",
                                Property.Color.Text, 12, "MC", 30, 18,
                                Property.Color.Black, Property.Color.Black,
                                Property.Color.Black, 2, Property.Rotation);
      }

      String_convertor = String(String(get_Sleep_time_constant()) + "s");

      Faceplate->TextBox_main(Property.first_draw || Touch.Releast, false, String_convertor,
                              X_ + 84, Y_2 + 50, 60, 36, 0, 0, "",
                              Property.Color.Text, 12, "MC", 30, 18,
                              Property.Color.Black, Property.Color.Black,
                              Property.Color.Black, 2, Property.Rotation);

      if (Faceplate->Button_main(Property.first_draw, false, true, "+",
                                 X_, Y_2 + 50, 70, 36, 5,
                                 Property.Color.Red, 18, "MC", 35, 13,
                                 Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                 Property.Color.Red, 3, Property.Rotation)) {
        Property.Pressing.ID = 47;
        Faceplate->TextBox_main(true, true, String_convertor,
                                X_ + 84, Y_2 + 50, 60, 36, 0, 0, "",
                                Property.Color.Text, 12, "MC", 30, 18,
                                Property.Color.Black, Property.Color.Black,
                                Property.Color.Black, 2, Property.Rotation);
      }
      if (Faceplate->Button_main(Property.first_draw, false, true, "-",
                                 X_ + 170, Y_2 + 50, 70, 36, 5,
                                 Property.Color.Red, 18, "MC", 35, 13,
                                 Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                 Property.Color.Red, 3, Property.Rotation)) {
        Property.Pressing.ID = 48;
        Faceplate->TextBox_main(true, true, String_convertor,
                                X_ + 84, Y_2 + 50, 60, 36, 0, 0, "",
                                Property.Color.Text, 12, "MC", 30, 18,
                                Property.Color.Black, Property.Color.Black,
                                Property.Color.Black, 2, Property.Rotation);
      }
    }


    //Back button
    if (Faceplate->Icon_button_advance_main(Property.first_draw, true, BackBlue, BackGreen,
                                            0, 280, 480, 40, 32, 32, 224, 4, 20,
                                            Property.Color.Background, Property.Color.Background, Property.Color.Focused,
                                            Property.Color.Blue, 3, Property.Rotation)) {
      page = Menu;
    }
    return true;
  }

  struct Temperature_struct {
    int last_Mode;
  };
  Temperature_struct Temp_Struct;
  bool fcTemperature_sensor() {

    if (Temp_Struct.last_Mode != Temp->get_Mode()) {
      Temp_Struct.last_Mode = Temp->get_Mode();
      Property.first_draw = true;
    }

    int X_SHT = 20, Y_SHT = 50;
    int X_HTU = 332, Y_HTU = Y_SHT;
    bool Draw_numbers = false;
    String String_convertor;

    if (Temp->get_Mode() == 1) {
      //SHT
      if (Faceplate->Icon_button_main(Property.first_draw, X_SHT, Y_SHT, 128, 128, 128, 128, 0, 0,
                                      true, temperature_control, temperature_control, Property.Color.Background)) {
        Draw_numbers = true;
      }
      //HTU
      if (Faceplate->Icon_button_main(Property.first_draw, X_HTU, Y_HTU, 128, 128, 128, 128, 0, 0,
                                      true, temperature_control_disable, temperature_control, Property.Color.Background)) {
        Temp->set_Mode(3);
        Temp->set_Temperature();
        Draw_numbers = true;
      }

    } else if (Temp->get_Mode() == 2) {
      //SHT
      if (Faceplate->Icon_button_main(Property.first_draw, X_SHT, Y_SHT, 128, 128, 128, 128, 0, 0,
                                      true, temperature_control_disable, temperature_control, Property.Color.Background)) {
        Temp->set_Mode(3);
        Temp->set_Temperature();
        Draw_numbers = true;
      }
      //HTU
      if (Faceplate->Icon_button_main(Property.first_draw, X_HTU, Y_HTU, 128, 128, 128, 128, 0, 0,
                                      true, temperature_control, temperature_control, Property.Color.Background)) {
        Draw_numbers = true;
      }

    } else {
      //SHT
      if (Faceplate->Icon_button_main(Property.first_draw, X_SHT, Y_SHT, 128, 128, 128, 128, 0, 0,
                                      true, temperature_control, temperature_control_disable, Property.Color.Background)) {
        Temp->set_Mode(2);
        Temp->set_Temperature();
        Draw_numbers = true;
      }
      //HTU
      if (Faceplate->Icon_button_main(Property.first_draw, X_HTU, Y_HTU, 128, 128, 128, 128, 0, 0,
                                      true, temperature_control, temperature_control_disable, Property.Color.Background)) {
        Temp->set_Mode(1);
        Temp->set_Temperature();
        Draw_numbers = true;
      }
    }
    //Draw
    if (true) {
      Faceplate->Icon_main(Property.first_draw, 176, Y_SHT + 98, 128, 30, 128, 128, 0, -49, line, Property.Color.Background);

      if (Temp->get_Temperature(3) >= oven_2->get_Upper_limit()) {
        Faceplate->TextBox_main(Property.first_draw, false, String(Temp->get_Temperature(3)),
                                176, Y_SHT, 128, 50, 0, 0, "",
                                Property.Color.Red, 24, "MC", 64, 25,
                                Property.Color.Background, Property.Color.Background,
                                Property.Color.White, 0, Property.Rotation);
      } else if (Temp->get_Temperature(3) <= oven_2->get_Lower_limit()) {
        Faceplate->TextBox_main(Property.first_draw, false, String(Temp->get_Temperature(3)),
                                176, Y_SHT, 128, 50, 0, 0, "",
                                Property.Color.Blue, 24, "MC", 64, 25,
                                Property.Color.Background, Property.Color.Background,
                                Property.Color.White, 0, Property.Rotation);
      } else {
        Faceplate->TextBox_main(Property.first_draw, false, String(Temp->get_Temperature(3)),
                                176, Y_SHT, 128, 50, 0, 0, "",
                                Property.Color.White, 24, "MC", 64, 25,
                                Property.Color.Background, Property.Color.Background,
                                Property.Color.White, 0, Property.Rotation);
      }
      Faceplate->TextBox_main(Property.first_draw, false, String(Temp->get_Humidity(3)),
                              176, Y_SHT + 60, 128, 40, 0, 0, "",
                              Property.Color.White, 18, "MC", 64, 20,
                              Property.Color.Background, Property.Color.Background,
                              Property.Color.White, 0, Property.Rotation);

      if (Faceplate->Icon_button_advance_main(Property.first_draw, true, testing, testingGreen,
                                              100, 180, 280, 40, 32, 32, 124, 4, 10,
                                              Property.Color.Background, Property.Color.Background, Property.Color.Background,
                                              Property.Color.Background, 0, Property.Rotation)) {
        Temp->test_Temperature();
        Temp->set_Temperature();
      }

      Message->set_Message("Aktualizacia:", "Update time:");
      Faceplate->TextBox_main(Property.first_draw, false, Message->get_Message(),
                              5, 227, 470, 40, 10, 10, "",
                              Property.Color.Text, 9, "ML", 5, 20,
                              Property.Color.Transparent, Property.Color.Transparent,
                              Property.Color.Background, 0, Property.Rotation);
    }

    int value = Temp->get_Update_time() / 1000;
    String_convertor = String(String(value) + "s");
    //Active
    if (true) {
      Faceplate->TextBox_main(Property.first_draw || Touch.Releast, false, String_convertor,
                              300, 229, 60, 36, 0, 0, "",
                              Property.Color.Text, 12, "MC", 30, 18,
                              Property.Color.Black, Property.Color.Black,
                              Property.Color.Black, 2, Property.Rotation);

      if (Faceplate->Button_main(Property.first_draw, false, true, "+",
                                 216, 229, 70, 36, 5,
                                 Property.Color.Red, 18, "MC", 35, 13,
                                 Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                 Property.Color.Red, 3, Property.Rotation)) {
        Property.Pressing.ID = 40;
        Draw_numbers = true;
        Faceplate->TextBox_main(true, true, String_convertor,
                                300, 229, 60, 36, 0, 0, "",
                                Property.Color.Text, 12, "MC", 30, 18,
                                Property.Color.Black, Property.Color.Black,
                                Property.Color.Black, 2, Property.Rotation);
      }
      if (Faceplate->Button_main(Property.first_draw, false, true, "-",
                                 386, 229, 70, 36, 5,
                                 Property.Color.Red, 18, "MC", 35, 13,
                                 Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                 Property.Color.Red, 3, Property.Rotation)) {
        Property.Pressing.ID = 41;
        Draw_numbers = true;
        if (Temp->get_Mode() == 1) Temp_Struct.last_Mode = 2;
        else Temp_Struct.last_Mode = 1;
        Faceplate->TextBox_main(true, true, String_convertor,
                                300, 229, 60, 36, 0, 0, "",
                                Property.Color.Text, 12, "MC", 30, 18,
                                Property.Color.Black, Property.Color.Black,
                                Property.Color.Black, 2, Property.Rotation);
      }
    }

    //Draw Numbers on icons
    if (true) {
      if (esp_now->get_ID() == 3) {
        if (Temp->get_Temperature(1) >= oven_2->get_Upper_limit()) {
          Faceplate->TextBox_main(true, false, String(Temp->get_Temperature(1)),
                                  X_SHT + 15, Y_SHT + 30, 90, 26, 0, 0, "",
                                  Property.Color.Red, 12, "MC", 45, 13,
                                  Property.Color.Transparent, Property.Color.Transparent,
                                  Property.Color.White, 0, Property.Rotation);
        } else if (Temp->get_Temperature(1) <= oven_2->get_Lower_limit()) {
          Faceplate->TextBox_main(true, false, String(Temp->get_Temperature(1)),
                                  X_SHT + 15, Y_SHT + 30, 90, 26, 0, 0, "",
                                  Property.Color.Blue, 12, "MC", 45, 13,
                                  Property.Color.Transparent, Property.Color.Transparent,
                                  Property.Color.White, 0, Property.Rotation);
        } else {
          Faceplate->TextBox_main(true, false, String(Temp->get_Temperature(1)),
                                  X_SHT + 15, Y_SHT + 30, 90, 26, 0, 0, "",
                                  Property.Color.Black, 12, "MC", 45, 13,
                                  Property.Color.Transparent, Property.Color.Transparent,
                                  Property.Color.White, 0, Property.Rotation);
        }
        Faceplate->TextBox_main(true, false, String(Temp->get_Humidity(1)),
                                X_SHT + 15, Y_SHT + 55, 90, 24, 0, 0, "",
                                Property.Color.Black, 9, "MR", 80, 12,
                                Property.Color.Transparent, Property.Color.Transparent,
                                Property.Color.White, 0, Property.Rotation);

        if (Temp->get_Temperature(2) >= oven_2->get_Upper_limit()) {
          Faceplate->TextBox_main(true, false, String(Temp->get_Temperature(2)),
                                  X_HTU + 15, Y_HTU + 30, 90, 26, 0, 0, "",
                                  Property.Color.Red, 12, "MC", 45, 13,
                                  Property.Color.Transparent, Property.Color.Transparent,
                                  Property.Color.White, 0, Property.Rotation);
        } else if (Temp->get_Temperature(2) <= oven_2->get_Lower_limit()) {
          Faceplate->TextBox_main(true, false, String(Temp->get_Temperature(2)),
                                  X_HTU + 15, Y_HTU + 30, 90, 26, 0, 0, "",
                                  Property.Color.Blue, 12, "MC", 45, 13,
                                  Property.Color.Transparent, Property.Color.Transparent,
                                  Property.Color.White, 0, Property.Rotation);
        } else {
          Faceplate->TextBox_main(true, false, String(Temp->get_Temperature(2)),
                                  X_HTU + 15, Y_HTU + 30, 90, 26, 0, 0, "",
                                  Property.Color.Black, 12, "MC", 45, 13,
                                  Property.Color.Transparent, Property.Color.Transparent,
                                  Property.Color.White, 0, Property.Rotation);
        }
        Faceplate->TextBox_main(true, false, String(Temp->get_Humidity(2)),
                                X_HTU + 15, Y_HTU + 55, 90, 24, 0, 0, "",
                                Property.Color.Black, 9, "MR", 80, 12,
                                Property.Color.Transparent, Property.Color.Transparent,
                                Property.Color.White, 0, Property.Rotation);
      } else {
        if (Temp->get_Temperature(1) >= oven_1->get_Upper_limit()) {
          Faceplate->TextBox_main(true, false, String(Temp->get_Temperature(1)),
                                  X_SHT + 15, Y_SHT + 30, 90, 26, 0, 0, "",
                                  Property.Color.Red, 12, "MC", 45, 13,
                                  Property.Color.Transparent, Property.Color.Transparent,
                                  Property.Color.White, 0, Property.Rotation);
        } else if (Temp->get_Temperature(1) <= oven_1->get_Lower_limit()) {
          Faceplate->TextBox_main(true, false, String(Temp->get_Temperature(1)),
                                  X_SHT + 15, Y_SHT + 30, 90, 26, 0, 0, "",
                                  Property.Color.Blue, 12, "MC", 45, 13,
                                  Property.Color.Transparent, Property.Color.Transparent,
                                  Property.Color.White, 0, Property.Rotation);
        } else {
          Faceplate->TextBox_main(true, false, String(Temp->get_Temperature(1)),
                                  X_SHT + 15, Y_SHT + 30, 90, 26, 0, 0, "",
                                  Property.Color.Black, 12, "MC", 45, 13,
                                  Property.Color.Transparent, Property.Color.Transparent,
                                  Property.Color.White, 0, Property.Rotation);
        }
        Faceplate->TextBox_main(true, false, String(Temp->get_Humidity(1)),
                                X_SHT + 15, Y_SHT + 55, 90, 24, 0, 0, "",
                                Property.Color.Black, 9, "MR", 80, 12,
                                Property.Color.Transparent, Property.Color.Transparent,
                                Property.Color.White, 0, Property.Rotation);

        if (Temp->get_Temperature(2) >= oven_1->get_Upper_limit()) {
          Faceplate->TextBox_main(true, false, String(Temp->get_Temperature(2)),
                                  X_HTU + 15, Y_HTU + 30, 90, 26, 0, 0, "",
                                  Property.Color.Red, 12, "MC", 45, 13,
                                  Property.Color.Transparent, Property.Color.Transparent,
                                  Property.Color.White, 0, Property.Rotation);
        } else if (Temp->get_Temperature(2) <= oven_1->get_Lower_limit()) {
          Faceplate->TextBox_main(true, false, String(Temp->get_Temperature(2)),
                                  X_HTU + 15, Y_HTU + 30, 90, 26, 0, 0, "",
                                  Property.Color.Blue, 12, "MC", 45, 13,
                                  Property.Color.Transparent, Property.Color.Transparent,
                                  Property.Color.White, 0, Property.Rotation);
        } else {
          Faceplate->TextBox_main(true, false, String(Temp->get_Temperature(2)),
                                  X_HTU + 15, Y_HTU + 30, 90, 26, 0, 0, "",
                                  Property.Color.Black, 12, "MC", 45, 13,
                                  Property.Color.Transparent, Property.Color.Transparent,
                                  Property.Color.White, 0, Property.Rotation);
        }
        Faceplate->TextBox_main(true, false, String(Temp->get_Humidity(2)),
                                X_HTU + 15, Y_HTU + 55, 90, 24, 0, 0, "",
                                Property.Color.Black, 9, "MR", 80, 12,
                                Property.Color.Transparent, Property.Color.Transparent,
                                Property.Color.White, 0, Property.Rotation);
      }
    }

    //Back button
    if (Faceplate->Icon_button_advance_main(Property.first_draw, true, BackBlue, BackGreen,
                                            0, 280, 480, 40, 32, 32, 224, 4, 20,
                                            Property.Color.Background, Property.Color.Background, Property.Color.Focused,
                                            Property.Color.Blue, 3, Property.Rotation)) {
      page = Menu;
    }

    return true;
  }

  struct Time_struct {
    bool Page_;
    bool prev_Page_;
    int Which = 0;
    int prev_Which = 0;
    uint8_t ID;
  };
  Time_struct Time_Struct;
  bool fcTime() {
    String String_convertor = "";
    String String_convertor_auxiliary = "";

    if ((Time_Struct.prev_Page_ != Time_Struct.Page_) || (Time_Struct.prev_Which != Time_Struct.Which)) {
      Time_Struct.prev_Page_ = Time_Struct.Page_;
      Time_Struct.prev_Which = Time_Struct.Which;
      tft->fillScreen(tft->color565(Property.Color.Background[0], Property.Color.Background[1], Property.Color.Background[2]));
      Property.first_draw = true;
    }



    if (Time_Struct.Page_) {
      //Draw
      if (true) {
        Message->set_Message("Cas", "Time");
        Faceplate->TextBox_main(Property.first_draw, false, Message->get_Message(),
                                240, 40, 240, 40, 0, 0, "",
                                Property.Color.Text, 12, "MC", 120, 20,
                                Property.Color.Transparent, Property.Color.Transparent,
                                Property.Color.Black, 0, Property.Rotation);

        Faceplate->TextBox_main(Property.first_draw, false, "",
                                230, 80, 245, 40, 10, 10, "TOP",
                                Property.Color.Text, 12, "MC", 115, 20,
                                Property.Color.Transparent, Property.Color.Transparent,
                                Property.Color.Text, 2, Property.Rotation);

        Faceplate->TextBox_main(Property.first_draw, false, "",
                                230, 118, 245, 40, 0, 0, "",
                                Property.Color.Text, 9, "ML", 5, 20,
                                Property.Color.Transparent, Property.Color.Transparent,
                                Property.Color.Text, 2, Property.Rotation);

        Faceplate->TextBox_main(Property.first_draw, false, "",
                                230, 156, 245, 40, 0, 0, "",
                                Property.Color.Text, 9, "ML", 5, 20,
                                Property.Color.Transparent, Property.Color.Transparent,
                                Property.Color.Text, 2, Property.Rotation);

        Faceplate->TextBox_main(Property.first_draw, false, "",
                                230, 194, 245, 40, 0, 0, "",
                                Property.Color.Text, 9, "ML", 5, 20,
                                Property.Color.Transparent, Property.Color.Transparent,
                                Property.Color.Text, 2, Property.Rotation);

        Faceplate->TextBox_main(Property.first_draw, false, "",
                                230, 232, 245, 40, 10, 10, "BOTTOM",
                                Property.Color.Text, 9, "ML", 5, 20,
                                Property.Color.Transparent, Property.Color.Transparent,
                                Property.Color.Text, 2, Property.Rotation);
      }

      int shift_X = 225;
      //Active Intervals
      if (true) {
        if (state->get_Interval_enable(Time_Struct.Which, 0)) {
          if (Faceplate->Button_main(Property.first_draw, true, true, "",
                                     shift_X + 15, 91 - 3, 30, 25, 10,
                                     Property.Color.Green, 18, "MC", 35, 13,
                                     Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                     Property.Color.Green, 15, Property.Rotation)) {
            state->set_Interval_enable(Time_Struct.Which, 0, false);
            if (Faceplate->Button_main(Property.first_draw, true, true, "",
                                       shift_X + 15, 91 - 3, 30, 25, 10,
                                       Property.Color.Red, 18, "MC", 35, 13,
                                       Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                       Property.Color.Red, 3, Property.Rotation)) {}
          }
        } else {
          if (Faceplate->Button_main(Property.first_draw, true, true, "",
                                     shift_X + 15, 91 - 3, 30, 25, 10,
                                     Property.Color.Red, 18, "MC", 35, 13,
                                     Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                     Property.Color.Red, 3, Property.Rotation)) {
            state->set_Interval_enable(Time_Struct.Which, 0, true);
            if (Faceplate->Button_main(Property.first_draw, true, true, "",
                                       shift_X + 15, 91 - 3, 30, 25, 10,
                                       Property.Color.Green, 18, "MC", 35, 13,
                                       Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                       Property.Color.Green, 15, Property.Rotation)) {}
          }
        }

        String_convertor = String(state->get_Interval_heating(Time_Struct.Which, true, 0) / 60, DEC);
        String_convertor = String(String_convertor + ":");
        String_convertor_auxiliary = String(state->get_Interval_heating(Time_Struct.Which, true, 0) % 60, DEC);
        if ((state->get_Interval_heating(Time_Struct.Which, true, 0) % 60) < 10) String_convertor = String(String_convertor + "0");
        String_convertor = String(String_convertor + String_convertor_auxiliary);
        if (Faceplate->Button_main(Property.first_draw, false, true, String_convertor,
                                   shift_X + 50, 82, 70, 36, 0,
                                   Property.Color.Text, 12, "MC", 35, 18,
                                   Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                   Property.Color.Black, 0, Property.Rotation)) {
          Property.Pressing.ID = 26;
        }


        Faceplate->TextBox_main(Property.first_draw, false, "-",
                                shift_X + 130, 82, 10, 36, 0, 0, "",
                                Property.Color.Text, 12, "MC", 5, 18,
                                Property.Color.Black, Property.Color.Black,
                                Property.Color.White, 0, Property.Rotation);

        String_convertor = String(state->get_Interval_heating(Time_Struct.Which, false, 0) / 60, DEC);
        String_convertor = String(String_convertor + ":");
        String_convertor_auxiliary = String(state->get_Interval_heating(Time_Struct.Which, false, 0) % 60, DEC);
        if ((state->get_Interval_heating(Time_Struct.Which, false, 0) % 60) < 10) String_convertor = String(String_convertor + "0");
        String_convertor = String(String_convertor + String_convertor_auxiliary);
        if (Faceplate->Button_main(Property.first_draw, false, true, String_convertor,
                                   shift_X + 150, 82, 70, 36, 0,
                                   Property.Color.Text, 12, "MC", 35, 18,
                                   Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                   Property.Color.Black, 0, Property.Rotation)) {
          Property.Pressing.ID = 26;
        }
        /////
        if (state->get_Interval_enable(Time_Struct.Which, 1)) {
          if (Faceplate->Button_main(Property.first_draw, true, true, "",
                                     shift_X + 15, 129 - 3, 30, 25, 10,
                                     Property.Color.Green, 18, "MC", 35, 13,
                                     Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                     Property.Color.Green, 15, Property.Rotation)) {
            state->set_Interval_enable(Time_Struct.Which, 1, false);
            if (Faceplate->Button_main(Property.first_draw, true, true, "",
                                       shift_X + 15, 129 - 3, 30, 25, 10,
                                       Property.Color.Red, 18, "MC", 35, 13,
                                       Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                       Property.Color.Red, 3, Property.Rotation)) {}
          }
        } else {
          if (Faceplate->Button_main(Property.first_draw, true, true, "",
                                     shift_X + 15, 129 - 3, 30, 25, 10,
                                     Property.Color.Red, 18, "MC", 35, 13,
                                     Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                     Property.Color.Red, 3, Property.Rotation)) {
            state->set_Interval_enable(Time_Struct.Which, 1, true);
            if (Faceplate->Button_main(Property.first_draw, true, true, "",
                                       shift_X + 15, 129 - 3, 30, 25, 10,
                                       Property.Color.Green, 18, "MC", 35, 13,
                                       Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                       Property.Color.Green, 15, Property.Rotation)) {}
          }
        }

        String_convertor = String(state->get_Interval_heating(Time_Struct.Which, true, 1) / 60, DEC);
        String_convertor = String(String_convertor + ":");
        String_convertor_auxiliary = String(state->get_Interval_heating(Time_Struct.Which, true, 1) % 60, DEC);
        if ((state->get_Interval_heating(Time_Struct.Which, true, 1) % 60) < 10) String_convertor = String(String_convertor + "0");
        String_convertor = String(String_convertor + String_convertor_auxiliary);
        if (Faceplate->Button_main(Property.first_draw, false, true, String_convertor,
                                   shift_X + 50, 120, 70, 36, 0,
                                   Property.Color.Text, 12, "MC", 35, 18,
                                   Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                   Property.Color.Black, 0, Property.Rotation)) {
          Property.Pressing.ID = 27;
        }


        Faceplate->TextBox_main(Property.first_draw, false, "-",
                                shift_X + 130, 120, 10, 36, 0, 0, "",
                                Property.Color.Text, 12, "MC", 5, 18,
                                Property.Color.Black, Property.Color.Black,
                                Property.Color.White, 0, Property.Rotation);

        String_convertor = String(state->get_Interval_heating(Time_Struct.Which, false, 1) / 60, DEC);
        String_convertor = String(String_convertor + ":");
        String_convertor_auxiliary = String(state->get_Interval_heating(Time_Struct.Which, false, 1) % 60, DEC);
        if ((state->get_Interval_heating(Time_Struct.Which, false, 1) % 60) < 10) String_convertor = String(String_convertor + "0");
        String_convertor = String(String_convertor + String_convertor_auxiliary);
        if (Faceplate->Button_main(Property.first_draw, false, true, String_convertor,
                                   shift_X + 150, 120, 70, 36, 0,
                                   Property.Color.Text, 12, "MC", 35, 18,
                                   Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                   Property.Color.Black, 0, Property.Rotation)) {
          Property.Pressing.ID = 27;
        }
        /////
        if (state->get_Interval_enable(Time_Struct.Which, 2)) {
          if (Faceplate->Button_main(Property.first_draw, true, true, "",
                                     shift_X + 15, 167 - 3, 30, 25, 10,
                                     Property.Color.Green, 18, "MC", 35, 13,
                                     Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                     Property.Color.Green, 15, Property.Rotation)) {
            state->set_Interval_enable(Time_Struct.Which, 2, false);
            if (Faceplate->Button_main(Property.first_draw, true, true, "",
                                       shift_X + 15, 167 - 3, 30, 25, 10,
                                       Property.Color.Red, 18, "MC", 35, 13,
                                       Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                       Property.Color.Red, 3, Property.Rotation)) {}
          }
        } else {
          if (Faceplate->Button_main(Property.first_draw, true, true, "",
                                     shift_X + 15, 167 - 3, 30, 25, 10,
                                     Property.Color.Red, 18, "MC", 35, 13,
                                     Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                     Property.Color.Red, 3, Property.Rotation)) {
            state->set_Interval_enable(Time_Struct.Which, 2, true);
            if (Faceplate->Button_main(Property.first_draw, true, true, "",
                                       shift_X + 15, 167 - 3, 30, 25, 10,
                                       Property.Color.Green, 18, "MC", 35, 13,
                                       Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                       Property.Color.Green, 15, Property.Rotation)) {}
          }
        }

        String_convertor = String(state->get_Interval_heating(Time_Struct.Which, true, 2) / 60, DEC);
        String_convertor = String(String_convertor + ":");
        String_convertor_auxiliary = String(state->get_Interval_heating(Time_Struct.Which, true, 2) % 60, DEC);
        if ((state->get_Interval_heating(Time_Struct.Which, true, 2) % 60) < 10) String_convertor = String(String_convertor + "0");
        String_convertor = String(String_convertor + String_convertor_auxiliary);
        if (Faceplate->Button_main(Property.first_draw, false, true, String_convertor,
                                   shift_X + 50, 158, 70, 36, 0,
                                   Property.Color.Text, 12, "MC", 35, 18,
                                   Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                   Property.Color.Black, 0, Property.Rotation)) {
          Property.Pressing.ID = 28;
        }


        Faceplate->TextBox_main(Property.first_draw, false, "-",
                                shift_X + 130, 158, 10, 36, 0, 0, "",
                                Property.Color.Text, 12, "MC", 5, 18,
                                Property.Color.Black, Property.Color.Black,
                                Property.Color.White, 0, Property.Rotation);

        String_convertor = String(state->get_Interval_heating(Time_Struct.Which, false, 2) / 60, DEC);
        String_convertor = String(String_convertor + ":");
        String_convertor_auxiliary = String(state->get_Interval_heating(Time_Struct.Which, false, 2) % 60, DEC);
        if ((state->get_Interval_heating(Time_Struct.Which, false, 2) % 60) < 10) String_convertor = String(String_convertor + "0");
        String_convertor = String(String_convertor + String_convertor_auxiliary);
        if (Faceplate->Button_main(Property.first_draw, false, true, String_convertor,
                                   shift_X + 150, 158, 70, 36, 0,
                                   Property.Color.Text, 12, "MC", 35, 18,
                                   Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                   Property.Color.Black, 0, Property.Rotation)) {
          Property.Pressing.ID = 28;
        }

        /////
        if (state->get_Interval_enable(Time_Struct.Which, 3)) {
          if (Faceplate->Button_main(Property.first_draw, true, true, "",
                                     shift_X + 15, 205 - 3, 30, 25, 10,
                                     Property.Color.Green, 18, "MC", 35, 13,
                                     Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                     Property.Color.Green, 15, Property.Rotation)) {
            state->set_Interval_enable(Time_Struct.Which, 3, false);
            if (Faceplate->Button_main(Property.first_draw, true, true, "",
                                       shift_X + 15, 205 - 3, 30, 25, 10,
                                       Property.Color.Red, 18, "MC", 35, 13,
                                       Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                       Property.Color.Red, 3, Property.Rotation)) {}
          }
        } else {
          if (Faceplate->Button_main(Property.first_draw, true, true, "",
                                     shift_X + 15, 205 - 3, 30, 25, 10,
                                     Property.Color.Red, 18, "MC", 35, 13,
                                     Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                     Property.Color.Red, 3, Property.Rotation)) {
            state->set_Interval_enable(Time_Struct.Which, 3, true);
            if (Faceplate->Button_main(Property.first_draw, true, true, "",
                                       shift_X + 15, 205 - 3, 30, 25, 10,
                                       Property.Color.Green, 18, "MC", 35, 13,
                                       Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                       Property.Color.Green, 15, Property.Rotation)) {}
          }
        }

        String_convertor = String(state->get_Interval_heating(Time_Struct.Which, true, 3) / 60, DEC);
        String_convertor = String(String_convertor + ":");
        String_convertor_auxiliary = String(state->get_Interval_heating(Time_Struct.Which, true, 3) % 60, DEC);
        if ((state->get_Interval_heating(Time_Struct.Which, true, 3) % 60) < 10) String_convertor = String(String_convertor + "0");
        String_convertor = String(String_convertor + String_convertor_auxiliary);
        if (Faceplate->Button_main(Property.first_draw, false, true, String_convertor,
                                   shift_X + 50, 196, 70, 36, 0,
                                   Property.Color.Text, 12, "MC", 35, 18,
                                   Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                   Property.Color.Black, 0, Property.Rotation)) {
          Property.Pressing.ID = 29;
        }


        Faceplate->TextBox_main(Property.first_draw, false, "-",
                                shift_X + 130, 196, 10, 36, 0, 0, "",
                                Property.Color.Text, 12, "MC", 5, 18,
                                Property.Color.Black, Property.Color.Black,
                                Property.Color.White, 0, Property.Rotation);

        String_convertor = String(state->get_Interval_heating(Time_Struct.Which, false, 3) / 60, DEC);
        String_convertor = String(String_convertor + ":");
        String_convertor_auxiliary = String(state->get_Interval_heating(Time_Struct.Which, false, 3) % 60, DEC);
        if ((state->get_Interval_heating(Time_Struct.Which, false, 3) % 60) < 10) String_convertor = String(String_convertor + "0");
        String_convertor = String(String_convertor + String_convertor_auxiliary);
        if (Faceplate->Button_main(Property.first_draw, false, true, String_convertor,
                                   shift_X + 150, 196, 70, 36, 0,
                                   Property.Color.Text, 12, "MC", 35, 18,
                                   Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                   Property.Color.Black, 0, Property.Rotation)) {
          Property.Pressing.ID = 29;
        }

        /////
        if (state->get_Interval_enable(Time_Struct.Which, 4)) {
          if (Faceplate->Button_main(Property.first_draw, true, true, "",
                                     shift_X + 15, 243 - 3, 30, 25, 10,
                                     Property.Color.Green, 18, "MC", 35, 13,
                                     Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                     Property.Color.Green, 15, Property.Rotation)) {
            state->set_Interval_enable(Time_Struct.Which, 4, false);
            if (Faceplate->Button_main(Property.first_draw, true, true, "",
                                       shift_X + 15, 243 - 3, 30, 25, 10,
                                       Property.Color.Red, 18, "MC", 35, 13,
                                       Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                       Property.Color.Red, 3, Property.Rotation)) {}
          }
        } else {
          if (Faceplate->Button_main(Property.first_draw, true, true, "",
                                     shift_X + 15, 243 - 3, 30, 25, 10,
                                     Property.Color.Red, 18, "MC", 35, 13,
                                     Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                     Property.Color.Red, 3, Property.Rotation)) {
            state->set_Interval_enable(Time_Struct.Which, 4, true);
            if (Faceplate->Button_main(Property.first_draw, true, true, "",
                                       shift_X + 15, 243 - 3, 30, 25, 10,
                                       Property.Color.Green, 18, "MC", 35, 13,
                                       Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                       Property.Color.Green, 15, Property.Rotation)) {}
          }
        }

        String_convertor = String(state->get_Interval_heating(Time_Struct.Which, true, 4) / 60, DEC);
        String_convertor = String(String_convertor + ":");
        String_convertor_auxiliary = String(state->get_Interval_heating(Time_Struct.Which, true, 4) % 60, DEC);
        if ((state->get_Interval_heating(Time_Struct.Which, true, 4) % 60) < 10) String_convertor = String(String_convertor + "0");
        String_convertor = String(String_convertor + String_convertor_auxiliary);
        if (Faceplate->Button_main(Property.first_draw, false, true, String_convertor,
                                   shift_X + 50, 234, 70, 36, 0,
                                   Property.Color.Text, 12, "MC", 35, 18,
                                   Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                   Property.Color.Black, 0, Property.Rotation)) {
          Property.Pressing.ID = 30;
        }


        Faceplate->TextBox_main(Property.first_draw, false, "-",
                                shift_X + 130, 234, 10, 36, 0, 0, "",
                                Property.Color.Text, 12, "MC", 5, 18,
                                Property.Color.Black, Property.Color.Black,
                                Property.Color.White, 0, Property.Rotation);

        String_convertor = String(state->get_Interval_heating(Time_Struct.Which, false, 4) / 60, DEC);
        String_convertor = String(String_convertor + ":");
        String_convertor_auxiliary = String(state->get_Interval_heating(Time_Struct.Which, false, 4) % 60, DEC);
        if ((state->get_Interval_heating(Time_Struct.Which, false, 4) % 60) < 10) String_convertor = String(String_convertor + "0");
        String_convertor = String(String_convertor + String_convertor_auxiliary);
        if (Faceplate->Button_main(Property.first_draw, false, true, String_convertor,
                                   shift_X + 150, 234, 70, 36, 0,
                                   Property.Color.Text, 12, "MC", 35, 18,
                                   Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                   Property.Color.Black, 0, Property.Rotation)) {
          Property.Pressing.ID = 30;
        }
      }

      shift_X = 10;
      int shift_Y = 45;
      //Active Days
      if (true) {
        for (int i = 0; i < 7; i++) {
          if (i == 0) Message->set_Message("Po", "Mon");
          else if (i == 1) Message->set_Message("Ut", "Tue");
          else if (i == 2) Message->set_Message("Ste", "Wed");
          else if (i == 3) Message->set_Message("Stv", "Thu");
          else if (i == 4) Message->set_Message("Pia", "Fri");
          else if (i == 5) {
            Message->set_Message("So", "Sat");
            shift_X = 10;
            shift_Y += 30;
          } else if (i == 6) Message->set_Message("Ne", "Sun");


          if (state->get_Interval_Day(Time_Struct.Which, i)) {
            if (Faceplate->Button_main(Property.first_draw, true, true, Message->get_Message(),
                                       shift_X, shift_Y, 50, 20, 0,
                                       Property.Color.Green, 12, "MC", 25, 10,
                                       Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                       Property.Color.Background, 0, Property.Rotation)) {

              state->set_Interval_Days(Time_Struct.Which, i, false);
              if (Faceplate->Button_main(Property.first_draw, true, true, Message->get_Message(),
                                         shift_X, shift_Y, 50, 20, 0,
                                         Property.Color.White, 12, "MC", 25, 10,
                                         Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                         Property.Color.Background, 0, Property.Rotation)) {}
            }
          } else {
            if (Faceplate->Button_main(Property.first_draw, true, true, Message->get_Message(),
                                       shift_X, shift_Y, 50, 20, 0,
                                       Property.Color.White, 12, "MC", 25, 10,
                                       Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                       Property.Color.Background, 0, Property.Rotation)) {

              state->set_Interval_Days(Time_Struct.Which, i, true);
              if (Faceplate->Button_main(Property.first_draw, true, true, Message->get_Message(),
                                         shift_X, shift_Y, 50, 20, 0,
                                         Property.Color.Green, 12, "MC", 25, 10,
                                         Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                         Property.Color.Background, 0, Property.Rotation)) {}
            }
          }
          shift_X += 50;
        }
      }

      //Active Choosing
      if (true) {
        if (Time_Struct.Which == 0) {
          if (Faceplate->Icon_button_main(Property.first_draw, 50, 125, 128, 128, 128, 128, 0, 0,
                                          true, number_1, number_1Green, Property.Color.Background)) {
            Time_Struct.Which = 1;
          }
        } else if (Time_Struct.Which == 1) {
          if (Faceplate->Icon_button_main(Property.first_draw, 50, 125, 128, 128, 128, 128, 0, 0,
                                          true, number_2, number_2Green, Property.Color.Background)) {
            Time_Struct.Which = 0;
          }
        }
      }

      //Back button
    if (Faceplate->Icon_button_advance_main(Property.first_draw, true, BackBlue, BackGreen,
                                            0, 280, 480, 40, 32, 32, 224, 4, 20,
                                            Property.Color.Background, Property.Color.Background, Property.Color.Focused,
                                            Property.Color.Blue, 3, Property.Rotation)) {
      page = Menu;
    }
    } else {
      //Draw
      if (true) {
        String_convertor = String(state->get_Interval_heating(Time_Struct.Which, true, Time_Struct.ID) / 60, DEC);
        String_convertor = String(String_convertor + ":");
        String_convertor_auxiliary = String(state->get_Interval_heating(Time_Struct.Which, true, Time_Struct.ID) % 60, DEC);
        if ((state->get_Interval_heating(Time_Struct.Which, true, Time_Struct.ID) % 60) < 10) String_convertor = String(String_convertor + "0");
        String_convertor = String(String_convertor + String_convertor_auxiliary);
        Faceplate->TextBox_main(Property.first_draw || Touch.Releast, false, String_convertor,
                                20, 124, 200, 60, 0, 0, "",
                                Property.Color.Text, 24, "MC", 100, 30,
                                Property.Color.Black, Property.Color.Black,
                                Property.Color.Black, 0, Property.Rotation);


        Faceplate->TextBox_main(Property.first_draw, false, "-",
                                230, 124, 20, 36, 0, 0, "",
                                Property.Color.Text, 24, "MC", 10, 18,
                                Property.Color.Black, Property.Color.Black,
                                Property.Color.White, 0, Property.Rotation);

        String_convertor = String(state->get_Interval_heating(Time_Struct.Which, false, Time_Struct.ID) / 60, DEC);
        String_convertor = String(String_convertor + ":");
        String_convertor_auxiliary = String(state->get_Interval_heating(Time_Struct.Which, false, Time_Struct.ID) % 60, DEC);
        if ((state->get_Interval_heating(Time_Struct.Which, false, Time_Struct.ID) % 60) < 10) String_convertor = String(String_convertor + "0");
        String_convertor = String(String_convertor + String_convertor_auxiliary);
        Faceplate->TextBox_main(Property.first_draw || Touch.Releast, false, String_convertor,
                                260, 124, 200, 60, 0, 0, "",
                                Property.Color.Text, 24, "MC", 100, 30,
                                Property.Color.Black, Property.Color.Black,
                                Property.Color.Black, 0, Property.Rotation);
      }
      //Active
      if (true) {
        if (Faceplate->Button_main(Property.first_draw, false, true, "+",
                                   10, 50, 107, 60, 5,
                                   Property.Color.Red, 24, "MC", 53, 23,
                                   Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                   Property.Color.Red, 3, Property.Rotation)) {
          Property.Pressing.ID = 31;

          String_convertor = String(state->get_Interval_heating(Time_Struct.Which, true, Time_Struct.ID) / 60, DEC);
          String_convertor = String(String_convertor + ":");
          String_convertor_auxiliary = String(state->get_Interval_heating(Time_Struct.Which, true, Time_Struct.ID) % 60, DEC);
          if ((state->get_Interval_heating(Time_Struct.Which, true, Time_Struct.ID) % 60) < 10) String_convertor = String(String_convertor + "0");
          String_convertor = String(String_convertor + String_convertor_auxiliary);

          Faceplate->TextBox_main(true, false, String_convertor,
                                  20, 124, 200, 60, 0, 0, "",
                                  Property.Color.Text, 24, "MC", 100, 30,
                                  Property.Color.Black, Property.Color.Black,
                                  Property.Color.Black, 0, Property.Rotation);
        }
        if (Faceplate->Button_main(Property.first_draw, false, true, "-",
                                   10, 200, 107, 60, 5,
                                   Property.Color.Red, 24, "MC", 53, 23,
                                   Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                   Property.Color.Red, 3, Property.Rotation)) {
          Property.Pressing.ID = 32;
          String_convertor = String(state->get_Interval_heating(Time_Struct.Which, true, Time_Struct.ID) / 60, DEC);
          String_convertor = String(String_convertor + ":");
          String_convertor_auxiliary = String(state->get_Interval_heating(Time_Struct.Which, true, Time_Struct.ID) % 60, DEC);
          if ((state->get_Interval_heating(Time_Struct.Which, true, Time_Struct.ID) % 60) < 10) String_convertor = String(String_convertor + "0");
          String_convertor = String(String_convertor + String_convertor_auxiliary);
          Faceplate->TextBox_main(true, false, String_convertor,
                                  20, 124, 200, 60, 0, 0, "",
                                  Property.Color.Text, 24, "MC", 100, 30,
                                  Property.Color.Black, Property.Color.Black,
                                  Property.Color.Black, 0, Property.Rotation);
        }
        if (Faceplate->Button_main(Property.first_draw, false, true, "+",
                                   127, 50, 107, 60, 5,
                                   Property.Color.Red, 24, "MC", 53, 23,
                                   Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                   Property.Color.Red, 3, Property.Rotation)) {
          Property.Pressing.ID = 33;

          String_convertor = String(state->get_Interval_heating(Time_Struct.Which, true, Time_Struct.ID) / 60, DEC);
          String_convertor = String(String_convertor + ":");
          String_convertor_auxiliary = String(state->get_Interval_heating(Time_Struct.Which, true, Time_Struct.ID) % 60, DEC);
          if ((state->get_Interval_heating(Time_Struct.Which, true, Time_Struct.ID) % 60) < 10) String_convertor = String(String_convertor + "0");
          String_convertor = String(String_convertor + String_convertor_auxiliary);

          Faceplate->TextBox_main(true, false, String_convertor,
                                  20, 124, 200, 60, 0, 0, "",
                                  Property.Color.Text, 24, "MC", 100, 30,
                                  Property.Color.Black, Property.Color.Black,
                                  Property.Color.Black, 0, Property.Rotation);
        }
        if (Faceplate->Button_main(Property.first_draw, false, true, "-",
                                   127, 200, 107, 60, 5,
                                   Property.Color.Red, 24, "MC", 53, 23,
                                   Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                   Property.Color.Red, 3, Property.Rotation)) {
          Property.Pressing.ID = 34;
          String_convertor = String(state->get_Interval_heating(Time_Struct.Which, true, Time_Struct.ID) / 60, DEC);
          String_convertor = String(String_convertor + ":");
          String_convertor_auxiliary = String(state->get_Interval_heating(Time_Struct.Which, true, Time_Struct.ID) % 60, DEC);
          if ((state->get_Interval_heating(Time_Struct.Which, true, Time_Struct.ID) % 60) < 10) String_convertor = String(String_convertor + "0");
          String_convertor = String(String_convertor + String_convertor_auxiliary);
          Faceplate->TextBox_main(true, false, String_convertor,
                                  20, 124, 200, 60, 0, 0, "",
                                  Property.Color.Text, 24, "MC", 100, 30,
                                  Property.Color.Black, Property.Color.Black,
                                  Property.Color.Black, 0, Property.Rotation);
        }

        ///////////

        if (Faceplate->Button_main(Property.first_draw, false, true, "+",
                                   246, 50, 107, 60, 5,
                                   Property.Color.Red, 24, "MC", 53, 23,
                                   Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                   Property.Color.Red, 3, Property.Rotation)) {
          Property.Pressing.ID = 35;

          String_convertor = String(state->get_Interval_heating(Time_Struct.Which, false, Time_Struct.ID) / 60, DEC);
          String_convertor = String(String_convertor + ":");
          String_convertor_auxiliary = String(state->get_Interval_heating(Time_Struct.Which, false, Time_Struct.ID) % 60, DEC);
          if ((state->get_Interval_heating(Time_Struct.Which, false, Time_Struct.ID) % 60) < 10) String_convertor = String(String_convertor + "0");
          String_convertor = String(String_convertor + String_convertor_auxiliary);
          Faceplate->TextBox_main(true, false, String_convertor,
                                  260, 124, 200, 60, 0, 0, "",
                                  Property.Color.Text, 24, "MC", 100, 30,
                                  Property.Color.Black, Property.Color.Black,
                                  Property.Color.Black, 0, Property.Rotation);
        }

        if (Faceplate->Button_main(Property.first_draw, false, true, "-",
                                   246, 200, 107, 60, 5,
                                   Property.Color.Red, 24, "MC", 53, 23,
                                   Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                   Property.Color.Red, 3, Property.Rotation)) {
          Property.Pressing.ID = 36;

          String_convertor = String(state->get_Interval_heating(Time_Struct.Which, false, Time_Struct.ID) / 60, DEC);
          String_convertor = String(String_convertor + ":");
          String_convertor_auxiliary = String(state->get_Interval_heating(Time_Struct.Which, false, Time_Struct.ID) % 60, DEC);
          if ((state->get_Interval_heating(Time_Struct.Which, false, Time_Struct.ID) % 60) < 10) String_convertor = String(String_convertor + "0");
          String_convertor = String(String_convertor + String_convertor_auxiliary);
          Faceplate->TextBox_main(true, false, String_convertor,
                                  260, 124, 200, 60, 0, 0, "",
                                  Property.Color.Text, 24, "MC", 100, 30,
                                  Property.Color.Black, Property.Color.Black,
                                  Property.Color.Black, 0, Property.Rotation);
        }

        if (Faceplate->Button_main(Property.first_draw, false, true, "+",
                                   363, 50, 107, 60, 5,
                                   Property.Color.Red, 24, "MC", 53, 23,
                                   Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                   Property.Color.Red, 3, Property.Rotation)) {
          Property.Pressing.ID = 37;

          String_convertor = String(state->get_Interval_heating(Time_Struct.Which, false, Time_Struct.ID) / 60, DEC);
          String_convertor = String(String_convertor + ":");
          String_convertor_auxiliary = String(state->get_Interval_heating(Time_Struct.Which, false, Time_Struct.ID) % 60, DEC);
          if ((state->get_Interval_heating(Time_Struct.Which, false, Time_Struct.ID) % 60) < 10) String_convertor = String(String_convertor + "0");
          String_convertor = String(String_convertor + String_convertor_auxiliary);
          Faceplate->TextBox_main(true, false, String_convertor,
                                  260, 124, 200, 60, 0, 0, "",
                                  Property.Color.Text, 24, "MC", 100, 30,
                                  Property.Color.Black, Property.Color.Black,
                                  Property.Color.Black, 0, Property.Rotation);
        }
        if (Faceplate->Button_main(Property.first_draw, false, true, "-",
                                   363, 200, 107, 60, 5,
                                   Property.Color.Red, 24, "MC", 53, 23,
                                   Property.Color.Background, Property.Color.Background, Property.Color.Yellow,
                                   Property.Color.Red, 3, Property.Rotation)) {
          Property.Pressing.ID = 38;

          String_convertor = String(state->get_Interval_heating(Time_Struct.Which, false, Time_Struct.ID) / 60, DEC);
          String_convertor = String(String_convertor + ":");
          String_convertor_auxiliary = String(state->get_Interval_heating(Time_Struct.Which, false, Time_Struct.ID) % 60, DEC);
          if ((state->get_Interval_heating(Time_Struct.Which, false, Time_Struct.ID) % 60) < 10) String_convertor = String(String_convertor + "0");
          String_convertor = String(String_convertor + String_convertor_auxiliary);
          Faceplate->TextBox_main(true, false, String_convertor,
                                  260, 124, 200, 60, 0, 0, "",
                                  Property.Color.Text, 24, "MC", 100, 30,
                                  Property.Color.Black, Property.Color.Black,
                                  Property.Color.Black, 0, Property.Rotation);
        }



        //Back button
        Message->set_Message("Ulozit", "Save");
        if (Faceplate->Button_main(Property.first_draw, true, true, Message->get_Message(),
                                   0, 280, 480, 40, 20,
                                   Property.Color.Blue, 12, "MC", 240, 20,
                                   Property.Color.Background, Property.Color.Background, Property.Color.Focused,
                                   Property.Color.Blue, 3, Property.Rotation)) {
          Time_Struct.Page_ = true;
        }
      }
    }


    return true;
  }

  struct ERROR_page_struct {
    int number_of_errors;
    int last_number_of_errors;
  };
  ERROR_page_struct ERROR_page;
  void fcERROR_page() {
    if (ERROR_page.number_of_errors != ERROR_page.last_number_of_errors) {
      ERROR_page.last_number_of_errors = ERROR_page.number_of_errors;
      Property.first_draw = true;
    }

    Faceplate->TextBox_main(Property.first_draw, false, "ERROR",
                            50, 50, 380, 250, 0, 0, "",
                            Property.Color.Red, 18, "TC", 190, 10,
                            Property.Color.Yellow, Property.Color.Red,
                            Property.Color.Red, 5, Property.Rotation);
    int Y_axis = 100;
    ERROR_page.number_of_errors = 0;

    if (Alarm.activated) {
      Serial.println("HMI -> HMI ERROR");
      Message->set_Message("HMI", "HMI");
      Faceplate->TextBox_main(Property.first_draw, false, Message->get_Message(),
                              52, Y_axis, 376, 40, 0, 0, "",
                              Property.Color.Black, 12, "ML", 10, 20,
                              Property.Color.Yellow, Property.Color.Yellow,
                              Property.Color.Red, 3, Property.Rotation);

      Y_axis = Y_axis + 37;
      ERROR_page.number_of_errors++;
    }

    if (esp_now->get_Alarm_activated()) {
      Serial.println("HMI -> ESPNOW ERROR");
      Message->set_Message("KOMUNIKACIA", "COMUNICATION");
      Faceplate->TextBox_main(Property.first_draw, false, Message->get_Message(),
                              52, Y_axis, 376, 40, 0, 0, "",
                              Property.Color.Black, 12, "ML", 10, 20,
                              Property.Color.Yellow, Property.Color.Yellow,
                              Property.Color.Red, 3, Property.Rotation);

      Y_axis = Y_axis + 37;
      ERROR_page.number_of_errors++;
    }
    if (Temp->get_Alarm_activated()) {
      Serial.println("HMI -> TEPLOMER ERROR");
      Message->set_Message("TEPLOTNY SENZOR", "TEMPERATURE SENSOR");
      Faceplate->TextBox_main(Property.first_draw, false, Message->get_Message(),
                              52, Y_axis, 376, 40, 0, 0, "",
                              Property.Color.Black, 12, "ML", 10, 20,
                              Property.Color.Yellow, Property.Color.Yellow,
                              Property.Color.Red, 3, Property.Rotation);

      ERROR_page.number_of_errors++;
    }

    Message->set_Message("Skontrolovat", "Check");
    if (Faceplate->Button_main(Property.first_draw, false, true, Message->get_Message(),
                               50, 250, 380, 50, 0,
                               Property.Color.Red, 18, "MC", 190, 25,
                               Property.Color.White, Property.Color.Yellow, Property.Color.Green,
                               Property.Color.Red, 5, Property.Rotation)) {

      if (esp_now->get_Alarm_activated()) {
        Serial.println("HMI -> ESPNOW -> RESET ISSUE : PREPARE");
        esp_now->Sending(false, 0);
        esp_now->Nominal_run();
        Serial.println("HMI -> ESPNOW -> RESET ISSUE : DONE");
      }
      if (Temp->get_Alarm_activated()) {
        Serial.println("HMI -> Teplomer -> RESET ISSUE : PREPARE");
        Temp->test_Temperature();
        Temp->set_Temperature();
        Serial.println("HMI -> Teplomer -> RESET ISSUE : DONE");
      }
      if (Alarm.activated) {
        Serial.println("HMI -> HMI -> RESET ISSUE");
        page = Main_page;
        prev_page = Menu;
        Property.first_draw = true;
      }
    }
    if (ERROR_page.number_of_errors == 0) {
      Serial.println("HMI -> ERRORS = 0, SET page to main");
      page = Main_page;
      prev_page = Menu;
    }
  }
};
