/*
 *    Firmware used to test semaphoric leds 
 *    AUTOR:   Carlos Eduardo Mayer de Oliveira
 *    DATA:    10/04/2025
 *    Esta aplicação foi desenvolvida utilizando as seguintes placas:
 *    1) Arduino Leonardo
 *    2) Shield Multifunctions: https://blog.eletrogate.com/guia-completo-do-shield-multi-funcoes-para-arduino/
 *    3) Shiel Relay: https://www.casadoresistor.com.br/modulo-rele-4-canais-arduino?gad_source=1&gclid=EAIaIQobChMI0f7BxdqijAMVzSZECB2QnxXtEAQYCCABEgJNkvD_BwE
 *    Com isso respeitar as PINAGEM do Arduino e dos Shields. (Observar os defines)
 *
 *   Através da chave S1, podemos ciclar o modo de operação.:
 *   OPMODE_OPERATING = 0, // Fazendo o Ciclo (VERDE-> VERMELHO PULSANTE - > VERMELHO)
 *   OPMODE_CONF_RED_TIME,  // Configuração do tempo de vermelho
 *   OPMODE_CONF_GREEN_TIME, // Configuração do Tempo de Verde
 *   OPMODE_CONF_PULSE_TIME, // Configuração do tempo de Pulsante
 *   OPMODE_CONF_PHASE_TEST // Acionamento independente das fases.
 *
 *    No modo OPERATING, as chaves S2/S3 aumentam e diminuem o tempo
 *    da FASE DO CICLO ATUAL (Verm, Verde, Piscante), embora a atualização só seja 
 *    realizada no CICLO SEGUINTE. Além disso se a FASE atual for VERDE, ao invés de 
 *    modificar o tempo de VERDE, será modificado o status de acionamento do AMARELO.
 *    Nos modos CONF, usar S2 e S3 para alterar os tempos normalmente.
 *    No modo CONF_PHASE_TEST, usar S2 e S3 para acionar as fases Vermelha e Verde. Se
 *    alguma das fases estiverem ligada nesse modo e for pressionada S1, então será
 *    acionada/desacionada a fase AMARELA. Só será possível retornar ao modo OPERATING,
 *    quando as fases (VD e VM) estiverem desligadas.
 *    Finalmente foi inserido uma definição VARIATION que faz o modo pulsante e cronometro
 *    ficarem variando seus tempos de acionamento. Dessa forma se VARIATION = 0, não existe
 *    variação, já se VARIATION = 1, o cronometro VERMELHO e o PISCANTE ficam variando entre
 *    +1s - 0s -1s a cada ciclo.
*/

#include <arduino-timer.h>


//DEFINES

#define LEDS_NUMBER     4
#define SWITCHES_NUMBER  3

//LEDs
#define LED_D1  13
#define LED_D2  12
#define LED_D3  11
#define LED_D4  10

//Relays
#define RELAY_RETURN  5 //IN1 (PLACA RELAYS)
#define RELAY_RED     6 //IN3 (PLACA RELAYS)
#define RELAY_GREEN   9 //IN2 (PLACA RELAYS)
#define RELAY_YELLOW  A5 //IN4 (PLACA RELAYS)


//Switches
#define SWITCH_SW1  A1
#define SWITCH_SW2  A2
#define SWITCH_SW3  A3

//Pins on the 74HC595 shift reg chip - correspondence with Arduino digital pins
#define RCLK    4 //ST_CP
#define SCKL    7 //SH_CP
#define SER     8 //DS

// Choose your Base time TICK
#define MICROSECONDS
//#define MILLISECONDS


#ifdef MICROSECONDS
  #define MULT_FACTOR   1000UL

#endif

#ifdef MILLISECONDS
  #define MULT_FACTOR   1
#endif

