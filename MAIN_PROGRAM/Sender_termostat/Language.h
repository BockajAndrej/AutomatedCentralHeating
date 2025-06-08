class Language{
private:
  String State_EN, State_SK;
  enum language_option{
    English = 0, 
    Slovak = 1
  };
  language_option language_option;
public:
  Language(){
    language_option = Slovak;
    State_EN = "Nothing";
    State_SK = "Nic";
  }
  void Set_language(String i){
    if(i == "EN") language_option = English;
    else if(i == "SK") language_option = Slovak;
  }
  void set_Message(String State_sk, String State_en){
    State_EN = State_en;
    State_SK = State_sk;
  }
  String get_language(){
    if(language_option == English) return "EN";
    else if(language_option == Slovak) return "SK";
    else return "";
  }
  String get_Message(){
    if(language_option == English) return State_EN;
    else return State_SK;
  }
};
