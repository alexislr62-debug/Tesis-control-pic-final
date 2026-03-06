#ifndef APP_CONTEXT_H
#define APP_CONTEXT_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    MODE_IDLE,          // Sistema detenido, esperando comandos
    MODE_ZSCAN,         // Z-Scan paso a paso con muestras
    MODE_INTERF,        // Interferometría continua con muestreo rápido
    MODE_FREEC,         // Posicionamiento libre sin muestreo
    MODE_WO_CHOPPER,    // Medición de cintura de haz (W0) con chopper
    MODE_ZSCAN_CHOPPER  // (Opcional futuro)
} OperationMode;

typedef struct
{
    // Estado General
    OperationMode mode;          // Modo de operación actual
    volatile bool experimentRunning;     // true cuando el experimento está activo
    volatile bool isFinished;    // true cuando se completaron los pasos/partes
    volatile bool isError;       // true si hay bloqueo o error físico
    volatile bool handshake_received; // <--- ¡AQUÍ ESTÁ! Seguridad de conexión


    // Control de Movimiento 
    int direction;               // 1 = Adelante, 0 = Atrás
    uint32_t totalStepsToRun;    // Pasos totales (para FREEC, INTERF o WO)
    uint32_t stepsExecutedTotal; // Pasos ejecutados en la corrida actual
    uint8_t  currentMotorPhase;  // Mantiene la sincronía del motor (0 a 9)

    // Parámetros Z-scan
    uint32_t stepsPerPart;       // Pasos entre cada medición
    uint32_t currentPart;        // Parte actual (contador)
    uint32_t totalParts;         // Total de particiones/muestras en Z

    // Sincronización Motor - Máquina de Estados - ADC
    volatile bool evtStepReady;  // El Timer1 avisa que ya dio un paso físico
    volatile bool evtSampleReq;  // Señal para disparar el ADC (ej. a los 4ms)
    volatile bool evtChopperTrg; // (Para W0) Disparo detectado del chopper
    uint16_t latestADCValue;     // Lectura temporal del láser
    
} AppContext;

extern AppContext ctx; // Variable global

void Context_Init(void);

#endif