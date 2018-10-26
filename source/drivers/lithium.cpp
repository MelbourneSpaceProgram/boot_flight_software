#include <source/drivers/lithium.h>
#include <source/drivers/ring_buffer.h>
#include "ti/devices/msp432e4/driverlib/driverlib.h"

#define LITHIUM_UART UART2_BASE

uint8_t lithium_buffer[lithium_buffer_max_len];
uint8_t lithium_buffer_len;

uint8_t lithium_data_space[lithium_buffer_max_len];

circ_bbuf_t lithium_ring_buffer = {.buffer = lithium_data_space,
                                   .head = 0,
                                   .tail = 0,
                                   .maxlen = lithium_buffer_max_len};

extern "C" {
void UART2_IRQHandler(void) {
    uint32_t ui32Status = MAP_UARTIntStatus(LITHIUM_UART, true);
    MAP_UARTIntClear(LITHIUM_UART, ui32Status);

    while (UARTCharsAvail(LITHIUM_UART)) {
        uint8_t data = (uint8_t)MAP_UARTCharGet(LITHIUM_UART);
        circ_bbuf_push(&lithium_ring_buffer, data);
    }
}
}

err_t lithiumBytesAvailable(uint8_t* bytes_available) {
    *bytes_available = ((lithium_ring_buffer.head - lithium_ring_buffer.tail) &
                        (lithium_ring_buffer.maxlen - 1));

    return 0;
}

err_t lithiumReadPacket(uint8_t lithium_buffer[255],
                        uint8_t* lithium_buffer_len) {
    // TODO(akremor): Determine Lithium format.

    return 0;
}
