/*

   _____ _____   _____               _                 _                  ____                   __   ___   _____ 
  / ____|  __ \ / ____|     /\      | |               | |                |  _ \                 /_ | / _ \ | ____|
 | |  __| |__) | (___      /  \   __| |_   _____ _ __ | |_ _   _ _ __ ___| |_) | _____  __ __   _| || | | || |__  
 | | |_ |  ___/ \___ \    / /\ \ / _` \ \ / / _ \ '_ \| __| | | | '__/ _ \  _ < / _ \ \/ / \ \ / / || | | ||___ \ 
 | |__| | |     ____) |  / ____ \ (_| |\ V /  __/ | | | |_| |_| | | |  __/ |_) | (_) >  <   \ V /| || |_| | ___) |
  \_____|_|    |_____/  /_/    \_\__,_| \_/ \___|_| |_|\__|\__,_|_|  \___|____/ \___/_/\_\   \_/ |_(_)___(_)____/ 
                                                                                                                                                                                                                                                   
GPS AdventureBox v 1.0.5
Copyright 2014, Garrett Kendrick
GPSAdventureBox.com


This work is licensed under the Creative Commons Attribution-NonCommercial 4.0 International License. 
To view a copy of this license, visit http://creativecommons.org/licenses/by-nc/4.0/deed.en_US.
You may modify and share this source code for personal use, but may not sell or use it for any 
commercial activities and this license must remain intact.

The GPS AdventureBox contains material duly licensed from The Sundial Group.


Connections:
 * Bypass Switch pin to digital pin 9
 * GPS RX pin to digital pin 10
 * GPS TX pin to digital pin 11
 * Servo Data pin to digital pin 12
 * SD MOSI pin to digital pin 50
 * SD MISO pin to digital pin 51
 * SD CLK pin to digital pin 52
 * SD CS pin to digital pin 53
 * LCD RS pin to digital pin 30
 * LCD Enable pin to digital pin 31
 * LCD D4 pin to digital pin 32
 * LCD D5 pin to digital pin 33
 * LCD D6 pin to digital pin 34
 * LCD D7 pin to digital pin 35
 * LCD R/W pin to ground
 
 */

#include <LiquidCrystal.h>
#include <SD.h>
#include <SoftwareSerial.h>
#include <TinyGPS.h>
#include <PWMServo.h>
#include <GKScroll.h>



File currentStep, configFile, totalStepsFile;
char current;
int stepToLoad = 1;
char inputChar;
float targetLat;
float targetLon;
String stepNumber;
int tolerance;
int stringIndex;
String hint;
char inputString [1280];
char stepArray[12];
int endOfLineNumber = 0;
int totalSteps = 1;
String firstLine, secondLine = "";
int stringLength, topLength, bottomLength, bottomStart = 0;
int gpsGood = 0;
int val = 0;
int lcdColumns = 16;
int lcdRows = 2;
int scrollBuffer = 5; //number of blank spaces to leave between end and beginning of scrolling text
String units = "feet";
float tolerance_meters = 0;
float distance_feet, distance_meters = 100000;
int speedMultiplier = 3; //increase this to increase the time between location checks and increase LCD scroll speed
int i,j,k = 0;
float lat, lon;// create variable for latitude and longitude object
unsigned long fix_age;
float DEST_LATITUDE;
float DEST_LONGITUDE;


    
SoftwareSerial gpsSerial(10, 11); // create gps sensor connection // RX, TX
TinyGPS gps; // create gps object
PWMServo servo; 

// initialize the LCD library with the numbers of the interface pins
LiquidCrystal lcd(30, 31, 32, 33, 34, 35);




void setup() {
  pinMode(53, OUTPUT); 
  pinMode(9,INPUT);
  servo.attach(12);
  servo.write(220);
  gpsSerial.begin (9600); // connect gps sensor
   
  // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);
  // Print a message to the LCD.
      val = digitalRead(9);
    if (val == HIGH){

lcd.clear();
lcd.setCursor(0, 0);
lcd.print(F("Bypass enabled"));
lcd.setCursor(0, 1);
lcd.print(F("(2000)")); 
    servo.write(100);
    delay(500);
    PowerOff();
   } 
lcd.clear();
lcd.setCursor(0, 0);
lcd.print(F("GPS AdventureBox"));
lcd.setCursor(0, 1);
lcd.print(F("v1.0.5")); 
  delay(5000);
  
  
  if (!SD.begin(36)) {
lcd.clear();
lcd.setCursor(0, 0);
lcd.print(F("Error reading SD"));
lcd.setCursor(0, 1);
lcd.print(F("(1000)")); 
delay(2500);
PowerOff();
  return;
  }
  
  // ########################    SD Section Begin   ##################################
  
  
char steps[100];
int charPos = 0;



if (SD.exists("curstep.txt")) {
currentStep = SD.open("curstep.txt");

if (currentStep) {
  while (currentStep.available()) {
  stepArray[j] = currentStep.read();
           j++;

  }
  currentStep.close();
  
  stepArray[j] = '\0';
  stepToLoad = atoi(stepArray);

} else {                            //didn't read CurrentStep.txt on SD card

lcd.clear();
lcd.setCursor(0, 0);
lcd.print(F("Error reading SD"));
lcd.setCursor(0, 1);
lcd.print(F("(1001)")); 

delay(2500);
PowerOff();                       
}
} else {
  currentStep = SD.open("curstep.txt", FILE_WRITE);
  currentStep.print(F("1"));
  currentStep.close();
  stepToLoad = 1;
}


 LoadConfig();


CheckIfFinished();

lcd.setCursor(0, 0);
lcd.print(F("Please wait...  ")); //scroll current hint
lcd.setCursor(0, 1);
lcd.print(F("Finding GPS fix ")); 
servo.detach();
  // ########################    SD Section End    ##################################
    
}


