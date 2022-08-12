/*

  modskier.ino
  Program to control a statue of a skier like a clock 
  On the hour, do hour number of iterations of movement
  On the quarter hour, do 1 quarter hour interation movement
  On the half hour, do 1 half hour iteration of movement
  On the 3/4's hour, do 1 iteration of 3/4 hour iteration of movement
  
  2022 Aug 07 - Initial write.
    re-write of original code to make it more modular.
    read real time clock and display the time.
    Drive the actuator for hour, quarter hour,
    half hour, and 3/4's hour actions
  
  2022 Aug 08 - Version 1
    Modules that run on a 3 second loop:
      readClock()     - reads the real time clock and formats the data we need
                        minutes, hours, and temperature
      timeForAction() - checks for the hour, 1/4 hour, 1/2 hour, and 3/4 hour
                        sets numberOfJumps to how many cycles we want to perform
      moveActuator()  - moves the actuator number of times requested by 
                        the timeForAction module

  2022 Aug 12 - update
    Added 10K potentiometer to allow for testing of
    the 4 times of day without having to wait for 
    the respective times of day

      
  Pinout of DS3231 module:
   +--------------------+
   | 1 - VCC +5         |
   | 2 - SCL    battery |
   | 3 - SDA      is    |
   | 4 - n/c     here   |
   | 5 - GND            |
   +--------------------+

  Pinout of motor controller
    01 +5     02 +5
    03 ENA 1  04 ENA 2
    05 IN 1   06 IN 3
    07 IN 2   08 IN 4
    09 GND    10 GND

  Truth table for motor controller
  IN 1   IN 2   ENA 1  Output
    0      0      x     Brake    
    1      1      x     Float
    1      0      1     FWD
    1      0     PWM    Variable speed FWD
    0      1      1     REV
    0      1     PWM    Variable speed REV    

*/

//=============== libraries ===============
#include "RTClib.h"

//=============== definitions ===============
  //=============== serial port ===============
  #define baudRate 9600                          // serial port speed

  //=============== potentiometer ===============
  #define potOutPin A0                          // center pin of potentiometer goes to A0

  //=============== motor controller ===============
  #define enablePin 3                           // pin 3 goes to pin 3 of controller (enable=1)
  #define forwardPin 2                          // pin 2 goes to pin 5 of controller
  #define reversePin 4                          // pin 4 goes to pin 7 of controller

//=============== objects ===============
  // create real time clock object
  RTC_DS3231 rtc;

//=============== variables ===============
String versionDate = "20220812";
String sketchName = "modskier.ino";

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"}; // days of week

float tempF;                                     // temperature in fahrenheit
float tempC;                                     // temperature in celsius

int minuteOfHour;                                // minutes from real time clock 0-59 no leading 0
int hourOfDay;                                   // hour of day from real time clock 0 - 23 no leading 0
int amPM = 0;                                    // flag to convert to 12 hour clock 
int numberOfJumps = 0;                           // number of jumps to perform
int potValue = 0;                                // value from potentiometer
bool actionCompleted = false;                    // flag to enable action once

bool extendState = LOW;                          // flag for actuator state LOW=retract HIGH=extend

long timeToExtend = 10000;                       // milliseconds to extend actuator
long timeToRetract = 10000;                      // milliseconds to retract actuator

unsigned long prevJumpTime = 0;                  // start of jump power timer

//=============== constants ===============
const long jumpOnTime = 10000;                   // time for actuator to run (in ms)

