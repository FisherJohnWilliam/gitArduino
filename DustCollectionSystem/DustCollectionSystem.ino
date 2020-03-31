/* 

This Sketch is for the Autromated Dust Collection System
Controller is a UNO with Servo Board, Dispaly and Keyboard
The Blast Gates are controller by a RS Servo


Revision Control
21 Mar 20 Started Writing Code

*/ 

/****************************************************************************************************************
 * 
 *                  Library Configuration for UNO
 *               Keypad, Servo, LCD, I2C, FlexiTimer
 *   
 ***************************************************************************************************************/

#define softwareVersion '1.1'

//**********************************
//*****  INCLUDES  *****************
//**********************************
#include <Key.h>
#include <Keypad.h>
#include <Servo.h>  // or Adafruit PCA9685 - https://github.com/adafruit/Adafruit-PWM-Servo-Driver-Library
#include <FlexiTimer2.h>
#include <LiquidCrystal_PCF8574.h> // or use #include <LiquidCrystal_I2C.h>
#include <Wire.h> // This library allows you to communicate with I2C / TWI devices


//**********************************
//**** typedef Struct  *************
//**********************************

// ****  Define Struct for Event Handler  ***********
typedef struct
{
  int Blower;
  int Gate;
  int InterruptTimer;
} stateDef;
stateDef event = {99,99,99};

#define test_None = 99
#define test_Keypad = 1
#define test_LCD = 2
#define test_DigitalDisplay = 3
#define test_ServoManual = 4
#define test_ServoReset = 4

// ****  Define Struct for Error Handler  ***********
typedef struct
{
  bool Display;
} errorDef;

errorDef error = {false};

// ****  Define Struct for Blower  ***********
typedef struct
{
  int State;
  int Vac_Relief; // // Which Gate to use as Vac Relief for blower
  unsigned long Run_Timer;
  unsigned long Shutdown_Timer;
  unsigned long Delay_Timer;
  unsigned long Shutdown_Max; 
  unsigned long Delay_Max;

  int Run_Outp;   // Dig ourput to Blower Relay
  
  int Run_Inp;  // feedback from motor start - future
  int Filter_AIN;  // Future
  long FilterPress; // Future  
} blowerDef;

blowerDef blower = {  0, 1, 0, 0, 0, 60, 10, 14, 0, 0,0};


// ****  Define Struct for Gates  ***********
//The analog input pins can be used as digital pins, referred to as A0, A1, etc.
#define Num_Tools 5
typedef struct 
{
  int State;
  char Tool;
  int Button_Inp;
  int LED_Out;
  int Servo_No;
  int ServoClosed;
  int ServoOpen;
  int ServoStep;
} gateDef ;

gateDef blastGates[Num_Tools] = 
{  //  Watch Index starts at Zero, nott 1
   // State  Tool       Button_Inp LED_Out   Servvo Stuff
  {    0,    'WorkBench',  15,         20,       1,0,0,0},
  {    0,    'TableSaw',   16,         21,       2,0,0,0},
  {    0,    'ChopSaw',    17,         22,       3,0,0,0},
  {    0,    'DrillPress', 18,         23,       4,0,0,0},
  {    0,    'Vac',        19,         24,       5,0,0,0},  
};


// ****  Define Struct for LCD Handler  ***********

typedef struct
{
  // Manual Write
  String Text;
  int Row;
  int Col;

  // Blast
  String Row0;
  String Row1;
  String Row2;
  String Row3;

  // Setup Stuff
  int BackLight;
  bool DisplayON;
  bool CursorON;
  bool BlinkON;
  
} lcdDef;

lcdDef lcd = {"",0,0,"","","","",50,true,true,true};

//**********************************
//*****   Servo  Library   *********
//**********************************

// Use Servo channel for LED if short Outputs??

//**********************************
//**** LCD DISPLAY Library *********
//**********************************

LiquidCrystal_PCF8574 lcdControl(0x27);



//**********************************
//****  KEYPAD Libray   ************
//**********************************

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


/****************************************************************************************************************
 * 
 *                  Procedures for Controling Dust Collection System
 *   Blower Handling, Gate Handling, LCD Handling, LED Handling, Keypad Handling
 *   
 ***************************************************************************************************************/


