class TON{
private:
  unsigned long Time_LOOP_constant;
  unsigned long Time_LOOP;
  bool enable;    //aby hnem po inicializacii nedalo return true pri prvom volani funkcie s parametrom true

public:
  TON(unsigned long i){
    Time_LOOP_constant = i;
    Time_LOOP = 0;
    enable = false;
  }
  bool Timer(bool state){
    if(state && enable == true){
      if(millis() >= Time_LOOP){
        Time_LOOP = millis() + Time_LOOP_constant;
        return true;
      }
    }
    else {
      Time_LOOP = millis() + Time_LOOP_constant;
    }
    enable = true; 
    return false;
  }
  void set_Time_LOOP_constant(unsigned long value){
    Time_LOOP_constant = value;
  }
  unsigned long get_Time_LOOP_constant(){
    return Time_LOOP_constant;
  }
};
