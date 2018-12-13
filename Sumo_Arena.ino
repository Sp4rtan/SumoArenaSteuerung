#include <FastLED.h>
#include <power_mgt.h>
#include <pixeltypes.h>

#define RGBORDER RGB

// Digital IO pin buttons----------------------------------------------------------------------
#define BTN01_PIN   2       // Button 01 - BUZZER                               
#define BTN02_PIN   3       // Button 02 - UP                  //CHRISTIAN                      
#define BTN03_PIN   4       // Button 03 - DOWN                //CHRISTIAN                         
//---------------------------------------------------------------------------------------------

// Digital LED Pins----------------------------------------------------------------------------
#define DATA_PIN 5        // Arena
#define DATA_PIN2 6       // Pillar
//---------------------------------------------------------------------------------------------

// Reserved for Serial Communication
// SS     10
// MISO   11
// MOSI   12
// SCK    13

// LED Anzahl
#define NUM_LEDS  300        // Pixel Count Arena
#define NUM_LEDS2 50         // Pixel Count Pillar

//NEO PIXELS-----------------------------------------------------------------------------------
CRGB leds[NUM_LEDS];
CRGB leds2[NUM_LEDS2];
CRGB pixelColor;
//---------------------------------------------------------------------------------------------

// Pillar Control------------------------------------------------------------------------------
#define ENDSTOP1   7       // Enstop Oben                      //CHRISTIAN
#define ENDSTOP2   8       // Endstop Unten                    //CHRISTIAN     
#define Motor1    A0       // Motor hoch                       //CHRISTIAN
#define Motro2    A1       // Motor runter                     //CHRISTIAN
                          
//---------------------------------------------------------------------------------------------


//TIMING----------------------------------------------------------------------------------------------
unsigned long 
  currentMillis = 0,
  thisMillis = 0,
 
  thisMillisRainbow = 0,
  prevMillisRainbow = 0,

  thisMillisRED = 0,
  prevMillisRED = 0,
  
  thisMillisGREEN = 0,
  prevMillisGREEN = 0,

  thisMillisBLACK = 0,
  prevMillisBLACK = 0,
  
  thisMillisFLASH = 0,
  prevMillisFLASH = 0,

  thisMillisFade = 0,
  prevMillisFade = 0,
  
  prevMillisSTROBE = 0,

  thisMillisFAIL = 0,
  prevMillisFAIL = 0;

  bool flash = false,
      strobe = false;
  int stage = 1;
//---------------------------------------------------------------------------------------------

//BUTTONS---------------------------------------------------------------------------------------------
bool 
  button01_oldState = HIGH,                       // button debounce variable
  button02_oldState = HIGH,                       // button debounce variable
  button03_oldState = HIGH;                       // button debounce variable

byte
  debounceDelay = 20;                             // time to wait for button change

bool
  button01 = false,                               // Initial button states
  button02 = false,                               //
  button03 = false;                               //
//--------------------------------------------------------------------------------------------

//FUNCTIONS-----------------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------------------------
void ReadInput();
void InterpretInput();

//--------------------------------------------------------------------------------------------------------------------------------

//Arena Modus
enum arenaMode {STANDBY=1,START, FIGHT, CANCEL, FINISH};
arenaMode currentMode = STANDBY;

//CYLON DUAL MODE-------------------------------------------------------------------------------------
byte 
  cylonSpeed = 30,                                 // animation speed (higher = faster)                                 /* CHANGEABLE */
  cylonTails = 7,                                 // lenght of tails                                                    /* CHANGEABLE */
  cylonBarSizes = 7;                               // lenght of bars                                                    /* CHANGEABLE */
//---------------------------------------------------------------------------------------------

bool COUNTDOWN = false,
     failure = false,
     startSequence = true,
     finish = false;
     
int brightness = 0,
    count = 0;

unsigned long  intervalRED = 2000;

long  intervalGREEN = 2000,
      intervalBLACK = 2000,
      intervalRainbow = 5,
      intervalSTROBE = 500,
      intervalFLASH = 50,
      intervalFlow = 200,
      prevMillisFlow = 0,
      flashPause = 50,
      intervalFade = 10,
      intervalFAIL = 3000;