//**********************************
//**** BLOWER HANDLING   ***********
//**********************************
/*
 * Blower States are
 * 0 (OFF) = Blower Relay De-Energerized, All timer OFF
 * 1 (ON) = Blower Relay Energerized, Run_Timer ON
 * 2 (Shutdown) = Blower Relay Energerized, Run_Timer ON, Shutdown Timer counting down
 * 3 (Delay) = Blower Relay De-Energerized, Run_Timer OFF, Delay Timer counting down before Start can happen
 * 
 * User can configure Shutdown and Delay start times
 */

// Blower Global Variables

 // Define blower.State conditions
 #define blower_OFF 0
 #define blower_ON 1
 #define blower_SHUTDOWN 2
 #define blower_DELAY 3
 #define blower_ERROR 4
 

 #define Timer_Service_Interval 1000  // Software Interrrupt for Timer Services  
 

 //*** ServiceBlowerTimrer  ***
// This function is called by the FlexiTimer2 to service Timers
// Update Blower Timers for Shutdown, Delay, Runtime
void ServiceBlowerTimers() 
{

 switch (blower.State)
 {
    case blower_OFF : 
      blower.Shutdown_Timer = 0;
      blower.Delay_Timer = 0;
      blower.Run_Timer = 0;
   break;

   case blower_ON:
      blower.Shutdown_Timer = 0;
      blower.Delay_Timer = 0;
      blower.Run_Timer += Timer_Service_Interval;
   break;

   case blower_SHUTDOWN:
      blower.Shutdown_Timer -= Timer_Service_Interval;
      blower.Delay_Timer = 0;
      blower.Run_Timer += Timer_Service_Interval;
   break;

   case blower_DELAY:
      blower.Shutdown_Timer =0;
      blower.Delay_Timer -= Timer_Service_Interval;
      blower.Run_Timer = 0;
   break;

   case blower_ERROR: //Error
      blower.Shutdown_Timer =0;
      blower.Delay_Timer = 0;
      blower.Run_Timer = 0;
   break;
 }
} // End of ServiceBlowerTimers


//*** NewBlowerRequest  ***
// A rountine handles a request to chnage the state iof the blower
// The system will only request ON or OFF
// Note:  This is only the logic to allow ON or OFF
// the blower is physically control in BlowerControl
void NewBlowerRequest( int blower_Request_State) 
{
  
  switch (blower_Request_State)
  {
    case blower_OFF:
      switch (blower.State)
      {  
        case blower_OFF:
          //nothing to do
          event.Blower = event.Clear;
        break;
        case blower_ON:
          //Goto Shutdown Mode
          blower.State = blower_SHUTDOWN; 
          blower.Shutdown_Timer = blower.Shutdown_Max;
          event.Blower = event.Clear;
        break;
        case blower_SHUTDOWN:
          // Just waiting
          event.Blower = event.Clear;
        break;
        case blower_DELAY: 
         // Nothing to Do
         event.Blower = event.Clear;
        break;
       case blower_ERROR:
        // Future
        event.Blower = event.Clear;
       break;
      } 
    break;

    case blower_ON :
      switch (blower.State)
      {  
        case blower_OFF:
          BlowerControl(true);
          event.Blower = event.Clear;
        break;
        
        case blower_ON: 
          //Nothing to do
          event.Blower = event.Clear;
        break;
        
        case blower_SHUTDOWN:  
          // Switch state to ON
          blower.State = blower_ON; 
          blower.Shutdown_Timer = 0;
          event.Blower = event.Clear;
        break;
        
        case blower_DELAY: 
          // Just gotta Wait Delay Timer to Clear
          // Do not clear Request  
          event.Blower = blower_ON;  // just to keep code consistant
        break;
        case blower_ERROR: //Error
          event.Blower = event.Clear;
        break;
      } 
    break;

    case blower_SHUTDOWN:  
      // Got here in error
      // just clear request
      event.Blower = event.Clear;
    break;
    case blower_DELAY: 
      // Got here in error
      // just clear request
       event.Blower = event.Clear;
    break;
    case blower_ERROR:
      // Got here in error
      // just clear request
      // event.Blower = event.Clear;
    break;
  }
} //NewBlowerRequest


