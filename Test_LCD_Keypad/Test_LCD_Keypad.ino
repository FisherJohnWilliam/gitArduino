#include <Wire.h> 
//#include <LiquidCrystal_I2C.h>
#include <LiquidCrystal_PCF8574.h> // or use #include <LiquidCrystal_I2C.h>
#include <Keypad.h>


//**********************************
//****  DISPLAY Library  ***********
//**********************************

#include <Key.h>
#include <Keypad.h>
LiquidCrystal_PCF8574 lcd(0x27);  // set the LCD address to 0x3F for a 20 chars and 4 line displa
int lcdSet_Backlight = 50;

//**********************************
//****  KEYPAD Libray   ************
//**********************************

//Setup Keypad in Uno
const byte ROWS = 4; // Four rows
const byte COLS = 4; // Four columns
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = { 9,8,7,6 };// Connect keypad ROW0, ROW1, ROW2 and ROW3 to these Arduino pins
byte colPins[COLS] = { 5,4,3,2 };// Connect keypad COL0, COL1 and COL2 to t
String  Key_String = "";
Keypad kpd = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

void setup(){

   // Setup lecDispaly
  lcd.begin(20, 4); 
  lcd.setBacklight (lcdSet_Backlight);
  lcd.setCursor(1,0);
  lcd.print("LCD TEst");
  lcd.setCursor(1,1);
  lcd.print("Version 1.0");
}

void loop(){
  char customKey = kpd.getKey();
  if (customKey){
    lcd.clear();
    lcd.setCursor(0, 0); 
    lcd.print(customKey);
  }
}
