#include "lithium.h"
#include "ti/devices/msp432e4/driverlib/driverlib.h"

#define LITHIUM_UART_BASE UART2_BASE

uint8_t lithium_buffer[255];
uint8_t lithium_buffer_len;

void UART2_IRQHandler(void) {
    uint32_t ui32Status = MAP_UARTIntStatus(LITHIUM_UART_BASE, true);
    MAP_UARTIntClear(UART2_BASE, ui32Status);

    int32_t counter = 0;

    while (MAP_UARTCharsAvail(LITHIUM_UART_BASE)) {
        lithium_buffer[counter] =
            (uint8_t)MAP_UARTCharGetNonBlocking(LITHIUM_UART_BASE);

        counter++;
    }

    lithium_buffer_len = counter - 1;  // To account for the final ++
}
