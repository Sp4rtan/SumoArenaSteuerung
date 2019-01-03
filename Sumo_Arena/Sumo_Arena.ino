#include <FastLED.h>
#include <power_mgt.h>
#include <pixeltypes.h>
#include <limits.h>

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
#define NUM_LEDS2 8         // Pixel Count Pillar

//NEO PIXELS-----------------------------------------------------------------------------------
CRGB leds[NUM_LEDS];
CRGB leds2[NUM_LEDS2];
CRGB pixelColor;
//---------------------------------------------------------------------------------------------

// Pillar Control------------------------------------------------------------------------------
#define ENDSTOP1   7       // Enstop Oben                      //CHRISTIAN
#define ENDSTOP2   8       // Endstop Unten                    //CHRISTIAN     
#define Motor1    A0       // Motor hoch                       //CHRISTIAN
#define Motor2    A1       // Motor runter                     //CHRISTIAN
                          
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
  prevMillisFAIL = 0,

  prevMillisBEACON = 0,
  fight_green_start = 0;

  bool flash = false,
      strobe = false;
  int start_stage = 1; // Zustaende waehrend der Startanimation
  int pixelPos = 0;

  byte fadeSpeed = 8;
//---------------------------------------------------------------------------------------------

//BUTTONS---------------------------------------------------------------------------------------------
bool buttonStates[5];
bool buttonFlanks[5];
long buttonActivation[5];

bool
  goUp = false,
  goDown = false;

byte
  debounceDelay = 20;                             // time to wait for button change
//--------------------------------------------------------------------------------------------

//FUNCTIONS-----------------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------------------------
void ReadInput();
void InterpretInput();
void moveObstacle();

//--------------------------------------------------------------------------------------------------------------------------------

//Arena Modus
enum arenaMode {STANDBY=1,START, FIGHT, STOPPING, FINISH};
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
      intervalFAIL = 3000,
      intervalBEACON = 90,
      interval_fight_start_green = 500;

byte fadeColor = 1;


void setup() {
  for (int i = 0; i < 5; i++) {
    buttonActivation[i] = 0;
  }
  
  FastLED.addLeds<WS2812B, DATA_PIN, RGBORDER>(leds, NUM_LEDS);
  FastLED.addLeds<WS2812B, DATA_PIN2, RGBORDER>(leds2, NUM_LEDS2);
  
  set_max_power_in_volts_and_milliamps(5, 19000);                        //(U in v, I in mA)                             /* CHANGEABLE */

  pinMode(BTN01_PIN, INPUT_PULLUP);
  pinMode(BTN02_PIN, INPUT_PULLUP);
  pinMode(BTN03_PIN, INPUT_PULLUP);
  Serial.begin(250000);
  //analogReference(EXTERNAL);
  //analogReference(INTERNAL);

  //show_at_max_brightness_for_power();                                   //FastLED.show();
  Serial.println("start");
  while (brightness < 255){
    thisMillisFade=millis();
    if(thisMillisFade - prevMillisFade >= intervalFade){
        brightness++;
        fill_solid( leds, NUM_LEDS, CRGB::Red);
        fill_solid( leds2, NUM_LEDS2, CRGB::Red);
        FastLED.setBrightness(brightness);
        show_at_max_brightness_for_power();
        prevMillisFade = thisMillisFade;
    }
  }
  Serial.println("finished setup");

//  fill_solid( leds2, NUM_LEDS2, CRGB::Black);
//  leds2[0] = CRGB::Orange;
//  leds2[4] = CRGB::Orange;
  
}

void ColorWipe() {
  
}

