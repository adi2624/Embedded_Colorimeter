// Serial Example
// Jason Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL Evaluation Board
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

// Hardware configuration:
// Red LED:
//   PF1 drives an NPN transistor that powers the red LED
// Green LED:
//   PF3 drives an NPN transistor that powers the green LED
// UART Interface:
//   U0TX (PA1) and U0RX (PA0) are connected to the 2nd controller
//   The USB on the 2nd controller enumerates to an ICDI interface and a virtual COM port
//   Configured to 115,200 baud, 8N1

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#define MAX_CHARS 80
#define MAX_FIELDS 5

//#define DEBUG

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "tm4c123gh6pm.h"

#define RED_LED      (*((volatile uint32_t *)(0x42000000 + (0x400053FC-0x40000000)*32 + 5*4)))
#define GREEN_LED    (*((volatile uint32_t *)(0x42000000 + (0x400243FC-0x40000000)*32 + 5*4)))
#define BLUE_LED    (*((volatile uint32_t *)(0x42000000 + (0x400243FC-0x40000000)*32 + 4*4)))

#define RED_LED_MASK 32
#define BLUE_LED_MASK 16
#define GREEN_LED_MASK 32

#define DEBUG

uint8_t pos[MAX_FIELDS];
uint8_t type[MAX_FIELDS];
uint8_t field_count = 0;
char strInput[MAX_CHARS];

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// Initialize Hardware
void initHw()
{
    // Configure HW to work with 16 MHz XTAL, PLL enabled, system clock of 40 MHz
    SYSCTL_RCC_R = SYSCTL_RCC_XTAL_16MHZ | SYSCTL_RCC_OSCSRC_MAIN | SYSCTL_RCC_USESYSDIV | (4 << SYSCTL_RCC_SYSDIV_S) | SYSCTL_RCC_USEPWMDIV | SYSCTL_RCC_PWMDIV_2 ;

    // Set GPIO ports to use APB (not needed since default configuration -- for clarity)
    SYSCTL_GPIOHBCTL_R = 0;

    // Enable GPIO port B and E peripherals
    SYSCTL_RCGC2_R = SYSCTL_RCGC2_GPIOE | SYSCTL_RCGC2_GPIOB | SYSCTL_RCGC2_GPIOA | SYSCTL_RCGC2_GPIOF;         // turn-on SSI2 clocking
    SYSCTL_RCGC0_R |= SYSCTL_RCGC0_PWM0;             // turn-on PWM0 module
    // Configure LED pin B5

    SYSCTL_RCGCADC_R |= 1;                           // turn on ADC module 0 clocking
    SYSCTL_RCGCUART_R |= SYSCTL_RCGCUART_R0;


    GPIO_PORTE_AFSEL_R |= 0x08;                      // select alternative functions for AN0 (PE3)
    GPIO_PORTE_DEN_R &= ~0x08;                       // turn off digital operation on pin PE3
    GPIO_PORTE_AMSEL_R |= 0x08;


    ADC0_CC_R = ADC_CC_CS_SYSPLL;                    // select PLL as the time base (not needed, since default value)
    ADC0_ACTSS_R &= ~ADC_ACTSS_ASEN3;                // disable sample sequencer 3 (SS3) for programming
    ADC0_EMUX_R = ADC_EMUX_EM3_PROCESSOR;            // select SS3 bit in ADCPSSI as trigger
    ADC0_SSMUX3_R = 0;                               // set first sample to AN0
    ADC0_SSCTL3_R = ADC_SSCTL3_END0;                 // mark first sample as the end
    ADC0_ACTSS_R |= ADC_ACTSS_ASEN3;                 // enable SS3 for operation


    GPIO_PORTB_DIR_R = RED_LED_MASK;  // bit 5 is an output
    GPIO_PORTB_DR2R_R = RED_LED_MASK; // set drive strength to 2mA (not needed since default configuration -- for clarity)
    GPIO_PORTB_DEN_R = RED_LED_MASK;  // enable LEDs
    GPIO_PORTB_AFSEL_R |= 0x20; // select auxilary function for bit 5
    GPIO_PORTB_PCTL_R = GPIO_PCTL_PB5_M0PWM3; // enable PWM on bit 5
    GPIO_PORTB_ODR_R = RED_LED_MASK;

    // Configure LED pin E4 E5
    GPIO_PORTE_DIR_R = GREEN_LED_MASK | BLUE_LED_MASK;  // bits 4 and 5 are outputs, other pins are inputs
    GPIO_PORTE_DR2R_R = GREEN_LED_MASK | BLUE_LED_MASK;// set drive strength to 2mA (not needed since default configuration -- for clarity)
    GPIO_PORTE_DEN_R = GREEN_LED_MASK | BLUE_LED_MASK;  // enable LEDs
    GPIO_PORTE_AFSEL_R |= 0x30; // select auxilary function for bits 4 and 5
    GPIO_PORTE_PCTL_R = GPIO_PCTL_PE4_M0PWM4 | GPIO_PCTL_PE5_M0PWM5; // enable PWM on bits 4 and 5
    GPIO_PORTE_ODR_R = GREEN_LED_MASK | BLUE_LED_MASK;

       __asm(" NOP");                                   // wait 3 clocks
       __asm(" NOP");
       __asm(" NOP");

       SYSCTL_SRPWM_R = SYSCTL_SRPWM_R0;                // reset PWM0 module
       SYSCTL_SRPWM_R = 0;                              // leave reset state
       PWM0_1_CTL_R = 0;                                // turn-off PWM0 generator 1
       PWM0_2_CTL_R = 0;                                // turn-off PWM0 generator 2
       PWM0_1_GENB_R = PWM_0_GENB_ACTCMPBD_ZERO | PWM_0_GENB_ACTLOAD_ONE;
                                                        // output 3 on PWM0, gen 1b, cmpb
       PWM0_2_GENA_R = PWM_0_GENA_ACTCMPAD_ZERO | PWM_0_GENA_ACTLOAD_ONE;
                                                        // output 4 on PWM0, gen 2a, cmpa
       PWM0_2_GENB_R = PWM_0_GENB_ACTCMPBD_ZERO | PWM_0_GENB_ACTLOAD_ONE;
                                                        // output 5 on PWM0, gen 2b, cmpb
       PWM0_1_LOAD_R = 1024;                            // set period to 40 MHz sys clock / 2 / 1024 = 19.53125 kHz
       PWM0_2_LOAD_R = 1024;

                                                        // invert outputs for duty cycle increases with increasing compare values
       PWM0_1_CMPB_R = 0;                               // red off (0=always low, 1023=always high)
       PWM0_2_CMPB_R = 0;                               // green off
       PWM0_2_CMPA_R = 0;                               // blue off

       PWM0_1_CTL_R = PWM_0_CTL_ENABLE;                 // turn-on PWM0 generator 1
       PWM0_2_CTL_R = PWM_0_CTL_ENABLE;                 // turn-on PWM0 generator 2
       PWM0_ENABLE_R = PWM_ENABLE_PWM3EN | PWM_ENABLE_PWM4EN | PWM_ENABLE_PWM5EN;
                                                        // enable outputs




    // Configure UART0 pins
       SYSCTL_RCGCUART_R |= SYSCTL_RCGCUART_R0;         // turn-on UART0, leave other uarts in same status
       GPIO_PORTA_DEN_R |= 3;                           // enable digital on UART0 pins: default, added for clarity
       GPIO_PORTA_AFSEL_R |= 3;                         // use peripheral to drive PA0, PA1: default, added for clarity
       GPIO_PORTA_PCTL_R = GPIO_PCTL_PA1_U0TX | GPIO_PCTL_PA0_U0RX;
                                                        // select UART0 to drive pins PA0 and PA1: default, added for clarity

       // Configure UART0 to 115200 baud, 8N1 format (must be 3 clocks from clock enable and config writes)
       UART0_CTL_R = 0;                                 // turn-off UART0 to allow safe programming
       UART0_CC_R = UART_CC_CS_SYSCLK;                  // use system clock (40 MHz)
       UART0_IBRD_R = 21;                               // r = 40 MHz / (Nx115.2kHz), set floor(r)=21, where N=16
       UART0_FBRD_R = 45;                               // round(fract(r)*64)=45
       UART0_LCRH_R = UART_LCRH_WLEN_8 | UART_LCRH_FEN; // configure for 8N1 w/ 16-level FIFO
       UART0_CTL_R = UART_CTL_TXE | UART_CTL_RXE | UART_CTL_UARTEN; // enable TX, RX, and module

}

