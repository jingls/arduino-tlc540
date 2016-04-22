

#include "SimpleTimer.h"
SimpleTimer timer;

// debug variables
#define MSGSIZE 900
static unsigned int rising = 0; // the number of rising edges
static unsigned int falling = 0; // the number of falling edges
static unsigned int timerEventNumber;
static char msg[MSGSIZE];
static uint8_t msgpos;

static int mystate = 0;
static bool clkSig = false;
static bool readFinished = false;
static uint8_t counter1 = 0, counter3 = 0, counter5 = 0;
static uint8_t bitpos, readpos; // readpos: bit position for reading from pin
static uint8_t input, lastInput; // adc input, 0 to 10 (11 inputs)
static uint8_t val; // receives one bit from the digital pin
static uint8_t adcresult; // holds the 8 bit result
static uint8_t teststate = 4;

// pins
static uint8_t   SYSCLK = 13;
static uint8_t   IOCLK = 12;
static uint8_t   ADDR = 8;
static uint8_t   DATA = 7;
static uint8_t   CS = 4;

void raiseclock() {
  rising++;
  digitalWrite(SYSCLK, HIGH);
  digitalWrite(IOCLK, HIGH);
}

void lowerclock() {
  falling++;
  digitalWrite(SYSCLK, LOW);
  digitalWrite(IOCLK, LOW);
}

void timerEvent() {
  timerEventNumber++;
  //Serial.print(" 0."); Serial.print(timerEventNumber);
  if (clkSig == true) {
    lowerclock();
    clkSig = false;
  } else {
    raiseclock();
    clkSig = true;
  }
  //Serial.print(" 1."); Serial.print(clkSig ? 't' : 'f');
  //Serial.print(" 2."); Serial.print(mystate);
  
  switch(mystate) {
    case 0:
      if (clkSig == true) {
        //Serial.print(" 3.");
        digitalWrite(CS, LOW);
        counter1 = 0;
        mystate = 1;
      }
    break;
    
    case 1:
      if (clkSig == true) {
        counter1++; // count if rising edge
        //Serial.print(" 4."); Serial.print(counter1);
      }
      if (counter1 == 2) {
        bitpos = B00001000; // 0000 1000
        readpos = B10000000; // 1000 0000
        mystate = 2;
      }
    break;
    
    case 2:
      // here clkSig should be false immediately after mystate = 2, but then it will be true, then false etc. We write to the address pin at every falling edge
      uint8_t code;
      if (clkSig == false) {
        code = (input & bitpos) && 1;
        //Serial.print(" 6."); Serial.print(code);
        digitalWrite(ADDR, code);
        bitpos = bitpos >> 1;
      } else { //rising edge, read the bit value
        val = digitalRead(DATA); // reads bits 7, 6, 5
        //Serial.print(" 8."); Serial.print(val);
        adcresult = (adcresult | (val ? readpos : 0));
        readpos = readpos >> 1;
        if (readpos == 0x04) {
          //Serial.print(" 9."); Serial.print(val);
          counter3 = 0;
          mystate = 3;
        }
      }
    break;
    
    case 3:
      if (clkSig == true) {
        val = digitalRead(DATA); // reads bit 4
        //Serial.print(" 10."); Serial.print(val);
        adcresult = (adcresult | (val ? readpos : 0));
        readpos = readpos >> 1;
      }
      if (clkSig == false) { // count if falling edge
        counter3++;
        //Serial.print(" 11."); Serial.print(counter3);
      }
      if (counter3 == 4) {
        mystate = 4;
      }
    break;
    
    case 4:
      if (clkSig == false) {
        //Serial.print(" 12.");
        digitalWrite(CS, HIGH);
        counter5 = 0;
        mystate = 5;
      }
    break;
    
    case 5:
      if (clkSig == false) { // count if falling edge
        counter5++;
        //Serial.print(" 13."); Serial.print(counter5);
      }
      if (counter5 == 37) {
        mystate = 6;
      }
    break;

    default:
      //Serial.print(" 14.");
      readFinished = true;
    break;
  }
}

uint8_t readTimer(uint8_t inputNumber) {
  int sht = 50; // 50 us
  readFinished = false;
  mystate = 0;
  input = inputNumber;
  rising = falling = timerEventNumber = msgpos = 0;
  counter1 = 0, counter3 = 0, counter5 = 0;
  adcresult = 0;
  for (int i = 0; i < MSGSIZE; i++) {
    msg[i] = 0;
  }
  int thistimer = timer.setInterval(sht, timerEvent);
  unsigned int count = 0;
  while (!readFinished) {
    count++;
    timer.run();
  }
  timer.deleteTimer(thistimer);
  
  //Serial.print("\r\n");

  rising = falling = timerEventNumber = 0;
  return adcresult;
}

void setup() {
  
  pinMode(SYSCLK, OUTPUT);
  pinMode(IOCLK, OUTPUT);
  pinMode(ADDR, OUTPUT);
  pinMode(DATA, INPUT);
  pinMode(CS, OUTPUT);
  
  Serial.begin(9600);
}

#define INPUTS 11
int count = 0;
uint8_t ins[INPUTS];

void loop() {
  input = count++ % INPUTS;
  
  if (lastInput == 0) {
    for (int i = 0; i < INPUTS; i++) {
      Serial.print(ins[i]);
      Serial.print("\t");
    }
    Serial.print("\r\n");
  }

  uint8_t val = readTimer(input);
  ins[lastInput] = val;
  // float voltage = (val/255.0f)*5; //255 because 8 bits, 5 is VCC = 5 V
  delay(10); // this gives more or less 5 samples per second
  lastInput = input;
}


