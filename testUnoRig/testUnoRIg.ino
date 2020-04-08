


//**********************************
//*****  INCLUDES  *****************
//**********************************
#include <Key.h> // do I need this ?
#include <Keypad.h>
//#include <Servo.h>  // or Adafruit PCA9685 - https://github.com/adafruit/Adafruit-PWM-Servo-Driver-Library
#include <FlexiTimer2.h>
#include <LiquidCrystal_PCF8574.h> // or use #include <LiquidCrystal_I2C.h>
#include <Wire.h> // This library allows you to communicate with I2C / TWI devices
#include <EEPROM.h>

String softwareVersion = "2.7";
#define numButtons 3



/* Programmer Note: 
 * for the compiler to work, order of the following rountines matter  
 * #define must be before the use, compiler does not look below code.
 */

//  led States
#define led_OFF 0
#define led_ON 1
#define led_Toggle 2

// button State
#define btn_OFF 1  // Buttoms are Failsafe
#define btn_ON 0


// event
#define eventClear 99  //clear Event  (0 is a event for gates)
#define eventInterrupt 1  //General Event

// User Configurable Variables

  int intervalService = 1000;  //Timer Service Interval in milliseconds
  int intervalDisplay = 2000;  //Display Update Interval in milliseconds
  int debounceCountMax = 3; //button debounce
  int blowerShutDownMax = 30; //seconds
  int blowerDelayMax = 10; //seconds

#define clearEEPROM 0
#define readEEPROM  1
#define writeEEPROM  2

//**************************************
//**************************************
//********  LCD Handling    ************
//**************************************
//**************************************

//***  LCD Library  ***
LiquidCrystal_PCF8574 lcdControl(0x27);  // set the LCD address to 0x3F for a 20 chars and 4 line display

 //*** lcd Variables ***
typedef struct
{
  String Row0;
  String Row1;
  String Row2;
  String Row3;
} lcdDef;
lcdDef lcd = {"row0","row1","row2","row3"};



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
    char key = GetKeyStroke();
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
//******** KEYPAD Handling  ************
//**************************************
//**************************************
/*  This set of procedures handle the keypad
 *  interface.  The get_key may be used in other 
 *  procedures
 */


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