void loop() {
  ReadInput();
  InterpretInput();

  //Aktueller Arena-Modus
  switch(currentMode){
    case STANDBY:                                               // Fill whole strip with color
      ColorFlow();
      //CylonDual();
      if (buttonFlanks[0]) {
        currentMode = START;
      }
    break;
    case START:                                               // Fill whole strip with color
      Start();

      if (failure) {
        currentMode = STOPPING;
      } else if (!COUNTDOWN) { 
        currentMode = FIGHT;
        fight_green_start = millis();
      }
    break;
    case FIGHT:                                               // Fill whole strip with color
      Pulse();

      if (buttonFlanks[0]) {
        currentMode = FINISH;
      }
    break;
    case STOPPING:                                               // Fill whole strip with color
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

  moveObstacle();

  FastLED.setBrightness(brightness);
  show_at_max_brightness_for_power();   
}
void ColorFlow(){
  thisMillis=millis();
  if(thisMillis - prevMillisFlow >= intervalFlow){
    pixelColor = CHSV(fadeColor++, 255, 255);
    fill_solid( leds, NUM_LEDS, pixelColor);
    if(goUp || goDown){
      Beacon();
    }
    else{
      fill_solid( leds2, NUM_LEDS2, pixelColor);
    }
    prevMillisFlow = thisMillis;
  }
}
      
void CylonDual(){
  fadeToBlackBy(leds, NUM_LEDS, cylonTails);       // dimm whole strip
  if(goUp || goDown){
      Beacon();
  }
  else{   
    fadeToBlackBy(leds2, NUM_LEDS2, 150);       // dimm whole strip
    fill_solid(&(leds2[beatsin8(cylonSpeed, 0, (NUM_LEDS2/2)+1-cylonBarSizes)]), cylonBarSizes, CRGB::Green);
    fill_solid(&(leds2[(NUM_LEDS2-cylonBarSizes)-beatsin8(cylonSpeed, 0, (NUM_LEDS2/2)+1-cylonBarSizes)]), cylonBarSizes, CRGB::Green);
  }
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
  if(goUp || goDown){
      Beacon();
  }
  else{
    fill_rainbow(leds2, NUM_LEDS2, fadeColor);
  }
}

void Strobe(){
  thisMillis=millis();
  if(thisMillis - prevMillisSTROBE >= intervalSTROBE){
    if(strobe == true){                                                                          // strobe interval
      fill_solid( leds, NUM_LEDS, CRGB::Orange);
      if(goUp || goDown){
        Beacon();
      }
      else{   
        fill_solid( leds2, NUM_LEDS2, CRGB::Orange);      
      } 
      strobe = !strobe;
      count++;
    }
    else{                                                                                        // off interval
      fill_solid( leds, NUM_LEDS, CRGB::Black);
      if(goUp || goDown){
        Beacon();
      }
      else{
        fill_solid( leds2, NUM_LEDS2, CRGB::Black);
      }
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
      if(goUp || goDown){
        Beacon();
      }
      else{
        fill_solid( leds2, NUM_LEDS2, CRGB::Red); 
      }      
      flash = !flash;
    }
    else{                                                                                        // off interval
      fill_solid( leds, NUM_LEDS, CRGB::Black);
      if(goUp || goDown){
        Beacon();
      }
      else{
        fill_solid( leds2, NUM_LEDS2, CRGB::Black);
      }
      flash = !flash;
    }    
    prevMillisFLASH = thisMillis;
  }
}

void Pulse(){
  if (millis() - fight_green_start < interval_fight_start_green) {
     fill_solid( leds, NUM_LEDS, CRGB::Green);
     if(goUp || goDown){
      Beacon();
     }
     else{
      fill_solid( leds2, NUM_LEDS2, CRGB::Green);
     }
  } else {
    pixelColor = CHSV( 96, 255, beatsin8(fadeSpeed));
    fill_solid( leds, NUM_LEDS, pixelColor);
    if(goUp || goDown){
      Beacon();
    }
    else{
      fill_solid( leds2, NUM_LEDS2, pixelColor);
    }
  }
}
void Start(){
  /*
   *   if(buttonFlanks[0]){
    }else if (currentMode == START ) {
    }
  }
   */

  //Dont fail afer stage 2, thats the green stage
  if (COUNTDOWN && buttonFlanks[0]) {  
    Serial.print("FAIL!");
    COUNTDOWN = false;
    start_stage = 1;
    failure = true;
    prevMillisFAIL = millis();
  }

  if(start_stage == 1){
    if (!COUNTDOWN) {
        prevMillisRED = millis();
        COUNTDOWN = true;
    }

    thisMillisRED=millis();
    fill_solid( leds, NUM_LEDS, CRGB::Red);
    fill_solid( leds2, NUM_LEDS2, CRGB::Red);
    if(thisMillisRED - prevMillisRED >= intervalRED){
      start_stage=2;
      prevMillisRED = thisMillisRED;
    }
  }

  if(start_stage == 2){
    Strobe();
    if(count > 3){
      Serial.println("FIGHT!");
      count = 0;
      start_stage = 1;
      COUNTDOWN = false;
      prevMillisGREEN = millis();
    }
  }
}

void Fail(){
  thisMillisFAIL=millis();
  Flash();
  if(thisMillisFAIL - prevMillisFAIL >= intervalFAIL){
    //brightness = 0;
    failure = false;
  }
}

void Finish(){
  thisMillisFAIL=millis();
  Flash();
  if(thisMillisFAIL - prevMillisFAIL >= intervalFAIL){
    //brightness = 0;
    finish = false;
  }
}

void ReadInput(){
  debounceButton(0, BTN01_PIN);
  debounceButton(1, BTN02_PIN);
  debounceButton(2, BTN03_PIN);
  debounceButton(3, ENDSTOP1);
  debounceButton(4, ENDSTOP2);
}

void debounceButton(int buttonNum, int pinNum) {
  if (buttonNum == 0) {
    bool currentButtonState = digitalRead(pinNum);
    
    buttonFlanks[buttonNum] = false;
    if (buttonFlanks[buttonNum]) {
      buttonFlanks[buttonNum] = false;
      buttonActivation[buttonNum] = 0;
    }
    if (!(currentButtonState)) {
      if  (buttonStates[buttonNum] && (buttonActivation[buttonNum] == 0)) {
          buttonActivation[buttonNum] = millis();
      } 
      
      long long waited = millis() - buttonActivation[buttonNum];
      if (waited > debounceDelay && waited < debounceDelay * 2) {
        buttonFlanks[buttonNum] = true;
        buttonActivation[buttonNum] = 0;
      }
    } else {
      buttonActivation[buttonNum] = 0;
    }
    buttonStates[buttonNum] = currentButtonState;
  }
}

void InterpretInput(){
  if(buttonFlanks[1]){
    goUp = true;
  }

  if(buttonFlanks[2]){
    goDown = true;
  }
}

void moveObstacle() {
  if (buttonFlanks[3]) {
    goUp = false;
  }
  if (buttonFlanks[4]) {
    goDown = false;
  }

  if (goUp) {
    digitalWrite(Motor1, HIGH);
    digitalWrite(Motor2, LOW);
  } else if (goDown) {
    digitalWrite(Motor1, LOW);
    digitalWrite(Motor2, HIGH);
  }
}

void Beacon(){
  thisMillis=millis();
  if(thisMillis - prevMillisBEACON >= intervalBEACON){
    fadeToBlackBy(leds2, NUM_LEDS2, 150);       // dimm whole strip
    //fill_solid( leds2, NUM_LEDS2, CRGB::Black);
    fill_solid(&(leds2[pixelPos]), 1, CHSV( 35, 255, 255));  
    //fill_solid(&(leds2[pixelPos]), 1, CRGB::Orange);*/
    pixelPos++;

    prevMillisBEACON = thisMillis;
    
    if(pixelPos >= NUM_LEDS2)
      pixelPos = 0;
  }
}