#define TICKS_TO_1_SECOND 1000 * MULT_FACTOR //TICKS until we have 1s
#define TICKS_TO_900_MS 900 * MULT_FACTOR //TICKS until we have 900ms
#define TICKS_TO_800_MS 800 * MULT_FACTOR //TICKS until we have 800ms
#define TICKS_TO_700_MS 700 * MULT_FACTOR //TICKS until we have 700ms
#define TICKS_TO_600_MS 600 * MULT_FACTOR //TICKS until we have 600ms
#define TICKS_TO_500_MS 500 * MULT_FACTOR //TICKS until we have 500ms
#define TICKS_TO_400_MS 400 * MULT_FACTOR //TICKS until we have 400ms
#define TICKS_TO_300_MS 300 * MULT_FACTOR //TICKS until we have 300ms
#define TICKS_TO_200_MS 200 * MULT_FACTOR //TICKS until we have 200ms
#define TICKS_TO_100_MS 100 * MULT_FACTOR //TICKS until we have 100ms



#define TASK_GET_SWITCH_STATUS_MS   10
#define TASK_EXECUTE_SWITCH_MS      50
#define TASK_UPDATE_LEDS_MS         100
#define TASK_UPDATE_COUNTER_MS      100
#define TASK_UPDATE_DISPLAY_MS      1

#define TASK_GET_SWITCH_STATUS_PERIOD   (TASK_GET_SWITCH_STATUS_MS * MULT_FACTOR)
#define TASK_EXECUTE_SWITCH_PERIOD      (TASK_EXECUTE_SWITCH_MS * MULT_FACTOR)
#define TASK_UPDATE_LEDS_PERIOD         (TASK_UPDATE_LEDS_MS * MULT_FACTOR)
#define TASK_UPDATE_COUNTER_PERIOD      (TASK_UPDATE_COUNTER_MS * MULT_FACTOR)
#define TASK_UPDATE_DISPLAY_PERIOD      (TASK_UPDATE_DISPLAY_MS * MULT_FACTOR)


#define T_UPD_CNT_PREDIV_ONE_SECOND   (TICKS_TO_1_SECOND/TASK_UPDATE_COUNTER_PERIOD) -1 //Number PREEMPTION OF UPDATE COUNTER TASK until we have 1s
#define T_UPD_CNT_PREDIV_900_MS       (TICKS_TO_900_MS/TASK_UPDATE_COUNTER_PERIOD) //Number PREEMPTION OF UPDATE COUNTER TASK until we have 900ms
#define T_UPD_CNT_PREDIV_800_MS       (TICKS_TO_800_MS/TASK_UPDATE_COUNTER_PERIOD) //Number PREEMPTION OF UPDATE COUNTER TASK until we have 800ms
#define T_UPD_CNT_PREDIV_700_MS       (TICKS_TO_700_MS/TASK_UPDATE_COUNTER_PERIOD) //Number PREEMPTION OF UPDATE COUNTER TASK until we have 700ms
#define T_UPD_CNT_PREDIV_600_MS       (TICKS_TO_600_MS/TASK_UPDATE_COUNTER_PERIOD) //Number PREEMPTION OF UPDATE COUNTER TASK until we have 600ms
#define T_UPD_CNT_PREDIV_500_MS       (TICKS_TO_500_MS/TASK_UPDATE_COUNTER_PERIOD) //Number PREEMPTION OF UPDATE COUNTER TASK until we have 500ms
#define T_UPD_CNT_PREDIV_400_MS       (TICKS_TO_400_MS/TASK_UPDATE_COUNTER_PERIOD) //Number PREEMPTION OF UPDATE COUNTER TASK until we have 400ms
#define T_UPD_CNT_PREDIV_300_MS       (TICKS_TO_300_MS/TASK_UPDATE_COUNTER_PERIOD) //Number PREEMPTION OF UPDATE COUNTER TASK until we have 400ms
#define T_UPD_CNT_PREDIV_200_MS       (TICKS_TO_200_MS/TASK_UPDATE_COUNTER_PERIOD) //Number PREEMPTION OF UPDATE COUNTER TASK until we have 200ms
#define T_UPD_CNT_PREDIV_100_MS       (TICKS_TO_100_MS/TASK_UPDATE_COUNTER_PERIOD) //Number PREEMPTION OF UPDATE COUNTER TASK until we have 100ms

