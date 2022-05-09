```c

/*
 * 
 * 
 * 
 * 
 * 
 *                        --------------------------------------
 *                       |
 *                       |P1.0
 *                       |1
 *                       |2
 *                       |3
 *                       |4
 *                       |5
 *                       |6
 *                       | 7
 *                       |
 *                       |
 *                       |
 *                       |
 *                       |
 *                       | 
 *                       |
 *                       |
 *                       | 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * */



#include <msp430.h> 


#include <stdio.h>

#include "gpio.h"
#include "lcd.h"


#define BUTTON  BIT1   // Porta P4.1 - botão usado como reset



#define BUTTON_PORT P2
#define BUTTON_0  BIT5



volatile uint8_t pulses = 0;



void hardware_init()
{
    /* Habilita resistor de pull up ou pull down */
    P4REN |= BUTTON;
    /* Habilita resitor como pull up */
    P4OUT |= BUTTON;
}




void config_ext_irq(){
    /* Pull up/down */
    PORT_REN(BUTTON_PORT) = BUTTON_0;

    /* Pull up */
    PORT_OUT(BUTTON_PORT) = BUTTON_0;

    /* Habilitação da IRQ apenas botão */
    PORT_IE(BUTTON_PORT) =  BUTTON_0;

    /* Transição de nível baixo para alto - borda de subida */
    /*Como queremos tudo em 0 não precisa do comando abaixo
     * Port x interrupt edge select
     * 0b = PxIFG flag is set with a low-to-high transition*/
    //PORT_IES(BUTTON_PORT) = BUTTON_0;

    /* Limpa alguma IRQ pendente */
    PORT_IFG(BUTTON_PORT) &= ~BUTTON_0;
}




void main(){
    
    char string[8];
    
    /* Configuração de hardware */
    WDTCTL = WDTPW | WDTHOLD;

#if defined (__MSP430FR2355__)
    /* Disable the GPIO power-on default high-impedance mode */
    PM5CTL0 &= ~LOCKLPM5;
#endif

    
    /* Incializa o hardware */
           hardware_init();
    
    
    /* Inicializa hardare: veja lcd.h para
         * configurar pinos */
        lcd_init_4bits();
        /* Escreve string */

        x = PIN15;

        /* Configura interupções */
        config_ext_irq();

        /* Habilita IRQs e desliga CPU */
        __bis_SR_register(GIE);

        lcd_send_data(LCD_LINE_0, LCD_CMD);
        lcd_write_string("Quant. pulsos:");

        _delay_cycles(100000);

        while (1){
            lcd_send_data(LCD_LINE_1, LCD_CMD);
            snprintf(string, 8, "%d", pulses);
            
            /* Botão 1: reset. Quando pressionado, o valor do contador é zerado.
             * Detecção na função main, sem interrupção externa. 
             * Quando o botão for pressionado manda 0, só que isso não daria verdadeiro então por isso o '!'*/
            if (!TST_BIT(P4IN, BUTTON)) /* Equivalente a: if (P4IN & BIT1) */
              pulses = 0;
            
            
                

            lcd_write_string(string);
            //i++;

            _delay_cycles(10000);
        }
    
    
    
}


/* Sensor 1: incrementar o valor a cada borda de subida.
 * Detecção é em uma interrupção externa.*/

/* Port 1 ISR (interrupt service routine) */
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=PORT2_VECTOR
__interrupt void Port_2(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(PORT1_VECTOR))) Port_1 (void)
#else
#error Compiler not supported!
#endif
{
    
    if(pulses <= 100000)
        pulses++;
    else
        lcd_send_data(LCD_LINE_1, LCD_CMD);
        lcd_write_string("Maximo atingido!");

    /* Limpa sinal de IRQ do botão 0 */
    PORT_IFG(BUTTON_PORT) &= ~BUTTON_0;
}

```
