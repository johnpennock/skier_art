/*

  skier.ino
  Program to control a statue of a skier like a clock 
  On the hour, do hour number of iterations of movement
  On the quarter hour, do 1 quarter hour interation movement
  On the half hour, do 1 half hour iteration of movement
  On the 3/4's hour, do 1 iteration of 3/4 hour iteration of movement
  
  2022 July 16 - Initial write.
    Initial code - read real time clock and display the time.
                   stub out functions for hour, quarter hour,
                   half hour, and 3/4's hour actions
  2022 July 19 - cleanup
    Cleaned up -   added leading zeros to hours/minutes
                   Added jump subroutines
  2022 August 03 - cleanup
    Cleaned up -   set baud rate to 9600
                   added comments on device pinouts


  Pinout of DS3231 module:
   +--------------------+
   | 1 - VCC +5         |
   | 2 - SCL    battery |
   | 3 - SDA    battery |
   | 4 - n/c    battery |
   | 5 - GND            |
   +--------------------+

  Pinout of motor controller
     +5      +5
  ENA 1   ENA 2
   IN 1    IN 3
   IN 2    IN 4
    GND     GND

  Truth table for motor controller
  IN 1   IN 2   ENA 1  Output
    0      0      x     Brake    
    1      1      x     Float
    1      0      1     FWD
    1      0     PWM    Variable speed FWD
    0      1      1     REV
    0      1     PWM    Variable speed REV



forwardHo(leftForwardBool, leftReverseBool, rightForwardBool, rightReverseBool, leftVelocity, rightVelocity); // invoke motor control routine

int forwardHo(bool leftForwardBool, bool leftReverseBool, bool rightForwardBool, bool rightReverseBool, int leftVelocity, int rightVelocity) {

  digitalWrite(leftForwardPin, leftForwardBool);
  digitalWrite(leftReversePin, leftReverseBool);
  digitalWrite(rightForwardPin, rightForwardBool);
  digitalWrite(rightReversePin, rightReverseBool);

  analogWrite(leftVelocityPin, leftVelocity); // left motor(s) velocity
  analogWrite(rightVelocityPin, rightVelocity); // right motor(s) velocity

}

forward();

void forward() {                            // go forward until we need to do something different
  digitalWrite(leftForwardPin, HIGH);
  digitalWrite(leftReversePin, LOW);
  digitalWrite(rightForwardPin, HIGH);
  digitalWrite(rightReversePin, LOW);

  analogWrite(leftVelocityPin, travelVelocity);
  analogWrite(rightVelocityPin, travelVelocity);
}



*/

//=============== libraries ===============
#include "RTClib.h"

//=============== objects ===============
// create real time clock object to communicate with any OneWire device
RTC_DS3231 rtc;

//=============== variables ===============
String versionDate = "20220803";
String sketchName = "skier.ino";
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"}; // days of week
float tempF;                                     // temperature in fahrenheit
float tempC;                                     // temperature in celsius
int minuteOfHour;                                // minutes from real time clock 0-59 no leading 0
int hourOfDay;                                   // hour of day from real time clock 0 - 23 no leading 0
int amPM = 0;                                    // flag to convert to 12 hour clock 
bool actionCompleted = false;                    // flag to enable action once

//=============== setup (run once after power on / reset) ===============
void setup () {
// setup serial port for testing
  Serial.begin(9600);  

//#ifndef ESP8266
//  while (!Serial); // wait for serial port to connect. Needed for native USB
//#endif

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

} // end of setup

//=============== loop (runs continuously after setup) ===============
void loop () {

// Print out file name and version
    Serial.println();                            // blank line
    Serial.print(sketchName);
    Serial.print(" ");
    Serial.print(versionDate);
    Serial.println();                            // blank line

// Print out Year/Month/Day (Day of Week) Time in 24 hour format
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

  Serial.print("Temperature: ");
  Serial.print(rtc.getTemperature());            // get temp from real time clock
  Serial.print(" C");
  Serial.println();                              // blank line

  tempF = (rtc.getTemperature());                // get temp from real time clock
  tempF = ((tempF * 1.8) + 32);                  // convert temp to F
  Serial.print("Temperature: ");
  Serial.print(tempF);                           // print temp in F
  Serial.print(" F");
  Serial.println();                              // blank line
    
// print out the time of day in 12 hour format
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
  
// main test loop for 4 times of day to perform an action
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
      hourJump();                                // perform jump cycle
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
      quarterAfterJump();                        // perform jump cycle
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
      halfPastJump();                            // perform jump cycle
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
      quarterTillJump();                         // perform jump cycle
      actionCompleted = true;                    // now that we've done this once, set flag saying so
    break;                                       // done with this case

    default:                                     // if nothing else matches, do the default
     actionCompleted = false;                    // any other time than above, set flag back to false
    break;
}
    delay(3000);                                 // run this loop every three seconde

} // end of main loop

void hourJump() {                                // jump hour of day number of times
  Serial.print("Top of the hour, we're jumping ");
  Serial.print(hourOfDay);
  Serial.print(" time(s).");
  Serial.println();                              // blank line
}

void quarterAfterJump() {                        // jump for quarter after hour
  Serial.print("Quarter after the hour, we're jumping one time.");
  Serial.println();                              // blank line
}

void halfPastJump() {                            // jump for half hour
  Serial.print("Half past the hour, we're jumping one time.");
  Serial.println();                              // blank line
}

void quarterTillJump() {                         // jump for quarter to the hour
  Serial.print("Quarter till the hour, we're jumping one time.");
  Serial.println();                              // blank line
}