#define BLINK_ON_PERIOD   T_UPD_CNT_PREDIV_600_MS //Period ON During BLINK 
#define BLINK_OFF_PERIOD  T_UPD_CNT_PREDIV_400_MS //Period OFF During BLINK

#define DEBOUNCE_PERIOD_MS  20 // Debounce time in MILLISECONDS

//MACROS
#define LED_D1_OFF      digitalWrite(LED_D1, HIGH)
#define LED_D2_OFF      digitalWrite(LED_D2, HIGH)
#define LED_D3_OFF      digitalWrite(LED_D3, HIGH)
#define LED_D4_OFF      digitalWrite(LED_D4, HIGH)

#define LED_D1_ON      digitalWrite(LED_D1, LOW)
#define LED_D2_ON      digitalWrite(LED_D2, LOW)
#define LED_D3_ON      digitalWrite(LED_D3, LOW)
#define LED_D4_ON      digitalWrite(LED_D4, LOW)

#define RELAY_RETURN_OFF      digitalWrite(RELAY_RETURN, HIGH)
#define RELAY_RED_OFF         digitalWrite(RELAY_RED, HIGH)
#define RELAY_GREEN_OFF       digitalWrite(RELAY_GREEN, HIGH)
#define RELAY_YELLOW_OFF      digitalWrite(RELAY_YELLOW, HIGH)


#define RELAY_RETURN_ON       digitalWrite(RELAY_RETURN, LOW)
#define RELAY_RED_ON          digitalWrite(RELAY_RED, LOW)
#define RELAY_GREEN_ON        digitalWrite(RELAY_GREEN, LOW)
#define RELAY_YELLOW_ON       digitalWrite(RELAY_YELLOW, LOW)

#define RELAY_RETURN_TOGGLE   digitalWrite(RELAY_RETURN,  !digitalRead(RELAY_RETURN))
#define RELAY_RED_TOGGLE      digitalWrite(RELAY_RED,     !digitalRead(RELAY_RED))
#define RELAY_GREEN_TOGGLE    digitalWrite(RELAY_GREEN,   !digitalRead(RELAY_GREEN))
#define RELAY_YELLOW_TOGGLE    digitalWrite(RELAY_YELLOW, !digitalRead(RELAY_YELLOW))

#define DEFAULT_RED_PHASE_PERIOD              10 //seconds
#define DEFAULT_GREEN_PHASE_PERIOD            8
#define DEFAULT_RED_PULSING_PHASE_PERIOD      4

#define MAX_RED_PHASE_PERIOD              300 //seconds
#define MAX_GREEN_PHASE_PERIOD            300
#define MAX_RED_PULSING_PHASE_PERIOD      300

#define MIN_RED_PHASE_PERIOD              0 //seconds
#define MIN_GREEN_PHASE_PERIOD            0
#define MIN_RED_PULSING_PHASE_PERIOD      0



//TYPEDEFS

typedef enum 
{
  OPMODE_OPERATING = 0,
  OPMODE_CONF_RED_TIME,  
  OPMODE_CONF_GREEN_TIME,
  OPMODE_CONF_PULSE_TIME,
  OPMODE_CONF_PHASE_TEST
}opMOde_t; // 0: operating mode, 1: config mode

typedef enum
{
  CYCLE_RED = 0,
  CYCLE_GREEN,
  CYCLE_PULSING_RED
}phaseCycle_t;

typedef enum
{
  SWITCH_NONE = 0,  
  SWITCH_MODE,
  SWITCH_UP,
  SWITCH_DOWN,  
}switch_t;


//GLOBALS

#ifdef MICROSECONDS
Timer<5, micros> timer;     // 6 concurrent tasks, using micros as resolution
#endif

#ifdef MILLISECONDS
Timer<5> timer;     // 6 concurrent tasks, using micros as resolution
#endif