// *** BlowerShutdownCheck   ***
// Just checks if Timer has expired and Turns pump off
// This procedure Physically STOPS the Blower
void BlowerShutdownCheck()
{
  // Just for error chacking, make sure in shutdown mode
  if (blower.State == blower_SHUTDOWN)
  {
    if (blower.Shutdown_Timer <= 0)
    {
      BlowerControl(false);
    }
  }
}

 
// *** BlowerControl() ***
// This procedure Physically Start and Stops
// the Blower thru a Digtal Output to Relay

 void BlowerControl( bool blowerRelay)
{
  switch (blowerRelay)
  {
     case true:
       digitalWrite(blower.Run_Outp,HIGH);
       blower.State = blower_ON;
     break;
    case false:
       digitalWrite(blower.Run_Outp,HIGH);
       blower.State = blower_OFF;
    break;     
  }
} // BlowerControl


//**********************************
//****   GATE HANDLING  ***********
//**********************************
/* Using blastGate(Num_Tools).State
 */
/* The current state of each BlastGate is stored in the array Blast_gate_Status
 *  Blast Gate States Are Closed, Open, Vac(cum Relief), Transition, Error
 * Always Open Gates first, then Close others
*/

// Define blastGates[].State & .Request
#define gate_CLOSED 0
#define gate_OPEN 1
#define gate_VAC 2
#define gate_TRANSITION 3  // future if feedback
#define gate_ERROR 4  // future


// Text for LCD Display
String gate_State_Text[5] = {"Closed","Open","Vac Ref","Trans","Error"};

//int event.Gate = event.Clear;


// *** NewGateRequest  ***
// handles a new gate request from Input or Keypad
// This is only the logic to ON or CLose gate
// actual control is in GateControl

void NewGateRequest(int Request_Gate)
{

  switch (blastGates[Request_Gate].State)
  {
     case gate_CLOSED:
       // Request to Open Gate, Request Blower On
       GateControl(Request_Gate , gate_OPEN);
       event.Blower = blower_ON;
      
      // Check Vacuum Releif Gate is Open
      if(Request_Gate != blower.Vac_Relief && blastGates[blower.Vac_Relief].State == gate_VAC )  GateControl(blower.Vac_Relief , gate_CLOSED);
       
     break;

     case gate_OPEN:
       // Request to Close Gate
  
       // Check if another valve is open
       int gate_Check = 0;
       for (int i = 0; i<10; ++i)
       {
        gate_Check += blastGates[i].State;
       }
  
        // Check if need to open Relief Valve
       if (gate_Check == 0) GateControl(blower.Vac_Relief , gate_VAC);
       GateControl(Request_Gate , gate_CLOSED);
       
      break;
      
  }
  //Clear Request
  //Request_Gate = event.Clear;
  
}

// ****  GateControl *********************
// This procedures physically Moves the blast gate gate_Num
void GateControl (int gate_Num, int gate_Todo)
{
  switch (gate_Todo)
  {
    case gate_OPEN: 
    blastGates[gate_Num].State = gate_OPEN;
    break;

    case gate_VAC: 
    // VAC state is open but potential could be partial open in future
    blastGates[gate_Num].State = gate_VAC;
    break;

    case gate_CLOSED:
    blastGates[gate_Num].State = gate_CLOSED;
    break;
    
  }
}



//**********************************
//****   LED HANDLING    ***********
//**********************************
/* This procedure updates the LEDs on the
 * buttons at each tool.
 * Light the LED of any Gate that is OPEN State
 */



void LEDHandling()
{
  for(int i = 0; i=Num_Tools; ++i)
  {
    switch (blastGates[i].State)
    {
      case gate_OPEN : digitalWrite(blastGates[i].LED_Out, HIGH);
      case gate_CLOSED : digitalWrite(blastGates[i].LED_Out, LOW);
      case gate_VAC : digitalWrite(blastGates[i].LED_Out, LOW);
      case gate_TRANSITION : digitalWrite(blastGates[i].LED_Out, LOW);
      case gate_ERROR : digitalWrite(blastGates[i].LED_Out, LOW);
    }
  }
}

//**********************************
//****  KEYPAD Handling ************
//**********************************

