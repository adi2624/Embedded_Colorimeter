

/**
 * main.c
 */
#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"

#define RED_LED      (*((volatile uint32_t *)(0x42000000 + (0x400053FC-0x40000000)*32 + 5*4)))
#define GREEN_LED    (*((volatile uint32_t *)(0x42000000 + (0x400243FC-0x40000000)*32 + 5*4)))
#define BLUE_LED  (*((volatile uint32_t *)(0x42000000 + (0x400243FC-0x40000000)*32 + 4*4)))


#define RED_LED_MASK 16              //PB5
#define GREEN_LED_MASK 16            //PE5
#define BLUE_LED_MASK 8            //PE4



void initHw()
{
    // Configure HW to work with 16 MHz XTAL, PLL enabled, system clock of 40 MHz
    SYSCTL_RCC_R = SYSCTL_RCC_XTAL_16MHZ | SYSCTL_RCC_OSCSRC_MAIN | SYSCTL_RCC_USESYSDIV | (4 << SYSCTL_RCC_SYSDIV_S);

    // Set GPIO ports to use APB (not needed since default configuration -- for clarity)
    SYSCTL_GPIOHBCTL_R = 0;

    // Enable GPIO port B and E peripherals
    SYSCTL_RCGC2_R = SYSCTL_RCGC2_GPIOB || SYSCTL_RCGC2_GPIOE;


    // Configure LED and pushbutton pins
    GPIO_PORTE_DIR_R = GREEN_LED_MASK | BLUE_LED_MASK;  // bits 2,3,4 and 5 are outputs, other pins are inputs
    GPIO_PORTE_DR2R_R= GREEN_LED_MASK | BLUE_LED_MASK; // set drive strength to 2mA (not needed since default configuration -- for clarity)
    GPIO_PORTE_DEN_R = GREEN_LED_MASK | BLUE_LED_MASK;  // enable LEDs and enable outputs for Push Buttons
    GPIO_PORTB_DIR_R = RED_LED_MASK;  // bits 2,3,4 and 5 are outputs, other pins are inputs
    GPIO_PORTB_DR2R_R= RED_LED_MASK; // set drive strength to 2mA (not needed since default configuration -- for clarity)
    GPIO_PORTB_DEN_R = RED_LED_MASK;
    GPIO_PORTE_ODR_R = GREEN_LED_MASK | BLUE_LED_MASK;
    GPIO_PORTB_ODR_R = RED_LED_MASK;
    //enable internal pull down resistor for push button 2
   //PUSH_BUTTON_BITS are inputs by default.
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

int main(void)
  {
    initHw();

   while(1){



    GREEN_LED=1; // Turn the RED LED on.


    waitMicrosecond(1000000);

    GREEN_LED = 0;

    waitMicrosecond(1000000);


   }

	return 0;
}