uint16_t preDiv = 0; //counting to one second counter
opMOde_t opMOde             = OPMODE_OPERATING;
switch_t lastPressedSwitch  = SWITCH_NONE;
switch_t pressedSwitch      = SWITCH_NONE;
unsigned long initialDebounceTime[SWITCHES_NUMBER] = {0};  // the last time that each switch was pressed
unsigned long lastDebounceTime[SWITCHES_NUMBER] = {0};  // the last time that each switch was pressed
signed int countCycle = 0; //Conta até a VARIATION
signed int risingOrFalling = 1; // Essa variavel fica variando entre 1 e -1. Onde 1 acrescenta pulsos e diminui cronometro, -1 faz o oposto.


uint32_t cyclePeriod[] = {DEFAULT_RED_PHASE_PERIOD, DEFAULT_GREEN_PHASE_PERIOD, DEFAULT_RED_PULSING_PHASE_PERIOD};

#define VARIATION   0 //Defina um valor para variar o período do cronometro (positivo e negativamente) e a quantidade pulsos a cada ciclo.
#define INITIAL_CYCLE   CYCLE_RED

phaseCycle_t phaseCycle     = INITIAL_CYCLE;
unsigned int display        = ((INITIAL_CYCLE == CYCLE_GREEN) ? DEFAULT_GREEN_PHASE_PERIOD : ((INITIAL_CYCLE == CYCLE_RED) ? DEFAULT_RED_PHASE_PERIOD : DEFAULT_RED_PULSING_PHASE_PERIOD)); //Used to update 7 Seg display



// Table to convert a hex digit into the matching 7-seg display segments(COMMOM ANODE)
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






/*==============================================================
========================TASKS===================================
==============================================================*/

//Executes Switches Comands - 50ms period
bool TaskExecuteSwitch(void *argument) 
{
    switch(pressedSwitch)
    {
      case SWITCH_MODE: 

        if(opMOde != OPMODE_CONF_PHASE_TEST)
        {
          opMOde = (opMOde+1) % (OPMODE_CONF_PHASE_TEST+1);
          phaseCycle = INITIAL_CYCLE;
          display = cyclePeriod[INITIAL_CYCLE];
          preDiv = 0; 

          RELAY_YELLOW_OFF;
          RELAY_RED_OFF;
          RELAY_GREEN_OFF;
          RELAY_RETURN_OFF;
        }   

        else if(opMOde == OPMODE_CONF_PHASE_TEST)
        {        
          if(!digitalRead(RELAY_GREEN) && !digitalRead(RELAY_RED))
          {
            RELAY_YELLOW_TOGGLE;
          }
          else
          {          
            opMOde = OPMODE_OPERATING;
            phaseCycle = INITIAL_CYCLE;
            display = cyclePeriod[INITIAL_CYCLE];
            preDiv = 0;           

            RELAY_RED_OFF;
            RELAY_YELLOW_OFF;
            RELAY_GREEN_ON;
            RELAY_RETURN_ON;
          }
        }          
        
        pressedSwitch = SWITCH_NONE;
        break;

      case SWITCH_UP:      
        if(opMOde == OPMODE_CONF_RED_TIME)
          cyclePeriod[CYCLE_RED]++;

        else if(opMOde == OPMODE_CONF_GREEN_TIME)
          cyclePeriod[CYCLE_GREEN]++;

        else if(opMOde == OPMODE_CONF_PULSE_TIME)
          cyclePeriod[CYCLE_PULSING_RED]++;
        
        else if(opMOde == OPMODE_OPERATING)
        {
          switch (phaseCycle)
          {
            case CYCLE_RED:
            {
              cyclePeriod[CYCLE_RED] = cyclePeriod[CYCLE_RED] < MAX_RED_PHASE_PERIOD ? cyclePeriod[CYCLE_RED]+1 : MAX_RED_PHASE_PERIOD;
              break;

            }
            case CYCLE_GREEN:
            {              
              RELAY_YELLOW_TOGGLE;
              break;
            }
            case CYCLE_PULSING_RED:
            {
              cyclePeriod[CYCLE_PULSING_RED] = cyclePeriod[CYCLE_PULSING_RED] < MAX_RED_PULSING_PHASE_PERIOD ? cyclePeriod[CYCLE_PULSING_RED]+1 : MAX_RED_PULSING_PHASE_PERIOD;
              break;
            }
          }            
        }          
        
        else if(opMOde == OPMODE_CONF_PHASE_TEST)
          RELAY_RED_TOGGLE;

        pressedSwitch = SWITCH_NONE;
        break;

      case SWITCH_DOWN:

        if(opMOde == OPMODE_CONF_RED_TIME)
            cyclePeriod[CYCLE_RED]--;

        else if(opMOde == OPMODE_CONF_GREEN_TIME)
          cyclePeriod[CYCLE_GREEN]--;

        else if(opMOde == OPMODE_CONF_PULSE_TIME)
          cyclePeriod[CYCLE_PULSING_RED]--;
        
        else if(opMOde == OPMODE_OPERATING)
          switch (phaseCycle)
          {
            case CYCLE_RED:
            {
              cyclePeriod[CYCLE_RED] = cyclePeriod[CYCLE_RED] > MIN_RED_PHASE_PERIOD ? cyclePeriod[CYCLE_RED]-1 : MIN_RED_PHASE_PERIOD;
              break;

            }
            case CYCLE_GREEN:
            {              
              RELAY_YELLOW_TOGGLE;
              break;
            }
            case CYCLE_PULSING_RED:
            {
              cyclePeriod[CYCLE_PULSING_RED] = cyclePeriod[CYCLE_PULSING_RED] > MIN_RED_PULSING_PHASE_PERIOD ? cyclePeriod[CYCLE_PULSING_RED]-1 : MIN_RED_PULSING_PHASE_PERIOD;
              break;
            }
          }            
        
        else if(opMOde == OPMODE_CONF_PHASE_TEST)
          RELAY_GREEN_TOGGLE;

        pressedSwitch = SWITCH_NONE;
        break;
    } 
    
    return true; // to repeat the action - false to stop
}