//=============== setup (run once after power on / reset) ===============
void setup () {

// setup serial port for testing
  Serial.begin(baudRate);  

//  if can't find real time clock, loop
  if (! rtc.begin()) {  
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }

// if power lost, set the real time clock to a default
  if (rtc.lostPower()) {  
    Serial.println("RTC lost power, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

  // When time needs to be re-set on a previously configured device, the
  // following line sets the RTC to the date & time this sketch was compiled
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // This line sets the RTC with an explicit date & time, for example to set
  // January 21, 2014 at 3am you would call:
  // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));

//=============== potentiometer setup ===============
  pinMode(potOutPin, INPUT);                     // set pin as input

//=============== motor controller outputs ===============

  pinMode(enablePin, OUTPUT);                    // actuator enable
  pinMode(forwardPin, OUTPUT);                   // actuator foward
  pinMode(reversePin, OUTPUT);                   // actuator reverse
  digitalWrite(enablePin,LOW);                   // ensure driver is off when starting up

// Print out file name and version
  Serial.println();                              // blank line
  Serial.print(sketchName);
  Serial.print(" ");
  Serial.print(versionDate);
  Serial.println();                              // blank line

// In the event it's extended, retract the actuator
  digitalWrite(forwardPin,LOW);                  // not forward
  digitalWrite(reversePin,HIGH);                 // set to reverse
  digitalWrite(enablePin,HIGH);                  // start motor
  Serial.print("Initial Retract");
  Serial.println();

  for (int delayCount = 10; delayCount >= 1; delayCount--) {
    Serial.print("Retracting: ");
    Serial.print(delayCount);
    Serial.println();
    delay(1000);                                 // give it enough time to retract fully
  }  

  digitalWrite(enablePin,LOW);                   // ensure driver is off when starting up
  digitalWrite(forwardPin,LOW);                  // not forward
  digitalWrite(reversePin,LOW);                  // not reverse

} // end of setup

//=============== loop (runs continuously after setup) ===============
void loop() {
  unsigned long currentMillis = millis();        // get the latest value of millis()

  readClock();                                   // read the clock and format the time data
  timeForAction();                               // is it time to make a move?
  checkPotValue();                               // module for manually triggering movement
  moveActuator(numberOfJumps);                   // move the actuator (numberOfJumps) times

  delay(3000);                                   // run this loop every three seconde

} // end of main loop

void readClock() {  // read the DS3231 RTC module
// Print out Year/Month/Day, (Day of Week), and Time in 24 hour format
  DateTime now = rtc.now();                      // get date/time data from real time clock
  Serial.print(now.year(), DEC);                 // year
  Serial.print('/');
  Serial.print(now.month(), DEC);                // month
  Serial.print('/');
  Serial.print(now.day(), DEC);                  // day
  Serial.print(" (");
  Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);  // derive day of week
  Serial.print(") ");
    if (now.hour() < 10) Serial.print("0");      // add leading zero to hours less than 10
  Serial.print(now.hour(), DEC);                 // hour (0-23 no leading 0 under 10)
  Serial.print(':');
    if (now.minute() < 10) Serial.print("0");    // add leading zero to minutes under 10
  Serial.print(now.minute(), DEC);               // minute (0-59 no leading 0 under 10)
  Serial.print(':');
    if (now.second() < 10) Serial.print("0");    // add leading zero to seconds under 10 
  Serial.print(now.second(), DEC);               // second 90-59 no leading 0 under 10)
  Serial.println();                              // blank line

//print out the temperature both in C and F
  tempC = (rtc.getTemperature());                // get temp from real time clock
  tempF = ((tempC * 1.8) + 32);                  // convert temp to F
  Serial.print("Temperature: ");
  Serial.print(tempC);                           // print out temp in C
  Serial.print(" C  |  ");
  Serial.print(tempF);                           // print temp in F
  Serial.print(" F");
  Serial.println();                              // blank line

// print out the time of day in 12 hour format with leading zeros
  minuteOfHour = (now.minute());
  hourOfDay = (now.hour());
    if (hourOfDay < 12) amPM = 0;                // flag to change 24 hour clock to 12 hour clock
    if (hourOfDay >= 12) amPM = 1;               // flag to change 24 hour clock to 12 hour clock
    if (hourOfDay > 12) hourOfDay = (hourOfDay - 12);  // if time is after noon, subtrack 12
    if (hourOfDay <10) Serial.print("0");        // if hours less than 10 add leading zero
  Serial.print(hourOfDay);                       // print out hour of day in 12 hour format
  Serial.print(":");
    if (minuteOfHour < 10) Serial.print("0");    // if minutes less than 10 add leading zero
  Serial.print(minuteOfHour);
    if (amPM == 0) Serial.println(" AM ");       // if morning it's AM
    if (amPM == 1) Serial.println(" PM ");       // if after noon it's PM
  Serial.println();                              // blank line

}  // end of readClock module

void checkPotValue() {  // check for manual run of system

  potValue = analogRead(potOutPin);              // read the value from the potentiometer (0-1023)
                                                 //   0 - 255  - do nothing
                                                 // 256 - 511  - hour cycle
                                                 // 512 - 767  - quarter after cycle
                                                 // 767 - 1023 - quarter till cycle

// print out value from potentiometer
  Serial.print("Potentiometer value: ");
  Serial.print(potValue);
  Serial.println();

  if(potValue < 24) {                            // check for no cycle
    Serial.print("No manual jump...");
    Serial.println();
  }
  if(potValue > 24 && potValue < 250) {          // check for hour cycle
    numberOfJumps = hourOfDay;                   // set (hourOfDay) jump cycle(s)
    actionCompleted = true;                      // now that we've done this once, set flag saying so
    Serial.print("Manual hour jump...");
    Serial.println();
  }
  if(potValue > 250 && potValue < 500) {         // check for quarter after cycle
    numberOfJumps = 1;                           // set 1 jump cycle(s)
    actionCompleted = true;                      // now that we've done this once, set flag saying so
    Serial.print("Manual quarter after jump...");
    Serial.println();
  }
  if(potValue > 500 && potValue < 750) {         // check for half hour cycle
    numberOfJumps = 2;                           // set 2 jump cycle(s)
    actionCompleted = true;                      // now that we've done this once, set flag saying so
    Serial.print("Manual half hour jump...");
    Serial.println();
  }
  if(potValue > 750 && potValue < 1023) {        // check for quarter till cycle
    numberOfJumps = 3;                           // set 3 jump cycle(s)
    actionCompleted = true;                      // now that we've done this once, set flag saying so
    Serial.print("Manual quarter till jump...");
    Serial.println();
  }
}  // end of checkPotValue module

void timeForAction() {  // check the time for our four action times
  numberOfJumps= 0;                              // clear number of jump cycle(s)

  switch (minuteOfHour) {
    case 0:                                      // on top of hour
      if (actionCompleted == true) break;        // if true, we've already done this, so break
      Serial.print("Top of hour ");
      if (minuteOfHour < 10) Serial.print("0");  // if minutes less than 10 add leading zero
      Serial.print(hourOfDay);
      if (amPM = 0) Serial.println(" AM ");      // if morning it's AM
      if (amPM = 1) Serial.println(" PM ");      // if after noon it's PM
      Serial.print("Do ");
      Serial.print(hourOfDay);                   // how many iterations of action do we complete
      Serial.print(" complete iterations of clock");
      Serial.println();                          // blank line
      numberOfJumps = hourOfDay;                 // set (hourOfDay) jump cycle(s)
      actionCompleted = true;                    // now that we've done this once, set flag saying so
    break;                                       // done with this case

    case 15:                                     // at quarter of hour
      if (actionCompleted == true) break;        // if true, we've already done this, so break
      Serial.print("Quarter after the hour ");    
      Serial.print(hourOfDay);    
      if (amPM = 0) Serial.println(" AM ");      // if morning it's AM
      if (amPM = 1) Serial.println(" PM ");      // if after noon it's PM
      Serial.print("Do 1 quarter of hour interation of clock");
      Serial.println();                          // blank line
      numberOfJumps = 1;                         // set 1 jump cycle(s)
      actionCompleted = true;                    // now that we've done this once, set flag saying so
    break;                                       // done with this case

    case 30:                                     // at bottom of hour
      if (actionCompleted == true) break;        // if true, we've already done this, so break
      Serial.print("Bottom of the hour ");    
      Serial.print(hourOfDay);    
      if (amPM = 0) Serial.println(" AM ");      // if morning it's AM
      if (amPM = 1) Serial.println(" PM ");      // if after noon it's PM
      Serial.print("Do 1 bottom of hour iteration of clock");
      Serial.println();                          // blank line
      numberOfJumps = 2;                         // set 2 jump cycle(s)
      actionCompleted = true;                    // now that we've done this once, set flag saying so
    break;                                       // done with this case

    case 45:                                     // at quarter to hour
      if (actionCompleted == true) break;        // if true, we've already done this, so break
      Serial.print("Quarter to the hour ");    
      Serial.print(hourOfDay);    
      if (amPM = 0) Serial.println(" AM ");      // if morning it's AM
      if (amPM = 1) Serial.println(" PM ");      // if after noon it's PM
      Serial.print("Do 1 quarter to hour iteration of clock");
      Serial.println();                          // blank line
      numberOfJumps = 1;                         // set 1 jump cycle(s)
      actionCompleted = true;                    // now that we've done this once, set flag saying so
    break;                                       // done with this case

    default:
      numberOfJumps= 0;                          // clear number of jump cycle(s)
      actionCompleted = false;                   // clear action completed flag
    break;
  }
}  // end of timeForAction module

void moveActuator(int numberOfJumps) {           // number of times to cycle actuator
  if (numberOfJumps > 0) {                       // only process if jumps greater than zero
    for (int jumpCount = 1; jumpCount <= numberOfJumps; jumpCount++) {
      Serial.print("Working on jump ");
      Serial.print(jumpCount);
      Serial.print(" of ");
      Serial.print(numberOfJumps);      
      Serial.print(" jump(s)");
      Serial.println();    

      digitalWrite(enablePin,LOW);               // ensure driver is off when starting up
      digitalWrite(reversePin,LOW);              // not reverse
      digitalWrite(forwardPin,HIGH);             // set to forward
      digitalWrite(enablePin,HIGH);              // start motor

      for (int delayCount = 10; delayCount >= 1; delayCount--) {  // 10 second movement delay
        Serial.print("Extending:  ");
        Serial.print(delayCount);
        Serial.println();
        delay(1000);                             // give it enough time to extend fully
      } 
                            
      digitalWrite(enablePin,LOW);               // ensure driver is off when starting up
      digitalWrite(forwardPin,LOW);              // not forward
      digitalWrite(reversePin,HIGH);             // set to reverse
      digitalWrite(enablePin,HIGH);              // start motor

      for (int delayCount = 10; delayCount >= 1; delayCount--) {  // 10 second movement delay
        Serial.print("Retracting: ");
        Serial.print(delayCount);
        Serial.println();
        delay(1000);                             // give it enough time to retract fully
      }
    }  
  }
  if (actionCompleted == true) numberOfJumps = 0;// clear the number of jumps
}  // end of moveActuator module
