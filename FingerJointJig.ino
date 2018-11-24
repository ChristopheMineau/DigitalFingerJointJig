/* Arduino Code for the digital FingerJoint Jig, by C.Mineau, 2017 may 1st, www.labellenote.fr
 *  
 *  setup : 2 IR LED modules are used and mounted as a quadature encoder on a wheel drilled with equidistant holes  
 *  If distance between 2 holes = 2*PI, the 2 leds are spaced by 2*PI/4 (+n*2PI) 
 *  In my case : 1 screw turn = 54 sectors with 27 holes , spaced by 4 mm . Leds spaced by 10mm
 *  
 *  1 screw turn = carriage moves by 1 screw pych value.
 *  In my case screw= M10, pitch= 1.5 mm
 *  
 *  Led A (right hand looking at the end of the screw) connected to Pin 3
 *  Led B (left hand looking at the end of the screw) connected to Pin 2
 *  
 *  Positive values of displacements correspond to right motion of the carriage (clockwise motion of the crank)
 */

#include <math.h>

#define VERSION "digital Finger Joint Jig by C.Mineau - version 3.0"
#define SCREW_PITCH 1.5   // mm
#define BLADE_WIDTH 8  // mm
#define STEP_PER_TURN 27
#define LED_A 3   // right
#define LED_A_MASK B00001000
#define LED_B 2   // left
#define LED_B_MASK B00000100
#define ROTARY_POWER 13
#define ROTARY_READ A1
#define DEBUG_PIN A0
#define DEBUG_BUTTON_PIN A4
#define DEBUG_PIN_MASK B00000001  // on  PORTC
#define LEFT -1
#define RIGHT 1
#define UP 1
#define DOWN 0
#define DEBOUNCING_TIME 600UL //Debouncing Time in microseconds
#define DEBUG_NO
#define TRACE_NO

// CLASSES Definition
struct S_bouncedInput {
  byte pinMask;   // pin number limited to 0<7 to keep on PORTD
  byte state;     // current state ;
                  // 0 = UP stable
                  // 1 = UP but DOWN change in confirmation
                  // 2 = DOWN stable
                  // 3 = DOWN but UP change in confirmation
  unsigned long lastConfirmationTime;  // in microseconds
  unsigned long pollCount;   
  unsigned long stableStateChangeCount;
  char id;
  
  S_bouncedInput (int inputPin,char id) : // constructor
    pinMask(1 << inputPin),
    
    pollCount(0),
    stableStateChangeCount(0),
    lastConfirmationTime(micros()),
    id(id)
    {
      PIND & pinMask ? state=0 : state=2;   // stable UP or DOWN upon the pin level read at init
    }
    
  byte stableState() {
    if (state >= 2) return DOWN; else return UP;
  }
  
  bool poll() {    // polls and integrate the states. Return true if rising edge on the stable state
    
    byte   pinTransientState =  PIND & pinMask;
    unsigned long currentTime =  micros();

    bool result=false;
  
    switch (state) {  
      case 0:      // 0 = UP stable 
        if (pinTransientState) 
          lastConfirmationTime=currentTime;  // No change, update confirmation time
        else
          state=1;                        // attempt to go to DOWN state, to be confirmed  
        break; 
            
      case 1:     // 1 = UP but DOWN change in confirmation
         if (pinTransientState) { 
           state=0;         // DOWN Not confirmed, coming back to stable UP
           lastConfirmationTime=currentTime;
         }  else {
           if ((currentTime-lastConfirmationTime)>=DEBOUNCING_TIME) {        //  DOWN confirmed, moving to stable DOWN 
             state=2;
             lastConfirmationTime=currentTime;
             stableStateChangeCount++;
           }
         }  
         break;
               
      case 2:     // 2 = DOWN stable
        if (pinTransientState) 
          state=3;       // attempt to go to UP state, to be confirmed      
        else
          lastConfirmationTime=currentTime;  // No change, update confirmation time                    
        break;
         
     case 3:      // 3 = DOWN but UP change in confirmation 
        if (pinTransientState) {
          if ((currentTime-lastConfirmationTime)>=DEBOUNCING_TIME) {    //  UP confirmed, moving to stable UP 
            state=0;
            lastConfirmationTime=currentTime;
            stableStateChangeCount++;
            result=true;     // rising edge
          }
        } else {
          state=2;    // UP Not confirmed, coming back to stable DOWN
          lastConfirmationTime=currentTime;
        } 
        break;   
    }
#ifdef DEBUG    
    debug(currentTime,pinTransientState,result);
#endif    
    pollCount++;
    return result;

  }

  void debug(unsigned long t, byte ts,bool r) {
      Serial.print(" time=");
      Serial.print(t);
      Serial.print(" id=");
      Serial.print(id);
      Serial.print(" pinTransientState=");
      Serial.print(ts);
      Serial.print(" result=");
      Serial.print(r);
      Serial.print(" state=");
      Serial.print(state);
      Serial.print(" stableStateChangeCount=");
      Serial.print(stableStateChangeCount);
      Serial.println("");    
  }
};