//Get Switch Status -  10ms period
bool TaskGetSwitchStatus(void *argument) 
{
  static uint8_t releaseSwitch[SWITCHES_NUMBER] = {1};

  if(pressedSwitch == SWITCH_NONE)
  {     
    if(!digitalRead(SWITCH_SW1))
    {
      if(releaseSwitch[SWITCH_MODE-1])
      {
        if(!initialDebounceTime[SWITCH_MODE-1])
        {
          
          initialDebounceTime[SWITCH_MODE-1] = millis();
          lastDebounceTime[SWITCH_MODE-1] = initialDebounceTime[SWITCH_MODE-1];
          // Serial.write("IniSW1: ");
          // Serial.print(lastDebounceTime[SWITCH_MODE-1]);
          // Serial.println();
        }
        else
        {
          lastDebounceTime[SWITCH_MODE-1] += millis();                   
          pressedSwitch = (lastDebounceTime[SWITCH_MODE-1] > (initialDebounceTime[SWITCH_MODE-1] + DEBOUNCE_PERIOD_MS)) ? SWITCH_MODE : pressedSwitch; 
          if(pressedSwitch)   
          {
            // Serial.write("PS: ");
            // Serial.print(pressedSwitch);    
            // Serial.println();
            releaseSwitch[SWITCH_MODE-1] = 0;
          } 
        }
      }      
    }    
    else
    {
      initialDebounceTime[SWITCH_MODE-1]  = 0;     
      releaseSwitch[SWITCH_MODE-1]        = 1; 
    }

//--------------------------------------------------    

    if(!digitalRead(SWITCH_SW2))
    {
      if(releaseSwitch[SWITCH_UP-1])
      {
        if(!initialDebounceTime[SWITCH_UP-1])
        {
          
          initialDebounceTime[SWITCH_UP-1] = millis();
          lastDebounceTime[SWITCH_UP-1] = initialDebounceTime[SWITCH_UP-1];
          // Serial.write("IniSW2: ");
          // Serial.print(lastDebounceTime[SWITCH_UP-1]);
          // Serial.println();
        }
        else
        {
          lastDebounceTime[SWITCH_UP-1] += millis();                   
          pressedSwitch = (lastDebounceTime[SWITCH_UP-1] > (initialDebounceTime[SWITCH_UP-1] + DEBOUNCE_PERIOD_MS)) ? SWITCH_UP : pressedSwitch; 
          if(pressedSwitch)   
          {
            // Serial.write("PS: ");
            // Serial.print(pressedSwitch);    
            // Serial.println();
            releaseSwitch[SWITCH_UP-1] = 0;
          } 
        }
      }      
    }    
    else
    {
      initialDebounceTime[SWITCH_UP-1]  = 0;     
      releaseSwitch[SWITCH_UP-1]        = 1; 
    }

//--------------------------------------------------

    if(!digitalRead(SWITCH_SW3))
    {
      if(releaseSwitch[SWITCH_DOWN-1])
      {
        if(!initialDebounceTime[SWITCH_DOWN-1])
        {
          
          initialDebounceTime[SWITCH_DOWN-1] = millis();
          lastDebounceTime[SWITCH_DOWN-1] = initialDebounceTime[SWITCH_DOWN-1];
          // Serial.write("IniSW3: ");
          // Serial.print(lastDebounceTime[SWITCH_DOWN-1]);
          // Serial.println();
        }
        else
        {
          lastDebounceTime[SWITCH_DOWN-1] += millis();                   
          pressedSwitch = (lastDebounceTime[SWITCH_DOWN-1] > (initialDebounceTime[SWITCH_DOWN-1] + DEBOUNCE_PERIOD_MS)) ? SWITCH_DOWN : pressedSwitch; 
          if(pressedSwitch)   
          {
            // Serial.write("PS: ");
            // Serial.print(pressedSwitch);    
            // Serial.println();
            releaseSwitch[SWITCH_DOWN-1] = 0;
          } 
        }
      }      
    }    
    else
    {
      initialDebounceTime[SWITCH_DOWN-1]  = 0;     
      releaseSwitch[SWITCH_DOWN-1]        = 1; 
    }    
  }  
    return true; // to repeat the action - false to stop
}

