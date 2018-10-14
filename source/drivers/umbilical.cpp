#include <source/drivers/payload.h>
#include <source/drivers/umbilical.h>
#include <string.h>
#include <ti/devices/msp432e4/driverlib/driverlib.h>
#include <source/globals.h>

uint8_t umbilical_buffer[UMBILICAL_BUFFER_MAX_LEN];
uint8_t umbilical_buffer_len;

extern "C" {
    void UART1_IRQHandler(void) {
        uint32_t ui32Status = MAP_UARTIntStatus(UMBILICAL_UART, true);
        MAP_UARTIntClear(UMBILICAL_UART, ui32Status);
        SYSTEM_FLAGS |= kUmbilicalReadBitMask;
    }
}

err_t umbilicalRead() {
    uint32_t counter = 0;

    // Read in the sync and size bytes
    while (counter < sizeof(uint8_t) * 3) {
        umbilical_buffer[counter] = (uint8_t)MAP_UARTCharGet(UMBILICAL_UART);
        counter++;
    }

    err_t validated_header = validateUmbilicalHeader(umbilical_buffer, 3);

    if (validated_header != UMB_NO_ERROR) {
        return validated_header;
    }

    // Read in the rest of the packet
    while (counter < umbilical_buffer[2]) {
        umbilical_buffer[counter] = (uint8_t)MAP_UARTCharGet(UMBILICAL_UART);
        counter++;
    }

    umbilical_buffer_len = counter - 1;  // To account for the final ++

    return UMB_NO_ERROR;
}

err_t validateUmbilicalHeader(uint8_t* buffer, uint8_t buffer_len) {
    // Check for full header
    if (buffer_len < 3) {
        return UMB_BAD_PACKET_LENGTH;
    }

    // Sync chars
    if (buffer[0] != kUmbilicalSyncChar1 || buffer[1] != kUmbilicalSyncChar2) {
        return UMB_BAD_PACKET_HEADER;
    }

    return UMB_NO_ERROR;
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

    return UMB_NO_ERROR;
}
