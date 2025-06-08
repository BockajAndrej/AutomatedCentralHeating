//#include "Free_Fonts.h"

#define FSSB9 &FreeSansBold9pt7b
#define FSSB12 &FreeSansBold12pt7b
#define FSSB18 &FreeSansBold18pt7b
#define FSSB24 &FreeSansBold24pt7b

class Faceplate_Class {
protected:
  TFT_eSPI *tft;
  TFT_eSprite *img;
  TFT_eSprite *img_icon;
  TFT_eSprite *img_background;

  bool Button_draw_state;

  struct Touch_struct {
    uint16_t x;
    uint16_t y;
    bool Pressed;
    bool prev_Pressed;
    bool Main_Pressed;
    bool Releast;
    bool Right_datum;
    bool prev_Right_datum;
  };
  Touch_struct Touch;

public:
  Faceplate_Class(TFT_eSPI *tft_l, TFT_eSprite *img_l, TFT_eSprite *img_icon_l, TFT_eSprite *img_background_l) {
    tft = tft_l;
    img = img_l;
    img_icon = img_icon_l;
    img_background = img_background_l;

    Button_draw_state = false;

    Touch.x = 0;
    Touch.y = 0;
    Touch.Pressed = false;
    Touch.prev_Pressed = false;
    Touch.Main_Pressed = false;
    Touch.Releast = false;
    Touch.Right_datum = false;
    Touch.prev_Right_datum = false;

    Scrools_textBox.WaitTime_LOOP_for_scrooling = 0;
    Scrools_textBox.x1 = 0;
    Scrools_textBox.x2 = 0;
    Scrools_textBox.Enable = true;
  }
  void Set_datum(uint16_t &t_x, uint16_t &t_y, bool Pressed, bool Releast) {
    Touch.x = t_x;
    Touch.y = t_y;
    Touch.Pressed = Pressed;
    Touch.Releast = Releast;
  }

  bool Button_main(bool First_run, bool PressedOrReleast, bool Enable, String msg,
                   int x, int y, int Width, int Height, int Rad,
                   int Text_color[], int Text_font, String text_datum, int x_text, int y_text,
                   int sprite_Background[], int default_Background[], int Focused_color[],
                   int Frame_color[], short Frame_size, short Rotation) {
    bool State = false;

    Touch.Right_datum = (Touch.x > x && (Touch.x < (Width + x)) && Touch.y > y && (Touch.y < (Height + y)));

    if ((Touch.Right_datum && Touch.Pressed) || (Touch.Right_datum && Touch.Releast) || Button_draw_state || First_run) {
      //if((Touch.Right_datum && Touch.Pressed) || (Touch.Right_datum && Touch.Releast) || (Touch.prev_Right_datum && (Touch.prev_Right_datum == false)) || Button_draw_state || First_run){

      Touch.prev_Right_datum = Touch.Right_datum;

      if (Touch.Pressed || Touch.Releast) Button_draw_state = true;
      else Button_draw_state = false;

      img->createSprite(Width, Height);
      img->setRotation(Rotation);

      img->fillScreen(img->color565(default_Background[0], default_Background[1], default_Background[2]));

      if (Enable) {
        if (Touch.Right_datum && Touch.Pressed) {
          img->fillSmoothRoundRect(0, 0, Width, Height, Rad, img->color565(Focused_color[0], Focused_color[1], Focused_color[2]), img->color565(default_Background[0], default_Background[1], default_Background[2]));
          img->fillSmoothRoundRect(0 + Frame_size, 0 + Frame_size, Width - (Frame_size * 2), Height - (Frame_size * 2), Rad, img->color565(sprite_Background[0], sprite_Background[1], sprite_Background[2]), img->color565(Focused_color[0], Focused_color[1], Focused_color[2]));
          img->setTextColor(img->color565(Focused_color[0], Focused_color[1], Focused_color[2]));
          if (PressedOrReleast == false) State = true;
        } else {
          img->fillSmoothRoundRect(0, 0, Width, Height, Rad, img->color565(Frame_color[0], Frame_color[1], Frame_color[2]), img->color565(default_Background[0], default_Background[1], default_Background[2]));
          img->fillSmoothRoundRect(0 + Frame_size, 0 + Frame_size, Width - (Frame_size * 2), Height - (Frame_size * 2), Rad, img->color565(sprite_Background[0], sprite_Background[1], sprite_Background[2]), img->color565(Frame_color[0], Frame_color[1], Frame_color[2]));
          img->setTextColor(img->color565(Text_color[0], Text_color[1], Text_color[2]));
        }
        if ((Touch.Right_datum && Touch.Releast) && (PressedOrReleast == true)) State = true;
      } else {
        img->fillSmoothRoundRect(0, 0, Width, Height, Rad, img->color565(100, 100, 100), img->color565(default_Background[0], default_Background[1], default_Background[2]));
        img->fillSmoothRoundRect(0 + Frame_size, 0 + Frame_size, Width - (Frame_size * 2), Height - (Frame_size * 2), Rad, img->color565(150, 150, 150), img->color565(100, 100, 100));
        img->setTextColor(img->color565(100, 100, 100));
      }

      img->setCursor(x_text, y_text);
      if (Text_font == 9) img->setFreeFont(FSSB9);
      else if (Text_font == 12) img->setFreeFont(FSSB12);
      else if (Text_font == 18) img->setFreeFont(FSSB18);
      else if (Text_font == 24) img->setFreeFont(FSSB24);

      if (text_datum == "TL") img->setTextDatum(TL_DATUM);       //top left
      else if (text_datum == "ML") img->setTextDatum(ML_DATUM);  //middle left
      else if (text_datum == "BL") img->setTextDatum(BL_DATUM);  //bottom left
      else if (text_datum == "TC") img->setTextDatum(TC_DATUM);  //top centre
      else if (text_datum == "MC") img->setTextDatum(MC_DATUM);  //middle centre
      else if (text_datum == "BC") img->setTextDatum(BC_DATUM);  //bottom centre
      else if (text_datum == "TR") img->setTextDatum(TR_DATUM);  //top right
      else if (text_datum == "MR") img->setTextDatum(MR_DATUM);  //middle right
      else if (text_datum == "BR") img->setTextDatum(BR_DATUM);  //bottom right

      img->drawString(msg, x_text, y_text);

      img->pushSprite(x, y, TFT_TRANSPARENT);
      img->deleteSprite();
    }
    return State;
  }