//Task that Updates 7 SEG display - 1ms period
bool TaskUpdatesDisplay(void *argument) 
{
    switch(opMOde)
    {
      case OPMODE_OPERATING:  
        if(display)      
          displayNumber( display );
        else
          displayNumber( 1 );

        break;
      case OPMODE_CONF_GREEN_TIME:
        displayNumber(cyclePeriod[CYCLE_GREEN]);
        break;
      case OPMODE_CONF_RED_TIME:
        displayNumber(cyclePeriod[CYCLE_RED]);
        break;
      case OPMODE_CONF_PULSE_TIME:
        displayNumber(cyclePeriod[CYCLE_PULSING_RED]);
        break;
    }
         
    // Serial.write("Contagem: ");
    // Serial.print(display);
    // Serial.write(" - Oper.Mode: ");
    // Serial.print(opMOde);
    // Serial.write("\n");
    return true; // to repeat the action - false to stop
}

//Task that updates the 4 LEDs - 100ms period
bool TaskUpdatesLeds(void *argument) 
{
  switch(opMOde)
  {
    case OPMODE_OPERATING:
      LED_D1_ON;
      LED_D2_OFF;
      LED_D3_OFF;
      LED_D4_OFF;
      break;
    case OPMODE_CONF_RED_TIME:
      LED_D1_OFF;
      LED_D2_ON;
      LED_D3_OFF;
      LED_D4_OFF;
      break;
    case OPMODE_CONF_GREEN_TIME:
      LED_D1_OFF;
      LED_D2_OFF;
      LED_D3_ON;
      LED_D4_OFF;
      break;
    case OPMODE_CONF_PULSE_TIME:
      LED_D1_OFF;
      LED_D2_OFF;
      LED_D3_OFF;
      LED_D4_ON;
      break;
    case OPMODE_CONF_PHASE_TEST:
      LED_D1_ON;
      LED_D2_ON;
      LED_D3_ON;
      LED_D4_ON;
      RELAY_RETURN_ON;
      break;
  }  
  return true; // to repeat the action - false to stop
}