struct LedBarGraph {
/*  LED Bargraph to display the carriage position on the eFingerJoint Jig
4 green Leds on the LEFT for the half finger on the left of a cut
3 red Leds in the middle for the cut indication
  - in case fingerWidth=bladeWidth : the three LEDS are lit up
  - in case the blade is narrower than the finger, led of Left means the blade is set for the Left edge of the cut
  and the right led is lit if the blade is set to cut the right edga of the cut.
  In case the finger is wider more than 2 times the width of the blade, the middle LED is lit in intermediate position. 

  * Constructor must be called giving the blade and finger width parameters
  * update method with a position parameter can be called in order to display this position on the bargraph
  * Position 0 correspond to the first cut, right edge of the cut.     

*/


  int  pinNumber[11] = { 4, 5, 6, 7, 8, 9, 10,11,12,16,17} ;  //  note:    A0 = 14 < A7=21
  //                     G  G  G  G  R  R  R  G  G  G  G        from right to left   
  //                     0  1  2  3  4  5  6   7  8  9 10
  //                     right                       left
  
  int greenLedIdx[8] = {7,8,9,10,0,1,2,3} ; //  order of green Leds to be lit according to distance to be moved

// all units expressed in steps in order to avoid any floating point calculation in runtime
  int bladeWidth;     // In Steps, can not be bigger than finger width
  int fingerWidth;    // In Steps, can be bigger than blade width
  long previousPosition;  // in Steps
  float greenStep;      // in steps
  
  void init(int bldeWidth, int fgerWidth){    // to be called in setup 
    bladeWidth=bldeWidth;
    fingerWidth=fgerWidth;
    greenStep = (float(fingerWidth+bladeWidth)/8); 
    for(int i=0;i<11;i++) {      
        pinMode(pinNumber[i], OUTPUT);
        digitalWrite(pinNumber[i],LOW);
    } 
    setRedLeds(0);    // The blade is supposed to be aligned with position 0 at initialization
  }
    
  void update(long position) {   //      to be called with a new position (in steps)
    
    if (position==previousPosition) {
      return;   // save time
    }
    int finger=floor(double(position)/(2*fingerWidth));         // number of full finger   (full finger = 1 cut + 1 finger)
    int offset= position-finger*2*fingerWidth;    // offset within the full finger 
    setRedLeds(offset);
    setGreenLeds(offset);
    previousPosition=position;
  }
    
  void setRedLeds(int offset) {    // offset is the displacement within the current finger cut
     approxR(offset,0) ? digitalWrite(pinNumber[4],HIGH):digitalWrite(pinNumber[4],LOW);    // right cut edge Led
     approxL(offset,fingerWidth-bladeWidth) ? digitalWrite(pinNumber[6],HIGH):digitalWrite(pinNumber[6],LOW);    // left cut edge Led
     offset <=fingerWidth-bladeWidth ? digitalWrite(pinNumber[5],HIGH):digitalWrite(pinNumber[5],LOW);    // middle cut Led
  }
   
  void setGreenLeds(int offset) {
        int g0 =fingerWidth-bladeWidth;  
        for (int i = 0; i<8; i++) {
          approx2(offset, g0+ i*greenStep) ? digitalWrite(pinNumber[greenLedIdx[i]],HIGH):digitalWrite(pinNumber[greenLedIdx[i]],LOW);  
        }        

  } 
  
  bool approxR(int p1, int p2) {   // checks if two positions are close enough (right edge)
    return  ((p1>=p2) && (p1-p2)<1) ?  true :  false;
  }  
  bool approxL(int p1, int p2) {   // checks if two positions are close enough  (left edge)
    return  ((p2>=p1) && (p2-p1)<1) ?  true :  false;
  }
  bool approx2(int p1, int p2) {   // checks if two positions are close enough
    return  ((p1>p2) && (p1-p2)<=greenStep) ?  true :  false;
  }

  void debug() {
    static bool alreadyPrint = false;
    if (!alreadyPrint) {   
      Serial.print(" bladeWidth=");
      Serial.print(bladeWidth);
      Serial.print(" fingerWidth=");
      Serial.print(fingerWidth);
      Serial.print(" greenStep=");
      Serial.print(greenStep);
      Serial.println("");  
    }
      alreadyPrint=true;
  }
    
}  ;

struct S_RotarySelector {
  int powerPin;  // to avoid consuming too much power, the Rotary is powered only during polling
  int readPin;

  void init(int pwerPin, int rdPin) {   // to be called in setup
    powerPin=pwerPin;
    readPin=rdPin;
    pinMode( powerPin, OUTPUT);
  }