  void TextBox_main(bool First_run, bool disable_only_first_run, String msg,
                    int x, int y, int Width, int Height, int Rad1, int Rad2, String Corner,
                    int Text_color[], int Text_font, String text_datum, int x_text, int y_text,
                    int sprite_Background[], int default_Background[],
                    int Frame_color[], short Frame_size, short Rotation) {

    if (First_run || disable_only_first_run) {
      img->createSprite(Width, Height);
      img->setRotation(Rotation);
      img->setTextColor(img->color565(Text_color[0], Text_color[1], Text_color[2]));


      if (sprite_Background[0] == 999 && Frame_color[0] == 999) img->fillSprite(TFT_TRANSPARENT);
      else {
        //img->fillScreen(img->color565(default_Background[0],default_Background[1], default_Background[2]));
        if (Corner == "TL") {
          img->fillSmoothRoundRect(0, 0, Width, Height, Rad1, img->color565(Frame_color[0], Frame_color[1], Frame_color[2]), img->color565(default_Background[0], default_Background[1], default_Background[2]));
          img->fillRect(Width / 2, 0, Width / 2, Height, img->color565(Frame_color[0], Frame_color[1], Frame_color[2]));
          img->fillRect(0, Height / 2, Width / 2, Height / 2, img->color565(Frame_color[0], Frame_color[1], Frame_color[2]));
          img->fillRect(Width / 2, Frame_size, (Width / 2) - Frame_size, Height / 2, TFT_TRANSPARENT);
          img->fillRect(Frame_size, Height / 2, Width - (Frame_size * 2), (Height / 2) - Frame_size, TFT_TRANSPARENT);
          if (sprite_Background[0] == 999) img->fillSmoothRoundRect(0 + Frame_size, 0 + Frame_size, Width - (Frame_size * 2), Height - (Frame_size * 2), Rad1, TFT_TRANSPARENT, TFT_TRANSPARENT);
          else img->fillSmoothRoundRect(0 + Frame_size, 0 + Frame_size, Width - (Frame_size * 2), Height - (Frame_size * 2), Rad1, img->color565(sprite_Background[0], sprite_Background[1], sprite_Background[2]), img->color565(default_Background[0], default_Background[1], default_Background[2]));
        } else if (Corner == "TR") {
          img->fillSmoothRoundRect(0, 0, Width, Height, Rad1, img->color565(Frame_color[0], Frame_color[1], Frame_color[2]), img->color565(default_Background[0], default_Background[1], default_Background[2]));
          img->fillRect(0, 0, Width / 2, Height, img->color565(Frame_color[0], Frame_color[1], Frame_color[2]));
          img->fillRect(Width / 2, Height / 2, Width / 2, Height / 2, img->color565(Frame_color[0], Frame_color[1], Frame_color[2]));
          img->fillRect(Frame_size, Frame_size, Width / 2, Height / 2, TFT_TRANSPARENT);
          img->fillRect(Frame_size, Height / 2, Width - (Frame_size * 2), (Height / 2) - Frame_size, TFT_TRANSPARENT);
          if (sprite_Background[0] == 999) img->fillSmoothRoundRect(0 + Frame_size, 0 + Frame_size, Width - (Frame_size * 2), Height - (Frame_size * 2), Rad1, TFT_TRANSPARENT, TFT_TRANSPARENT);
          else img->fillSmoothRoundRect(0 + Frame_size, 0 + Frame_size, Width - (Frame_size * 2), Height - (Frame_size * 2), Rad1, img->color565(sprite_Background[0], sprite_Background[1], sprite_Background[2]), img->color565(default_Background[0], default_Background[1], default_Background[2]));
        } else if (Corner == "BL") {
          img->fillSmoothRoundRect(0, 0, Width, Height, Rad1, img->color565(Frame_color[0], Frame_color[1], Frame_color[2]), img->color565(default_Background[0], default_Background[1], default_Background[2]));
          img->fillRect(Width / 2, 0, Width / 2, Height, img->color565(Frame_color[0], Frame_color[1], Frame_color[2]));
          img->fillRect(0, 0, Width / 2, Height / 2, img->color565(Frame_color[0], Frame_color[1], Frame_color[2]));
          img->fillRect(Frame_size, Frame_size, Width - (Frame_size * 2), (Height / 2) - Frame_size, TFT_TRANSPARENT);
          img->fillRect(Width / 2, Height / 2, Width / 2 - Frame_size, (Height / 2) - Frame_size, TFT_TRANSPARENT);
          if (sprite_Background[0] == 999) img->fillSmoothRoundRect(0 + Frame_size, 0 + Frame_size, Width - (Frame_size * 2), Height - (Frame_size * 2), Rad1, TFT_TRANSPARENT, TFT_TRANSPARENT);
          else img->fillSmoothRoundRect(0 + Frame_size, 0 + Frame_size, Width - (Frame_size * 2), Height - (Frame_size * 2), Rad1, img->color565(sprite_Background[0], sprite_Background[1], sprite_Background[2]), img->color565(default_Background[0], default_Background[1], default_Background[2]));
        } else if (Corner == "BR") {
          img->fillSmoothRoundRect(0, 0, Width, Height, Rad1, img->color565(Frame_color[0], Frame_color[1], Frame_color[2]), img->color565(default_Background[0], default_Background[1], default_Background[2]));
          img->fillRect(0, 0, Width, Height / 2, img->color565(Frame_color[0], Frame_color[1], Frame_color[2]));
          img->fillRect(0, Height / 2, Width / 2, Height / 2, img->color565(Frame_color[0], Frame_color[1], Frame_color[2]));
          img->fillRect(Frame_size, Frame_size, Width - (Frame_size * 2), Height / 2, TFT_TRANSPARENT);
          img->fillRect(Frame_size, Height / 2, Width / 2, (Height / 2) - Frame_size, TFT_TRANSPARENT);
          if (sprite_Background[0] == 999) img->fillSmoothRoundRect(0 + Frame_size, 0 + Frame_size, Width - (Frame_size * 2), Height - (Frame_size * 2), Rad1, TFT_TRANSPARENT, TFT_TRANSPARENT);
          else img->fillSmoothRoundRect(0 + Frame_size, 0 + Frame_size, Width - (Frame_size * 2), Height - (Frame_size * 2), Rad1, img->color565(sprite_Background[0], sprite_Background[1], sprite_Background[2]), img->color565(default_Background[0], default_Background[1], default_Background[2]));
        } else if (Corner == "TOP") {
          img->fillSmoothRoundRect(0, 0, Width, Height, Rad1, img->color565(Frame_color[0], Frame_color[1], Frame_color[2]), img->color565(default_Background[0], default_Background[1], default_Background[2]));
          img->fillRect(0, Height / 2, Width, Height / 2, img->color565(Frame_color[0], Frame_color[1], Frame_color[2]));
          img->fillRect(Frame_size, Height / 2, Width - (Frame_size * 2), (Height / 2) - Frame_size, TFT_TRANSPARENT);
          if (sprite_Background[0] == 999) img->fillSmoothRoundRect(0 + Frame_size, 0 + Frame_size, Width - (Frame_size * 2), Height - (Frame_size * 2), Rad1, TFT_TRANSPARENT, TFT_TRANSPARENT);
          else img->fillSmoothRoundRect(0 + Frame_size, 0 + Frame_size, Width - (Frame_size * 2), Height - (Frame_size * 2), Rad1, img->color565(sprite_Background[0], sprite_Background[1], sprite_Background[2]), img->color565(default_Background[0], default_Background[1], default_Background[2]));
        } else if (Corner == "BOTTOM") {
          img->fillSmoothRoundRect(0, 0, Width, Height, Rad1, img->color565(Frame_color[0], Frame_color[1], Frame_color[2]), img->color565(default_Background[0], default_Background[1], default_Background[2]));
          img->fillRect(0, 0, Width, Height / 2, img->color565(Frame_color[0], Frame_color[1], Frame_color[2]));
          img->fillRect(Frame_size, Frame_size, Width - (Frame_size * 2), (Height / 2) - Frame_size, TFT_TRANSPARENT);
          if (sprite_Background[0] == 999) img->fillSmoothRoundRect(0 + Frame_size, 0 + Frame_size, Width - (Frame_size * 2), Height - (Frame_size * 2), Rad1, TFT_TRANSPARENT, TFT_TRANSPARENT);
          else img->fillSmoothRoundRect(0 + Frame_size, 0 + Frame_size, Width - (Frame_size * 2), Height - (Frame_size * 2), Rad1, img->color565(sprite_Background[0], sprite_Background[1], sprite_Background[2]), img->color565(default_Background[0], default_Background[1], default_Background[2]));
        } else {
          img->fillSmoothRoundRect(0, 0, Width, Height, Rad1, img->color565(Frame_color[0], Frame_color[1], Frame_color[2]), img->color565(default_Background[0], default_Background[1], default_Background[2]));
          if (sprite_Background[0] == 999) img->fillSmoothRoundRect(0 + Frame_size, 0 + Frame_size, Width - (Frame_size * 2), Height - (Frame_size * 2), Rad1, TFT_TRANSPARENT, TFT_TRANSPARENT);
          else img->fillSmoothRoundRect(0 + Frame_size, 0 + Frame_size, Width - (Frame_size * 2), Height - (Frame_size * 2), Rad2, img->color565(sprite_Background[0], sprite_Background[1], sprite_Background[2]), img->color565(default_Background[0], default_Background[1], default_Background[2]));
        }
      }

      //img->setCursor(x_text, y_text);
      //if((Text_font != 9) || (Text_font != 12) || (Text_font != 18) || (Text_font != 24)) img->setTextSize(Text_font);
      if (Text_font == 9) img->setFreeFont(FSSB9);
      else if (Text_font == 12) img->setFreeFont(FSSB12);
      else if (Text_font == 18) img->setFreeFont(FSSB18);
      else if (Text_font == 24) img->setFreeFont(FSSB24);

      if (text_datum == "TL") img->setTextDatum(TL_DATUM);       //top left
      else if (text_datum == "ML") img->setTextDatum(ML_DATUM);  //middle left
      else if (text_datum == "BL") img->setTextDatum(BL_DATUM);  //bottom left
      else if (text_datum == "TC") img->setTextDatum(TC_DATUM);  //top centre
      else if (text_datum == "MC") img->setTextDatum(MC_DATUM);  //middle centre
      else if (text_datum == "BC") img->setTextDatum(BC_DATUM);  //bottom centre
      else if (text_datum == "TR") img->setTextDatum(TR_DATUM);  //top right
      else if (text_datum == "MR") img->setTextDatum(MR_DATUM);  //middle right
      else if (text_datum == "BR") img->setTextDatum(BR_DATUM);  //bottom right

      img->drawString(msg, x_text, y_text);

      if (sprite_Background[0] == 999) img->pushSprite(x, y, TFT_TRANSPARENT);
      else img->pushSprite(x, y);
      img->deleteSprite();
    }
  }
  void degrees_Celsius(bool First_run,
                       int x, int y, int Width, int Height,
                       int Text_color[], int Text_font,
                       short Frame_size, short Rotation) {
    if (First_run) {
      img->createSprite(Width, Height);
      img->setRotation(Rotation);
      img->setTextColor(img->color565(Text_color[0], Text_color[1], Text_color[2]));

      img->fillCircle(Frame_size, Frame_size, Frame_size, img->color565(Text_color[0], Text_color[1], Text_color[2]));
      img->fillCircle(Frame_size, Frame_size, Frame_size / 1.5, img->color565(0, 0, 0));

      if (Text_font == 9) img->setFreeFont(FSSB9);
      else if (Text_font == 12) img->setFreeFont(FSSB12);
      else if (Text_font == 18) img->setFreeFont(FSSB18);
      else if (Text_font == 24) img->setFreeFont(FSSB24);

      img->setTextDatum(ML_DATUM);

      img->drawString("C", Frame_size + (Frame_size / 2), Height / 2);

      img->pushSprite(x, y, TFT_TRANSPARENT);
      img->deleteSprite();
    }
  }
  struct Scrools_textBox_struct {
    int x1, x2;
    bool Enable;
    unsigned long WaitTime_LOOP_for_scrooling;
  };
  Scrools_textBox_struct Scrools_textBox;
  void Reset_Scrools_TextBox() {
    Scrools_textBox.Enable = false;
  }
  void Scrools_TextBox_main(String msg, uint8_t Speed, bool Direction, int time_for_wait,
                            int x, int y, int Width, int Height, int Rad, int datum_starts_next,
                            int Text_color[], int Text_font, String text_datum, int x_text, int y_text,
                            int sprite_Background[], int default_Background[],
                            int Frame_color[], short Frame_size, short Rotation) {

    img->createSprite(Width, Height);
    img->setRotation(Rotation);
    img->setTextColor(img->color565(Text_color[0], Text_color[1], Text_color[2]));
    img->fillScreen(img->color565(default_Background[0], default_Background[1], default_Background[2]));
    if (sprite_Background[0] == 999 && Frame_color[0] == 999) img->fillSprite(TFT_TRANSPARENT);
    else {
      img->fillSmoothRoundRect(0, 0, Width, Height, Rad, img->color565(Frame_color[0], Frame_color[1], Frame_color[2]), img->color565(default_Background[0], default_Background[1], default_Background[2]));
      img->fillSprite(img->color565(Frame_color[0], Frame_color[1], Frame_color[2]));
      if (sprite_Background[0] == 999) img->fillRect(0 + Frame_size, 0 + Frame_size, Width - (Frame_size * 2), Height - (Frame_size * 2), TFT_TRANSPARENT);
      else img->fillRect(0 + Frame_size, 0 + Frame_size, Width - (Frame_size * 2), Height - (Frame_size * 2), img->color565(sprite_Background[0], sprite_Background[1], sprite_Background[2]));
    }

    if (Text_font == 9) img->setFreeFont(FSSB9);
    else if (Text_font == 12) img->setFreeFont(FSSB12);
    else if (Text_font == 18) img->setFreeFont(FSSB18);
    else if (Text_font == 24) img->setFreeFont(FSSB24);

    if (text_datum == "TL") img->setTextDatum(TL_DATUM);       //top left
    else if (text_datum == "ML") img->setTextDatum(ML_DATUM);  //middle left
    else if (text_datum == "BL") img->setTextDatum(BL_DATUM);  //bottom left
    else if (text_datum == "TC") img->setTextDatum(TC_DATUM);  //top centre
    else if (text_datum == "MC") img->setTextDatum(MC_DATUM);  //middle centre
    else if (text_datum == "BC") img->setTextDatum(BC_DATUM);  //bottom centre
    else if (text_datum == "TR") img->setTextDatum(TR_DATUM);  //top right
    else if (text_datum == "MR") img->setTextDatum(MR_DATUM);  //middle right
    else if (text_datum == "BR") img->setTextDatum(BR_DATUM);  //bottom right

    if (Scrools_textBox.Enable == false) {
      Scrools_textBox.x1 = x_text;
      Scrools_textBox.x2 = Width + 100;
      if (millis() >= Scrools_textBox.WaitTime_LOOP_for_scrooling) {
        Scrools_textBox.Enable = true;
      }
      img->drawString(msg, x_text, y_text);
    } else {
      if (Direction) {
        Scrools_textBox.x1 = Scrools_textBox.x1 - Speed;
        Scrools_textBox.x2 = Scrools_textBox.x2 - Speed;
      } else {
        Scrools_textBox.x1 = Scrools_textBox.x1 + Speed;
        Scrools_textBox.x2 = Scrools_textBox.x2 + Speed;
      }
      if (Scrools_textBox.x1 == datum_starts_next) Scrools_textBox.x2 = Width;
      if (Scrools_textBox.x2 == datum_starts_next) Scrools_textBox.x1 = Width;

      if (Scrools_textBox.x1 == x_text || Scrools_textBox.x2 == x_text || Scrools_textBox.x1 == x_text + 1 || Scrools_textBox.x2 == x_text + 1) {
        Scrools_textBox.Enable = false;
        Scrools_textBox.WaitTime_LOOP_for_scrooling = millis() + time_for_wait;
      }
      img->drawString(msg, Scrools_textBox.x1, y_text);
      img->drawString(msg, Scrools_textBox.x2, y_text);
    }

    if (sprite_Background[0] == 999) img->pushSprite(x, y, TFT_TRANSPARENT);
    else img->pushSprite(x, y);
    img->deleteSprite();
  }
  int Slider_main(int Value_raw, bool Direction, bool touch,
                  int x, int y, int Width, int Height, int Rad,
                  int Text_color[], int Text_font, String text_datum, int x_text, int y_text,
                  int sprite_Background[], int default_Background[], int Value_color[],
                  int Frame_color[], short Frame_size, short Rotation) {

    Touch.Right_datum = ((Touch.x > x) && (Touch.x < (Width + x)) && (Touch.y > y) && (Touch.y < (Height + y)));

    img->createSprite(Width, Height);
    img->setRotation(Rotation);
    img->setTextColor(img->color565(Text_color[0], Text_color[1], Text_color[2]));
    img->fillScreen(img->color565(default_Background[0], default_Background[1], default_Background[2]));
    if (sprite_Background[0] == 999 && Frame_color[0] == 999) img->fillSprite(TFT_TRANSPARENT);
    else {
      img->fillSmoothRoundRect(0, 0, Width, Height, Rad, img->color565(Frame_color[0], Frame_color[1], Frame_color[2]), img->color565(default_Background[0], default_Background[1], default_Background[2]));
      if (sprite_Background[0] == 999) img->fillSmoothRoundRect(0 + Frame_size, 0 + Frame_size, Width - (Frame_size * 2), Height - (Frame_size * 2), Rad, TFT_TRANSPARENT, TFT_TRANSPARENT);
      else img->fillSmoothRoundRect(0 + Frame_size, 0 + Frame_size, Width - (Frame_size * 2), Height - (Frame_size * 2), Rad, img->color565(sprite_Background[0], sprite_Background[1], sprite_Background[2]), img->color565(Frame_color[0], Frame_color[1], Frame_color[2]));
    }

    double Value = 0.0;

    if (Touch.Right_datum && touch) {
      if (Direction) {
        Value = map((Touch.y - (y + (Frame_size))), Frame_size, Height - (Frame_size * 2), 1, 100);
        Value = (Height * Value) / 100;
        img->fillSmoothRoundRect(0 + Frame_size, (Height - Value) + Frame_size, Width - (Frame_size * 2), Value - (Frame_size * 2), Rad, img->color565(Value_color[0], Value_color[1], Value_color[2]), img->color565(Frame_color[0], Frame_color[1], Frame_color[2]));
      } else {
        Value = map(Touch.x - (x + (Frame_size)), Frame_size, Width - (Frame_size * 2), 1, 100);
        Value = (Width * Value) / 100;
        img->fillSmoothRoundRect(Frame_size, Frame_size, Value - (Frame_size * 2), Height - (Frame_size * 2), Rad, img->color565(Value_color[0], Value_color[1], Value_color[2]), img->color565(Frame_color[0], Frame_color[1], Frame_color[2]));
      }
    } else {
      if (Direction) {
        Value = (Height * Value_raw) / 100;
        img->fillSmoothRoundRect(0 + Frame_size, (Height - Value) + Frame_size, Width - (Frame_size * 2), Value - (Frame_size * 2), Rad, img->color565(Value_color[0], Value_color[1], Value_color[2]), img->color565(Frame_color[0], Frame_color[1], Frame_color[2]));
      } else {
        Value = (Width * Value_raw) / 100;
        img->fillSmoothRoundRect(0 + Frame_size, 0 + Frame_size, Value - (Frame_size * 2), Height - (Frame_size * 2), Rad, img->color565(Value_color[0], Value_color[1], Value_color[2]), img->color565(Frame_color[0], Frame_color[1], Frame_color[2]));
      }
    }

    //img->setCursor(x_text, y_text);
    if (Text_font == 9) img->setFreeFont(FSSB9);
    else if (Text_font == 12) img->setFreeFont(FSSB12);
    else if (Text_font == 18) img->setFreeFont(FSSB18);
    else if (Text_font == 24) img->setFreeFont(FSSB24);

    if (text_datum == "TL") img->setTextDatum(TL_DATUM);       //top left
    else if (text_datum == "ML") img->setTextDatum(ML_DATUM);  //middle left
    else if (text_datum == "BL") img->setTextDatum(BL_DATUM);  //bottom left
    else if (text_datum == "TC") img->setTextDatum(TC_DATUM);  //top centre
    else if (text_datum == "MC") img->setTextDatum(MC_DATUM);  //middle centre
    else if (text_datum == "BC") img->setTextDatum(BC_DATUM);  //bottom centre
    else if (text_datum == "TR") img->setTextDatum(TR_DATUM);  //top right
    else if (text_datum == "MR") img->setTextDatum(MR_DATUM);  //middle right
    else if (text_datum == "BR") img->setTextDatum(BR_DATUM);  //bottom right

    img->drawNumber(Value_raw, x_text, y_text);

    if (sprite_Background[0] == 999) img->pushSprite(x, y, TFT_TRANSPARENT);
    else img->pushSprite(x, y);
    img->deleteSprite();

    if (Touch.Right_datum) return map(Touch.x - (x + (Frame_size)), 0, Width, 0, 100);
    return Value_raw;
  }
  void Icon_main(bool First_run, int x, int y, int Width_BCK, int Height_BCK, int Width, int Height, int x_i, int y_i, const short unsigned int *data, int Background[]) {
    if (First_run) {
      img_background->createSprite(Width_BCK, Height_BCK);
      img_icon->createSprite(Width, Height);

      img_background->fillSprite(img_background->color565(Background[0], Background[1], Background[2]));

      img_icon->pushImage(0, 0, Width, Height, data);
      img_icon->pushToSprite(img_background, x_i, y_i, TFT_BLACK);

      img_background->pushSprite(x, y);

      img_icon->deleteSprite();
      img_background->deleteSprite();
    }
  }
  bool Icon_button_main(bool First_run, int x, int y, int Width_BCK, int Height_BCK, int Width, int Height, int x_i, int y_i,
                        bool PressedOrReleast, const short unsigned int *MAIN_data, const short unsigned int *FOCUSED_data, int Background[]) {
    bool State = false;

    Touch.Right_datum = (Touch.x > x && (Touch.x < (Width + x)) && Touch.y > y && (Touch.y < (Height + y)));

    if ((Touch.Right_datum && Touch.Pressed) || (Touch.Right_datum && Touch.Releast) || Button_draw_state || First_run) {

      Touch.prev_Right_datum = Touch.Right_datum;

      if (Touch.Pressed || Touch.Releast) Button_draw_state = true;
      else Button_draw_state = false;

      img_background->createSprite(Width_BCK, Height_BCK);
      img_icon->createSprite(Width, Height);

      img_background->fillSprite(img_background->color565(Background[0], Background[1], Background[2]));

      if (Touch.Right_datum && Touch.Pressed) {
        img_icon->pushImage(0, 0, Width, Height, FOCUSED_data);
        if (PressedOrReleast == false) State = true;
      } else {
        img_icon->pushImage(0, 0, Width, Height, MAIN_data);
      }
      if ((Touch.Right_datum && Touch.Releast) && (PressedOrReleast == true)) State = true;

      img_icon->pushToSprite(img_background, x_i, y_i, TFT_BLACK);

      img_background->pushSprite(x, y);

      img_icon->deleteSprite();
      img_background->deleteSprite();
    }
    return State;
  }
  bool Icon_button_advance_main(bool First_run, bool PressedOrReleast, const short unsigned int *MAIN_data, const short unsigned int *FOCUSED_data,
                                int x, int y, int Width_BCK, int Height_BCK, int Width, int Height, int x_i, int y_i, int Rad,
                                int sprite_Background[], int default_Background[], int Focused_color[],
                                int Frame_color[], short Frame_size, short Rotation) {
    bool State = false;

    Touch.Right_datum = (Touch.x > x && (Touch.x < (Width_BCK + x)) && Touch.y > y && (Touch.y < (Height_BCK + y)));

    if ((Touch.Right_datum && Touch.Pressed) || (Touch.Right_datum && Touch.Releast) || Button_draw_state || First_run) {

      Touch.prev_Right_datum = Touch.Right_datum;

      if (Touch.Pressed || Touch.Releast) Button_draw_state = true;
      else Button_draw_state = false;

      img->createSprite(Width_BCK, Height_BCK);
      img_icon->createSprite(Width, Height);
      img->setRotation(Rotation);

      img->fillScreen(img->color565(default_Background[0], default_Background[1], default_Background[2]));

      if (Touch.Right_datum && Touch.Pressed) {
        img->fillSmoothRoundRect(0, 0, Width_BCK, Height_BCK, Rad, img->color565(Focused_color[0], Focused_color[1], Focused_color[2]), img->color565(default_Background[0], default_Background[1], default_Background[2]));
        img->fillSmoothRoundRect(0 + Frame_size, 0 + Frame_size, Width_BCK - (Frame_size * 2), Height_BCK - (Frame_size * 2), Rad, img->color565(sprite_Background[0], sprite_Background[1], sprite_Background[2]), img->color565(Focused_color[0], Focused_color[1], Focused_color[2]));
        img_icon->pushImage(0, 0, Width, Height, FOCUSED_data);
        if (PressedOrReleast == false) State = true;
      } else {
        img->fillSmoothRoundRect(0, 0, Width_BCK, Height_BCK, Rad, img->color565(Frame_color[0], Frame_color[1], Frame_color[2]), img->color565(default_Background[0], default_Background[1], default_Background[2]));
        img->fillSmoothRoundRect(0 + Frame_size, 0 + Frame_size, Width_BCK - (Frame_size * 2), Height_BCK - (Frame_size * 2), Rad, img->color565(sprite_Background[0], sprite_Background[1], sprite_Background[2]), img->color565(Frame_color[0], Frame_color[1], Frame_color[2]));
        img_icon->pushImage(0, 0, Width, Height, MAIN_data);
      }
      if ((Touch.Right_datum && Touch.Releast) && (PressedOrReleast == true)) State = true;

      img_icon->pushToSprite(img, x_i, y_i, TFT_BLACK);

      img->pushSprite(x, y);

      img_icon->deleteSprite();
      img->deleteSprite();
    }
    return State;
  }
};