/*  Keypad_State and keypad_Request handling
 *   keypad_Request are prefined keystokes to request a function such as Valve or Configuration
 *   keypad_RequestGate requires Ag# where "A" is request char , g = gate number, "#" is continue
 *      THe Gate infor is dispalyed and thenext key activates
 *      THis is keypad_State = 1
 *      
 *   keypad+RequestConfig requires Bc# where "B" is request char, c = config code, "#" is continue
 *      The config Name and current value are dispaly and used can neter new valve and "#"
 *      THis is keypad_State = 2
 *      
 */
#define keypad_RequestGate "A"
#define keypad_RequestConfig "B"
#define keypad_State_Free 0
#define keypad_State_Gate 1
#define keypad_State_Config 2

int keypad_State = 0;  // Code used to indicate waiting on additional keystrokes
String keypad_Message = "";
String keypad_Hold = "";

//***  KeypadActionRquested   ***
// This handles the user entering values at the Keypad
// Collect Key stokes until # then take actions
void KeypadActionRequested (char Key) 
{

  //Show keytoke on LCD Line 3
  int lcdCol = keypad_Message.length()+1;
  lcd.Row = 3;
  lcd.Col = lcdCol; 
  lcd.Text = Key;
  //LCD
  keypad_Message += Key;
  String lcd_Text = "";
  char msgFunction;

  // determine keystoke meaning based on keypad_State
  switch (keypad_State)
  {
   case keypad_State_Free:
     // Keep collecting keystokes in keypad_Message until "#"
     if (Key = '#')
     {
        msgFunction = keypad_Message.charAt(1);       
       
       // Take Action based on msgFunction  
         switch (msgFunction)
        {
          case 'A':
            // decode Message
            int msgLength = keypad_Message.length();   
            String msgString = keypad_Message.substring(2 , msgLength-1);
            int msgValue = msgString.toInt();   // Need error checking
            
            // Gate Request, update display wait for Confirm to Cancel
            keypad_State = keypad_State_Gate;
            keypad_Hold = msgString;
            keypad_Message = "";

            lcd.Row = 2;
            lcd.Col = 0;
            lcd.Text =  blastGates[msgValue].Tool + ' is ' + gate_State_Text[blastGates[msgValue].State];
            //LCD
            lcd.Row = 3;
            lcd.Col = 0;
            lcd.Text =  'Press # to Change';
            //LCD
          break;
          
          case 'B':
          // Manaul Operation Request
          
          break;
      
          case 'C':
          // Configure Variables
          
          break;
      
          case 'D':
          // Alt Display Modes
          
          break;
        }
      
    case keypad_State_Gate:
      // Looking for confirmation - "#" on next Char
      msgFunction = keypad_Message.charAt(1);  //should only be one char
      
      if (msgFunction == '#') 
      {  // Confirmed , Let Go
        int myGate = keypad_Hold.toInt();  // Need Error Checking
        NewGateRequest(myGate);  // Call Procedure to decide how to change state
        lcd.Row = 3;
        lcd.Col = 0;
        lcd.Text =  'Accepted';
        //LCD;
      }
      else
      {
        // Cancel
        lcd.Row = 3;
        lcd.Col = 0;
        lcd.Text = 'Cancel';
       //LCD
      }
      // Final clean-up
     keypad_State = keypad_State_Free;
     keypad_Message = "";

    break;

    case keypad_State_Config:

    break;
    }
    
  } //switch keypadState
}  //KeypadActionRequested


// KeypadConfigureRequest
// This function handles the request to change a configuration
void KeyPadConfigureRequest ( char kyp_Control, long kyp_Data )
{
  switch (kyp_Control)
  {
    case '1':
    // Change Shutdown Time
    lcd.Row = 3;
    lcd.Col = 0;
    lcd.Text = "blower_Shutdown_Max = " + blower.Shutdown_Max;
    //LCD;
    
    blower.Shutdown_Max = kyp_Data;
    break;
    
    case '2':
    // Change delay Time
    blower.Delay_Max = kyp_Data;
    break;

    case '3':
    //  Change lcd Backlight
    //lcdParm.BackLight = kyp_Data;
    break;   
  }
} //KeyPadConfigureRequest

