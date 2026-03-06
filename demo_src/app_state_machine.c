#include "app_state_machine.h"
#include "app_context.h"
#include "timer1.h"
#include "motor_core.h"
#include <stdio.h>
#include <string.h>

// Librerías de la MLA para usar el CDC
#include "usb.h"
#include "usb_device_cdc.h"

// Definimos los estados exactos que tenías en tu main
typedef enum {
    STATE_USB_NOT_CONFIGURED,
    STATE_IDLE,
    STATE_RUNNING,
    STATE_SEND_FINISHED
} SystemState;

static SystemState state = STATE_USB_NOT_CONFIGURED;

// Función auxiliar rápida para responder a C#
static void sendUSBImmediate(const char *str) {
    if(USBUSARTIsTxTrfReady()) {
        putUSBUSART((uint8_t*)str, strlen(str));
    }
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
                ctx.isFinished = false;
                TMR1_Start();               // Arranca el Timer1 (10ms)
                state = STATE_RUNNING;
            }
            break;

        case STATE_RUNNING:
            // Si C# mandó un STOP abrupto a mitad de camino
            if (!ctx.experimentRunning) {
                TMR1_Stop();
                Motor_Disable_Coils();
                state = STATE_IDLE;
                break;
            }

            // Atendemos la bandera de la interrupción del motor
            if (ctx.evtStepReady) {
                ctx.evtStepReady = false;
                ctx.stepsExecutedTotal++;
                // NUEVO: Enviar el paso actual a la consola de C#
                // Usamos un formato simple para no saturar el ancho de banda
                char stepMsg[20];
                sprintf(stepMsg, "S:%lu\r\n", ctx.stepsExecutedTotal);
                sendUSBImmediate(stepMsg);

                // ¿Ya llegamos a la cantidad de pasos que pidió C#?
                if (ctx.stepsExecutedTotal >= ctx.totalStepsToRun) {
                    ctx.isFinished = true;
                }
            }

            // Si ya terminó el trayecto
            if (ctx.isFinished) {
                TMR1_Stop(); // Detenemos el motor para tomar lecturas estables
                state = STATE_SEND_FINISHED;
            }
            break;

        case STATE_SEND_FINISHED:
            // 1. Verificamos si el USB está listo para aceptar un nuevo paquete
            if (USBUSARTIsTxTrfReady()) 
            {
                // 2. Enviamos el mensaje de finalización
                // Usamos \r\n para asegurar la compatibilidad con el ReadLine de C#
                putUSBUSART((uint8_t*)"FINISHED\r\n", 10);

                // 3. Pasamos a un estado de limpieza solo cuando confirmemos el envío
                state = STATE_IDLE; // O un estado intermedio de RESET si lo prefieres

                // 4. Reiniciamos banderas de control
                ctx.isFinished = false;
                ctx.experimentRunning = false; 
                ctx.stepsExecutedTotal = 0;

                // El Timer ya se detuvo en el paso anterior (STATE_RUNNING)
            }
            break;
            
        default:
            state = STATE_IDLE;
            break;
    }
}