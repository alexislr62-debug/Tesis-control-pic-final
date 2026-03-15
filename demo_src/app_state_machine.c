#include "app_state_machine.h"
#include "app_context.h"
#include "timer1.h"
#include "motor_core.h"
#include <stdio.h>
#include <string.h>

// Librerías de la MLA para usar el CDC
#include "usb.h"
#include "usb_device_cdc.h"

// Frecuencia requerida para usar __delay_ms()
#define _XTAL_FREQ 48000000 
#include <xc.h>
#include <math.h> // Necesario para el cos() de la simulación

// Definimos los estados exactos que tenías en tu main
typedef enum {
    STATE_USB_NOT_CONFIGURED,
    STATE_IDLE,
    STATE_RUNNING,
    STATE_SAMPLING,
    STATE_SEND_FINISHED
} SystemState;

static SystemState state = STATE_USB_NOT_CONFIGURED;

// Función auxiliar rápida para responder a C#
static void sendUSBImmediate(const char *str) {
    // 1. Protección de seguridad por si el cable no está conectado
    if (USBGetDeviceState() < CONFIGURED_STATE) {
        return; 
    }

    // 2. Esperar activamente a que el buffer USB esté libre
    // (Llamando a CDCTxService para que el módulo USB haga su trabajo)
    while (!USBUSARTIsTxTrfReady()) {
        CDCTxService(); 
    }

    // 3. Poner el dato en el buffer
    putUSBUSART((uint8_t*)str, strlen(str));

    // 4. Forzar la transmisión inmediata
    CDCTxService(); 
}
// Función real de muestreo y diezmado (oversampling)
static uint16_t getDecimatedADC(void) {
    uint32_t acumulador = 0;
    const int muestras = 64; // Las 64-128 muestras de tu algoritmo

    for (int i = 0; i < muestras; i++) {
        ADCON0bits.GO = 1;
        while (ADCON0bits.GO_nDONE); // Espera rapidísima de hardware
        acumulador += ((uint16_t)ADRESH << 8) | ADRESL;
    }

    return (uint16_t)(acumulador / muestras);
}

