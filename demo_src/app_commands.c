#include "app_commands.h"
#include "app_context.h"
#include <string.h>
#include <stdlib.h>

// Librerías de la MLA para usar el CDC
#include "usb.h"
#include "usb_device_cdc.h"

// Función auxiliar para responder a C# usando la MLA
static void sendUSBImmediate(const char *str) {
    if(USBUSARTIsTxTrfReady()) {
        // En la MLA, putUSBUSART pide un puntero (uint8_t*)
        putUSBUSART((uint8_t*)str, strlen(str));
    }
}

void Dispatch_Command(char* command) {
    
    // ---------------------------------------------------------
    // 1. EVALUACIÓN DE SEGURIDAD (HANDSHAKE)
    // ---------------------------------------------------------
    if (command[0] == 'H') {
        ctx.handshake_received = true;
        sendUSBImmediate("PIC_OK*\r\n");
        return; 
    }
    
    // Si la PC no ha mandado la 'H', ignoramos cualquier otro comando por seguridad
    if (!ctx.handshake_received) {
        sendUSBImmediate("ERR,NO_HANDSHAKE\r\n");
        return;
    }

    // ---------------------------------------------------------
    // 2. SELECCIÓN DE MODO DE OPERACIÓN
    // ---------------------------------------------------------
    if (strncmp(command, "MODE,IDLE", 9) == 0) {
        ctx.mode = MODE_IDLE;
        sendUSBImmediate("ACK,MODE,IDLE\r\n");
    } 
    else if (strncmp(command, "MODE,ZSCAN_C", 12) == 0) { // ZSCAN_CHOPPER
        ctx.mode = MODE_ZSCAN_CHOPPER;
        sendUSBImmediate("ACK,MODE,ZSCAN_C\r\n");
    }
    else if (strncmp(command, "MODE,ZSCAN", 10) == 0) {
        ctx.mode = MODE_ZSCAN;
        sendUSBImmediate("ACK,MODE,ZSCAN\r\n");
    } 
    else if (strncmp(command, "MODE,INTERF", 11) == 0) {
        ctx.mode = MODE_INTERF;
        sendUSBImmediate("ACK,MODE,INTERF\r\n");
    } 
    else if (strncmp(command, "MODE,FREEC", 10) == 0) {
        ctx.mode = MODE_FREEC;
        sendUSBImmediate("ACK,MODE,FREEC\r\n");
    } 
    else if (strncmp(command, "MODE,WO_CH", 10) == 0) { // WO_CHOPPER
        ctx.mode = MODE_WO_CHOPPER;
        sendUSBImmediate("ACK,MODE,WO_CH\r\n");
    }

    // ---------------------------------------------------------
    // 3. RECEPCIÓN DE PARÁMETROS MATEMÁTICOS
    // ---------------------------------------------------------
    else if (strncmp(command, "STEPS,", 6) == 0) {
        // Uso general para FREEC, INTERF o total del experimento
        ctx.totalStepsToRun = atol(&command[6]);
        sendUSBImmediate("ACK,STEPS\r\n");
    } 
    else if (strncmp(command, "PARTS,", 6) == 0) {
        // Total de muestras/particiones en el eje Z
        ctx.totalParts = atol(&command[6]);
        sendUSBImmediate("ACK,PARTS\r\n");
    } 
    else if (strncmp(command, "SPP,", 4) == 0) {
        // Steps Per Part (Pasos entre cada muestra en Z-scan)
        ctx.stepsPerPart = atol(&command[4]);
        sendUSBImmediate("ACK,SPP\r\n");
    }
    else if (strncmp(command, "DIR,", 4) == 0) {
        // Dirección: 1 Adelante, 0 Atrás
        ctx.direction = atoi(&command[4]);
        sendUSBImmediate("ACK,DIR\r\n");
    }

    // ---------------------------------------------------------
    // 4. CONTROL DE EJECUCIÓN
    // ---------------------------------------------------------
    else if (strncmp(command, "START", 5) == 0) {
        // Limpiamos los contadores dinámicos antes de arrancar
        ctx.stepsExecutedTotal = 0;
        ctx.currentPart = 0;
        ctx.isFinished = false;
        ctx.isError = false;
        
        ctx.experimentRunning = true; // Esta bandera "despierta" a la máquina de estados
        
        sendUSBImmediate("ACK,START\r\n");
    }
    else if (strncmp(command, "STOP", 4) == 0) {
        // Parada de emergencia o fin de conexión
        ctx.experimentRunning = false;
        ctx.handshake_received = false; // Exigimos un nuevo handshake para reconectar
        sendUSBImmediate("DISCONNECTING\r\n");
    }
    else {
        // Comando no reconocido
        sendUSBImmediate("ERR,INVALID_CMD\r\n");
    }
}