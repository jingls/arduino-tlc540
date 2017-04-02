
#include "SimpleTimer.h"
SimpleTimer timer;

// debug variables
#define MSGSIZE 1
static unsigned int rising = 0; // the number of rising edges
static unsigned int falling = 0; // the number of falling edges
static unsigned int timerEventNumber;
static char msg[MSGSIZE];
static uint8_t msgpos;

static int mystate = 0;
static bool clkSig = false;
static bool readFinished = false;
static uint8_t counter1 = 0, counter3 = 0, counter5 = 0;
static uint8_t input; // used in the state machine function timerEvent
static uint8_t bitpos, readpos; // readpos: bit position for reading from pin
static uint8_t val; // receives one bit from the digital pin
static uint8_t adcresult; // holds the 8 bit result
static uint8_t teststate = 4;
static uint8_t cur_CS = 0; // the current active chip

// pins
const byte ANALOG_PINS[] = { A0, A1, A2, A3, A4, A5 }; // we also use these pins as voltage inputs
static uint8_t   SYSCLK = 13;
static uint8_t   IOCLK = 12;
static uint8_t   ADDR = 8;
static uint8_t   DATA = 7;
static uint8_t   CS1 = 4; // CS of the chip 1
static uint8_t   CS2 = 3; // CS of the chip 2

// number of inputs in each chip (0 to 10) (11 inputs)
#define INPUTS 11

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

// this is the state machine
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
        digitalWrite(cur_CS, LOW);
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
        digitalWrite(cur_CS, HIGH);
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

void select_chip(int CS) {

  // disable both chips
  digitalWrite(CS1, HIGH);
  digitalWrite(CS2, HIGH);

  for (int i = 0; i < 3; i++) {
    lowerclock();
    delayMicroseconds(50);
    raiseclock();
    delayMicroseconds(50);
  }
  lowerclock();
  
  digitalWrite(CS, LOW); // enable chip

  // spend 3 clock cycles
  for (int i = 0; i < 3; i++) {
    lowerclock();
    delayMicroseconds(50);
    raiseclock();
    delayMicroseconds(50);
  }
  lowerclock();
}

void setup() {
  
  pinMode(SYSCLK, OUTPUT);
  pinMode(IOCLK, OUTPUT);
  pinMode(ADDR, OUTPUT);
  pinMode(DATA, INPUT);
  pinMode(CS1, OUTPUT);
  pinMode(CS2, OUTPUT);

  cur_CS = CS1;
  select_chip(cur_CS);
  
  Serial.begin(9600);
}

uint8_t cur_input, lastInput; // current input, will appear in next reading cycle. lastInput will appear in this reading cycle
uint8_t ins[INPUTS];

void loop() {
  
  if (cur_CS == CS1) {
    cur_CS = CS2;
  } else {
    cur_CS = CS1;
    for (int i = 0; i < 6; i++) { // show the voltages at the analog inputs A0 to A5
      Serial.print(analogRead(ANALOG_PINS[i]));
      Serial.print(" ");
    }
    Serial.print(millis()/1000.0); // last column is time in seconds
    Serial.print("\r\n");
  }
  select_chip(cur_CS);

  for (int i = 0; i < (INPUTS + 1);  i++) { // we read one value more than the number of inputs, because the chip returns the previous input, not the current one
    cur_input = i % INPUTS; // i goes until INPUTS, but cur_input goes until (INPUTS - 1) and then 0
    uint8_t val = readTimer(cur_input);
    ins[lastInput] = val;
    lastInput = i;
  }
  
  Serial.print(cur_CS);
  Serial.print(" ");
  
  for (int i = 0; i < INPUTS; i++) {
    Serial.print(ins[i]);
    Serial.print(" ");
  }
}




