class Hardware
{
private:
  HMI* hmi_hardware;
  Oven* oven_1;
  Oven* oven_2;
  Ultrasonic* ultrasonic_hardware;
  Cooler* cooler_hardware;
  Time* real_time;
  ButtonAB* on_off_Btn;

  TON timer_for_cooler = TON(60000);
  TON timer_for_ultrasonic = TON(5000);

  struct Pins_struct{
    int qRele_1, qRele_2;
    int iTemperature;
    int qCooler;
    int qUltrasonic, iUltrasonic;
    int qBrightness;
    int iButton;
  };
  Pins_struct Pins;

public:
  Hardware(HMI *hmi_hardware_l, Oven *oven_1_l, Oven* oven_2_l, Ultrasonic* ultrasonic_hardware_l, Cooler* cooler_hardware_l, Time* real_time, ButtonAB* on_off_Btn){
    hmi_hardware = hmi_hardware_l;
    oven_1 = oven_1_l;
    oven_2 = oven_2_l;
    ultrasonic_hardware = ultrasonic_hardware_l;
    cooler_hardware = cooler_hardware_l;
    this->real_time = real_time;
    this->on_off_Btn = on_off_Btn;

    ////////////////////
    //      Tags      //
    ////////////////////
    Pins.qRele_1 = 4; //14;    
    Pins.qRele_2 = 12; //35;
    Pins.iTemperature = 15;
    Pins.qCooler = 27;
    Pins.qUltrasonic = 26;
    Pins.iUltrasonic = 5;
    Pins.qBrightness = 25;
    Pins.iButton = 34;
  }
  void Setup(){
    real_time->Setup();
    Serial.println("Real_time: Setup");
    hmi_hardware->Setup(Pins.qBrightness);
    Serial.println("Real_time: Setup");
    oven_1->Setup(Pins.qRele_1);
    oven_2->Setup(Pins.qRele_2);
    Serial.println("Oven: Setup");
    cooler_hardware->Setup(Pins.iTemperature, Pins.qCooler);
    Serial.println("Cooler: Setup");
    ultrasonic_hardware->Setup(Pins.qUltrasonic, Pins.iUltrasonic);
    Serial.println("Ultrasonic: Setup");
    on_off_Btn->Setup(Pins.iButton);
    Serial.println("Button: Setup");
    
    delay(50);

    cooler_hardware->set_Temperature();
    ultrasonic_hardware->set_Distance();
  }
  
public:
  void Inputs(){
    hmi_hardware->set_touch();
    on_off_Btn->set_State();
    
    timer_for_cooler.set_Time_LOOP_constant(cooler_hardware->get_Update_time());
    if(timer_for_cooler.Timer(true)) cooler_hardware->set_Temperature();
    
    timer_for_ultrasonic.set_Time_LOOP_constant(ultrasonic_hardware->get_Update_time());
    if(timer_for_ultrasonic.Timer(true)) ultrasonic_hardware->set_Distance();
  }
  void Outputs(){
    oven_1->Initialization();
    oven_2->Initialization();
    cooler_hardware->Initialization();
  }
};