char GetKeyStroke()
/*  Add this procedure to keep the physical interface to keypad in this section
 *   of code
 */
{
  char key = kpd.getKey();
  return key ;
}
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
//********  BUTTON HANDLING  ***********
//**************************************
//**************************************



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
  {0,0,0,12},
};


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
      if (button[i].DebounceCount >= debounceCountMax)
      {
        button[i].State = thsButton;
        if(thsButton == btn_ON) {eventNum = i;}
        button[i].DebounceCount = 0;
      }
      button[i].DebounceCount += 1   ; 
    }
    else 
    {
      button[i].DebounceCount = 0;
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


void testButtons()
{ // Button testing for setup 
  //         "                    "  
  lcd.Row0 = "   Button Test      " ;
  lcd.Row1 = "  Press A to Start  ";
  lcd.Row2 = "   Button Tests     ";
  lcd.Row3 = "Any Oth Key Cancels ";
  lcdUpdateByRow( true,true,true,true);
  
  int  stringPosition = 0;
  bool testRunning = true;
  bool buttonRunning = true;
  char key = NO_KEY;

  do   //Wait for a keystroke (only one)
  {
    key = GetKeyStroke();
    
    // Run Test or not  
    if (key != NO_KEY)
      if (key == 'A') 
      { 
        
        String testStatus = "BTN: ";
        for (int i=0;i<numButtons;i++)
        {testStatus += "R ";
        Serial.println (testStatus);
        }
        
        int cycleTime = 1000;
        unsigned long sysTimer = millis();
        bool testRunning = true ;
        
        lcd.Row0 = "   Button Test      " ;
        lcd.Row1 = "Test Button: " ;
        lcd.Row2 = testStatus;
        lcd.Row3 = " B to fail and Skip " ;
        lcdUpdateByRow( true,true,true,true);
    
        for (int i=0;i<numButtons;i++)
        {
          stringPosition = 5+i*2;  // first char in string is position 0
          lcd.Row1 += i;
          lcd.Row2.setCharAt((stringPosition),'T'); 
          lcdUpdateByRow( true,true,true,true);
          
          sysTimer = millis();
          buttonRunning = true;
          do  //blink LED until Button press, B on keypad
          { 
            int eventBtn = CheckButtonEvent() ;
            if ( eventBtn == i) 
              {
                buttonRunning = false; 
                lcd.Row2.setCharAt((stringPosition),'P'); 
                lcdUpdateByRow(false,false,true,false);
                ledHandling(i,led_OFF);
              }
            
            key = GetKeyStroke();
            if (key == 'B')
              {
                buttonRunning = false; 
                lcd.Row2.setCharAt((stringPosition),'F');
                lcdUpdateByRow(false,false,true,false);
                ledHandling(i,led_OFF);}
            
            if (millis() >= sysTimer)
              {
                ledHandling(i,led_Toggle);
                sysTimer = millis() + cycleTime;
              }
          } while (buttonRunning == true);
     
        } // next button
          lcd.Row3 = "Press * to Continue " ;  
          lcdUpdateByRow( false,false,false,true);
     } else {
        testRunning = false;
      }
  } while (testRunning == true); // User selects run test
  
  lcdClearRow(3);
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



typedef struct
{
  int State ;
  int Output;
} ledDef;

ledDef led[numButtons] = {{0,A2}, {0,A1},{0,A0}};


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
    case led_Toggle:
    {  // Toogle
      if (led[ledNum].State == led_OFF) {digitalWrite (led[ledNum].Output,LOW); led[ledNum].State = led_ON ;}
      else                              {digitalWrite (led[ledNum].Output,HIGH); led[ledNum].State = led_OFF ;}
      break;
    }
  }
}  

void BlinkAllLeds (int blinkTime)
/*  This just blinks the LEDS at a 2 Hz rate for 
 *   the blinkTime in SECONDS)
 */
{
 for (int i = 0; i< numButtons ; i++) {ledHandling(i,led_ON);}
 delay(500);
 for (int i = 0; i< numButtons ; i++) {ledHandling(i,led_OFF); }
}
    
void setupLED()
/* This procedure is called from Setup
 *  and handles linking LED to UNO
 */
{
  for (int i = 0; i< numButtons ; i++)
  {
    pinMode(led[i].Output,OUTPUT);
    for (int j = 0; j<3; j++)
    { // blick LED to verify functional
      ledHandling(i,led_OFF);  
    }
  }
}


void testLED()
{ // LED testing for setup 
  //         "                    "  
  lcd.Row0 = "     LED Test       " ;
  lcd.Row1 = "  Press A to Start  ";
  lcd.Row2 = "Any Oth Key Cancels ";
  lcd.Row3 = "                    ";
  lcdUpdateByRow( true,true,true,true);
  delay(1000);
  int cycleTime = 1000;
  
  bool testRunning = true;
  char key = NO_KEY;

  do   //Wait for a keystroke (only one)
  {
    key = GetKeyStroke();
    
    // Run Test or not  
    if (key != NO_KEY)
    {
      if (key == 'A') 
      {
        lcd.Row1 = "   Press Any Key    ";
        lcd.Row2 = "  blinking each LED ";
        lcdUpdateByRow( false,true,true,false);
        
        unsigned long sysTimer = millis() + cycleTime;
        bool ledRunning = true;
        do // keep looping until next keystroke
        {
         key = GetKeyStroke();   
         if (key != NO_KEY) {ledRunning = false;}
         
         if(millis() >= sysTimer) 
         {
           for(int i = 0 ;i< numButtons; i++)
           {
            ledHandling(i,led_Toggle);
            delay(100);
           }
           sysTimer = millis() + cycleTime;
         }
        } while (ledRunning == true);
        
        // turn off the leds
        lcd.Row1 = "   Test Complete    ";
        lcd.Row2 = "   Turn LED OFF     ";
        lcd.Row3 = "Press * to Continue " ; 
        lcdUpdateByRow( false,true,true,true);
        for(int i = 0; i< numButtons; i++) {ledHandling(i,led_OFF);}  
      }  else   { // Any key but A skips tet
        testRunning = false ;
      }
   }  
  } while (testRunning == true);
}               