//***   KeyPadTestMode
// Simply display the keystroke on the bottom row of LCD
//  Exit on  Time (15 sec) or Num of Char tested = 20
void KeyPadTestMode ()
{
  bool testRun = true;
  int charCount = 0;
  unsigned long testTimeout = millis() + 15000;
  //LCDClearRow(3);
  do
  {
    char key = kpd.getKey();   
    if (key != NO_KEY)
    {
      //lcd.Row = 3;
      //lcd.Col = charCount;
      //lcd.Text = Key;
      //LCD;  // display
      charCount += 1;
    }

    // Exit Check
    if (charCount == 20) testRun = false;
    if (millis() >= testTimeout) testRun = false;  // need rollover check on millis()??
  } while (testRun) ;
  
} // KeyPadTestMode



//**********************************
//****   LCD Handling   ************
//**********************************

/* These procedures physically Changes the LCE display
 *  by using the lcd struct to  pass paramters to rountine
 *  https://readthedocs.org/projects/arduinoliquidcrystal/downloads/pdf/latest/
 */
/*
 * Procedures for Updating Rows as well as a Manual_Mode using lcd struct
 */


void lcdUpdate()
{
  lcdControl.clear
  lcdControl.setCursor(0,0)
  lcdControl.write(lcd.Row0)
  lcdControl.setCursor(0,1)
  lcdControl.write(lcd.Row1)
  lcdControl.setCursor(0,2)
  lcdControl.write(lcd.Row2)
  lcdControl.setCursor(0,3)
  lcdControl.write(lcd.Row3)  
}

// *** lcdRow0 ***
/* Updates the Status Row of the LCD
 *  G###__bbb__tttt
 *  G# = Gate Open (allows mulible gates Open)
 *  bbb is blower status = _ON, OFF, SHD, DLY, ERR
 *  tttt is Timer (mins for RunTime)
 */
void lcdRow0()
{
    String gatesOpen = "";
    String txtGate = "G";
    String txtBlower = "";
    String txtTime = "";
    
  if (lcd.DisplayON) // Only update if display is ON
  {
     // Gate Status
    for (int i = 0 ; i= Num_Tools; i++)
    {
      if(blastGates[i].State == gate_OPEN) gatesOpen +=i;
      if(blastGates[i].State == gate_VAC) gatesOpen +=i;
    }
    txtGate += gatesOpen;

    // Blower Status and Timers
    switch (blower.State)
    {
      case blower_ON: txtBlower = " ON"; txtTime = blower.Run_Timer;
      case blower_OFF: txtBlower = "OFF"; txtTime = "    ";
      case blower_SHUTDOWN: txtBlower = "SHD"; txtTime = blower.Shutdown_Timer;
      case blower_DELAY: txtBlower = "DLY"; txtTime = blower.Delay_Timer;
      case blower_ERROR: txtBlower = "ERR"; txtTime = "    ";
    }  

   // Display Sttus
   
   lcdControl.setCursor(0,0);
   lcdControl.write("                    ")
   lcdControl.write(txtGate+txtBLower+xtxTime);
  }
}

void lcdDateEntry()
{
  
}


 
// *** lcd Display Handling
/*  this is a separate procedure to allow
 *  contain the Physical LCD comments to one place
 */
/*
void LCD ()
{
 //if (lcd.DisplayON)  lcddisplay.display;
 //if (!lcd.DisplayON)  lcddisplay.noDisplay; 
 if (lcd.CursorON)  lcddisplay.cursor;
 if (!lcd.CursorON)  lcddisplay.noCursor; 
 if (lcd.BlinkON)  lcddisplay.blink;
 if (!lcd.BlinkON)  lcddisplay.noBlink; 
 lcddisplay.setBacklight (lcd.BackLight);

 lcddisplay.setCursor (lcd.Row,lcd.Col);
 lcddisplay.setCursor (lcd.Row,lcd.Col);
 lcddisplay.write lcd.Text;
}

//*** LCD Setup
// This procedure is called from setup
 void LCDSetup()
 {
  lcd.begin(lcdParm.Cols, lcdParm.Rows); 
  //lcddisplay.display;
  //lcddisplay.cursor;
  //lcddisplay.blink; 
  //lcddisplay.setBacklight (lcdParm.BackLight);
  
  lcdTestDisplay();
  
  lcddisplay.setCursor(1,0);
  lcddisplay.print("Dust Collection System");
  lcddisplay.setCursor(1,1);
  lcddisplay.print("Version: " + softwareVersion);
  lcddisplay.setCursor(1,2);
  lcddisplay.print("John Fisher");

 }
*/