  int poll() {
    digitalWrite(powerPin,HIGH);
    delay(100);
    int val=analogRead(readPin);
    digitalWrite(powerPin,LOW);

    if (val>=0 && val<85) return 8;
    if (val>86 && val<255) return 10; 
    if (val>256 && val<425) return 12;
    if (val>426 && val<595) return 14;    
    if (val>596 && val<765) return 16;
    if (val>766 && val<935) return 20;    
    if (val>936) return 24;
  }

  void debug() {  // never ends
    Serial.print(" S_RotarySelector.powerPin=");
    Serial.print(powerPin);
    Serial.print(" S_RotarySelector.readPin=");
    Serial.println(readPin);
    while (1) {
        digitalWrite(powerPin,HIGH); 
        int val=analogRead(readPin);
        Serial.print(" analogRead=");
        Serial.print(val); 
        Serial.print(" poll()=");
        Serial.println(poll());
       delay(1000);
    }
    digitalWrite(powerPin,LOW);
  }
  
} ;

// GLOBALS :

// GLobal variables
long  Steps = 0;   // keeps the current step position in steps
int fingerWidth=0;
long previousSteps = 0;
int  Direction = 0;        // current Direction = LEFT or RIGHT
float  Position = 0;       // Current Position in mm 
float stepRatio = SCREW_PITCH / STEP_PER_TURN;   // mm / step
int Finger = 0;            // current finger number 
unsigned long risingEdgeCount=0;


// Global objects
// Rotary Encoder led objects :
S_bouncedInput ledA(LED_A,'A');
S_bouncedInput ledB(LED_B,'B');
// Rotary selector
S_RotarySelector rotarySelector;
// Display bargraph object :
LedBarGraph bargraph;



void setup() {
  Serial.begin (19200);
  Serial.println("--------------------------------------------------");
  Serial.println(VERSION);
  Serial.println("--------------------------------------------------");
  

  pinMode(LED_A, INPUT);
  pinMode(LED_B, INPUT);
  pinMode(DEBUG_BUTTON_PIN, INPUT_PULLUP);   // for debug, wire a button to trigger traces on serial link
  pinMode(DEBUG_PIN, OUTPUT);   // for debug : changes state at each step
  
  rotarySelector.init(ROTARY_POWER, ROTARY_READ);
  fingerWidth=rotarySelector.poll(); 
  bargraph.init(int(BLADE_WIDTH/stepRatio) , int(fingerWidth/stepRatio));

  Serial.print(" BladeWidth=");
  Serial.print(BLADE_WIDTH);
  Serial.print(" FingerWidth=");
  Serial.println(fingerWidth);
  
//  bargraph.debug();
//  rotarySelector.debug();
}

void loop() {
  // Polling
  ledB.poll();
  if (ledA.poll()) {    // If rising edge on Led A 
    ledB.stableState()  ?  Direction=LEFT : Direction=RIGHT;   // Have a look at led B level to know the direction
    PORTC ^= DEBUG_PIN_MASK ;   // For debug purpose, reverts DEBUG_PIN at each step (use a probe on that pin to check if lost steps, compared to LEDA)
    Steps += Direction;
    risingEdgeCount++;
    bargraph.update(Steps);
  }

    
  if (digitalRead(DEBUG_BUTTON_PIN)==LOW) {                        // if trace button wired on A4
    int finger=floor(double(Steps)/(2*180));         // number of full finger   (full finger = 1 cut + 1 finger)  here 180=fingerWidth=10mm
    int offset= Steps-finger*2*180;  
    Serial.print(" Steps=");
    Serial.print(Steps);
    Serial.print(" finger=");
    Serial.print(finger);
    Serial.print(" offset=");
    Serial.print(offset);
    Serial.println("");
  }

#ifdef TRACE
  Position=Steps*stepRatio;
  Finger= int(Position/(2*BLADE_WIDTH));
  Display();
  previousSteps=Steps;
#endif
}




void Display() {
  static long display_delay ;

  display_delay+=1;

#ifdef DEBUG
  if (display_delay==1000) {
    Serial.print(" ledA.state=");
    Serial.print(ledA.state);
    Serial.print(" ledA.stableStateChangeCount=");
    Serial.print(ledA.stableStateChangeCount);
    Serial.print(" ledB.state=");
    Serial.print(ledB.state);
    Serial.print(" ledB.stableStateChangeCount=");
    Serial.print(ledB.stableStateChangeCount);
    Serial.println("");
  }
#endif

  if (previousSteps!=Steps) {
    Serial.print(" risingEdgeCount=");
    Serial.print(risingEdgeCount);
    Serial.print(" Steps=");
    Serial.print(Steps);
    Serial.print(" Direction=");
    Serial.print(Direction);
    Serial.print(" Position=");
    Serial.print(Position);
    Serial.print(" Finger=");
    Serial.print(Finger);
    Serial.println("");
    display_delay=0;
  }

  if (display_delay==1000) {
    display_delay=0;
  }
   
}