//**************************************
//**************************************
//******  Event Handling    ************
//**************************************
//**************************************

/* This set of procedures handles the Event
 *  / Stage Gate for the system
 */

typedef struct
{
  int svcBlower;
  int svcGate;
  int svcTimer;
  int svcDisplay;
  int svcAlarm;
} eventDef;
eventDef event = {eventClear,eventClear,eventClear,eventClear,eventClear};

typedef struct
{
  unsigned long itrpServiceTimer; // ms
  unsigned long itrpServiceDisplay; // ms
  unsigned long Time; // seconds
  unsigned long RunTime; // seconds
} timerDef;
timerDef sys = {0,0,0,0};

typedef struct
{
  String Label;
  String Value;
  String dataType;
} configDef;
int configLoopSize = 5;
configDef configLoop[5] ;

int CheckTimer()
/*  This procedure is a seudo
 *   interrupt timer
 */
{
  if(millis() >=  sys.itrpServiceTimer)
  {
    sys.itrpServiceTimer = millis() + intervalService;
    sys.Time  += intervalService/1000;
    sys.RunTime += intervalService/1000;
  }

  if(millis() >=  sys.itrpServiceDisplay)
  {
    event.svcDisplay = eventInterrupt;
    sys.itrpServiceDisplay = millis() + intervalDisplay;
  }
}

void updateDisplay()
{ 
  char lcdTime[40];
  sprintf(lcdTime ,"%05d",sys.Time);
  lcd.Row0 = "Time " + String(lcdTime);
  
  lcd.Row1 = "LED ";
  for (int i =0; i < numButtons; i++)
  {
    if(led[i].State == led_OFF) {lcd.Row1 += "0";}
    else {lcd.Row1 += "1";} 
  }
  lcd.Row1 += " BTN ";
  for (int i =0; i < numButtons; i++)
  {
    if(button[i].State == btn_OFF) {lcd.Row1 += "0";}
    else {lcd.Row1 += "1";} 
  }
  lcd.Row2 = "                   ";
  lcd.Row3 = "                   ";
  lcdUpdateByRow( true,true,true,true); 


  event.svcDisplay = eventClear;
}




void changeConfiguration()
// Scrool thru variables to change
/*
 * to add new variable to list need to
 * update .Label abd .Value below and added
 * new case in if (key == '#')
 * and update  handleEEPTOM
 */
{

  //                    "012345678901234"
  configLoop[0].Label = "ServiceTime ms ";
  configLoop[1].Label = "DisplayTime ms ";
  configLoop[2].Label = "Debounce Ctn   ";
  configLoop[3].Label = "ShutDown sec   ";
  configLoop[4].Label = "Delay sec      ";
        
  configLoop[0].Value = String(intervalService);
  configLoop[1].Value = String(intervalDisplay);
  configLoop[2].Value = String(debounceCountMax);
  configLoop[3].Value = String(blowerShutDownMax);
  configLoop[4].Value = String(blowerDelayMax);

  // Update Display
  lcd.Row2 = configLoop[0].Label + configLoop[0].Value;      
  lcdUpdateByRow(false,false,true,false);

  int loopCount = 0;
  char intDisplay[40];
  bool loopConfig = true;
  do
  {
    char key = GetKeyStroke();
    if (key != NO_KEY)
    { 
      if (key == '*') 
      {
       loopCount += 1;
       if (loopCount == configLoopSize) loopCount = 0;
       lcd.Row3 ="";
       lcdClearRow(3);
      }
      if (key == 'B') {loopConfig = false; }

      if (isdigit(key)) 
      { 
       lcd.Row3 += key;
       lcdUpdateByRow(false,false,false,true);
      }
      if (key == '#')
      {
        // write values
        switch (loopCount)
        {
          case 0: intervalService   = lcd.Row3.toInt(); configLoop[loopCount].Value = lcd.Row3   ;break;
          case 1: intervalDisplay   = lcd.Row3.toInt(); configLoop[loopCount].Value = lcd.Row3   ;break;
          case 2: debounceCountMax  = lcd.Row3.toInt(); configLoop[loopCount].Value = lcd.Row3   ;break;
          case 3: blowerShutDownMax = lcd.Row3.toInt(); configLoop[loopCount].Value = lcd.Row3   ;break;
          case 4: blowerDelayMax    = lcd.Row3.toInt(); configLoop[loopCount].Value = lcd.Row3   ;break;
        }
        
        handleEEPROM(writeEEPROM);
        lcd.Row3 ="";
        lcdClearRow(3);
      }
      
      lcd.Row2 = configLoop[loopCount].Label + configLoop[loopCount].Value;      
      lcdUpdateByRow(false,false,true,false);
    }
 } while (loopConfig == true);
}