//***  LCDClearRow(int Row)  ****
// CLears the row on LCD
void LCDClearRow(int row)
{
  char txt = ' ';
  for(int col = 1;col=20;col++)
  {
    lcddisplay.setCursor(col,row);
    lcddisplay.write(txt);
  }
}

//***  lcdTestDisplay  ***
// This routine cycles thru the didplay to verify
void LCDTestDisplay()
{
  String myMessage = String("ABCDEFGHIJKLMNOPQRSTYZWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*():;{}<>?/");
  // Scroll message accross screen

  lcddisplay.setCursor(0,0);
  //lcddisplay.leftToRight;
  //lcddisplay.autoscroll;
  for (int i=0;i<41;i++)
  {
    char myChar = myMessage.charAt(i);
    lcddisplay.print (myChar);
  }
  delay(2000);
  //lcddisplay.clear;
  //lcddisplay.noAutoscroll
}



//**********************************
//****  Button Handling  ***********
//**********************************


int Button_Prev_State [5] = {0,0,0,0,0};
unsigned long LastDebounceTime[5] = {0,0,0,0,0};
int DebounceDelay = 50;


// ReadButtons
// This procedure reads the inouts for the Buttons
// and determines if a Valve Request is needed

void ReadButtons ()
{
    // Scroll thru and see if any button pushed
  
  if (true) //(event.Gate == event.Clear)
  // Make sure we are not processing another request
  {
    
    for (int i = 0;  i<5  ; ++i)
    {     
   
      int thsButton = digitalRead( blastGates[i].Button_Inp);
      if (thsButton != Button_Prev_State[i])
      {
        LastDebounceTime[i] = millis();
      }
      
     if ((millis() - LastDebounceTime[i]) > DebounceDelay) 
     //  Wait for debounce time
     {   
        // see if the button state has changed:
        if (thsButton != Button_Prev_State[i]) 
        {
          // Request Blast_Gate Change
          event.Gate = i;
          Button_Prev_State[i] = thsButton;
          LastDebounceTime[i] = 0;
          break;  //  process this button first
        }
     }
    }
  }
}



/****************************************************************************************************************
 * 
 *                  Setup and Event Handler (Loop) for Dust Collection System
 *   
 ***************************************************************************************************************/
void TimerInterrupt()
{
  event.InterruptTimer = 1  // Anything but Clear
}



//**********************************
//********  SETUP  *****************
//**********************************
void setup() 
{

  // Start Service Timer Interrupts
  FlexiTimer2::set(Timer_Service_Interval , TimerInterrupt); 
  FlexiTimer2::start();
  
  //Set Default Values
  blower.State = blower_OFF;
  //event.Blower = event.Clear;
  //event.Gate = event.Clear;
  blower.Shutdown_Timer = 0;
  blower.Delay_Timer = 0;

  //***  LCD Startup  ***
  // Verify Address
  Wire.beginTransmission(address);
  byte error = Wire.endTransmission();
  if (error == 0) 

  
  Serial.begin(9600);
  
}

//**********************************
//******  Event Handler  ***********
//**********************************
// Event Handler:  Input Change, Keypad Entry, Shutdown Timer Event, Blower Delay Event

void loop() 
{

  //*** Check Keypad  ***
  // set event.Gate or event.Blower is needed from keystokes
  // all others jeystokes are handle in procedure
  char key = kpd.getKey();   
  if (key != NO_KEY) KeypadActionRequested(key);

  //***  Check Button Inputs
  // sets event.Gate if button is pressed
  ReadButtons();

  //***  Check New Valve Request
  //if (event.Gate != event.Clear) NewGateRequest(event.Gate);

  //*** Check Blower Request
 // if (event.Blower != event.Clear) NewBlowerRequest(event.Blower);

  //** Check Blower Shutdown Timer
  if (blower.State == blower_SHUTDOWN) BlowerShutdownCheck();

  //*** Service Timer Event
  if (event.InterruptTimer != event.clear)
  {
    ServiceBlowerTimers();
    LCDRow0() ;// update LCD Status Line
    //event.InterruptTimer = event.Clear;
  }

  //***  Set Button LEDS  ***
  LEDHandling();

  
}