//Task that updates counter
bool TaskUpdatesCounter(void *argument) 
{ 

    if(opMOde == OPMODE_OPERATING)
    {
      preDiv = (++preDiv) % (T_UPD_CNT_PREDIV_ONE_SECOND);
      
      switch(phaseCycle)
      {
        case CYCLE_RED:
          RELAY_RED_ON;
          RELAY_GREEN_OFF;
          RELAY_RETURN_ON;   

          if(display)
          {
            if(!preDiv)
              display--;
          }
            
          else
          {
            if(!preDiv)
            {
              RELAY_RED_OFF;
              RELAY_GREEN_ON;
              RELAY_RETURN_ON;   

              display = cyclePeriod[CYCLE_GREEN];
              phaseCycle = (phaseCycle+1) % (CYCLE_PULSING_RED + 1);
            }
            
          }

          // if(display < cyclePeriod[CYCLE_RED])
          // {
          //   if(!preDiv)
          //     display++;
          // }
            
          // else
          // {
          //   RELAY_RED_OFF;
          //   RELAY_GREEN_OFF;
          //   RELAY_RETURN_OFF;   

          //   display = 0;
          //   phaseCycle = (phaseCycle+1) % (CYCLE_PULSING_RED + 1);
          // }
          break;

        case CYCLE_GREEN:

          RELAY_RED_OFF;
          RELAY_GREEN_ON;
          RELAY_RETURN_ON;   

          if(display)
          {
            if(!preDiv)
              display--;
          }
          else
          {
            if(!preDiv)
            {
              //unsigned int countCycle = 0; //Conta até a VARIATION              

              

              //countCycle = 1;

              

              // if(!countCycle)
              //   risingOrFalling *= -1;

              RELAY_GREEN_OFF;
              RELAY_RED_ON;
              RELAY_RETURN_ON;                

              countCycle = risingOrFalling * ((++countCycle) % (VARIATION+1));

              Serial.write("\nCounting: ");
              Serial.print(countCycle);

              if(!countCycle)
                risingOrFalling *= -1;

              //cyclePeriod[CYCLE_PULSING_RED] = cyclePeriod[CYCLE_PULSING_RED] + countCycle;              

              display = cyclePeriod[CYCLE_PULSING_RED] + countCycle;

              Serial.write("\nCYCLE_PULSING_RED: ");
              Serial.print(display);

              

              phaseCycle = (phaseCycle+1) % (CYCLE_PULSING_RED+1);
              if(!cyclePeriod[CYCLE_PULSING_RED])
                phaseCycle = (phaseCycle+1) % (CYCLE_PULSING_RED+1);

            } 
          }
          break;

        case CYCLE_PULSING_RED:

        if(!digitalRead(RELAY_RED))
        {
          preDiv = (preDiv % BLINK_ON_PERIOD) == 0 ? 0 : preDiv;  
          //Serial.write("\nON: ");
          //Serial.print(preDiv);
        }
          
        else
        {
          preDiv = (preDiv % BLINK_OFF_PERIOD) == 0 ? 0 : preDiv;  
          //Serial.write("\nOFF: ");
          //Serial.print(preDiv);
        }      

          if(display)
            {
              if(!preDiv)
              {
                  
                  RELAY_GREEN_OFF;
                  RELAY_RED_TOGGLE;                
                  RELAY_RETURN_TOGGLE;
                  if(digitalRead(RELAY_RED))
                    display--;
              }
              
          }
          else
          { 
            if(!preDiv)
            {

              RELAY_GREEN_OFF;  
              RELAY_RED_ON;          
              RELAY_RETURN_ON;   
              
              
              //display = cyclePeriod[CYCLE_RED];
              display = cyclePeriod[CYCLE_RED] - countCycle;

              Serial.write("\nCYCLE_RED: ");
              Serial.print(display);

              phaseCycle = (phaseCycle+1) % (CYCLE_PULSING_RED+1);
            }            
          }
          break;
      }      
    }    
    return true; // to repeat the action - false to stop
}

