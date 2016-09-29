//Βιβλιοθήκες Data Logger
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <RTClib.h>

//Επαφές Arduino
const int pvMeasure  = A0;    // Μέτρηση πάνελ
const int batMeasure  = A1;   // Μέτρηση μπαταρίας
const int pulsePot  = A2;     // Επιλογή ρυθμού παλμού
const int coil  = 2;          // Έξοδος προς πηνίο
const int batLed  = 3;        // Led ένδειξης μπαταρίας
const int emLedOne  = 5;      // Led κατάστασης βραχυκυκλώματος
const int emLedTwo  = 6;      // Led κατάστασης βραχυκυκλώματος
const int emLedThree  = 7;    // Led κατάστασης βραχυκυκλώματος
const int coilCon = 8;        // Είσοδος σύνδεσης πηνίου
const int chipSelect = 10;	  // Επαφή για DATALOGGER

/******************************************************/
//ΜΕΤΑΒΛΗΤΕΣ ΛΕΙΤΟΥΡΓΙΑΣ ΦΟΡΤΙΣΤΗ
boolean emergState;     // Κατάσταση βραχυκυκλώματος
boolean testingState;
boolean coilState;
boolean lowV;
boolean veryLowV;
boolean dayTime;
int cTestnum;
int fTestnum;
float batVolts;         // Μετρούμενη ταση μπαταρίας
float prebatVolts;      // Αποθηκευμένη ταση μπαταρίας
float pvVolts;          // Μετρούμενη τάση φωτοβολταικού
float vDiff;
int potVal;
int pulseDelay;
int randled;
int onDelay;
int offDelay;
float bVout;
float pvVout;
float rOneBat;
float rTwoBat;
float rDivBat;
float rOnePv;
float rTwoPv;
float rDivPv;
/**************************************/

/**************************************/
//ΜΕΤΑΒΛΗΤΕΣ & ΣΥΝΑΡΤΗΣΕΙΣ ΑΠΟΘΗΚΕΥΣΗΣ ΔΕΔΟΜΕΝΩΝ
RTC_DS1307 RTC; // Define real time clock object.
File logfile; // the logging file
char filename[] = "DATA.CSV";
//Χρονικές μεταβλητές
long hourTime= 3600000;
long nowTime;
long lastMesTime;
long timePassed;
long timestmp; //timestamp from RTC
//Μεταβλητές κατάστασης λειτουργίας
int funcState;
float pulseRate; //Ρυθμός παλμού ανά δευτ
//Συναρτήσεις αποθήκευσης
void dataStoring();
void storeData();
float getPulseRate ();

/****************************************************/
//ΜΕΤΑΒΛΗΤΕΣ ΚΑΙ ΣΥΝΑΡΤΗΣΕΙΣ ΑΠΟΣΤΟΛΗΣ ΚΑΙ ΛΗΨΗΣ ΔΕΔΟΜΕΝΩΝ
//Μεταβλητές
int sendNum;	  	  // Αριθμός που στέλνεται ΣΤΟ  Arduino 	
char recLet;		  // Γράμμα που λαμβάνεται ΑΠΟ ΤΟ Android
String () dataString;

//Συναρτήσεις
void isBtNear ();
void sendData ();

/****************************************************/

/****************************************************/
//ΣΥΝΑΡΤΗΣΕΙΣ ΛΕΙΤΟΥΡΓΙΑΣ ΦΟΡΤΙΣΤΗ
float getBvolts ();     //Μέτρηση τάσης μπαταρίας
float getPvVolts ();    //Μέτρηση τάσης ηλιακού
void veryLowBat ();     //Τάση μπαταρίας < 
void lowBat ();
void normalFunc ();
int getDelay ();
boolean testFence ();
boolean getDaytime ();
void emergFuncDay ();
void emergFuncNight ();
void coilPulse ();

void setup() {
  Serial.begin(9600);
  pinMode (pvMeasure, INPUT);
  pinMode (batMeasure, INPUT);
  pinMode (pulsePot, INPUT);
  pinMode (coil, OUTPUT);
  pinMode (batLed, OUTPUT);
  pinMode (emLedOne, OUTPUT);
  pinMode (emLedTwo, OUTPUT);
  pinMode (emLedThree, OUTPUT);
  pinMode (coilCon, INPUT);
  pinMode(10,OUTPUT);
  randomSeed (analogRead (3));    // randomize
  rOneBat = 100000.0;
  rTwoBat = 10000.0;
  rDivBat = rTwoBat/(rOneBat + rTwoBat);
  rOnePv = 100000.0;
  rTwoPv = 10000.0;
  rDivPv = rTwoPv/(rOnePv + rTwoPv);
  emergState = false ;     // Κατάσταση βραχυκυκλώματος
  testingState = false;
  coilState = LOW;
  lowV = false;
  veryLowV = false;
  dayTime = false;
  cTestnum = 0;
  fTestnum = 0;
  randled =0;
  onDelay= 0;
  offDelay= 0;
  lastMesTime = 0;
  timePassed = 0;
  funcState = 0;
  pulseRate = 0.0; 
 
}

