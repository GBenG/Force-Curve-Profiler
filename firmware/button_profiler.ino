#include <Arduino.h>
#include "BasicStepperDriver.h"
#include "HX711.h"
#include <stdlib.h>
#include <GyverOLED.h>
#include <avr/wdt.h>

//Config print
#define MAX_STRING_LENGTH 24   // Define a maximum buffer size for the string

// Motor steps per revolution. Most steppers are 200 steps or 1.8 degrees/step
#define MOTOR_STEPS 200
#define RPM 120

// Since microstepping is set externally, make sure this matches the selected mode
// If it doesn't, the motor will move at a different RPM than chosen
// 1=full step, 2=half step etc.
#define MICROSTEPS 16

// All the wires needed for full functionality
#define DIR 8
#define STEP 9
#define SLEEP 10
BasicStepperDriver stepper(MOTOR_STEPS, DIR, STEP, SLEEP);

// HX711 circuit wiring
HX711 scale;
const int LOADCELL_DOUT_PIN = 3;
const int LOADCELL_SCK_PIN = 4;

//Display
GyverOLED<SSD1306_128x64, OLED_BUFFER> oled;

//Input
#define HOME_PIN A0
#define BUTN_PIN A1
#define SYNC_PIN A2
#define SYST_PIN 12

//Led
const int ledPin = 13;    // the number of the LED pin

//3D cube screensaver
double vectors[8][3] = {{20, 20, 20}, { -20, 20, 20}, { -20, -20, 20}, {20, -20, 20}, {20, 20, -20}, { -20, 20, -20}, { -20, -20, -20}, {20, -20, -20}};
double perspective = 100.0f;
int deltaX, deltaY, deltaZ, iter = 0;

//Config
//#define START_POSITION (-2100)    //With kycaps
#define START_POSITION (-3240)    //Without kycaps
#define SCAN_DEPTH      (500)     //Scan Depth (change also in PC App)


void setup() {
    Serial.begin(115200);
    Serial.println("Force-Curve Profiler :: SPS TECH :: 2024");

    //Input
    pinMode(HOME_PIN, INPUT_PULLUP);
    pinMode(BUTN_PIN, INPUT_PULLUP);
    pinMode(SYNC_PIN, INPUT_PULLUP);
    pinMode(SYST_PIN, INPUT_PULLUP);

    // initialize the LED pin as an output:
    pinMode(ledPin, OUTPUT);

    // Initialize library with data output pin, clock input pin and gain factor.
    // Channel selection is made by passing the appropriate gain:
    // - With a gain factor of 64 or 128, channel A is selected
    // - With a gain factor of 32, channel B is selected
    // By omitting the gain factor parameter, the library
    // default "128" (Channel A) is used here.
    scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
    scale.set_scale(2280.f);                      // this value is obtained by calibrating the scale with known weights; see the README for details
    scale.tare();               // reset the scale to 0
  
    //Stepper
    stepper.begin(RPM, MICROSTEPS);
    stepper.setEnableActiveState(LOW);

    //Display
    oled.init();
    Wire.setClock(800000L);

    oled.setScale(2);
    oled.setCursorXY(15, 20);
    oled.print("SPS TECH");
    oled.setScale(1);
    oled.setCursorXY(50, 40);
    oled.print("2024");
    oled.update();
    
    delay(3000);

    park_home();
}


void loop() {

    //Wait for start
    while(digitalRead(BUTN_PIN) == HIGH){
      screensaver();
    }

    //Prepare allowed
    oled.setScale(2);
    oled.setCursorXY(10, 20);
    oled.print("GET READY");
    oled.update();

    //Goto start position
    stepper.enable();
    stepper.rotate(START_POSITION);
    stepper.disable();

    //Wait for start
    while(digitalRead(BUTN_PIN) == HIGH){
      screensaver();
    }

    //Scan allowed
    //Tare double check
    if(scale.get_units(10) < 0){
      oled.clear();
      oled.setScale(2);
      oled.setCursorXY(40, 20);
      oled.print("TARE");
      oled.update();
      scale.tare();
    }
    
    stepper.enable();

    //Press
    for(uint16_t steps = 0; steps<SCAN_DEPTH; steps++){

      stepper.rotate(-1);

      float force = scale.get_units(1);
      if(force < 0) force = 0;

      Serial.print(steps);
      Serial.print("\t");
      Serial.print(force);
      Serial.println("\tPRESS");

      oled.clear();
      oled.setScale(1);
      oled.setCursorXY(0, 0);
      oled.print(steps);

      oled.setScale(3);
      if(force < 10 ){
        oled.setCursorXY(30, 20);
      }else if(force > 100 ){
        oled.setCursorXY(10, 20);
      }else{
        oled.setCursorXY(20, 20);
      }
      oled.print(force);
      oled.update();

      if( digitalRead(BUTN_PIN) == LOW ){
        reboot();
      }
    }

    //Release
    for(uint16_t steps = SCAN_DEPTH; steps>0; steps--){

      stepper.rotate(1);

      float force = scale.get_units(1);
      if(force < 0) force = 0;

      Serial.print(steps);
      Serial.print("\t");
      Serial.print(force);
      Serial.println("\tRELEASE");

      oled.clear();
      oled.setScale(1);
      oled.setCursorXY(0, 0);
      oled.print(steps);

      oled.setScale(3);
      if(force < 10 ){
        oled.setCursorXY(30, 20);
      }else if(force > 100 ){
        oled.setCursorXY(10, 20);
      }else{
        oled.setCursorXY(20, 20);
      }
      oled.print(force);
      oled.update();

      if( digitalRead(BUTN_PIN) == LOW ){
        reboot();
      }
    }

    park_home();
    stepper.disable();
    oled.setScale(2);
    oled.setCursorXY(40, 20);
    oled.print("TARE");
    oled.update();
    scale.tare();
    oled.clear();
    oled.update(); 
    
    while(digitalRead(BUTN_PIN) != LOW){
      screensaver();
    };
}