int16_t readAdc0Ss3()
{
    ADC0_PSSI_R |= ADC_PSSI_SS3;                     // set start bit
    while (ADC0_ACTSS_R & ADC_ACTSS_BUSY);           // wait until SS3 is not busy
    return ADC0_SSFIFO3_R;                           // get single result from the FIFO
}

void waitMicrosecond(uint32_t us)
{
    __asm("WMS_LOOP0:   MOV  R1, #6");          // 1
    __asm("WMS_LOOP1:   SUB  R1, #1");          // 6
    __asm("             CBZ  R1, WMS_DONE1");   // 5+1*3
    __asm("             NOP");                  // 5
    __asm("             NOP");                  // 5
    __asm("             B    WMS_LOOP1");       // 5*2 (speculative, so P=1)
    __asm("WMS_DONE1:   SUB  R0, #1");          // 1
    __asm("             CBZ  R0, WMS_DONE0");   // 1
    __asm("             NOP");                  // 1
    __asm("             B    WMS_LOOP0");       // 1*2 (speculative, so P=1)
    __asm("WMS_DONE0:");                        // ---
                                                // 40 clocks/us + error
}

void putcUart0(char c)
{
    while (UART0_FR_R & UART_FR_TXFF);               // wait if uart0 tx fifo full
    UART0_DR_R = c;                                  // write character to fifo
}

// Blocking function that writes a string when the UART buffer is not full
void putsUart0(char* str)
{
    uint8_t i;
    for (i = 0; i < strlen(str); i++)
      putcUart0(str[i]);
}

