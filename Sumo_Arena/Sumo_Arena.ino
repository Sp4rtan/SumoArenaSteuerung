#include <FastLED.h>
#include <power_mgt.h>
#include <pixeltypes.h>
#include <limits.h>
#include <avr/wdt.h>

#define RGBORDER RGB

// Digital IO pin buttons----------------------------------------------------------------------
#define BTN_BUZZER 8
#define BTN_COUNTDOWN 7
#define BTN_POLLER 4
#define BTN_ONOFF A5

//---------------------------------------------------------------------------------------------

// Digital LED Pins----------------------------------------------------------------------------
#define DATA_PIN 5        // Arena
#define DATA_PIN2 6       // Pillar
#define STATUS_LED A4       // BUZZER                   // VERIFIZIEREN OB FREI!!!!
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
#define ENDSTOP1   2       // Enstop Oben                      //CHRISTIAN
#define ENDSTOP2   3         // Endstop Unten                    //CHRISTIAN
#define Motor1    A1       // Motor hoch                       //CHRISTIAN
#define Motor2    A0       // Motor runter                     //CHRISTIAN
#define MotorEnable 9

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
  fight_green_start = 0,

  prevMillisWaitMovement = 0,

  prevMillisTHECOUNT = 0;

  bool flash = false,
      strobe = false;
  int start_stage = 1; // Zustaende waehrend der Startanimation
  int pixelPos = 0;

  byte fadeSpeed = 8;
//---------------------------------------------------------------------------------------------

//BUTTONS---------------------------------------------------------------------------------------------
bool buttonStates[3];
bool buttonFlanks[3];
long buttonActivation[3];

bool
  goUp = false,
  goDown = true;

byte
  debounceDelay = 20;                             // time to wait for button change
//--------------------------------------------------------------------------------------------

//FUNCTIONS-----------------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------------------------
void ReadInput();
void moveObstacle();

//--------------------------------------------------------------------------------------------------------------------------------

//Arena Modus
enum arenaMode {STANDBY=1,START, FIGHT, STOPPING, FINISH, WAIT_MOVEMENT, TIMEOUT, START_SEQUENCE};
arenaMode currentMode = START_SEQUENCE;

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
      intervalFLASH = 100,
      intervalFlow = 200,
      prevMillisFlow = 0,
      flashPause = 50,
      intervalFade = 10,
      intervalFAIL = 3000,
      intervalTHECOUNT = 1000,                     // 1sec
      intervalBEACON = 90,
      interval_fight_start_green = 500;

byte fadeColor = 1;

void endstop1ISR() {
  if (!digitalRead(ENDSTOP1)) {
    goUp = false;
    digitalWrite(Motor1, LOW);
    digitalWrite(Motor2, LOW);
    analogWrite(MotorEnable, 0);
  } else {
    Serial.println("Nicht relevant?");
  }
}

void endstop2ISR() {
  if (!digitalRead(ENDSTOP2)) {
    goDown = false;
    digitalWrite(Motor1, LOW);
    digitalWrite(Motor2, LOW);
    analogWrite(MotorEnable, 0);
    Serial.println("Endstop 2");
  } else {
    Serial.println("Nicht relevant2?");
  }
}

void setup() {
  attachInterrupt(digitalPinToInterrupt(ENDSTOP1), endstop1ISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(ENDSTOP2), endstop2ISR, FALLING);
  digitalWrite(STATUS_LED, HIGH);      // BUZZER LED aus
  for (int i = 0; i < 5; i++) {
    buttonActivation[i] = 0;
  }

  FastLED.addLeds<WS2812B, DATA_PIN, RGBORDER>(leds, NUM_LEDS);
  FastLED.addLeds<WS2812B, DATA_PIN2, RGBORDER>(leds2, NUM_LEDS2);

  set_max_power_in_volts_and_milliamps(5, 19000);                        //(U in v, I in mA)                             /* CHANGEABLE */

  pinMode(BTN_BUZZER, INPUT_PULLUP);
  pinMode(BTN_COUNTDOWN, INPUT_PULLUP);
  pinMode(BTN_POLLER, INPUT_PULLUP);
  pinMode(BTN_ONOFF, INPUT_PULLUP);

  pinMode(Motor1, OUTPUT);
  pinMode(Motor2, OUTPUT);

  pinMode(ENDSTOP1, INPUT_PULLUP);
  pinMode(ENDSTOP2, INPUT_PULLUP);

  pinMode(STATUS_LED, OUTPUT);

  Serial.begin(9600);
  //analogReference(EXTERNAL);
  //analogReference(INTERNAL);

  //show_at_max_brightness_for_power();                                   //FastLED.show();
  Serial.println("start");
  Serial.println("finished setup");
}

void StartSequence() {
  thisMillisFade=millis();
  if(thisMillisFade - prevMillisFade >= intervalFade){
      brightness++;
      fill_solid( leds, NUM_LEDS, CRGB::Red);
      fill_solid( leds2, NUM_LEDS2, CRGB::Red);
      FastLED.setBrightness(brightness);
      show_at_max_brightness_for_power();
      prevMillisFade = thisMillisFade;
      if(brightness >= 255){
        startSequence = false;
        digitalWrite(STATUS_LED, HIGH);   // BUZZER LED ein
      }
  }

}

