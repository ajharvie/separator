/*
Arduino sketch for automated separator.

Written for a Teensy 3.2 (www.pjrc.com/teensy) but should work on any arduino board that allows two hardware interrupts.

Two TSL235R-LF light-freqency converters (at 3.3V) are used to monitor light transmission through each separator 
output channel. Segmented flow is detected in a given channel by comparing the differential RSD between
the channels. The valve position is updated by a servo.


Andrew J Harvie Feb 2019
 */

#include <Servo.h>
#include <math.h>

/////////////////////////////PARAMETERS TO CHECK///////////////////////////////////////////////////
//Pin assignment
#define ledPin 13

//Pins for light-frequency converters
//through channel
#define ltfPin1 23
//side channel
#define ltfPin2 22

//servo pin
#define servoPin 9

//Servo travel limits (depends on manufacturer)
#define maxPosition 2000
#define minPosition 1000

/////////////////////////////PARAMETERS TO TUNE////////////////////////////////////////////////////
/*
Modify the following parameters if separator performance is unsatisfactory for a particular
solvent system. Instructions on modification are available in supporting information.
*/

//multiply these two numbers to find how long getRSDs() takes
#define samplingTime 20 //it's in ms
#define numberBins 200

//others
#define maxSteps 40 // *2 for number of degrees turned by servo
#define accDiffRSD 0.002 //Value of differential RSD to accept (no adjustment)
#define delayTime 2000 //number of ms to wait before starting a new measurement
#define P_coeff 5000 //proportional coefficient (*2 for value in degrees)

///////////////////////////////////////////////////////////////////////////////////////////////////

Servo winchServo;

//specify starting position here (1000 is fully closed)
int servoPosition = 1000;

//arrays for intensity values from each channel
int throughCounts[numberBins]; 
int sideCounts[numberBins];

volatile unsigned long cnt1;
volatile unsigned long cnt2;
unsigned long last = 0;

float throughMean = 0.0;
float sideMean = 0.0;
float throughRSD = 0.0;
float sideRSD = 0.0;

// IRQs for frequency counting
void irq1(){
  cnt1++;
}

void irq2(){
  cnt2++;
}

void setup() {
  pinMode(ledPin, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(ltfPin1), irq1, RISING);
  attachInterrupt(digitalPinToInterrupt(ltfPin2), irq2, RISING);
  winchServo.attach(servoPin, minPosition, maxPosition);
  //intitialise servo position
  winchServo.writeMicroseconds(servoPosition);
  Serial.begin(9600);
  digitalWrite(ledPin, HIGH);
}

void loop() {
  //delay between iterations
  delay(delayTime);
  
  // update RSD values
  getRSDs();
  
  //calculate differential RSD
  float diffRSD = throughRSD - sideRSD;
  
  //absolute value
  float incorrectitude = abs(diffRSD);
  
  //decide number of steps
  int steps = int(incorrectitude * P_coeff);

  //decide direction
  if(diffRSD > accDiffRSD){
    tighten(steps);
  }
  else if (diffRSD < -1*accDiffRSD){
    loosen(steps);
  }
  
  // Monitoring
  Serial.print(throughRSD, 5);
  Serial.print(" ");
  Serial.print(sideRSD, 5);
  Serial.print(" ");
  Serial.print(diffRSD, 5);
  Serial.print(" ");
  Serial.println(servoPosition);

 
}


void loosen(int steps){
  //increases pwm value (opens valve) more through channel
  if (steps > maxSteps){
    steps = maxSteps;
  }
  servoPosition = servoPosition + steps;
  // check if outside servo range
  if (servoPosition > maxPosition){
    servoPosition = maxPosition;
    errorlight();
  }
  // set position
  winchServo.writeMicroseconds(servoPosition);
}

void tighten(int steps){
  //decreases pwm value (closes valve) more side channel
  if (steps > maxSteps){
    steps = maxSteps;
  }
  servoPosition = servoPosition - steps;
  // check if outside servo range
  if (servoPosition < minPosition){
    servoPosition = minPosition;
    errorlight();
  }
  // set position
  winchServo.writeMicroseconds(servoPosition);
}

void getRSDs(){
  //updates values of global RSDs

  digitalWrite(ledPin, HIGH);
  int throughSum = 0;
  int sideSum = 0;
  float throughSquareSum = 0;
  float sideSquareSum = 0;

  last = millis();
  
  //populate arrays of light intensity vs time
  for(int i = 0; i < numberBins; ++i){
    //reset counters
    cnt1 = 0;
    cnt2 = 0;

    // timing 
    while(millis() - last < samplingTime){
      //don't do anything - waiting on interrupt counters
      //this is better than using delay() which sometimes behaves badly with interrupts
    }
    
    int grab1 = cnt1;
    int grab2 = cnt2;

    //add to array
    throughCounts[i] = grab1;
    sideCounts[i] = grab2;
    
    //increment sum (for calculation of mean)
    throughSum = throughSum + grab1;
    sideSum = sideSum + grab2;

    last = millis();
    
  }
  
  //calculate means
  throughMean = float(throughSum)/float(numberBins);
  sideMean = float(sideSum)/float(numberBins);

  //calculate  RSDs
  for(int i = 0; i < numberBins; ++i){
    throughSquareSum = throughSquareSum  + sq(throughCounts[i] - throughMean);
    sideSquareSum = sideSquareSum  + sq(sideCounts[i] - sideMean);
  }
  
  throughRSD = sqrt(throughSquareSum/float(numberBins)) / throughMean;
  sideRSD = sqrt(sideSquareSum/float(numberBins)) / sideMean;
  
}

void errorlight(){
  //flash light 20 times if there's an error (usually reached max servo travel)
  for(int i = 0; i<20; ++i){
    digitalWrite(13, HIGH);
    delay(50);
    digitalWrite(13, LOW);
    delay(50);
  }
  
}