void loop() {



gpsSerial.available(); 
 

  
 if(gps.encode(gpsSerial.read())){ // encode gps data
 gpsGood = 0;

 for(int g = 0; g < speedMultiplier; g++){ 
  lcd.setCursor(0, 0); 
  lcd.print(ScrollLine(hint, scrollBuffer, lcdColumns));
  delay(280);  
 }

  if (millis() >= 900000) //power save after 15 minutes
  PowerOff();

   gps.f_get_position(&lat,&lon, &fix_age); // get latitude and longitude
   distance_meters = TinyGPS::distance_between(lat, lon, DEST_LATITUDE, DEST_LONGITUDE);
   distance_feet = distance_meters * 3.28084; 
   
   lcd.setCursor(0, 1); 
  if (fix_age < 5000){ //check if fix is stale
  //fix is good
  
  
  if (units == "meters"){ //check to see what unit of measure to use
  //it's meters (metric)
   if (distance_meters < 1000){        
    lcd.print(distance_meters);       
    lcd.print(F(" meters      "));       
  } else {
    lcd.print((distance_meters/1000)); 
    lcd.print(F(" kilometers  "));        
  } 
  } else {
   //it's feet (imperial)
      if (distance_feet < 5280){        
    lcd.print(distance_feet);       
    lcd.print(F(" feet      "));       
  } else {
    lcd.print((distance_feet/5280)); 
    lcd.print(F(" miles     "));        
  } 
  }
  
  
  } else { //fix is stale
   lcd.print(F("Finding GPS fix "));
  }
 
 if(distance_meters <= tolerance_meters){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Step "));
  lcd.print(stepToLoad);
  lcd.setCursor(0, 1);
  lcd.print(F("complete!"));
  delay(2500);
  stepToLoad++;
  
  SD.remove("curstep.txt");
  delay(100);
  currentStep = SD.open("curstep.txt", FILE_WRITE);
  currentStep.print(stepToLoad);
  currentStep.close();
  CheckIfFinished();
  software_Reset();

}
 
 } else {// close the encode gps data
 gpsGood++;
 if( gpsGood >=20000){
   gpsGood = 0;
  lcd.setCursor(0, 0); 
  lcd.print(ScrollLine(hint, scrollBuffer, lcdColumns));
  delay(280); 
  lcd.setCursor(0, 1); 
  lcd.print(F("Finding GPS fix "));  
   
 }
 }
 
//} //close the while loop
} //close the main loop



void CheckIfFinished()
{

if(stepToLoad >= totalSteps){
  //you did it! unlock and power off
   servo.attach(12);
   servo.write(100);
lcd.clear();
lcd.setCursor(0, 0);
lcd.print(F("You did it!"));
lcd.setCursor(0, 1);
lcd.print(F("Powering off...")); 
  delay(5000);
 PowerOff();

}
}



void PowerOff()
{
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(F("Shutting down..."));
  
  digitalWrite(15, HIGH);
  delay(1000);
  software_Reset();
} 



void LoadConfig()
{
  //  #################  get number of steps  ####################
  if (SD.exists("config~1.txt")) {
totalStepsFile = SD.open("config~1.txt");

if (totalStepsFile) {
  while (totalStepsFile.available()) {
  char inputChar;  
  inputChar = totalStepsFile.read(); // Gets one byte from serial buffer
  if (inputChar == '^'){ // define breaking char here 
  totalSteps++;
    } 
   
  }

  totalStepsFile.close();



} else {                            //didn't read SD card

lcd.clear();
lcd.setCursor(0, 0);
lcd.print(F("Error reading SD"));
lcd.setCursor(0, 1);
lcd.print(F("(1002)")); 

delay(2500);
PowerOff();                        
}
}
  
  
  
//  #################  now actually get the config  ####################
  if (SD.exists("config~1.txt")) {  //since file names can only be 8 characters, we use this name.
  
 configFile = SD.open("config~1.txt");

if (configFile) {

endOfLineNumber = 0;
stringIndex = 0;
  while (configFile.available()) {
  
     inputChar = configFile.read(); // Gets one byte from serial buffer
  if (inputChar != '^'){ // define breaking char here 

  
    inputString[stringIndex] = inputChar; // Store it
    stringIndex++; // Increment where to write next
    
  } else { //end of line character detected, "^"
 endOfLineNumber++;

 if (endOfLineNumber == stepToLoad){
 break;  
 } else {
  stringIndex = 0; 
  memset(inputString, 0, sizeof inputString);
 }
    
  }
  
   

  }
  configFile.close();
  
  //charArray[i] = '\0';
 
 targetLat = atof(strtok(inputString, "|"));
 targetLon = atof(strtok(NULL, "|"));
 tolerance = atoi(strtok(NULL, "|"));
 hint = strtok(NULL, "|"); 
 units = strtok(NULL, "|");
     
DEST_LATITUDE = (targetLat);
DEST_LONGITUDE =  (targetLon);

if (units == "meters"){
tolerance_meters = tolerance;  
}else{
tolerance_meters = tolerance / 3.28084;
}
  
  } else {

lcd.clear();
lcd.setCursor(0, 0);
lcd.print(F("Error reading SD"));
lcd.setCursor(0, 1);
lcd.print(F("(1003)"));  
delay(2500);
PowerOff();

  }


}else {
lcd.clear();
lcd.setCursor(0, 0);
lcd.print(F("Error reading SD"));
lcd.setCursor(0, 1);
lcd.print(F("(1005)")); 
delay(2500);
PowerOff();
  }
 
  
}


void software_Reset() // Restarts program from beginning but does not reset the peripherals and registers
{
asm volatile ("  jmp 0");  
}  



