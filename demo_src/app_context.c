#include "app_context.h"

// ¡ESTA ES LA LÍNEA QUE LE FALTABA AL COMPILADOR!
// El {0} obliga a XC8 a apartar la memoria RAM y poner todo en cero
AppContext ctx = {0};

void Context_Init(void){
    ctx.mode = MODE_IDLE;
    ctx.experimentRunning = false;
    ctx.isFinished = false;
    ctx.isError = false;

    ctx.direction = 1; // Por defecto, adelante
    ctx.totalStepsToRun = 0;
    ctx.stepsExecutedTotal = 0;
    ctx.currentMotorPhase = 0;

    ctx.stepsPerPart = 0;
    ctx.currentPart = 0;
    ctx.totalParts = 0;

    ctx.evtStepReady = false;
    ctx.evtSampleReq = false;
    ctx.evtChopperTrg = false;
    ctx.latestADCValue = 0;    
}