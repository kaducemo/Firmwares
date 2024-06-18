/*
 *    Shield Multifunction test. 
 *    I´ve used a multitask aproach through arduino-timer library
 *    AUTOR:   Carlos Eduardo Mayer de Oliveira
 *    DATA:    16/06/2024
*/

#include <arduino-timer.h>

Timer<3, micros> timer; // 3 concurrent tasks, using micros as resolution

unsigned int counter = 0; // Counter used to update 7 Seg display


//Pins on the 74HC595 shift reg chip - correspondence with Arduino digital pins
#define LED_D1  13
#define LED_D2  12
#define LED_D3  11
#define LED_D4  10

#define RCLK    4 //ST_CP
#define SCKL    7 //SH_CP
#define SER     8 //DS

// Table to convert a hex digit into the matching 7-seg display segments
//COMMOM ANODE
int hexDigitValue[] = 
{
    0xFC,    /* Segments to light for 0  */
    0x60,    /* Segments to light for 1  */
    0xDA,    /* Segments to light for 2  */
    0xF2,    /* Segments to light for 3  */
    0x66,    /* Segments to light for 4  */
    0xB6,    /* Segments to light for 5  */
    0xBE,    /* Segments to light for 6  */
    0xE0,    /* Segments to light for 7  */
    0xFE,    /* Segments to light for 8  */
    0xF6     /* Segments to light for 9  */
};

//Task that Updates 7 SEG display - 1ms period
bool TaskUpdatesDisplay(void *argument) 
{
    displayNumber( counter ); 
    return true; // to repeat the action - false to stop
}

//Task that updates the 4 LEDs - 500ms period
bool TaskUpdatesLeds(void *argument) 
{
  static int ledCounter = 0;
  switch(ledCounter)
  {
    case 0:
      digitalWrite(LED_D1, !digitalRead(LED_D1));
      break;
    case 1:
      digitalWrite(LED_D2, !digitalRead(LED_D2));
      break;
    case 2:
      digitalWrite(LED_D3, !digitalRead(LED_D3));
      break;
    case 3:
      digitalWrite(LED_D4, !digitalRead(LED_D4));
      break;

  }
  ledCounter = (ledCounter+1) % 4;
  return true; // to repeat the action - false to stop
}

//Task that updates the counter 1s period
bool TaskUpdatesCounter(void *argument) 
{  
    counter = (counter+1) % 10000;    // Decimal  
    return true; // to repeat the action - false to stop
}


/* Set display digit to a hexadecimal value '0'..'9','A'..'F'
 *  
 *    row = row number of digit to light up 0..3 where 3 is most significant (leftmost) display digit
 *    dgit = value 0 to 15 
 *    decimalPoint = true/false : whether to light thde decimal point or not
 */

void setDigit(unsigned int row, unsigned int digit, boolean decimalPoint)
{
      unsigned int rowSelector;
      unsigned int data;

      rowSelector = bit(row)<<4;
      data =  ~  hexDigitValue[ digit & 0xF ] ;
      if(decimalPoint)
        data &= 0xFE;
        
      // First 8 data bits all the way into the second 74HC595 
      shiftOut(SER, SCKL, LSBFIRST, data );

      // Now shift 4 row bits into the first 74HC595 and latch
      digitalWrite(RCLK, LOW);
      shiftOut(SER, SCKL, LSBFIRST, rowSelector );
      digitalWrite(RCLK, HIGH);
      
}

/* Displays a number as a 4-digit decimal number on the display
 *   Note this is multiplexed, so you need to keep calling it
 *   or you'll end up with just one digit lit.
 */
void displayNumber(unsigned int number)
{
    for(int i=0; i<4; i++)
    {
      setDigit(i, number % 10, false); // display righmost 4 bits (1 digit)
      number = number / 10;  // roll on to the next digit
      delay(1);
    } 
}

void setup() 
{
  
  // put your setup code here, to run once:
  pinMode(SER,  OUTPUT);
  pinMode(SCKL, OUTPUT);  
  pinMode(RCLK, OUTPUT);
  
  pinMode(LED_D1, OUTPUT);
  pinMode(LED_D2, OUTPUT);
  pinMode(LED_D3, OUTPUT);
  pinMode(LED_D4, OUTPUT);

  timer.every(500000, TaskUpdatesLeds);
  timer.every(1000000, TaskUpdatesCounter);
  timer.every(1000, TaskUpdatesDisplay);

}

void loop() 
{
  timer.tick();
}
