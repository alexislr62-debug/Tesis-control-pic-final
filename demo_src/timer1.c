#include <xc.h>
#include "timer1.h"

#define TMR1_RELOAD_H 0xC5
#define TMR1_RELOAD_L 0x68

void TMR1_Initialize(void) {
    T1CON = 0b00110000; // Prescaler 1:8, Fosc/4, Timer apagado
    TMR1H = TMR1_RELOAD_H;
    TMR1L = TMR1_RELOAD_L;
    
    PIR1bits.TMR1IF = 0;
    PIE1bits.TMR1IE = 1;
}

void TMR1_Start(void) {
    TMR1_Reload();
    T1CONbits.TMR1ON = 1;
}

void TMR1_Stop(void) {
    T1CONbits.TMR1ON = 0;
}

void TMR1_Reload(void) {
    TMR1H = TMR1_RELOAD_H;
    TMR1L = TMR1_RELOAD_L;
}