void loop() {
  if (digitalRead(BTN_ONOFF)) {
    Serial.println("BTN OFF!");
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    fill_solid(leds2, NUM_LEDS2, CRGB::Black);
    FastLED.setBrightness(0);
    show_at_max_brightness_for_power();

    digitalWrite(STATUS_LED, LOW);      // BUZZER LED aus
    if (digitalRead(ENDSTOP2)) {
      goDown = true;
    }
    while(digitalRead(BTN_ONOFF)) {
      if (goDown) {
        digitalWrite(Motor1, LOW);
        digitalWrite(Motor2, HIGH);
        analogWrite(MotorEnable, 150);
        Serial.println("go down");
      } else {
        digitalWrite(Motor1, LOW);
        digitalWrite(Motor2, LOW);
        analogWrite(MotorEnable, 0);
        Serial.println("stop");
      }
    }
    Serial.println("finished waiting for Lever");
    delay(100);
    wdt_enable(WDTO_15MS);
    for(;;){}
  }


  ReadInput();

  //Aktueller Arena-Modus
  switch(currentMode){
    case START_SEQUENCE:
      StartSequence();
      if (!startSequence) {
        currentMode = STANDBY;
      }
    break;
    case STANDBY:                                               // Fill whole strip with color
      ColorFlow();
      //CylonDual();
      if (buttonFlanks[0]) {
        //TODO:Send Notification
        currentMode = START;
      }
    break;
    case START:                                               // Fill whole strip with color
      Start();

      if (failure) {
        //TODO:Send Notification
        currentMode = STOPPING;
      } else if (!COUNTDOWN) {
        //TODO:Send Notification
        currentMode = FIGHT;
        fight_green_start = millis();
      }
    break;
    case FIGHT:                                               // Fill whole strip with color
      Pulse();

      if (buttonFlanks[0]) {
        currentMode = FINISH;
        failure = true;
        prevMillisFAIL = millis();
      }
      if (buttonFlanks[1]) {
        Serial.println("The Count");
        prevMillisWaitMovement = millis();
        currentMode = WAIT_MOVEMENT;
        //TODO:Send Notification
      }
    break;
    case WAIT_MOVEMENT:
      if (millis() - prevMillisWaitMovement > 10000) {
        currentMode = TIMEOUT;
      }
      TheCount();

      if (buttonFlanks[1]) {
        //TODO:Send Notification
        currentMode = FIGHT;
      }
    break;
    case TIMEOUT:
      Serial.println("TIMEOUT");
      //TODO:Send Notification
      currentMode = STANDBY;
    break;
    case STOPPING:                                               // Fill whole strip with color
      Fail();
      if (!failure) {
        currentMode = STANDBY;
      }
    break;
    case FINISH:   // Fill whole strip with color
      Fail();
      if (!failure) {
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
    if(strobe == true){
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
    if(flash == true){
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
    if(goUp || goDown){
      Beacon();
    }
    else{
      fill_solid( leds2, NUM_LEDS2, CRGB::Red);
    }
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
  debounceButton(0, BTN_BUZZER);
  debounceButton(1, BTN_COUNTDOWN);
  debounceButton(2, BTN_POLLER);
}

void debounceButton(int buttonNum, int pinNum) {
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

void moveObstacle() {
  if (!digitalRead(ENDSTOP1)) {
    goUp = false;
  }
  if (!digitalRead(ENDSTOP2)) {
    goDown = false;
  }

  if (buttonFlanks[2]) {
    Serial.println("DRIVE");
    if (!digitalRead(ENDSTOP1)) {
      goDown = true;
    } else if (!digitalRead(ENDSTOP2)) {
      goUp = true;
    } else {
      goDown = true;
    }
  }

  if (goUp) {
    digitalWrite(Motor1, HIGH);
    digitalWrite(Motor2, LOW);

    analogWrite(MotorEnable, 150);
  } else if (goDown) {
    digitalWrite(Motor1, LOW);
    digitalWrite(Motor2, HIGH);

    analogWrite(MotorEnable, 150);
  } else {
    digitalWrite(Motor1, LOW);
    digitalWrite(Motor2, LOW);

    analogWrite(MotorEnable, 0);
  }
}

void Beacon(){
  thisMillis=millis();
  fadeToBlackBy(leds2, NUM_LEDS2, 255);
  fill_solid(leds2, NUM_LEDS2, CHSV( 35, 255, 255));
  if(thisMillis - prevMillisBEACON >= intervalBEACON){
    pixelPos++;

    prevMillisBEACON = thisMillis;

    if(pixelPos >= NUM_LEDS2)
      pixelPos = 0;
  }
}

void TheCount(){
  thisMillis=millis();
  if(thisMillis - prevMillisTHECOUNT >= intervalTHECOUNT){
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
      fadeToBlackBy(leds, NUM_LEDS, 50);       // dimm whole strip
      if(goUp || goDown){
        Beacon();
      }
      else{
        fadeToBlackBy(leds2, NUM_LEDS2, 5);       // dimm whole strip
      }
      flash = !flash;
    }
    prevMillisTHECOUNT = thisMillis;
  }
}
