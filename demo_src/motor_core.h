#ifndef MOTOR_CORE_H
#define MOTOR_CORE_H

void Motor_Initialize(void);
void Motor_Execute_Step(void); // Da un solo paso basado en la dirección de la máquina de estados
void Motor_Disable_Coils(void);

#endif