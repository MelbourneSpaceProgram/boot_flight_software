#include "lithium.h"
#include "ti/devices/msp432e4/driverlib/driverlib.h"
#include <string.h>

uint8_t lithium_buffer[LITHIUM_BUFFER_MAX_LEN];
uint8_t lithium_buffer_len;

void UART2_IRQHandler(void) {
    uint32_t ui32Status = MAP_UARTIntStatus(LITHIUM_UART, true);
    MAP_UARTIntClear(UART2_BASE, ui32Status);

    int32_t counter = 0;

    while (MAP_UARTCharsAvail(LITHIUM_UART)) {
        lithium_buffer[counter] =
            (uint8_t)MAP_UARTCharGetNonBlocking(LITHIUM_UART);

        counter++;
    }

    lithium_buffer_len = counter - 1;  // To account for the final ++
}

void CalcLithiumChecksum(uint8_t* checksum, const uint8_t* data,
                                const uint16_t data_size) {
    uint8_t check_byte_a = 0;
    uint8_t check_byte_b = 0;
    uint16_t i;

    for (i = 0; i < data_size; i++) {
        check_byte_a = check_byte_a + data[i];
        check_byte_b = check_byte_b + check_byte_a;
    }

    checksum[0] = check_byte_a;
    checksum[1] = check_byte_b;
}

err_t validateLithiumPacket(const uint8_t* buffer, uint8_t buffer_len) {
    // Check sync chars
    if (!(buffer[0] == kLithiumSyncCharOne &&
            buffer[1] == kLithiumSyncCharTwo)) {
        return LITHIUM_BAD_SYNC_BITS;
    }
    // Check direction
    if (buffer[2] != kLithiumDirectionOut) {
        return LITHIUM_BAD_DIRECTION;
    }
    // Check checksum over non sync bytes (4 bytes)
    uint8_t checksum[2];
    CalcLithiumChecksum(checksum, buffer + 2, 4);
    if (!(checksum[0] == buffer[6] && checksum[1] == buffer[7])) {
        return LITHIUM_BAD_HEADER_CHECKSUM;
    }
    return LITHIUM_NO_ERROR;
}

err_t getLithiumPacket(uint8_t* destination, uint8_t* buffer_len) {
    // Disable UART to ensure we do not get interrupted half way through copying
    // the packet
    MAP_UARTDisable(LITHIUM_UART);

    if (lithium_buffer_len == 0) {
        MAP_UARTEnable(LITHIUM_UART);
        return LITHIUM_NO_PACKET;
    }
    memcpy(destination, lithium_buffer, lithium_buffer_len);
    *buffer_len = lithium_buffer_len;
    MAP_UARTEnable(LITHIUM_UART);

    return LITHIUM_NO_ERROR;
}
