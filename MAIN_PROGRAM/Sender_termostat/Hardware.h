class Hardware
{
private:
  HMI* hmi_hardware;
  Oven* oven_1_hardware_hardware;
  Oven* oven_2_hardware_hardware;
  Temperature_sensor* Temp;
  Battery* battery;

  TON timer_for_Temperature = TON(60000);
  TON timer_for_Battery = TON(60000);

  struct Pins_struct{
    int qTemperature_sensors_enable;
    int qHmi_Brightness;
    int qHmi_Brightness_enable;
    int iBattery_chrg;
    int iBattery_stdby;
    int iBattery_Capacite;
  };
  Pins_struct Pins;

public:
  Hardware(HMI *hmi_hardware_l, Oven *oven_1_hardware_hardware_l, Oven* oven_2_hardware_hardware_l, Temperature_sensor* Temp_l, Battery* battery){
    hmi_hardware = hmi_hardware_l;
    oven_1_hardware_hardware = oven_1_hardware_hardware_l;
    oven_2_hardware_hardware = oven_2_hardware_hardware_l;
    Temp = Temp_l;
    this->battery = battery;

    ////////////////////
    //      Tags      //
    ////////////////////
    Pins.qTemperature_sensors_enable = 26;
    Pins.qHmi_Brightness = 15;
    Pins.qHmi_Brightness_enable = 4;
    Pins.iBattery_chrg = 34;
    Pins.iBattery_stdby = 35;
    Pins.iBattery_Capacite = 14;
  }
  void Setup(bool value){
    //this is set pwm signal or disable pwm signal
    if(value) hmi_hardware->Setup(Pins.qHmi_Brightness_enable, Pins.qHmi_Brightness, value);
    else hmi_hardware->Setup(Pins.qHmi_Brightness, Pins.qHmi_Brightness_enable, value);

    Temp->Setup(Pins.qTemperature_sensors_enable);

    battery->Setup(Pins.iBattery_Capacite, Pins.iBattery_chrg, Pins.iBattery_stdby);

    oven_1_hardware_hardware->Setup();
    oven_2_hardware_hardware->Setup();

    delay(100);
    
    Temp->set_Temperature();
    battery->set_Battery();
  }
  
public:
  void Inputs(){
    hmi_hardware->set_touch();
    
    timer_for_Temperature.set_Time_LOOP_constant(Temp->get_Update_time());
    if(timer_for_Temperature.Timer(true)) Temp->set_Temperature();

    timer_for_Battery.set_Time_LOOP_constant(battery->get_Update_time());
    if(timer_for_Battery.Timer(true)) battery->set_Battery();

  }
  void Outputs(){
  }
};
