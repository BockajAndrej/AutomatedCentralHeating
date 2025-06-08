class ButtonAB {
private:
  bool State;
  bool Current_State;
  bool Prev_State;
  bool Mode;
  int Pin;

public:
  ButtonAB(bool ModeL) {
    Mode = ModeL;
    State = false;
    Current_State = false;
    Prev_State = false;
    Pin = 0;
  }
  void Setup(int PinL) {
    Pin = PinL;
    pinMode(Pin, INPUT);
  }
  void set_State() {
    if (Mode) {
      State = digitalRead(Pin);
    } else {
      Current_State = digitalRead(Pin);
      if (Prev_State != Current_State) {
        Prev_State = Current_State;
        if (State) State = false;
        else State = true;
      }
    }
  }

  bool get_State() {
    return State;
  }
};