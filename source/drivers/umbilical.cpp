#include "umbilical.h"
#include <string.h>
#include "source/drivers/payload.h"
#include "ti/devices/msp432e4/driverlib/driverlib.h"

#define UMBILICAL_UART UART1_BASE

uint8_t umbilical_buffer[UMBILICAL_BUFFER_MAX_LEN];
uint8_t umbilical_buffer_len;

void UART1_IRQHandler(void) {
    uint32_t ui32Status = MAP_UARTIntStatus(UMBILICAL_UART, true);
    MAP_UARTIntClear(UMBILICAL_UART, ui32Status);

    int32_t counter = 0;

    while (MAP_UARTCharsAvail(UMBILICAL_UART) &&
           counter < UMBILICAL_BUFFER_MAX_LEN) {
        umbilical_buffer[counter] =
            (uint8_t)MAP_UARTCharGetNonBlocking(UMBILICAL_UART);

        counter++;
    }

    umbilical_buffer_len = counter - 1;  // To account for the final ++
}

err_t validateUmbilicalPacket(uint8_t* buffer, uint8_t buffer_len) {
    // Check for full header
    if (buffer_len < 5) {
        return UMB_BAD_PACKET_LENGTH;
    }

    // Sync chars
    if (buffer[0] != UMB_PACKET_SYNC_CHAR_1 ||
        buffer[1] != UMB_PACKET_SYNC_CHAR_2) {
        return UMB_BAD_PACKET_HEADER;
    }

    // Length of packet (excl sync chars)
    if (buffer[2] != buffer_len) {
        return UMB_BAD_PACKET_LENGTH;
    }

    if (buffer[3] != PAYLOAD_FIRMWARE_PART_PACKET &&
        buffer[3] != PAYLOAD_GET_STATUS_PACKET) {
        return UMB_BAD_PACKET_COMMAND;
    }

    // TODO Checksum test buffer[4]

    return 0;
}

err_t getUmbilicalPacket(uint8_t* destination, uint8_t* buffer_len) {
    // Disable UART to ensure we do not get interrupted half way through copying
    // the packet
    MAP_UARTDisable(UMBILICAL_UART);

    if (umbilical_buffer_len == 0) {
        MAP_UARTEnable(UMBILICAL_UART);
        return UMB_NO_PACKET;
    }
    memcpy(destination, umbilical_buffer, umbilical_buffer_len);
    *buffer_len = umbilical_buffer_len;
    MAP_UARTEnable(UMBILICAL_UART);

    // TODO Need to set len to 0

    return UMB_NO_ERROR;
}
