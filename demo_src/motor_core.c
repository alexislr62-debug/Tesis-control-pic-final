#include <xc.h>
#include "motor_core.h"
#include "app_context.h"

typedef struct {
    uint8_t portD_value;
    uint8_t pinB_value;
} Step;

static const Step sequence[10] = {
    {0b10000101, 0b00}, {0b10100001, 0b00}, {0b00100001, 0b01},
    {0b00101000, 0b01}, {0b01001000, 0b01}, {0b01001010, 0b00},
    {0b01010010, 0b00}, {0b00010010, 0b10}, {0b00010100, 0b10},
    {0b10000100, 0b10}
};

void Motor_Initialize(void) {
    TRISD = 0x00;
    TRISBbits.TRISB0 = 0;
    TRISBbits.TRISB1 = 0;
    TRISEbits.RE1 = 0;
    Motor_Disable_Coils();
}

void Motor_Disable_Coils(void) {
    LATD = 0x00;
    LATBbits.LATB0 = 0;
    LATBbits.LATB1 = 0;
    LATEbits.LATE1 = 0;
}

void Motor_Execute_Step(void) {
    if (ctx.direction == 1) {
        ctx.currentMotorPhase = (ctx.currentMotorPhase + 1) % 10;
    } else {
        ctx.currentMotorPhase = (ctx.currentMotorPhase == 0) ? 9 : (ctx.currentMotorPhase - 1);
    }

    LATD = sequence[ctx.currentMotorPhase].portD_value;
    LATBbits.LATB0 = sequence[ctx.currentMotorPhase].pinB_value & 0x01;
    LATBbits.LATB1 = (sequence[ctx.currentMotorPhase].pinB_value >> 1) & 0x01;
}