
//*** UNO Board ***
/* Details:
 * Memory: Flash 32K, SRAM 2K  EEPROM 1K
 * Timer1 is used for Servo
 * 
 */
 
//**********************************
//*****  INCLUDES  *****************
//**********************************
#include <Wire.h>
#include <RTClib.h>
#include <EEPROM.h>
#include <FlexiTimer2.h>

//**************************************
// SOFTWARE VERSION
// Must be an INTERGER (easier to store in EEPROM)
//**************************************
int softwareVersion = 21 ;


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

//*** Device
  #define lights 0 
  #define compressor 1 
  #define numButtons 2

//*** Event Codes
  #define no_event 99

//*** Device States
  #define state_OFF 0
  #define state_ON 1
  #define state_Toggle 2

//*** button State
  #define btn_OFF 1  // Buttons are Failsafe
  #define btn_ON 0

//*** EEPROM
  #define clearEEPROM 0
  #define readEEPROM  1
  #define writeEEPROM  2
  #define updateEEPROM 3
  #define printEEPROM 4

//*** RTC
  #define RTCChangeTime 1
  #define RTCChangeDate 2

//*** I/O Defined
 #define lightButton 13
 #define lightLED 12
 #define compressorButton 11
 #define compressorLED 10
 #define lightPowerRelay 9
 #define compressorPowerRelay 8
 
 
 
//*** User Configurable Variables
/* Need to add to serial Command and LCD Maintenance mode */
/* to be able to edit                                     */

  byte intervalService = 10;  //Timer Service Interval in 1/100 second
  byte displayCycleCountMax = 2;  //INTERVAL of intervalService
  byte debounceCountMax = 3; //button debounce
  int compressorONTime = 60; //minutes
  int lightOffTime = 230; //  minutes past midnight (1440 max)


  
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
ledDef led[numButtons] = {{0,lightLED}, {0,compressorLED}}; 


