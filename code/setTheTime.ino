/*

  setTheTime.ino
  Program to set the real time clock in 
  the event it forgets what the actual time is
  
  2022 Aug 07 - Initial write.
    Edit this file with the correct time/date
    (give it a few minutes in the future).
    Download it, and when it's actually the correct
    time, hit reset.  This will set the time, and 
    then reinstall the correct operating software.


      
  Pinout of DS3231 module:
   +--------------------+
   | 1 - VCC +5         |
   | 2 - SCL    battery |
   | 3 - SDA      is    |
   | 4 - n/c     here   |
   | 5 - GND            |
   +--------------------+

*/

//=============== libraries ===============
#include "RTClib.h"

//=============== definitions ===============
  //=============== serial port ===============
  #define baudRate 9600                          // serial port speed

//=============== objects ===============
  // create real time clock object
  RTC_DS3231 rtc;

//=============== variables ===============
int minuteOfHour;                                // minutes from real time clock 0-59 no leading 0
int hourOfDay;                                   // hour of day from real time clock 0 - 23 no leading 0
int amPM = 0;                                    // flag to convert to 12 hour clock 

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"}; // days of week

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
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC
    rtc.adjust(DateTime(2022, 8, 26, 19, 50));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));


// Print out file name and version
  Serial.println();                              // blank line
  Serial.print("setTheTime.ino");
  Serial.print(" ");
  Serial.print("20220826");
  Serial.println();                              // blank line

} // end of setup

//=============== loop (runs continuously after setup) ===============
void loop() {
  readClock();                                   // read the clock and format the time data
  delay(3000);                                   // run this loop every three seconde

} // end of main loop

void readClock() {  // read the DS3231 RTC module
  DateTime now = rtc.now();                      // get date/time data from real time clock
  minuteOfHour = (now.minute());
  hourOfDay = (now.hour());

// Print out Year/Month/Day, (Day of Week), and Time in 24 hour format
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

// print out the time of day in 12 hour format with leading zeros
  if (hourOfDay < 12) amPM = 0;                  // flag to change 24 hour clock to 12 hour clock
  if (hourOfDay >= 12) amPM = 1;                 // flag to change 24 hour clock to 12 hour clock
  if (hourOfDay > 12) hourOfDay = (hourOfDay - 12);  // if time is after noon, subtrack 12
  if (hourOfDay <10) Serial.print("0");          // if hours less than 10 add leading zero
  Serial.print(hourOfDay);                       // print out hour of day in 12 hour format
  Serial.print(":");
    if (minuteOfHour < 10) Serial.print("0");    // if minutes less than 10 add leading zero
  Serial.print(minuteOfHour);
    if (amPM == 0) Serial.println(" AM ");       // if morning it's AM
    if (amPM == 1) Serial.println(" PM ");       // if after noon it's PM
  Serial.println();                              // blank line

}  // end of readClock module
