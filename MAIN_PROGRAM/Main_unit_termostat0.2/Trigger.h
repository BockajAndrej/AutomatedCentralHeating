class Trigger{
private:
  bool Mode;  //false = falling
  bool prev_state;
public:
  Trigger(int value){
    Mode = value;
    prev_state = false;
  }
  bool Check(bool value){
    if(Mode){
      if((value != prev_state) && value){
        prev_state = value;
        return true;
      }
    }
    else{
      if((value != prev_state) && (value == false)){
        prev_state = value;
        return true;
      }
    }
    return false;
  }
};