/*******************************************************************************
Copyright 2016 Microchip Technology Inc. (www.microchip.com)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

To request to license the code under the MLA license (www.microchip.com/mla_license), 
please contact mla_licensing@microchip.com
*******************************************************************************/

/** INCLUDES *******************************************************/
#include "system.h"

#include <stdint.h>
#include <string.h>
#include <stddef.h>

#include "usb.h"

#include "app_device_cdc_basic.h"
#include "usb_config.h"

#include "app_commands.h" 
#include "app_context.h"

/** VARIABLES ******************************************************/

static uint8_t readBuffer[CDC_DATA_OUT_EP_SIZE];
static uint8_t writeBuffer[CDC_DATA_IN_EP_SIZE];

/*********************************************************************
* Function: void APP_DeviceCDCBasicDemoInitialize(void);
*
* Overview: Initializes the demo code
*
* PreCondition: None
*
* Input: None
*
* Output: None
*
********************************************************************/
void APP_DeviceCDCBasicDemoInitialize()
{   
    line_coding.bCharFormat = 0;
    line_coding.bDataBits = 8;
    line_coding.bParityType = 0;
    line_coding.dwDTERate = 9600;

}



void APP_DeviceCDCBasicDemoTasks()
{
    // Si el USB no está configurado o está suspendido, no hacemos nada
    if( USBGetDeviceState() < CONFIGURED_STATE )
    {
        return;
    }

    if( USBIsDeviceSuspended()== true )
    {
        return;
    }
        
    // Si el PIC está listo para transmitir datos
    if( USBUSARTIsTxTrfReady() == true)
    {
        uint8_t numBytesRead;

        // Leemos lo que llega de la computadora
        numBytesRead = getsUSBUSART(readBuffer, sizeof(readBuffer));

        if(numBytesRead > 0)
        {
            readBuffer[numBytesRead] = '\0';
            
            Dispatch_Command((char*)readBuffer); 

        }
    }

    // Le decimos a la librería MLA que procese la transmisión
    CDCTxService();
}