void loop() {
// ΛΗΨΗ ΜΕΤΡΗΣΕΩΝ
  getBvolts ();
  getPvVolts ();
  getDaytime ();
  
// ΣΥΝΑΡΤΗΣΕΙΣ ΔΕΔΟΜΕΝΩΝ
  storeData();
  dataStoring();

//ΛΕΙΤΟΥΡΓΙΑ ΣΥΣΚΕΥΗΣ
  coilState = digitalRead(coilCon);
    if (coilState == HIGH){
         if (dayTime == false){ // NIGHT TIME
        batCheck ();
        if (lowV == false && veryLowV == false){
          funcState = 0;
          normalFunc ();
      }
        else if (lowV == true && veryLowV == false){
          funcState = 1;
          lowBat ();
        }
        else if (lowV == false && veryLowV == true){
          funcState = 2;
          veryLowBat ();
    }
       }
       
    else if (dayTime == true){ // DAY TIME
      if (batVolts >11.2){ 
        if (lowV == false && veryLowV == false){
          funcState = 0;
          normalFunc ();
      }
        else if (lowV == true && veryLowV == false){
          funcState = 1;
          lowBat ();
        }
        else if (lowV == false && veryLowV == true){
          funcState = 2;
          veryLowBat ();
       }
      }
      else if (batVolts <=11.2){
        funcState = 3;
        emergFuncDay ();
     } 
    }
    }
    else if (coilState == LOW){
      funcState = 4;
      if (dayTime == false){ // Δοκιμαστική λειτουργία
       emergFuncNight ();
       }
       else if (dayTime == true){ // Δοκιμαστική λειτουργία
       emergFuncDay ();
       }  
        }  
}

float getBvolts (){
  //5V = ADC value 1024 => 1 ADC value = (5/1024)Volt= 0.0048828Volt
  // Vout=Vin*R2/(R1+R2) => Vin = Vout*(R1+R2)/R2   R1=100K and R2=10K
  batVolts  = analogRead(batMeasure);
  delay (1);
  bVout = batVolts*5.0/1024.0;
  batVolts= bVout / rDivBat;
  if (batVolts <0.09){
    batVolts = 0.0;
  }
  return batVolts;
}

float getPvVolts (){
   pvVolts  = analogRead(pvMeasure);
  delay (1);
  pvVout = pvVolts*5.0/1024.0;
  pvVolts= pvVout / rDivPv;
  if (pvVolts <0.09){
    pvVolts = 0.0;
  }
  return pvVolts;
}

boolean batCheck (){
  getBvolts ();
   if (batVolts >11.8){
          lowV = false;
          veryLowV = false;
      }
        else if (batVolts <= 11.8 && batVolts >= 11.2){
          lowV = true;
          veryLowV = false;
        }
        else if (batVolts < 11.2){
          lowV = false;
          veryLowV = true;
    }
  return lowV;
  return veryLowV;
}

void veryLowBat (){
    digitalWrite (batLed, HIGH);
    digitalWrite (coil,LOW);
    if (dayTime == false){ // Δοκιμαστική λειτουργία
       emergFuncNight ();
       }
       else if (dayTime == true){ // Δοκιμαστική λειτουργία
       emergFuncDay ();
       }   
}

void lowBat (){
    digitalWrite (batLed, HIGH);
    digitalWrite (emLedOne, HIGH);
    delay (1000);
    digitalWrite (batLed, LOW);
    digitalWrite (emLedOne, LOW);
    normalFunc ();  
}

void normalFunc (){
    if (emergState == true){
      if (dayTime == false){ // Δοκιμαστική λειτουργία
       emergFuncNight ();
       }
       else if (dayTime == true){ // Δοκιμαστική λειτουργία
       emergFuncDay ();
       }  
    }
    else if (emergState == false && testingState == true){
      testFence ();  
    }
    else if (emergState == false && testingState == false){
      coilPulse ();
    } 
}

int getDelay (){
 potVal = analogRead (pulsePot);
 delay (1);
 pulseDelay = map(potVal, 1,1024, 500, 200);
 return pulseDelay;
}

float getPulseRate (){
  pulseRate = 1000/pulseDelay;
  return pulseRate;
}

