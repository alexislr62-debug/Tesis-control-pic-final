#include <xc.h>
#include "adc.h"

void ADC_Initialize(void) {
    // Configura AN0 como entrada analógica (Pin 2)
    TRISA0 = 1;
    ADCON1 = 0x0E; // VREF- = VSS, VREF+ = VDD, AN0 es analógico
    
    // ADCON2: Right justified, 20 TAD, FOSC/64 para 48MHz
    ADCON2 = 0xBE; 
    
    ADCON0bits.ADON = 1; // Encender módulo ADC
}

uint16_t ADC_Read(uint8_t channel) {
    ADCON0bits.CHS = channel; // Seleccionar canal
    ADCON0bits.GO_DONE = 1;   // Iniciar conversión
    
    while(ADCON0bits.GO_DONE); // Esperar a que termine
    
    return ((uint16_t)((ADRESH << 8) + ADRESL));
}