byte fadeColor = 1;


void setup() {
  FastLED.addLeds<WS2812B, DATA_PIN, RGBORDER>(leds, NUM_LEDS);

  set_max_power_in_volts_and_milliamps(5, 19000);                        //(U in v, I in mA)                             /* CHANGEABLE */

  pinMode(BTN01_PIN, INPUT_PULLUP);
  pinMode(BTN02_PIN, INPUT_PULLUP);
  pinMode(BTN03_PIN, INPUT_PULLUP);
  Serial.begin(250000);
  //analogReference(EXTERNAL);
  //analogReference(INTERNAL);

  show_at_max_brightness_for_power();                                   //FastLED.show();
}

void ColorWipe() {
  
}

void loop() {
  while (brightness < 255 && startSequence == true){
    thisMillisFade=millis();
    if(thisMillisFade - prevMillisFade >= intervalFade){
        brightness++;
        fill_solid( leds, NUM_LEDS, CRGB::Red);
        FastLED.setBrightness(brightness);
        show_at_max_brightness_for_power();
        prevMillisFade = thisMillisFade;
    }
  }
  startSequence = false;
  
  ReadInput();
  InterpretInput();

  //Aktueller Arena-Modus
  switch(currentMode){
    case STANDBY:                                               // Fill whole strip with color
      ColorFlow();

      if (button01) {
        currentMode = START;
      }
    break;
    case START:                                               // Fill whole strip with color
      Start();

      if (button01) {
        currentMode = CANCEL;
      }
      if (!COUNTDOWN) { 
        currentMode = FIGHT;
      }
    break;
    case FIGHT:                                               // Fill whole strip with color
      ColorFlow();

      if (button01) {
        currentMode = FINISH;
      }
    break;
    case CANCEL:                                               // Fill whole strip with color
      Fail();
      if (!failure) {
        currentMode = STANDBY;
      }
    break;
    case FINISH:                                               // Fill whole strip with color
      Finish();
      if (!finish) {
        currentMode = STANDBY;
      }
    break;
  }
  
//  if(!COUNTDOWN && !failure){
//    thisMillisFade=millis();
//    if(thisMillisFade - prevMillisFade >= intervalFade){
//      if(brightness < 255)
//        brightness++;
//      prevMillisFade = thisMillisFade;
//    }
//    //rainbow();
//    CylonDual();
//    //ColorFlow();
//  }
//  else if(COUNTDOWN && !failure){
//    Start();
//  }
//  else
//    Fail();
//    
  FastLED.setBrightness(brightness);
  show_at_max_brightness_for_power();   
}
void ColorFlow(){
  thisMillis=millis();
      if(thisMillis - prevMillisFlow >= intervalFlow){
        pixelColor = CHSV(fadeColor++, 255, 255);
        fill_solid( leds, NUM_LEDS, pixelColor);
        prevMillisFlow = thisMillis;
      }
}
      
void CylonDual(){
  fadeToBlackBy(leds, NUM_LEDS, cylonTails);       // dimm whole strip
  fill_solid(&(leds[beatsin8(cylonSpeed, 0, (NUM_LEDS/2)+1-cylonBarSizes)]), cylonBarSizes, CRGB::Green);
  fill_solid(&(leds[(NUM_LEDS-cylonBarSizes)-beatsin8(cylonSpeed, 0, (NUM_LEDS/2)+1-cylonBarSizes)]), cylonBarSizes, CRGB::Green);
}

void rainbow(){
  thisMillis=millis();
    if(thisMillisRainbow - prevMillisRainbow >= intervalRainbow){
      fadeColor++;
      prevMillisRainbow = thisMillis;
    }
  fill_rainbow(leds, NUM_LEDS, fadeColor);
}

void Strobe(){
  thisMillis=millis();
  if(thisMillis - prevMillisSTROBE >= intervalSTROBE){
    if(strobe == true){                                                                          // strobe interval
      fill_solid( leds, NUM_LEDS, CRGB::Yellow);       
      strobe = !strobe;
      count++;
    }
    else{                                                                                        // off interval
      fill_solid( leds, NUM_LEDS, CRGB::Black);
      strobe = !strobe;
    }    
    prevMillisSTROBE = thisMillis;
  }
}