void APP_StateMachine_Tasks(void)
{
    // 1. Monitorear conexión física del USB (Seguridad ante desconexión)
    if (USBGetDeviceState() < CONFIGURED_STATE) {
        state = STATE_USB_NOT_CONFIGURED;
        ctx.handshake_received = false; 
        ctx.experimentRunning = false;
        TMR1_Stop();           // Apagamos hardware por seguridad
        Motor_Disable_Coils(); 
        return;
    }

    // 2. Transiciones de la Máquina de Estados
    switch (state) {
        
        case STATE_USB_NOT_CONFIGURED:
            // Si el hardware USB ya enumeró, pasamos a esperar comandos
            if (USBGetDeviceState() >= CONFIGURED_STATE) {
                state = STATE_IDLE;
            }
            break;

        case STATE_IDLE:
            if (ctx.experimentRunning) {
                ctx.stepsExecutedTotal = 0; // Reiniciamos contador antes de empezar
                ctx.currentPart = 0;        // Reiniciamos particiones
                ctx.isFinished = false;
                TMR1_Start();               // Arranca el Timer1 (10ms)
                state = STATE_RUNNING;
            }
            break;

        case STATE_RUNNING:
            if (!ctx.experimentRunning) {
                TMR1_Stop();
                Motor_Disable_Coils();
                state = STATE_IDLE;
                break;
            }

            if (ctx.evtStepReady) {
                ctx.evtStepReady = false;
                ctx.stepsExecutedTotal++; // Cuenta los pasos del tramo actual

                // RAMA 1: Movimiento Libre (Solo imprime progreso, no mide)
                if (ctx.mode == MODE_FREEC) {
                    if ((ctx.stepsExecutedTotal % 5 == 0) || (ctx.stepsExecutedTotal == ctx.totalStepsToRun)) {
                        char stepMsg[20];
                        sprintf(stepMsg, "S:%lu\r\n", ctx.stepsExecutedTotal);
                        sendUSBImmediate(stepMsg);
                    }
                    if (ctx.stepsExecutedTotal >= ctx.totalStepsToRun) {
                        ctx.isFinished = true;
                    }
                }
                // RAMA 2: Interferómetro (Mide "al vuelo" sin detener el motor)
                else if (ctx.mode == MODE_INTERF) {
                    if (ctx.stepsExecutedTotal >= ctx.totalStepsToRun) {
                        // NO llamamos a TMR1_Stop(). El motor sigue rodando.
                        state = STATE_SAMPLING; 
                    }
                }
                // RAMA 3: Z-Scan y Anchos (Se detiene a medir)
                else if (ctx.mode == MODE_ZSCAN || ctx.mode == MODE_WO_CHOPPER) {
                    if (ctx.stepsExecutedTotal >= ctx.totalStepsToRun) {
                        TMR1_Stop();            // Pausa física del motor
                        state = STATE_SAMPLING; // Brincamos a medir
                    }
                }
            }

            if (ctx.isFinished) {
                TMR1_Stop(); 
                state = STATE_SEND_FINISHED;
            }
            break;

        case STATE_SAMPLING:
            // 1. ESPERAR TIA (4ms de estabilización)
            __delay_ms(4);

            // 2. SELECCIÓN DE MÉTODOS DE MUESTREO
            if (ctx.mode == MODE_INTERF) {
                // EMULACIÓN DE INTERFERÓMETRO: 4 * sin^2(10x) * e^(-x^2 / 2)
                
                // A) Mapeamos la partición a un rango de x entre -3 y +3 (donde la gaussiana tiene su mayor efecto)
                float x = -3.0f + (6.0f * (float)ctx.currentPart) / (float)(ctx.totalParts > 1 ? ctx.totalParts - 1 : 1);
                
                // B) Calculamos las partes de la ecuación
                float seno = sin(10.0f * x);
                float gauss = exp(-(x * x) / 2.0f);
                float f_x = 4.0f * (seno * seno) * gauss; // Va de 0 a ~4.0
                
                // C) Mapeamos al ADC (0 - 1023). Multiplicamos por 200 y sumamos 100 de "base".
                float adc_simulado = 100.0f + (200.0f * f_x); 
                
                // Limitadores de seguridad
                if(adc_simulado > 1023.0f) adc_simulado = 1023.0f;
                if(adc_simulado < 0.0f) adc_simulado = 0.0f;
                
                ctx.latestADCValue = (uint16_t)adc_simulado; 
            }
            else if (ctx.mode == MODE_ZSCAN || ctx.mode == MODE_WO_CHOPPER) {
                // EMULACIÓN Z-SCAN (n2 < 0) que ya teníamos
                float x = -5.0f + (10.0f * (float)ctx.currentPart) / (float)(ctx.totalParts > 1 ? ctx.totalParts - 1 : 1);
                float t_val = -x / ((x*x + 1.0f) * (x*x + 1.0f));
                
                float adc_simulado = 512.0f + (250.0f * t_val);
                if(adc_simulado > 1023.0f) adc_simulado = 1023.0f;
                if(adc_simulado < 0.0f) adc_simulado = 0.0f;
                
                ctx.latestADCValue = (uint16_t)adc_simulado;
            }

            // ------------------------------------------------------------------
            // ⚠️ PARA EL LABORATORIO: COMENTA LO DE ARRIBA Y DESCOMENTA ESTO ⚠️
            // ctx.latestADCValue = getDecimatedADC();
            // ------------------------------------------------------------------

            // 3. ENVÍO A C#
            char dataMsg[30];
            sprintf(dataMsg, "Z:%lu,%u\r\n", ctx.currentPart, ctx.latestADCValue);
            sendUSBImmediate(dataMsg);

            // 4. LÓGICA DEL BUCLE
            ctx.currentPart++; 

            if (ctx.currentPart >= ctx.totalParts) {
                ctx.isFinished = true;
                state = STATE_SEND_FINISHED;
            } else {
                ctx.stepsExecutedTotal = 0; 
                
                if (ctx.mode == MODE_ZSCAN || ctx.mode == MODE_WO_CHOPPER) {
                    TMR1_Start(); // Revivir el motor si estaba en modo de pausas
                }
                state = STATE_RUNNING;
            }
            break;
        case STATE_SEND_FINISHED:
            // Usamos nuestra función a prueba de balas en lugar de putUSBUSART directo
            sendUSBImmediate("FINISHED\r\n");
            
            // Retornamos al reposo de forma segura
            state = STATE_IDLE; 
            ctx.isFinished = false;
            ctx.experimentRunning = false; 
            ctx.stepsExecutedTotal = 0;
            break;
            break;
            
        default:
            state = STATE_IDLE;
            break;
    }
}