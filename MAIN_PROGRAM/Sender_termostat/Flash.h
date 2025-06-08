#include <EEPROM.h>

class Flash{
  private:
    int EEPROM_SIZE;

    struct alarm{
      bool activated;
      Language* message = new Language();
    };
    alarm Alarm;
  
  public:
    Flash(int EEPROM_SIZE){
      this->EEPROM_SIZE = EEPROM_SIZE;
    }
    void Setup(){
      EEPROM.begin(EEPROM_SIZE);
    }
    
    void set_Flash(uint16_t number, int value){
      if((value > 255) && (value < 0)) Alarm.activated = true;
      else{
        Serial.print("IN FLASH = uloz sa");
        Alarm.activated = false;
        EEPROM.write(number, value);
        EEPROM.commit();
      }
    }

    int get_Flash(int number){
      int value = EEPROM.read(number);
      return value;
    }
};