boolean testFence (){
     getBvolts ();
     delay (1);
     prebatVolts = batVolts;
     digitalWrite (coil,HIGH);
     delay (2);
     digitalWrite (coil,LOW);
     getBvolts ();
     delay (1);
     vDiff = prebatVolts - batVolts;
     if (vDiff < 0.5) {
            fTestnum = 0;
            testingState = false;
          }
     else if (vDiff >= 0.5){  
      delay (50);
      testingState = true;
      fTestnum++;
        if (fTestnum >=3){
          testingState = false;
          emergState = true;
                         }  
      }
 return testingState;
 return emergState;      
                }

void coilPulse (){
        getDelay();
        getBvolts ();
        prebatVolts = batVolts;
        digitalWrite (coil,HIGH);
        delay (10);
        digitalWrite (coil,LOW);
        getBvolts ();
        vDiff = prebatVolts - batVolts;
          if (vDiff < 0.5) {
            delay (pulseDelay);
            getPulseRate ();
          }
          else if (vDiff >= 0.5){
           testFence (); 
          }
}

boolean getDaytime (){
    getPvVolts ();
     if (pvVolts < 13.0){ 
       dayTime = false; // NIGHT
       }
     else if (pvVolts >= 13.0){ // Δοκιμαστική λειτουργία
        dayTime = true;
       }
    return dayTime;  
}

void emergFuncNight (){
    randled = random(4,7); 
    onDelay= random(200,500);
    offDelay= random(200,500);
    digitalWrite (randled, HIGH);
    delay (onDelay);
    digitalWrite (randled, LOW);
    delay (offDelay);
  }
  
void emergFuncDay () {
    digitalWrite (emLedOne, HIGH);
    delay (1000);
    digitalWrite (emLedOne, LOW);
    delay (1000);
    digitalWrite (emLedTwo, HIGH);
    delay (1000);
    digitalWrite (emLedTwo, LOW);
    delay (1000);
    digitalWrite (emLedThree, HIGH);
    delay (1000);
    digitalWrite (emLedThree, LOW);
    delay (1000);
  }

  /*******************************************/
  //ΣΥΝΑΡΤΗΣΕΙΣ ΑΠΟΘΗΚΕΥΣΗΣ ΔΕΔΟΜΕΝΩΝ
void dataStoring(){
   nowTime = millis();
   timePassed = nowTime - lastMesTime;
   if (timePassed < 0) {
       lastMesTime = nowTime;
       }
   else if (timePassed >= hourTime){
        storeData (); 
       } 
  }

void storeData(){
  DateTime now=RTC.now();
  timestmp = now.unixtime();
  
  if (!SD.begin(chipSelect)) {
      lastMesTime = nowTime; // Επιστροφή σε 1 ώρα
      dataStoring(); }
      
  logfile=SD.open(filename,FILE_WRITE);
  if (!logfile) {
  	return; }
  	
  else {
  	//Καταχώρηση μίας γραμμής
    logfile.print(timestmp); logfile.print(',');
    logfile.print(funcState); logfile.print(',');
    logfile.print(pulseRate); logfile.print(',');
    logfile.print(pvVolts); logfile.print(',');
    logfile.println(batVolts); 
    logfile.flush(); // write to file
    logfile.close(); // Κλείσιμο αρχείου
    }      
}

/****************************************************/

void isBtNear (){
// if we get a valid byte, read analog ins:
  if (Serial.available() > 0) {
  	recLet = Serial.read(); 	// get incoming byte:
  	
    	if (recLet == 'a'){
    		sendNum = 1;
  			Serial.write(sendNum);
  			delay (3);
  			Serial.write(funcState);
  			delay (3);
  			Serial.write(pulseRate);
  			delay (3);
  			Serial.write(pvVolts);
  			delay (3);
  			Serial.write(batVolts);
  			delay (3);
			// Αποστολή παροντων μετρήσεων και αναμονή λήψης "b"
			// για την αποστολή ΟΛΟΚΛΗΡΟΥ του αρχείου
			// (Η επεξεργασία θα γίνει στη συσκευή Android)
			}
								
    	else if (recLet == 'b'){	
    		sendData ();
    	}
    	}
  }
   
void sendData ()
{
	if (recLet == 'b'){
	logfile=SD.open(filename,FILE_READ);
		if (logfile) {
    while (logfile.available())
    {
      String  dataString = logfile.read();
      Serial.write(dataString);
    }
    logfile.close();
    sendNum = 2;				// Αποστολή στο Android οτι στάλθηκε το αρχείο 
  	Serial.write(sendNum);
  	delay (3);

					}
}

  else {
  		sendNum = 3;			// Αποστολή στο Android οτι δεν ανοίχτηκε το αρχείο 
  		Serial.write(sendNum);
  		delay (3);
    	logfile.close(); 		// Κλείσιμο αρχείου
    }      
} 
  

