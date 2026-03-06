#include "system.h"
#include "app_device_cdc_basic.h"

#include "usb.h"
#include "usb_device.h"
#include "usb_device_cdc.h"

// --- NUESTROS MÓDULOS BARE-METAL ---
#include "app_context.h"
#include "app_state_machine.h"
#include "motor_core.h"
#include "timer1.h"
#include "interrupt_manager.h"

MAIN_RETURN main(void)
{
    // 1. Inicialización de Hardware Propio
    Context_Init();
    Motor_Initialize();
    TMR1_Initialize();
    Interrupt_Initialize(); 

    // 2. Inicialización USB (MLA)
    SYSTEM_Initialize(SYSTEM_STATE_USB_START);
    USBDeviceInit();
    USBDeviceAttach();
    
    // 3. Bucle Principal
    while(1)
    {
        SYSTEM_Tasks();

        #if defined(USB_POLLING)
            USBDeviceTasks();
        #endif

        // Escucha comandos USB y actualiza 'ctx'
        APP_DeviceCDCBasicDemoTasks();
        
        // El cerebro toma decisiones basadas en 'ctx' (El FSM)
        APP_StateMachine_Tasks();

    }//end while
}//end main