// Blocking function that returns with serial data once the buffer is not empty
char getcUart0()
{
    while (UART0_FR_R & UART_FR_RXFE);               // wait if uart0 rx fifo empty
    return UART0_DR_R & 0xFF;                        // get character from fifo
}


void getsUart0(char *str, uint8_t maxChars)
{
    int count = 0;

    while(1)
    {
    char input = getcUart0();
    if(input==127 && count!=0)
    {
        count--;
    }

    else if(input==13)
    {
        str[count] = '\0';
        return;
    }

    else if(input>=32)
    {
            if(input>=65 && input<=90)
            {
                input = input + 32;
                str[count]=input;
                count++;

            }

            else
            {
                str[count]=input;
                count++;

            }

            if(count==maxChars)
                                   {
                                       str[count] = '\0';
                                       return;
                                   }

    }

   }
}






int isAlpha(char c)
{
    if( (c>=65 && c<=90) || (c>=97 && c<=122))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int isNum(char c)
{
    if(c>=48 && c<=57)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int isDelim(char c)
{
    if( c==32 || c==44 || c==58 || c==59 || c==124 )
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int tokenize_string(char *str, int field_count)
{
    int i=0;
    field_count = 0;
    while(str[i]!='\0')
    {
        if(i==0)
        {
            if( isAlpha(str[i]) )
            {
                pos[field_count] = i;
                field_count++;

            }
              // do nothing for the first character
        }
        else
        {
            if( isDelim(str[i-1]) && isAlpha(str[i]) )
            {
                pos[field_count] = i;
                field_count++;

            }

            else if( isDelim(str[i-1]) && isNum(str[i]) )
            {
                pos[field_count] = i;
                field_count++;
            }


        }

        i++;
    }

    i=0;

    while(str[i]!='\0')
    {
        if( isDelim(str[i]) )
        {
            str[i] = '\0';
        }
        i++;
    }

    return field_count;

}


bool isCommand(char *str, uint8_t min_args)
{
    //assumption made that the first word in a command is the operation.

    char* command = &strInput[pos[0]];
    uint8_t i=0;
    if(strcmp(command,str)==0)
    {
        if( (field_count-1) >= min_args)
        {
            return true;
        }

        else
        {
            return false;
        }
    }

    else
    {
        return false;
    }
}

char* itoA(uint16_t number)
{
    uint16_t digit = 0;
    char value[10];
    uint8_t char_counter = 0;
    while(number>0)
    {
        digit = number%10;
        number = number/10;
        value[char_counter] = digit + 48;
        char_counter++;
    }
    char_counter--;
    uint8_t start_counter = 0;
    uint8_t end_counter = char_counter;
    while(end_counter > char_counter/2 )
        {
                    char temp = value[end_counter];
                    value[end_counter] = value[start_counter];
                    value[start_counter] = temp;
                    start_counter++;
                    end_counter--;
        }

    return value;
}
char* getString(uint8_t arg_number)
{
    char* arg = &strInput[pos[arg_number]];
    return arg;
}


uint16_t myAtoi(char* str)
{
    uint16_t res = 0;
    uint16_t i =0;   // Initialize result

    // Iterate through all characters of input string and
    // update result
    for (i = 0; str[i] != '\0'; ++i)
        res = res*10 + str[i] - '0';

    return res;     // return result.
}

uint16_t getValue(uint8_t arg_number)
{
    char* value = &strInput[pos[arg_number]];
    uint16_t return_value = myAtoi(value);
    return return_value;
}



void setRgbColor(uint16_t red, uint16_t green, uint16_t blue)
{
    PWM0_1_CMPB_R = red;
    PWM0_2_CMPA_R = green;           //blue
    PWM0_2_CMPB_R = blue;          //green
}


int main(void)
{
    // Initialize hardware
        initHw();



        while(1)
        {
            int j=0;
            putsUart0("Colorimeter Commands: \n 1. RGB x,x,x \n 2. light \n ");
            getsUart0(strInput,MAX_CHARS);
            field_count = tokenize_string(strInput,field_count);



          if(isCommand("blue",1))
              {
                  char* toggle_switch;
                  toggle_switch = getValue(1);
                  putsUart0(toggle_switch);
                  if(toggle_switch==22)
                      {
                          putsUart0("Success");
                          while(1)
                              {
                                  BLUE_LED = 0;
                                  RED_LED = 0;
                                  GREEN_LED = 0;
                              }
                      }
              else
                  {
                      putsUart0("Failed to turn on BLUE LED");
                  }

          }

          else if (isCommand("rgb",3))
              {
                  uint16_t red_arg = getValue(1);
                  uint16_t green_arg = getValue(2);
                  uint16_t blue_arg = getValue(3);


                  setRgbColor(red_arg,green_arg,blue_arg);



                  putsUart0("Successfully turned on LED ");

              }

          else if (isCommand("light",0))
              {
                  uint16_t adc_value = 0;
                  adc_value =  readAdc0Ss3();
                  char *value = itoA(adc_value);
                  value[4] = '\0';
                  putsUart0(value);
                  putsUart0("\r\n");
              }

          else
              {
                  putsUart0("Failed to turn on LED");
              }

        }


}

