#include <xc.h>
#include "interrupt_manager.h"
#include "timer1.h"
#include "motor_core.h"
#include "app_context.h"

void Interrupt_Initialize(void) {
    // Mantenemos el modo de compatibilidad (sin prioridades IPEN=0)
    // para que la interrupción no choque con la librería USB de Microchip
    RCONbits.IPEN = 0; 
    
    INTCONbits.PEIE = 1; // Habilitar interrupciones de periféricos
    INTCONbits.GIE = 1;  // Habilitar interrupciones globales
}

// Vector de interrupción principal de XC8 (Se llama automáticamente por hardware)
void __interrupt() ISR_Manager(void) {
    
    // 1. Verificamos si la interrupción fue causada por el desbordamiento del Timer1 (10ms)
    if (PIE1bits.TMR1IE == 1 && PIR1bits.TMR1IF == 1) {
        
        PIR1bits.TMR1IF = 0; // Obligatorio: Bajar la bandera primero
        TMR1_Reload();       // Recargar cuenta inmediatamente para no perder tiempo
        
        // Si la máquina de estados mandó a correr el experimento...
        if (ctx.experimentRunning) {
            Motor_Execute_Step();    // Mueve el motor un pasito físicamente
            ctx.evtStepReady = true; // Levanta la bandera para que la Máquina de Estados la procese
        }
    }
    
    // 2. Aquí en el futuro agregaremos el código de tu interrupción externa (INT0/INT1)
    // para detectar el disparo del Chopper óptico.
}