/*==============================================================
=======================OTHER FUNCTIONS==========================
==============================================================*/

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
      //delay(1);
    } 
}

/*==============================================================
=======================ARDUINO DEFAULT==========================
==============================================================*/

void setup() 
{
  //Init Display Control pins  
  pinMode(SER,  OUTPUT);
  pinMode(SCKL, OUTPUT);  
  pinMode(RCLK, OUTPUT);
  
  //Init LEDs Pins
  pinMode(LED_D1, OUTPUT);
  pinMode(LED_D2, OUTPUT);
  pinMode(LED_D3, OUTPUT);
  pinMode(LED_D4, OUTPUT);

  //Init Relay Pins
  pinMode(RELAY_RETURN, OUTPUT);
  pinMode(RELAY_RED, OUTPUT);
  pinMode(RELAY_GREEN, OUTPUT); 
  pinMode(RELAY_YELLOW, OUTPUT); 

  RELAY_RED_OFF;
  RELAY_GREEN_OFF;
  RELAY_YELLOW_OFF;
  RELAY_RETURN_OFF;   

  //Init Switches Pins
  pinMode(SWITCH_SW1, INPUT);
  pinMode(SWITCH_SW2, INPUT);
  pinMode(SWITCH_SW3, INPUT);

  //Init Serial Port
  Serial.begin(115200);

  //Init TASKS PERIODS

  
  
  // Serial.print(TASK_GET_SWITCH_STATUS_PERIOD);
  // Serial.print(TASK_GET_SWITCH_STATUS_PERIOD);
  // Serial.print(TASK_EXECUTE_SWITCH_PERIOD);
  // Serial.print(TASK_UPDATE_LEDS_PERIOD);
  // Serial.print(TASK_UPDATE_COUNTER_PERIOD);
  // Serial.print(TASK_UPDATE_DISPLAY_PERIOD);

  timer.every(TASK_GET_SWITCH_STATUS_PERIOD, TaskGetSwitchStatus);
  timer.every(TASK_EXECUTE_SWITCH_PERIOD, TaskExecuteSwitch);
  timer.every(TASK_UPDATE_LEDS_PERIOD, TaskUpdatesLeds);
  timer.every(TASK_UPDATE_COUNTER_PERIOD, TaskUpdatesCounter);
  timer.every(TASK_UPDATE_DISPLAY_PERIOD, TaskUpdatesDisplay);

  // Serial.write("\nCYCLE_PULSING_RED: ");
  // Serial.print(cyclePeriod[CYCLE_RED]);
}

void loop() 
{

  // Serial.write("\nCounting times to 1s: ");  
  // Serial.print(COUNTING_TIMES_UNTIL_ONE_SECOND);

  
  // Serial.write("\nSwitch Status: ");  
  // Serial.print(TASK_GET_SWITCH_STATUS_PERIOD);

  // Serial.write("\nExecute Switch: ");
  // Serial.print(TASK_EXECUTE_SWITCH_PERIOD);

  // Serial.write("\nUpdate LEDs: ");
  // Serial.print(TASK_UPDATE_LEDS_PERIOD);

  // Serial.write("\nUpdate Counter: ");
  // Serial.print(TASK_UPDATE_COUNTER_PERIOD);

  // Serial.write("\nUpdate Display: ");
  // Serial.print(TASK_UPDATE_DISPLAY_PERIOD);   

  timer.tick();
  // delay(1000)  ;
}
