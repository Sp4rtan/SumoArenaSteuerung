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
  prevMillisFAIL = 0;

  bool flash = false,
      strobe = false;
  int start_stage = 1; // Zustaende waehrend der Startanimation

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
      intervalFAIL = 3000;

byte fadeColor = 1;


void setup() {
  for (int i = 0; i < 5; i++) {
    buttonActivation[i] = 0;
  }
  
  FastLED.addLeds<WS2812B, DATA_PIN, RGBORDER>(leds, NUM_LEDS);

  set_max_power_in_volts_and_milliamps(5, 19000);                        //(U in v, I in mA)                             /* CHANGEABLE */

  pinMode(BTN01_PIN, INPUT_PULLUP);
  pinMode(BTN02_PIN, INPUT_PULLUP);
  pinMode(BTN03_PIN, INPUT_PULLUP);
  Serial.begin(250000);
  //analogReference(EXTERNAL);
  //analogReference(INTERNAL);

  //show_at_max_brightness_for_power();                                   //FastLED.show();

  while (brightness < 255){
    thisMillisFade=millis();
    if(thisMillisFade - prevMillisFade >= intervalFade){
        brightness++;
        fill_solid( leds, NUM_LEDS, CRGB::Red);
        FastLED.setBrightness(brightness);
        show_at_max_brightness_for_power();
        prevMillisFade = thisMillisFade;
    }
  }
  Serial.println("finished setup");
}

void ColorWipe() {
  
}

void loop() {
  Serial.print("currentMode: ");
  Serial.println(currentMode);
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

void Pulse(){
  pixelColor = CHSV( 96, 255, beatsin8(fadeSpeed));
  fill_solid( leds, NUM_LEDS, pixelColor);
}
void Start(){
  if(start_stage == 1){
    thisMillisRED=millis();
    fill_solid( leds, NUM_LEDS, CRGB::Red);
    if(thisMillisRED - prevMillisRED >= intervalRED){
      start_stage=2;
      prevMillisRED = thisMillisRED;
    }
  }

  if(start_stage == 2){
    Strobe();
    if(count > 3){
      count = 0;
      start_stage=3;
      prevMillisGREEN = millis();
    }
  }
  
  if(start_stage == 3){
    thisMillisGREEN=millis();
    fill_solid( leds, NUM_LEDS, CRGB::Green);
    if(thisMillisGREEN - prevMillisGREEN >= intervalGREEN){
      start_stage=4;
      prevMillisBLACK = millis();
    }
  }

  if(start_stage == 4){
    thisMillisBLACK=millis();
    fill_solid( leds, NUM_LEDS, CRGB::Black);
    if(thisMillisBLACK - prevMillisBLACK >= intervalBLACK){
      //brightness = 0;
      start_stage = 1;
      COUNTDOWN = false;
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
    Serial.print("currentButtonState: ");
    Serial.println(currentButtonState);
    Serial.print("buttonStates: ");
    Serial.println(buttonStates[0]);
    
    buttonFlanks[buttonNum] = false;
    if (buttonFlanks[buttonNum]) {
      buttonFlanks[buttonNum] = false;
      buttonActivation[buttonNum] = 0;
    }
    if (!(currentButtonState)) {
      if  (buttonStates[buttonNum] && (buttonActivation[buttonNum] == 0)) {
          buttonActivation[buttonNum] = millis();
          Serial.println("SEETT");
      } 
      
      long long waited = millis() - buttonActivation[buttonNum];
      Serial.print("waited");
      Serial.println((long)waited);
      Serial.print("activation");
      Serial.println(buttonActivation[buttonNum]);
      Serial.print("millis");
      Serial.println(millis());
      if (waited > debounceDelay && waited < debounceDelay * 2) {
        buttonFlanks[buttonNum] = true;
        buttonActivation[buttonNum] = 0;
      }
    } else {
      buttonActivation[buttonNum] = 0;
    }
    Serial.print("buttonFlanks: ");
    Serial.println(buttonFlanks[0]);
    buttonStates[buttonNum] = currentButtonState;
  }
}

void InterpretInput(){
  if(buttonFlanks[0]){
    if(start_stage == 1 && !COUNTDOWN){
     prevMillisRED = millis();
     COUNTDOWN = true;
    }else{
      Serial.print("FAIL!");
      COUNTDOWN = false;
      start_stage = 1;
      failure = true;
      prevMillisFAIL = millis();
    }
  }
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