//*** ledHandling
void ledHandling( byte ledNum, byte ledState)
/* Toggles the request LED ON or OFF
 *  This procedure is the PHYSICAL interface to the LEDS
 *  ledON: 0 = OFF, 1 = ON, 2 = SWITCH
 */
{  
  switch (ledState)
  {
    case state_OFF: {digitalWrite (led[ledNum].Output,HIGH); led[ledNum].State = state_OFF ; break;}
    case state_ON : {digitalWrite (led[ledNum].Output,LOW) ; led[ledNum].State = state_ON  ; break;}
    case state_Toggle:
    {  // Toogle
      if (led[ledNum].State == state_OFF) {digitalWrite (led[ledNum].Output,LOW) ; led[ledNum].State = state_ON  ; break;}
      else                                {digitalWrite (led[ledNum].Output,HIGH); led[ledNum].State = state_OFF ; break;}
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
   for (byte i = 0; i< numButtons ; i++) {ledHandling(i,state_ON) ;}
   delay(500);
   for (byte i = 0; i< numButtons ; i++) {ledHandling(i,state_OFF);}
   delay(500);
 }
}

//*** setupLED    
void setupLED()
/* This procedure is called from Setup
 *  and handles linking LED to UNO
 */
{
    for (byte j = 0; j<=numButtons; j++)
    { 
      pinMode(led[j].Output,OUTPUT);
      ledHandling(j,state_OFF);  
    }
    BlinkAllLeds(2);
}

//**************************************
//**************************************
//****** Power Relay HANDLING **********
//**************************************
//**************************************
/* THis procedure PHYSICALLY handles the power relays for
 *  the lights and compressor
 *  THis procedure alos chnage the LED state as needed
 */
typedef struct
{ byte State ; byte Output; } powerRelayDef;

powerRelayDef powerRelay[numButtons] = {{0,lightPowerRelay}, {0,compressorPowerRelay}};

//*** PowerRelayHandling
void PowerRelayHandling ( byte device , byte powerRelayState)
/* Toggles the request PowerRelay ON or OFF
 *  This procedure is the PHYSICAL interface to the relays
 */
{  
  switch (powerRelayState)
  {
    case state_OFF:        {digitalWrite (powerRelay[device].Output,HIGH); powerRelay[device].State= state_OFF ; ledHandling(device,state_OFF); break;}
    case state_ON:         {digitalWrite (powerRelay[device].Output,LOW) ; powerRelay[device].State= state_ON  ; ledHandling(device,state_ON) ; break;}
    case state_Toggle:
    {
      if (powerRelay[device].State= state_OFF)   
            {digitalWrite (powerRelay[device].Output,HIGH); powerRelay[device].State= state_OFF ; ledHandling(device,state_OFF); break;}
      else  {digitalWrite (powerRelay[device].Output,HIGH); powerRelay[device].State= state_ON  ; ledHandling(device,state_ON ); break;}
      break;
    }
  }
}  



//*** setupPowerRelay    
void setupPowerRelay()
/* This procedure is called from Setup
 *  and handles linking LED to UNO
 */
{

  for (byte j = 0; j<=numButtons; j++)
  { 
    pinMode(led[j].Output,OUTPUT);
    PowerRelayHandling(j,state_OFF);  
  } 
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
{ {0,0,0,lightButton}, {0,0,0,compressorButton}};  //updated for protoshield

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

//**************************************
//**************************************
//***   OLED LCD Handlng    ************
//**************************************
//**************************************
/* This procedure PHYSICALLY the OLED
 *  
 *  -Size: 0.96inch
 *  -Resolution: 128X64
 *  -Viewing angle: greater than 160 degrees
 *  -Supported platforms: for arduino, 51 series, MSP430 series, STIM32 / 2, SCR chips
 *  -Low power consumption: 0.04W during normal operation
 *  -Support wide voltage: 3.3V-5V DC
 *  -Driver IC: SSD1306
 *  -Communication: IIC, only two I / O ports
 *  -Interface: VCC: 3.3-5V; GND: Ground; SCL: Serial Clock; SDA: Serial Data
*/ 


//**************************************
//**************************************
//***  RT Clock Handling    ************
//**************************************
//**************************************
/* This procedure PHYSICALLY the 
 *  Real TIme Clock
 *  
 *  AMAZOM Description
 *  The 24C32 address can be modified by shorting A0 / A1 / A2. The default address is 0x57
 *
 *  struct ts {
 *    byte Hour;
 *    byte Minute;
 *    byte Second;
 *    byte Day;
 *    byte DayofWeek; // Sunday is day 0 
 *    byte Month;     // Jan is month 0
 *    byte Year;      // the Year minus 1900  
 */

// RTC_DS3231  clock;

int lightAutoOffTime;
int compressorAutoOffTime;

void updateRTC( String sFunc, int iOne, int iTwo, int iThree)
{
 //DateTime now = clock.now();
 //if      (sFunc == "time"){ clock.adjust(DateTime(now.year(),now.month(), now.day(), iOne, iTwo, iThree));}
 //else if (sFunc == "date"){ clock.adjust(DateTime(iThree, iTwo, iOne, now.hour(), now.minute(), now.second()));}
}


//*** CalcBaseTIme
 /* This procedure calcultes the Auto Off times  
  *  for the compressor and lights
  *  Time is an int of # of 10 minutes since midnight
 */
 void CalcBaseTime()
{
  //DateTime now = clock.now();
  if (lightOffTime > 1440) lightOffTime = 0;
  if (compressorONTime > 1440) compressorONTime = 120;
  lightAutoOffTime = lightOffTime;
  //compressorAutoOffTime = ((now.hour()*60) + (now.minute()) + compressorONTime);
  
}

 int CalcCuurentTime()
{
  int temp;
  //temp = ((now.hour()*60) + now.minute();
  return temp  ;
}

String tellTime()
{
  String temp = "";
  //DateTime now = clock.now();
  //temp = String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second()) + "  " + String(now.month()) +"/" + String(now.day()) + "/" + String(now.year());
  return temp;
}

void setupRTC()
{
  //wire.begin();
  //clock.begin();
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
/*  this procedure PHYSICALLY handles writing and reading the EEPROM
 *  EEPROM Address are hard coded here
 */
void handleEEPROM(int action)
{
  switch (action)
  { 
    case readEEPROM:
    {
      EEPROM.get (0, softwareVersion);
      EEPROM.get (2 , intervalService);
      EEPROM.get (4 , displayCycleCountMax) ;
      EEPROM.get (6 , debounceCountMax) ;
      EEPROM.get (8 , compressorONTime) ;
      EEPROM.get (10 , lightOffTime) ;  
      break;
    }

    case writeEEPROM:
    { 
      EEPROM.put (0, softwareVersion);
      EEPROM.put (2 , intervalService);
      EEPROM.put (4 , displayCycleCountMax) ;
      EEPROM.put (6 , debounceCountMax) ;
      EEPROM.put (8 , compressorONTime) ;
      EEPROM.put (10 , lightOffTime) ;  
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
      EEPROM.get (8, eCheck); if(eCheck != compressorONTime) EEPROM.put (0, compressorONTime);
      EEPROM.get (10, eCheck); if(eCheck != lightOffTime) EEPROM.put (0, lightOffTime);
      break;
    }

    case printEEPROM:
    {
      int col = 0;
      Serial.println("EEPROM Memory");
      for (int index = 0 ; index < EEPROM.length() ; index++) 
      {
          Serial.print (EEPROM[ index ]); Serial.print (" ")  ;
      }
      Serial.println("*************");
      break;
    }
  }
}

//*** setupEEPROM
void setupEEPROM()
{
  handleEEPROM(readEEPROM);
}


//**************************************
//**************************************
//***  Serial Port Handlng  ************
//**************************************
//**************************************
/* This procedure PHYSICALLY the serial port
 * 
 */

String serialPortMessage = "";

//*** readSerialPort
/* read any characters in the serial buffer 
 * and returns a true when a complete message is received 
*/
bool readSerialPort() 
{
  char endOfMessage = '\n';
 // check if any char in the serial buffer
  if (Serial.available() > 0) 
  {
    char serialPortChar = Serial.read();
    if (serialPortChar == endOfMessage) {return true; }
    else {serialPortMessage += String(serialPortChar) ; return false;}
  } 
  else {return false;}
}


//*** serialPortCommands
/* THis procedure handles the commands from the
 *  serial port
 * next step is error checking for properly formated message
 */
void serialPortCommands( String sCommand)
{
  // look for command Function (text before ':')
  String sFunction = "" ;   
  String sVars; 
  String sData;
  int indexFunction = 0;
  
  for(int  i= 0; i <= sCommand.length(); i++)
  {
    char oneChar = sCommand.charAt(i);
    if (oneChar == ':') indexFunction = i;
  }
  
  sFunction = sCommand.substring(0,indexFunction);
  
  if (sFunction == "now")  //******
  {
    Serial.println("now");
  }
  
  else if(sFunction == "time")  //******
  {
    Serial.println("time changed");
  }
  else if (sFunction == "date")  //******
  {
    Serial.println("date changed");
  }
  else if (sFunction == "PROM")  //******
  { // PROM:xxxxx where xxxxx is command
    String sProm = sCommand.substring(indexFunction+1,sCommand.length());
    Serial.println(sProm);
    if       (sProm == "read") {handleEEPROM(readEEPROM); Serial.println("!read");}
    else if  (sProm == "save") {handleEEPROM(writeEEPROM);Serial.println("!save");}
    else if  (sProm == "clear") {handleEEPROM(clearEEPROM);Serial.println("!clear");}
    else if  (sProm == "print") {handleEEPROM(printEEPROM);}
    else if  (sProm == "help") {Serial.println( "read save clear print help");}
    else {Serial.println("Invaild Command PROM");}
  }
  else if (sFunction == "vars")  //******
  {
    // break part message into vars and data
    // separate by '=' char
    
    int indexLoc = 0;
    for(int  i= indexFunction+1; i <= sCommand.length(); i++)
    {
      char oneChar = sCommand.charAt(i);
      if (oneChar == '=') indexLoc = i;
    }
    if (indexLoc !=0)
      {
        sVars = sCommand.substring(indexFunction+1,indexLoc);
        sData = sCommand.substring(indexLoc+1, sCommand.length());
      }
      else
      {
        sVars = sCommand.substring(indexFunction+1, sCommand.length());
        sData = "";
      }
      
    Serial.print("PROM:save to save changes");
      
    if       (sVars == "intervalService")      {intervalService = sData.toInt();}
    else if  (sVars == "displayCycleCountMax") {displayCycleCountMax = sData.toInt();}
    else if  (sVars == "debounceCountMax")     {debounceCountMax = sData.toInt();}
    else if  (sVars == "compressorONTime")     {compressorONTime = sData.toInt();}
    else if  (sVars == "lightOffTime")         {lightOffTime = sData.toInt();}
    else if  (sVars == "help")
    {
      Serial.print ("intervalService = ")     ; Serial.println( String(intervalService));
      Serial.print ("displayCycleCountMax = "); Serial.println( String(displayCycleCountMax));
      Serial.print ("debounceCountMax = ")    ; Serial.println( String(debounceCountMax));
      Serial.print ("compressorONTime = ")   ; Serial.println( String(compressorONTime));
      Serial.print ("lightOffTime = ")      ; Serial.println( String(lightOffTime));
    }
    else {Serial.println("Invaild vars command");}
  }
}


//*** setupSerialPort
void setupSerialPort() 
{
 Serial.begin(9600);
 Serial.println("**** Basement Control ****");
 Serial.print ("Software Ver: "); Serial.println(String(softwareVersion));
 Serial.println("**** Serial Port Enabled ****");
 Serial.println("Avaliable Serial Commands");
 Serial.println("now:");
 Serial.println("time:<hh.mm.ss>");
 Serial.println("date:<mm/dd/yy");
 Serial.println("PROM:<read><save><clear><print>");
 Serial.println("vars:<name>=<data> or <help>");
}

//**************************************
//**************************************
//*** Timer Interrupt handling *********
//**************************************
//**************************************

bool TimerEvent = false;
void CheckTimer ()
{
  TimerEvent = true ;
}


void setupTimerInterrupt()
{
  FlexiTimer2::set(1000, CheckTimer); // 1000ms period
  FlexiTimer2::start();
}



//****************************************************************
//************ setup and loop  ************************************
//****************************************************************

void setup()
{
  setupPowerRelay;
  setupSerialPort();
  setupEEPROM;
  setupLED;
  setupButton;
  
  //setupRTC();
}


void loop() 
{
    //Check Serial Port for command
  if(readSerialPort() == true) {serialPortCommands(serialPortMessage);serialPortMessage="";}

   // check for Button Press
   // if pressed, change state of device
  int btnEvent = CheckButtonEvent(); 
  if (btnEvent != no_event ) PowerRelayHandling(btnEvent,state_Toggle);

  // check Clock for turing off
  if (TimerEvent == true)
  {
    TimerEvent = false;
    int currentTimeinMinutes = CalcCuurentTime();
    if (currentTimeinMinutes >= lightAutoOffTime) {PowerRelayHandling(lights,state_OFF);}
    if (currentTimeinMinutes >= compressorAutoOffTime) {PowerRelayHandling(compressor,state_OFF);}
  }
}