//**************************************
//**************************************
//********   OTHER STUFF    ************
//**************************************
//************************************** 
/* These procedures are misc setup calls
 *  
 */

void handleEEPROM(int action)
{
  switch (action)
  { 
    case readEEPROM:
    {
      EEPROM.get (0 , intervalService);
      EEPROM.get (2 , intervalDisplay) ;
      EEPROM.get (4 , debounceCountMax) ;
      EEPROM.get (6 , blowerShutDownMax) ;
      EEPROM.get (8 , blowerDelayMax) ;  
      break;
    }

    case writeEEPROM:
    {
      EEPROM.put (0 , intervalService);
      EEPROM.put (2 , intervalDisplay) ;
      EEPROM.put (4 , debounceCountMax) ;
      EEPROM.put (6 , blowerShutDownMax) ;
      EEPROM.put (8 , blowerDelayMax) ;
      break;
    }

    case clearEEPROM:
    {
      for (int i = 0 ; i < EEPROM.length() ; i++) 
      {
        if(EEPROM.read(i) != 0) EEPROM.write(i, 0);
      }  
      break;
    }
  }
}


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



void setup() 
{
  setupSerialPort(); 
  setupLCD();
  setupLED();
  setupButton();
  handleEEPROM(readEEPROM);
  
  
  
  lcd.Row0 = "*System Initization*";
  lcd.Row1 = " Dust Collection SW ";
  lcd.Row2 = " Version " + softwareVersion;
  lcd.Row3 = "Startup Test Press A";
  lcdUpdateByRow( true,true,true,true); 
  bool startUpTest = true;
  do
  {
    char key = GetKeyStroke();
    if (key != NO_KEY) 
    {
      if (key == 'A')
      {
        // Run System Test
        testLCDDisplay();
        testKeyPad();
        testLED();
        testButtons(); 
      }
      startUpTest=false;
    }
  } while (startUpTest == true);
  
  // Start Interrupt
  sys.itrpServiceTimer = millis(); // update timers on first pass
  sys.itrpServiceDisplay = millis(); // update display on first pass

}
 
void loop() 
{
  // Interrupt Timer 
  CheckTimer();
  if (event.svcDisplay != eventClear) {updateDisplay(); }

  // Keypad Stroke
  char key = GetKeyStroke();
  switch (key) 
  {
      case 'A': BlinkAllLeds(2); break; // blink led
      case 'B': changeConfiguration(); break; //
      case 'C': handleEEPROM(clearEEPROM) ; handleEEPROM(writeEEPROM); break;
  }

  // Button Press
  int btnEvent = CheckButtonEvent();  // toggle LED
  if (btnEvent != 99) {ledHandling(btnEvent,led_Toggle); btnEvent = 99; }  

}