void park_home(){
    oled.clear();
    oled.setScale(2);
    oled.setCursorXY(40, 20);
    oled.print("HOME");
    oled.update();

    stepper.enable();
    while(digitalRead(HOME_PIN) == HIGH){
        stepper.rotate(1);
        digitalWrite(ledPin, HIGH);
        digitalWrite(ledPin, LOW);
    }
    stepper.disable();
    oled.clear();
    oled.update(); 
}

//Screensaver functions

void screensaver(){
  oled.clear();
  oled.home();
  drawVectors();
  if (iter == 0) {
    deltaX = random(7) - 3;
    deltaY = random(7) - 3;
    deltaZ = random(7) - 3;
    iter   = random(250) + 5;
  }
  rotateX(deltaX);
  rotateY(deltaY);
  rotateZ(deltaZ);
  iter--;
  oled.update();
}

int translateX(double x, double z) {
  return (int)((x + 64) + (z * (x / perspective)));
}

int translateY(double y, double z) {
  return (int)((y + 32) + (z * (y / perspective)));
}

void rotateX(int angle) {
  double rad, cosa, sina, Yn, Zn;
  rad = angle * PI / 180;
  cosa = cos(rad);
  sina = sin(rad);
  for (int i = 0; i < 8; i++) {
    Yn = (vectors[i][1] * cosa) - (vectors[i][2] * sina);
    Zn = (vectors[i][1] * sina) + (vectors[i][2] * cosa);
    vectors[i][1] = Yn;
    vectors[i][2] = Zn;
  }
}

void rotateY(int angle) {
  double rad, cosa, sina, Xn, Zn;
  rad = angle * PI / 180;
  cosa = cos(rad);
  sina = sin(rad);
  for (int i = 0; i < 8; i++)
  {
    Xn = (vectors[i][0] * cosa) - (vectors[i][2] * sina);
    Zn = (vectors[i][0] * sina) + (vectors[i][2] * cosa);
    vectors[i][0] = Xn;
    vectors[i][2] = Zn;
  }
}

void rotateZ(int angle) {
  double rad, cosa, sina, Xn, Yn;
  rad = angle * PI / 180;
  cosa = cos(rad);
  sina = sin(rad);
  for (int i = 0; i < 8; i++) {
    Xn = (vectors[i][0] * cosa) - (vectors[i][1] * sina);
    Yn = (vectors[i][0] * sina) + (vectors[i][1] * cosa);
    vectors[i][0] = Xn;
    vectors[i][1] = Yn;
  }
}

void drawVectors() {
  oled.line(translateX(vectors[0][0], vectors[0][2]), translateY(vectors[0][1], vectors[0][2]), translateX(vectors[1][0], vectors[1][2]), translateY(vectors[1][1], vectors[1][2]));
  oled.line(translateX(vectors[1][0], vectors[1][2]), translateY(vectors[1][1], vectors[1][2]), translateX(vectors[2][0], vectors[2][2]), translateY(vectors[2][1], vectors[2][2]));
  oled.line(translateX(vectors[2][0], vectors[2][2]), translateY(vectors[2][1], vectors[2][2]), translateX(vectors[3][0], vectors[3][2]), translateY(vectors[3][1], vectors[3][2]));
  oled.line(translateX(vectors[3][0], vectors[3][2]), translateY(vectors[3][1], vectors[3][2]), translateX(vectors[0][0], vectors[0][2]), translateY(vectors[0][1], vectors[0][2]));
  oled.line(translateX(vectors[4][0], vectors[4][2]), translateY(vectors[4][1], vectors[4][2]), translateX(vectors[5][0], vectors[5][2]), translateY(vectors[5][1], vectors[5][2]));
  oled.line(translateX(vectors[5][0], vectors[5][2]), translateY(vectors[5][1], vectors[5][2]), translateX(vectors[6][0], vectors[6][2]), translateY(vectors[6][1], vectors[6][2]));
  oled.line(translateX(vectors[6][0], vectors[6][2]), translateY(vectors[6][1], vectors[6][2]), translateX(vectors[7][0], vectors[7][2]), translateY(vectors[7][1], vectors[7][2]));
  oled.line(translateX(vectors[7][0], vectors[7][2]), translateY(vectors[7][1], vectors[7][2]), translateX(vectors[4][0], vectors[4][2]), translateY(vectors[4][1], vectors[4][2]));
  oled.line(translateX(vectors[0][0], vectors[0][2]), translateY(vectors[0][1], vectors[0][2]), translateX(vectors[4][0], vectors[4][2]), translateY(vectors[4][1], vectors[4][2]));
  oled.line(translateX(vectors[1][0], vectors[1][2]), translateY(vectors[1][1], vectors[1][2]), translateX(vectors[5][0], vectors[5][2]), translateY(vectors[5][1], vectors[5][2]));
  oled.line(translateX(vectors[2][0], vectors[2][2]), translateY(vectors[2][1], vectors[2][2]), translateX(vectors[6][0], vectors[6][2]), translateY(vectors[6][1], vectors[6][2]));
  oled.line(translateX(vectors[3][0], vectors[3][2]), translateY(vectors[3][1], vectors[3][2]), translateX(vectors[7][0], vectors[7][2]), translateY(vectors[7][1], vectors[7][2]));
}

//SW Reboot
void reboot() {
  wdt_disable();
  wdt_enable(WDTO_15MS);
  while (1) {}
}
