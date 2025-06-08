#include <EEPROM.h>

class Flash{
  private:
    int EEPROM_SIZE;

  public:
    Flash(int EEPROM_SIZE){
      this->EEPROM_SIZE = EEPROM_SIZE;
    }
    void Setup(){
      EEPROM.begin(EEPROM_SIZE);
    }
    
    void set_Flash(int number, int value){
      if(value > 255);
      else{
        EEPROM.write(number, value);
        EEPROM.commit();
      }
    }

    int get_Flash(int number){
      return EEPROM.read(number);
    }
};
