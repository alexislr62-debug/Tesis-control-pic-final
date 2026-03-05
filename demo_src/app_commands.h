#ifndef APP_COMMANDS_H
#define APP_COMMANDS_H

#include <stdint.h>
#include <stdbool.h>

// Prototipo de la función que decodifica la trama USB
void Dispatch_Command(char* command);
    
#endif