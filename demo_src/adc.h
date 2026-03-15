#ifndef ADC_H
#define ADC_H

#include <stdint.h>

void ADC_Initialize(void);
uint16_t ADC_Read(uint8_t channel);

#endif