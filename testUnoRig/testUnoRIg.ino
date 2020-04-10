
//*** UNO Board ***
/* Details:
 * Memory: Flash 32K, SRAM 2K  EEPROM 1K
 * Timer1 is used for Servo
 * 
 */
 
//**********************************
//*****  INCLUDES  *****************
//**********************************
#include <Key.h> // do I need this ?
#include <Keypad.h>
//#include <Servo.h>  // or Adafruit PCA9685 - https://github.com/adafruit/Adafruit-PWM-Servo-Driver-Library
#include <LiquidCrystal_PCF8574.h> // or use #include <LiquidCrystal_I2C.h>
#include <Wire.h> // This library allows you to communicate with I2C / TWI devices
#include <EEPROM.h>
#include <FlexiTimer2.h>

//**************************************
// SOFTWARE VERSION
// Must be an INTERGER (easier to store in EEPROM)
//**************************************
int softwareVersion = 29 ;


//**************************************
//**************************************
//********   #define        ************
//**************************************
//**************************************
/* Programmer Note: 
 * for the compiler to work, order of the following rountines matter  
 * #define must be before the use, compiler does not look below code.
 * 
 * Defined constants in arduino don’t take up any program memory space 
 * on the chip. The compiler will replace references to these constants with the defined value at compile time.
*/

//*** led States
  #define led_OFF 0
  #define led_ON 1
  #define led_Toggle 2

//*** button State
  #define numButtons 3
  #define btn_OFF 1  // Buttons are Failsafe
  #define btn_ON 0

//*** event handling
  #define eventClear 99  //clear Event  (0 is valild state ??)
  #define eventBlower  1
  #define eventGate  2
  #define eventService 3
  #define eventKeyPad 4
  #define eventDisplay 5
  #define eventAlarm 6

//*** EEPROM
  #define clearEEPROM 0
  #define readEEPROM  1
  #define writeEEPROM  2
  #define updateEEPROM 3


//*** User Configurable Variables
  byte intervalService = 10;  //Timer Service Interval in 1/100 second
  byte displayCycleCountMax = 2;  //INTERVAL of intervalService
  byte debounceCountMax = 3; //button debounce
  byte blowerShutDownMax = 30; //seconds
  byte blowerDelayMax = 10; //seconds




//**************************************
//**************************************
//********  LCD Handling    ************
//**************************************
//**************************************
/*  The following routines PHYSICALY control
 *  the LCD Display device (lcdControl)
 */

//***  LCD Library  ***
LiquidCrystal_PCF8574 lcdControl(0x27);


//***  lcdClearRow
void lcdClearRow( byte rowNum)
// Clear the request Row
// Zero clears all rows
{
  if (rowNum == 99) { lcdControl.clear();}
  else { lcdControl.setCursor(0,rowNum); lcdControl.print("                    ");}
}

//***  lcdUpdateByPosition
void lcdUpdateByPosition( byte ColPos, byte RowPos, String Text)
// Update the lcd by setting the cursor at ColPos and RowPos
// and printing the Text
{
  lcdControl.setCursor(ColPos,RowPos);
  lcdControl.print(Text);
} //***********************

//***  lcdUpdateByRow
void lcdUpdateByRow(String r0, String r1, String r2, String r3)
// Update the ROW of the lcd Display
// if String is null"" skip
{

  if (r0 != "") {lcdClearRow(0); lcdControl.setCursor(0,0); lcdControl.print(r0); Serial.println(r0);}
  if (r1 != "") {lcdClearRow(1); lcdControl.setCursor(0,1); lcdControl.print(r1); Serial.println(r1);}
  if (r2 != "") {lcdClearRow(2); lcdControl.setCursor(0,2); lcdControl.print(r2); Serial.println(r2);}
  if (r3 != "") {lcdClearRow(3); lcdControl.setCursor(0,3); lcdControl.print(r3); Serial.println(r3);}
 } //***********************

//*** setupLCD
void setupLCD()
/* THis handles all the LCD Setup stuff 
 */
{

  Wire.begin();
  lcdControl.begin(20,4);
  lcdControl.setBacklight(1);
  lcdControl.noAutoscroll();
}