void Flash(){
  thisMillis=millis();
  if(thisMillis - prevMillisFLASH >= intervalFLASH){
    if(flash == true){                                                                          // strobe interval
      fill_solid( leds, NUM_LEDS, CRGB::Red);       
      flash = !flash;
    }
    else{                                                                                        // off interval
      fill_solid( leds, NUM_LEDS, CRGB::Black);
      flash = !flash;
    }    
    prevMillisFLASH = thisMillis;
  }
}

void Start(){
  if(stage == 1){
    thisMillisRED=millis();
    fill_solid( leds, NUM_LEDS, CRGB::Red);
    if(thisMillisRED - prevMillisRED >= intervalRED){
      stage=2;
      prevMillisRED = thisMillisRED;
    }
  }

  if(stage == 2){
    Strobe();
    if(count > 3){
      count = 0;
      stage=3;
      prevMillisGREEN = millis();
    }
  }
  
  if(stage == 3){
    thisMillisGREEN=millis();
    fill_solid( leds, NUM_LEDS, CRGB::Green);
    if(thisMillisGREEN - prevMillisGREEN >= intervalGREEN){
      stage=4;
      prevMillisBLACK = millis();
    }
  }

  if(stage == 4){
    thisMillisBLACK=millis();
    fill_solid( leds, NUM_LEDS, CRGB::Black);
    if(thisMillisBLACK - prevMillisBLACK >= intervalBLACK){
      brightness = 0;
      stage = 1;
      COUNTDOWN = false;
    }
  }

  

}

void Fail(){
  thisMillisFAIL=millis();
  Flash();
  if(thisMillisFAIL - prevMillisFAIL >= intervalFAIL){
    brightness = 0;
    failure = false;
  }
}

void Finish(){
  thisMillisFAIL=millis();
  Flash();
  if(thisMillisFAIL - prevMillisFAIL >= intervalFAIL){
    brightness = 0;
    finish = false;
  }
}

void ReadInput(){

  bool button01_newState = digitalRead(BTN01_PIN);
  bool button02_newState = digitalRead(BTN02_PIN);
  bool button03_newState = digitalRead(BTN03_PIN);
//BUTTON 01------------------------------------------------------------------  
  // Check if state changed from high to low (button press).
  if (button01_newState == LOW && button01_oldState == HIGH) {
    // Short delay to debounce button.
    delay(debounceDelay);
    // Check if button is still low after debounce.
    button01_newState = digitalRead(BTN01_PIN);
    if (button01_newState == LOW)
      button01 = true;
  }
  else
    button01 = false;
   
  // Set the last button state to the old state.
  button01_oldState = button01_newState;
//---------------------------------------------------------------------------  
  
//BUTTON 02------------------------------------------------------------------   
  // Check if state changed from high to low (button press).
  if (button02_newState == LOW && button02_oldState == HIGH) {
    // Short delay to debounce button.
    delay(debounceDelay);
    // Check if button is still low after debounce.
    button02_newState = digitalRead(BTN02_PIN);
    if (button02_newState == LOW)
      button02 = true;
  }
  else
    button02 = false;
    
  // Set the last button state to the old state.
  button02_oldState = button02_newState;
//---------------------------------------------------------------------------    

//BUTTON 03------------------------------------------------------------------    
  // Check if state changed from high to low (button press).
  if (button03_newState == LOW && button03_oldState == HIGH) {
    // Short delay to debounce button.
    delay(debounceDelay);
    // Check if button is still low after debounce.
    button03_newState = digitalRead(BTN03_PIN);
    if (button03_newState == LOW)
      button03 = true;
  }
  else
    button03 = false;
    
  // Set the last button state to the old state.
  button03_oldState = button03_newState;
//---------------------------------------------------------------------------    
}

void InterpretInput(){
  if(button01 == true){
    if(stage == 1 && !COUNTDOWN){
     prevMillisRED = millis();
     COUNTDOWN = true;
    }else{
      Serial.print("FAIL!");
      COUNTDOWN = false;
      stage = 1;
      failure = true;
      prevMillisFAIL = millis();
    }
  }
  if(button02 == true){
    
  }

  if(button03 == true){
   
  }
}
