//weiss =schawz
//rot = gelb


//rl-bild
//scharz = orange
//gruen = blaz

/* minimal solution - reading gauge indicator (here a device from Absolute-System)
   @author torsten roehl
   @see this code is based on https://www.instructables.com/id/Interfacing-a-Digital-Micrometer-to-a-Microcontrol/
   (there is a little big in the code from instructables - only ocurs for high inch values! )
*/


/*  ADJUST AREA START  */
int stepCounter;
int steps = 200;
int stepSize = 4;
int currentStep = 0;
int run = 0;
/* pin configuration 1*/
int pinREQC1 = 14;
int pinDATAC1 = 2;
int pinCLKC1 = 3;

/* pin configuration 2*/
int pinREQC2 = 14;
int pinDATAC2 = 2;
int pinCLKC2 = 3;

/* pooling intervall */
int sleep = 300;
/* baudrate */
int bd = 9600;

float data[2][200] = {};
/*  ADJUST AREA END  */

/* Keep FIXED! - data protocol format */
byte digits[13];
float value;


void setUpGauge(int pinREQ, int pinCLK, int pinDATA) {
  /*    SERIAL_8N1 (the default)  */
  Serial.begin(bd);

  pinMode(pinREQ, OUTPUT);
  pinMode(pinCLK, INPUT_PULLUP);
  pinMode(pinDATA, INPUT_PULLUP);
  digitalWrite(pinREQ, LOW); // set request at high
}

void setUpStepper() {
  pinMode(38, OUTPUT); // Enable
  pinMode(A0, OUTPUT); // Step
  pinMode(A1, OUTPUT); // Richtung

  digitalWrite(38,LOW);
  digitalWrite(A1,HIGH); // im Uhrzeigersinn
}

void setup()
{
  setUpGauge(pinREQC1, pinCLKC1, pinDATAC1);  
  setUpGauge(pinREQC2, pinCLKC2, pinDATAC2);  
  setUpStepper();
}

void step() {
    for(int i = 0; i < stepSize; i++) {
      makeStep();
      delay(3);
    }    
    currentStep++;
}

void loop()
{
  if(run < 100) {
    measure();
    step();
    if(currentStep>=steps) {
      run++;
      printValues();
      currentStep = 0;
    }
  }
}

void measure() {
  readDialGauge(pinREQC1, pinCLKC1, pinDATAC1, 0);  
  readDialGauge(pinREQC2, pinCLKC2, pinDATAC2, 1);  
}

void makeStep() {
  digitalWrite(A0,HIGH);
  delayMicroseconds(500);
  digitalWrite(A0,LOW);
  delayMicroseconds(500);
}

/*
   Read the dial gauge.
   After a call to this function the array digits[13] contains all data according to the data protocol.
   For normal data reading the first four digits should by all 'F', the function returns false if this is not the case.
*/
void readDialGauge(int pinREQ, int pinCLK, int pinDATA, int clockIndex) {

  /**
     Generate request by making REQ active (low level)!
  */
  digitalWrite(pinREQ, HIGH); // generate set request

  /* read 13 nibble (52 bit) */
  for (int i = 0; i < 13; i++ ) {
    int digit = 0;
    for (int j = 0; j < 4; j++) {

      while ( digitalRead(pinCLK) == LOW) {}/* Wait for CLOCK=HIGH */
      while ( digitalRead(pinCLK) == HIGH) {} /* Wait for CLOCK=LOW  */

      bitWrite(digit, j /*pos*/, (digitalRead(pinDATA) & 0x1));
    }

    digits[i] = digit;
  }

  digitalWrite(pinREQ, LOW);

  /* d1-d4 should be all oxF */
  int header = digits[0] + digits[1] + digits[2] + digits[3];  
  if(header == 60)
    data[clockIndex][currentStep] = decodeDialGauge(clockIndex);
}


/*
   decodeDialGauge (according to the digomatic-protocol)
   Note: A call to readDialGauge is necessary to use this function!

   i) value - contains the (signed) value read out from the gauge indicator
   ii) units - if units is equal to 0 then mm is used, otherwise (1) inch is in use
   iii) decimal - the number of decimal. mm corresponds to 3 decimal numbers, whereas inch has five decimal numbers.

   return (signed) measured value
*/
float decodeDialGauge(int clockIndex) {

  int units = digits[12];    // 0=mm or 1=inch
  int decimal = digits[11];  // 3 for mm and 5 for inch

  value = (float)digits[5] * 10000 + (float)digits[6] * 10000 + (float) digits[7] * 1000 + (float)digits[8] * 100 + (float)digits[9] * 10 + (float) digits[10];
  /* assuming digits[11] is allways 3 */
  value /= 1000.0;

  if (units == 1 )
    value /= 100.0;

  if (digits[4] == 8)
    value *= -1.0;
  return value;
}

/*
   Helper function: This function decodes and writes the data from the dial gauge to the arduino serial monitor for debugging purposes.
   Note: A call to readDialGauge and decodeGauge is necessary to use this  function!

*/
void printValues() {
  int decimal = digits[11];// 3 for mm and 5 for inch
 
  for(int clockIndex = 0; clockIndex < 2; clockIndex++) {    
    for(int i = 0; i < 200; i++) {
      if(i > 0)
        Serial.print(";");
      Serial.print(data[clockIndex][i], decimal);
    }   
    Serial.println("") ;
  }
}