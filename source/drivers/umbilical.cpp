#include "umbilical.h"
#include <string.h>
#include "source/drivers/payload.h"
#include "ti/devices/msp432e4/driverlib/driverlib.h"

#define UMBILICAL_UART UART1_BASE

void UART1_IRQHandler(void) {
    // TODO
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

    // TODO Checksum test buffer[4]

    return 0;
}

err_t getUmbilicalPacket(uint8_t* destination, uint8_t* buffer_len) {
    // TODO

    return UMB_NO_ERROR;
}
