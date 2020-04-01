
//**********************************
//*****  INCLUDES  *****************
//**********************************
#include <Key.h>
#include <Keypad.h>
//#include <Servo.h>  // or Adafruit PCA9685 - https://github.com/adafruit/Adafruit-PWM-Servo-Driver-Library
#include <FlexiTimer2.h>
#include <LiquidCrystal_PCF8574.h> // or use #include <LiquidCrystal_I2C.h>
#include <Wire.h> // This library allows you to communicate with I2C / TWI devices

String softwareVersion = "2.3";
#define numButtons 2

//**************************************
//**************************************
//******** KEYPAD Handling  ************
//**************************************
//**************************************
/*  This set of procedures handle the keypad
 *  interface.  The get_key may be used in other 
 *  procedures
 */

 //*** lcd Variables ***
typedef struct
{
  String Row0;
  String Row1;
  String Row2;
  String Row3;
} lcdDef;
lcdDef lcd = {"row0","row1","row2","row3"};

int keyCount = 0;
String sKey = "";
//Setup Keypad in Uno
const byte ROWS = 4; // Four rows
const byte COLS = 4; // Four columns
char keys[ROWS][COLS] = 
{
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = { 9,8,7,6 };// Connect keypad ROW0, ROW1, ROW2 and ROW3 to these Arduino pins
byte colPins[COLS] = { 5,4,3,2 };// Connect keypad COL0, COL1 and COL2 to t
Keypad kpd = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

void testKeyPad()
// Keypad testing for setup 
{
  //         "                    "  
  lcd.Row0 = "   KeyPay Test      " ;
  lcd.Row1 = "  Press Each Key    ";
  lcd.Row2 = " ** to Cancel Test  ";
  lcd.Row3 = "                    ";
  lcdUpdateByRow( true,true,true,true);

  bool testRunning = true;
  do
  {
    char key = kpd.getKey();   
    if (key != NO_KEY)
    {
    sKey = sKey + key ;
    lcdUpdateByPosition(1,3,sKey);
    keyCount += 1;
    if ((keyCount == 2) && (sKey == "**")) {testRunning = false;}
    if (keyCount == 16) { testRunning = false; delay (2000);}
    }  
  } while (testRunning);
  
  lcdClearRow(99);
}  


//**************************************
//**************************************
//********  LCD Handling    ************
//**************************************
//**************************************

//***  LCD Library  ***
LiquidCrystal_PCF8574 lcdControl(0x27);  // set the LCD address to 0x3F for a 20 chars and 4 line display




//***  LCD Control Procedures ***
/*  The following routines PHYSICALY control
 *  the LCD Display device (lcdControl)
 */
 
void lcdClearRow( int rowNum)
// Clear the request Row
// Zero clears all rows
{
  if (rowNum == 99) { lcdControl.clear();}
  else { lcdControl.setCursor(0,rowNum); lcdControl.print("                    ");}
}

void lcdUpdateByPosition( int ColPos, int RowPos, String Text)
// Update the lcd by setting the cursor at ColPos and RowPos
// and printing the Text
{
  lcdControl.setCursor(ColPos,RowPos);
  lcdControl.print(Text);
} //***********************

void lcdUpdateByRow(bool r0, bool r1, bool r2, bool r3)
// Update the ROW of the lcd Display
// using the lcd.Rowx variables
{

  if (r0 == true) {lcdClearRow(0); lcdControl.setCursor(0,0); lcdControl.print(lcd.Row0);}
  if (r1 == true) {lcdClearRow(1); lcdControl.setCursor(0,1); lcdControl.print(lcd.Row1);}
  if (r2 == true) {lcdClearRow(2); lcdControl.setCursor(0,2); lcdControl.print(lcd.Row2);}
  if (r3 == true) {lcdClearRow(3); lcdControl.setCursor(0,3); lcdControl.print(lcd.Row3);}
 } //***********************

//***  LCD Control Initilization ***
/*  The following routines handles
 *   LCD setup and start=up tests
 */

void setupLCD()
/* THis handles all the LCD Setup stuff
 * 
 */
{

  Wire.begin();
  lcdControl.begin(20,4);
  lcdControl.setBacklight(1);
  lcdControl.noAutoscroll();
  
  lcd.Row0 = "*System Initization*";
  lcd.Row1 = " Dust Collection SW ";
  lcd.Row2 = " Version " + softwareVersion;
  lcd.Row3 = "Begin Startup Test ";
  lcdUpdateByRow( true,true,true,true); 
  delay (2000);
}

void testLCDDisplay()
{ // This is the Test routine for setup()
  //         "                    "  
  lcd.Row0 = "     LCD Test       " ;
  lcd.Row1 = "   Press # Key to   ";
  lcd.Row2 = " Cycle thru LCD     ";
  lcd.Row3 = "Any oth Key to SKIP ";
  lcdUpdateByRow( true,true,true,true);
   
  bool testRunning = true;
  do
  {
    char key = kpd.getKey();   
    if (key != NO_KEY)
    { 
      
      if(key == '#') 
      {
         String myMessage = String("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*():;{}<>?/");
         String myChar;
         int i = 0;
          // Scroll message accross screen
          lcdClearRow(99);
          for (int row=0; row<4; row++)
            {
              for (int col = 0 ;col <= 19; col++)
              {    
                myChar  = myMessage.charAt(i);
                lcdUpdateByPosition(col,row, myChar);
                i += 1;
                delay(100);
            }
          }
      }
      testRunning = false;
    } 
  } while (testRunning);
  lcdClearRow(99);
}


//**************************************
//**************************************
//********  BUTTON HANDLING  ***********
//**************************************
//**************************************

#define btn_OFF 1  // Buttos are Failsafe
#define btn_ON 0

typedef struct
{
  int State;
  int Prev_State;
  int DebounceCount;
  int Input;
} buttonDef;

buttonDef button[numButtons] =
{
  {0,0,0,10},
  {0,0,0,11},
};
int debounceCountMax= 5;

int CheckButtonEvent ()
/*  This FUNCTION returns the hit button with a RISING Edge
 *  the failing edge is recorded, but does not generate a Event
 */
{
  int eventNum = 99;
  for(int i = 0; i < numButtons; i++)
  {
    int thsButton = digitalRead( button[i].Input);
    if (thsButton != button[i].State)
    {
      button[i].DebounceCount += 1;
      if ( button[i].DebounceCount >= debounceCountMax )
      {
        button[i].State = thsButton ; button[i].DebounceCount = 0;
        if(thsButton == btn_ON) {eventNum = i; break;};    
      }
    }
  }
  return eventNum;
}


void setupButton()
/* THis procedure is called from Setup
 *  and handles linking LED to UNO
 */
{
  for(int i=0; i<numButtons; i++)
  {
    pinMode(button[i].Input, INPUT_PULLUP);  
  }
}


//**************************************
//**************************************
//********   LED HANDLING    ***********
//**************************************
//**************************************
/* This procedure updates the LEDs on the
 * buttons at each tool.
 * Light the LED of any Gate that is OPEN State
 */

#define led_OFF 0
#define led_ON 1
#define led_Toogle 2

typedef struct
{
  int State ;
  int Output;
} ledDef;

ledDef led[numButtons] = {{0,13}, {0,12}};


void ledHandling( int ledNum, int ledState)
/* Toggles the request LED ON or OFF
 *  This procedure is the PHYSICAL interface to the LEDS
 *  ledON: 0 = OFF, 1 = ON, 2 = SWITCH
 */
{  
  switch (ledState)
  {
    case led_OFF: {digitalWrite (led[ledNum].Output,HIGH); led[ledNum].State = led_OFF ;break;}
    case led_ON: {digitalWrite (led[ledNum].Output,LOW);; led[ledNum].State = led_ON ; break;}
    case led_Toogle:
    {  // Toogle
      if (led[ledNum].State == 0) {digitalWrite (led[ledNum].Output,HIGH); led[ledNum].State = led_ON ;}
      if (led[ledNum].State == 1) {digitalWrite (led[ledNum].Output,LOW); led[ledNum].State = led_OFF ;}
      break;
    }
  }
}  


void setupLED()
/* THis procedure is called from Setup
 *  and handles linking LED to UNO
 */
{
  for (int i = 0; i< numButtons ; i++)
  {
    pinMode(led[i].Output,OUTPUT);
    for (int j = 0; j<3; j++)
    { // blick LED to verify functional
      ledHandling(i,led_ON);
      delay(500);
      ledHandling(i,led_OFF);  
    }
  }
}




//**************************************
//**************************************
//********   SETUP TESTS    ************
//**************************************
//************************************** 
/* These procedures are misc setup calls
 *  
 */

//*** Setup Serial Port
void setupSerialPort()
{
  Serial.begin(9600);
  Serial.print("\nRun Setup ");
  Serial.print (softwareVersion);
}

 
 /****************************************************************************************************************
 * 
 *                  Setup and Event Handler (Loop) 
 *   
 ***************************************************************************************************************/
void TimerInterrupt()
{
  delay (100);
}


void setup() 
{
  setupSerialPort(); 
  setupLCD();
  setupLED();
  setupButton();

  

  // Run System Test
  testLCDDisplay();
  testKeyPad();

 // Prep Dispaly
  lcd.Row0 = "Run Time ";
  lcd.Row1 = "Press Button to ";
  lcd.Row2 = "Toggle LED";
  lcdUpdateByRow( true,true,true,false); 

}
 
void loop() 
{
  delay (1000);
  unsigned long sysTime = (millis()/1000);
  char lcdTime[40];
  sprintf(lcdTime ,"%05d",sysTime);
  lcd.Row0 = "Run Time " + String(lcdTime);
  
  lcd.Row1 = "LED ";
  for (int i =0; i < numButtons; i++)
  {
    if(led[i].State == led_OFF) {lcd.Row1 += "OFF";}
    else {lcd.Row1 += "ON ";} 
    lcd.Row1 += " : ";
  }
  
  lcd.Row2 = "BTN ";
  for (int i =0; i < numButtons; i++)
  {
    if(button[i].State == btn_OFF) {lcd.Row2 += "OFF";}
    else {lcd.Row2 += "ON ";} 
    lcd.Row2 += " : ";
  }
  lcdUpdateByRow( true,true,true,false); 

  int btnEvent = CheckButtonEvent();
  if (btnEvent != 99) {ledHandling(btnEvent,led_Toogle);}  
}