//*** testLCDDisplay
void testLCDDisplay()
{ // This is the Test routine for setup()
  //         "                    "  
  lcdUpdateByRow
  (
  //"                    "
    "     LCD Test       ",
    "   Press # Key to   ",
    " Cycle thru LCD     ",
    "Any oth Key to SKIP "
  );
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
         byte i = 0;
          // Scroll message accross screen
          lcdClearRow(99);
          for (byte row=0; row<4; row++)
            {
              for (byte col = 0 ;col <= 19; col++)
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


//Setup Keypad in Uno

#define ROWS 4 // Four rows
#define COLS 4 // Four columns
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


//*** GetKeyStroke
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
  
  lcdUpdateByRow
  (
  //"                    "  
    "   KeyPay Test      ",
    "  Press Each Key    ",
    " ** to Cancel Test  ",
    "                    "
  );

  bool testRunning = true;
  String sKey = "";
  byte keyCount = 0;

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
  byte State;
  byte Prev_State;
  byte DebounceCount;
  byte Input;
} buttonDef;

buttonDef button[numButtons] =
{ {0,0,0,10}, {0,0,0,11}, {0,0,0,12}};

//***  CheckButtonEvent
int CheckButtonEvent ()
/*  This FUNCTION returns the hit button with a RISING Edge
 *  the failing edge is recorded, but does not generate a Event
 */
{
  int eventNum = 99;
  for(byte i = 0; i < numButtons; i++)
  {
    byte thsButton = digitalRead( button[i].Input);
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

//*** setupButton
void setupButton()
/* THis procedure is called from Setup
 *  and handles linking LED to UNO
 */
{
  for(byte i=0; i<numButtons; i++)
  {
    pinMode(button[i].Input, INPUT_PULLUP);  
  }
}

//***  testButtons
void testButtons()
{ // Button testing for setup 
  
  lcdUpdateByRow
  (
  //"                    "    
    "   Button Test      ",
    "  Press A to Start  ",
    "   Button Tests     ",
    "Any Oth Key Cancels "
  );
  
  byte  stringPosition = 0;
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
        
        String testStatus = "BTN: "; for (byte i=0;i<numButtons;i++) {testStatus += "R ";}
        String testButton = "Test Button: ";
        int cycleTime = 1000;
        unsigned long sysTimer = millis();
        bool testRunning = true ;
        
        lcdUpdateByRow
        (
        //"                    "    
          "   Button Test      " ,
          testButton,
          testStatus,
          " B to fail and Skip " 
        );
    
        for (byte i=0;i<numButtons;i++)
        {
          stringPosition = 5+i*2;  // first char in string is position 0
          testStatus.setCharAt((stringPosition),'T');
          lcdUpdateByRow
          ( 
            "",
            testButton += String(i),
            testStatus, 
            ""
          );
          
          sysTimer = millis();
          buttonRunning = true;
          do  //blink LED until Button press, B on keypad
          { 
            int eventBtn = CheckButtonEvent() ;
            if ( eventBtn == i) 
              {
                buttonRunning = false; 
                testStatus.setCharAt((stringPosition),'P');
                lcdUpdateByRow("", testStatus, "", "");
                ledHandling(i,led_OFF);
              }
            
            key = GetKeyStroke();
            if (key == 'B')
              {
                buttonRunning = false;
                testStatus.setCharAt((stringPosition),'F'); 
                lcdUpdateByRow("",testStatus , "", "");
                ledHandling(i,led_OFF);}
            
            if (millis() >= sysTimer)
              {
                ledHandling(i,led_Toggle);
                sysTimer = millis() + cycleTime;
              }
          } while (buttonRunning == true);
     
        } // next button
          lcdUpdateByRow( "","","","Press * to Continue ");
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
{ byte State ; byte Output; } ledDef;
ledDef led[numButtons] = {{0,A2}, {0,A1},{0,A0}};


//*** ledHandling
void ledHandling( byte ledNum, byte ledState)
/* Toggles the request LED ON or OFF
 *  This procedure is the PHYSICAL interface to the LEDS
 *  ledON: 0 = OFF, 1 = ON, 2 = SWITCH
 */
{  
  switch (ledState)
  {
    case led_OFF: {digitalWrite (led[ledNum].Output,HIGH); led[ledNum].State = led_OFF ;break;}
    case led_ON: {digitalWrite  (led[ledNum].Output,LOW) ; led[ledNum].State = led_ON ; break;}
    case led_Toggle:
    {  // Toogle
      if (led[ledNum].State == led_OFF) {digitalWrite (led[ledNum].Output,LOW);  led[ledNum].State = led_ON ;}
      else                             {digitalWrite (led[ledNum].Output,HIGH); led[ledNum].State = led_OFF ;}
      break;
    }
  }
}  

//*** BlikkAllLeds
void BlinkAllLeds (byte blinkTime)
/*  This just blinks the LEDS at a 2 Hz rate for 
 *   the blinkTime in loops)
 *   REMEMBER - Nothing else happens while linking LEDS
 */
{
 for (byte j=0;j<=blinkTime;j++)
 {
   for (byte i = 0; i< numButtons ; i++) {ledHandling(i,led_ON);}
   delay(500);
   for (byte i = 0; i< numButtons ; i++) {ledHandling(i,led_OFF); }
   delay(500);
 }
}

//*** setupLED    
void setupLED()
/* This procedure is called from Setup
 *  and handles linking LED to UNO
 */
{
    for (byte j = 0; j<numButtons; j++)
    { 
      pinMode(led[j].Output,OUTPUT);
      ledHandling(j,led_OFF);  
    }
}


//*** testLED
void testLED()
{ // LED testing for setup 
  lcdUpdateByRow
  (
  //"                    "  
    "     LED Test       ",
    "  Press A to Start  ",
    "Any Oth Key Cancels ",
    "                    "
  );
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
        lcdUpdateByRow
        (
        //"                    "  
        "",
        "   Press Any Key      ",
        "  blinking each LED   ",
        ""
        );
        
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
        lcdUpdateByRow
        (
        //"                    "  
          "",
          "   Test Complete    ",
          "   Turn LED OFF     ",
          "Press * to Continue "  
        );
        for(int i = 0; i< numButtons; i++) {ledHandling(i,led_OFF);}  
      }  else   { // Any key but A skips tet
        testRunning = false ;
      }
   }  
  } while (testRunning == true);
}               

//**************************************
//**************************************
//*****  EEPROM Handling    ************
//**************************************
//************************************** 
/* This handles the PHYSICAL handling of EEPROM
 *  
 */

//*** handleEEPROM
void handleEEPROM(int action)
/*  this procedure PHYSICALLY handles writing and reading the EEPROM
 *  EEPROM Address are hard coded here
 */
{
  switch (action)
  { 
    case readEEPROM:
    {
      EEPROM.get (0, softwareVersion);
      EEPROM.get (2 , intervalService);
      EEPROM.get (4 , displayCycleCountMax) ;
      EEPROM.get (6 , debounceCountMax) ;
      EEPROM.get (8 , blowerShutDownMax) ;
      EEPROM.get (10 , blowerDelayMax) ;  
      break;
    }

    case writeEEPROM:
    { 
      EEPROM.put (0, softwareVersion);
      EEPROM.put (2 , intervalService);
      EEPROM.put (4 , displayCycleCountMax) ;
      EEPROM.put (6 , debounceCountMax) ;
      EEPROM.put (8 , blowerShutDownMax) ;
      EEPROM.put (10 , blowerDelayMax) ;  
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

    case updateEEPROM:
    { // check if same before writing
      int eCheck = 0;
      EEPROM.get (0, eCheck); if(eCheck != softwareVersion) EEPROM.put (0, softwareVersion);
      EEPROM.get (2, eCheck); if(eCheck != intervalService) EEPROM.put (0, intervalService);
      EEPROM.get (4, eCheck); if(eCheck != displayCycleCountMax) EEPROM.put (0, displayCycleCountMax);
      EEPROM.get (6, eCheck); if(eCheck != debounceCountMax) EEPROM.put (0, debounceCountMax);
      EEPROM.get (8, eCheck); if(eCheck != blowerShutDownMax) EEPROM.put (0, blowerShutDownMax);
      EEPROM.get (10, eCheck); if(eCheck != blowerDelayMax) EEPROM.put (0, blowerDelayMax);
    }
  }
}

//*** softwareVerEEPROM
bool softwareVerEEPROM()
/* read addretss 0/1 for SW version
 *  compare to #define softwareVersion
 *  and true true if same
 */
{
  int temp = 0;
  EEPROM.get (0, temp); 
  Serial.print ("Ver:");Serial.println(softwareVersion);
  Serial.print ("EEp:");Serial.println(temp);
  if(temp == softwareVersion) {return true;} else {return false;}
}

//*** testEEPROM
void testEEPROM()
{
  lcdUpdateByRow
  (
  //"                    "  
    "*EEPROM Initization*",
    " A to READ  ",
    " B to Write ",
    " C to CLEAR "
  );   

  bool startUpTest = true;
  do
  {
    char key = GetKeyStroke();
    if (key != NO_KEY) 
    {
      if (key == 'A') handleEEPROM(readEEPROM);
      if (key == 'B') handleEEPROM(writeEEPROM);
      if (key == 'C') handleEEPROM(clearEEPROM);  
      startUpTest=false;
    }
  } while (startUpTest == true);
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
  byte State;
  byte subCode;
  int Time; // seconds
  byte displayCycleCount;
} eventDef;
eventDef event = {eventClear,0,0,0};



//*** updateDisplay ***
void updateDisplay()
{ 
  char lcdTime[40];
  sprintf(lcdTime ,"%05d",event.Time);
  String r0 = "Time " + String(lcdTime);
  
  String r1 = "LED ";
  for (byte i =0; i < numButtons; i++)
  {
    if(led[i].State == led_OFF) {r1 += "0";}
    else {r1 += "1";} 
  }
  r1 += " BTN ";
  for (byte i =0; i < numButtons; i++)
  {
    if(button[i].State == btn_OFF) {r1 += "0";}
    else {r1 += "1";} 
  }
  lcdUpdateByRow(r0,r1,"",""); 

  event.State = eventClear;
}

//*** maintenanceMode
void maintenanceMode()
/*  THis procedure makes the test procedures
 *   avaliable
 *   DO WE NEED TO STOP FLEXTIMER OR VERIFY BLOWER IS OFF?
 */
{
  int loopCount = 0;
  bool loopDisplay = true;
  int displayLoopSize = 5;
  String displayLoop[displayLoopSize] =
  {
    "EEPROM",
    "LCD",
    "KeyPad",
    "LED",
    "Buttons",
  };
  lcdUpdateByRow
  (
  //"                    "  
    "   Maintenace Mode  ",
    " Press * to Scroll  ",
    "  Press # to Run    ",
    displayLoop[0]
  );

  do
  {
    char key = GetKeyStroke();
    if (key != NO_KEY)
    { 
      if (key == '*') 
      {
       loopCount += 1;
       if (loopCount == displayLoopSize) loopCount = 0;
       lcdUpdateByRow("","","",String(displayLoop[loopCount]));
      }
      else if (key == '#') 
      {
        switch(loopCount)
        {
          case 0: testEEPROM();break;
          case 1: testLCDDisplay();break;
          case 2: testKeyPad();break;
          case 3: testLED();break;
          case 4: testButtons();break;
        }
        loopDisplay = false;  
      }
      else
      { // any other key
        loopDisplay = false;
      }
    }
  } while (loopDisplay == true);
}

//*** changeConfiguration
void changeConfiguration()
// Scrool thru variables to change
/*
 * to add new variable to list need to
 * update .Label abd .Value below and added
 * new case in if (key == '#')
 * and update  handleEEPTOM
 */
{

  int configLoopSize = 5;

  //                    "012345678901234"
  String configLoopLabel[configLoopSize] = 
  {
  //"012345678901234"
    "ServiceTime ms ",
    "DisplayCycle Ctn ",
    "Debounce Ctn   ",
    "ShutDown sec   ",
    "Delay sec      ",
  };
  String configLoopValue[configLoopSize] =       
  {
   String(intervalService),
   String(displayCycleCountMax),
   String(debounceCountMax),
   String(blowerShutDownMax),
   String(blowerDelayMax),
  };
  // Update Display
  lcdUpdateByRow("","",configLoopLabel[0] + configLoopValue[0],"");
  int loopCount = 0;
  char intDisplay[40];
  bool loopConfig = true;
  String keyStroke = "";
  do
  {
    char key = GetKeyStroke();
    if (key != NO_KEY)
    { 
      if (key == '*') 
      {
       loopCount += 1;
       if (loopCount == configLoopSize) loopCount = 0;
       lcdUpdateByRow("","",configLoopLabel[loopCount] + configLoopValue[loopCount],"          ");
       keyStroke ="";
      }
      else if (key == 'B') {keyStroke =""; loopConfig = false; }

      else if (isdigit(key)) 
      { 
       keyStroke += key;
       lcdUpdateByRow("","","",keyStroke);
      }
      else if (key == '#')
      {
        // write values
        switch (loopCount)
        {
          case 0: intervalService   = keyStroke.toInt(); configLoopValue[loopCount] = keyStroke   ;break;
          case 1: displayCycleCountMax   = keyStroke.toInt(); configLoopValue[loopCount] = keyStroke   ;break;
          case 2: debounceCountMax  = keyStroke.toInt(); configLoopValue[loopCount] = keyStroke   ;break;
          case 3: blowerShutDownMax = keyStroke.toInt(); configLoopValue[loopCount] = keyStroke   ;break;
          case 4: blowerDelayMax    = keyStroke.toInt(); configLoopValue[loopCount] = keyStroke   ;break;
        }
        
        handleEEPROM(updateEEPROM);
        lcdUpdateByRow("","","","EEPROM Updated");
        keyStroke ="";
        lcdUpdateByRow("","","","          ");
      }
      else
      {
        keyStroke ="";
        lcdUpdateByRow("","","","          ");
      }
      lcdUpdateByRow("","",configLoopLabel[loopCount] + configLoopValue[loopCount],"");
    }
 } while (loopConfig == true);
}

//**************************************
//**************************************
//*** Timer Interrupt handling *********
//**************************************
//**************************************


//*** CheckTimer ***
void CheckTimer()
/*  This procedure is a seudo
 *   interrupt timer
 *   Updates event.Time in seconds for general  use
 *   DOES NOT check for Roll over yetr
 *   Updates Events:  
 *      eventDisplay
 */
{ 
  //Update Cycle Counters for this pass
  event.displayCycleCount += 1;
  Serial.print("CheckTimer ");Serial.print(millis())  ;

  
  // Update event.Time
  event.Time  += intervalService/10;  //interval Service in 1/100 sec
    

  // *** Trigger any Events as needed ***

  //Update Display
  if(event.displayCycleCount >=  displayCycleCountMax)
  {event.State = eventDisplay; event.displayCycleCount = 0;}
}

void setupTimerInterrupt()
{
  FlexiTimer2::set(1000, CheckTimer); // 1000ms period
  FlexiTimer2::start();
}

//**************************************
//**************************************
//*****  OTHER STUFF        ************
//**************************************
//************************************** 
/* These procedures are misc setup calls
 *  
 */

//*** Setup Serial Port
void setupSerialPort()
{
  Serial.begin(9600);
  Serial.println(" ");
  Serial.println("*** Run Setup ***");
}

//***  Setup SoftwareVersion
void setupINIT()
{
  // New SW Check
  bool swVer;
  String swNote = "** NEW VERSION DETECTED ** ";
  swVer = softwareVerEEPROM();
  if (swVer == true) swNote = "  //               ";
  lcdUpdateByRow
  (
  //"                    "  
    "*Dust Collection SW*",
    "**Version " + String(softwareVersion),
    swNote,
    "Startup Test Press A" 
  );

  // Wait for User Input
  bool startUpTest = true;
  do
  {
    char key = GetKeyStroke();
    if (key != NO_KEY) 
    {
      if (key == 'A')
      {
        // Run System Test
        testEEPROM();
        testLCDDisplay();
        testKeyPad();
        testLED();
        testButtons(); 
      }
      else
      {
        handleEEPROM(readEEPROM);
      }
      startUpTest=false;
    }
  } while (startUpTest == true);
 
}




 /****************************************************************************************************************
 * 
 *                  Setup and Event Handler (Loop) 
 *   
 ***************************************************************************************************************/

void setup() 
{
  //config board
  setupSerialPort(); 
  setupLCD();
  setupLED();
  setupButton();
  setupINIT();  

  // prep system for runtime
  setupTimerInterrupt();
  updateDisplay(); // only updates row 1 & 2
  lcdClearRow(2);lcdClearRow(3);
  
}
 
void loop() 
{
  // FlexiTimer2 also sets event.State for timed events
  
  if (event.State == eventDisplay) {updateDisplay(); }

  // check for Keypad Stroke
  char key = GetKeyStroke();
  if(key != NO_KEY)
  {
    switch (key) 
    {
        case 'A': BlinkAllLeds(2); break; // blink led
        case 'B': changeConfiguration(); break;
        case 'C': maintenanceMode(); break;
    }
    updateDisplay();
    lcdClearRow(2);lcdClearRow(3);
  }
  
  // check for Button Press
  int btnEvent = CheckButtonEvent();  // toggle LED
  if (btnEvent != 99) {ledHandling(btnEvent,led_Toggle); btnEvent = 99; updateDisplay();lcdClearRow(2);lcdClearRow(3);}  

}
