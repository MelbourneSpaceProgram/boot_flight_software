#include <source/drivers/payload.h>
#include <source/drivers/ring_buffer.h>
#include <source/drivers/umbilical.h>
#include <source/globals.h>
#include <string.h>
#include <ti/devices/msp432e4/driverlib/driverlib.h>

uint8_t umbilical_data_space[UMBILICAL_BUFFER_MAX_LEN];

circ_bbuf_t umbilical_ring_buffer = {.buffer = umbilical_data_space,
                                     .head = 0,
                                     .tail = 0,
                                     .maxlen = UMBILICAL_BUFFER_MAX_LEN};

extern "C" {
void UART1_IRQHandler(void) {
    uint32_t ui32Status = MAP_UARTIntStatus(UMBILICAL_UART, true);
    MAP_UARTIntClear(UMBILICAL_UART, ui32Status);

    while (UARTCharsAvail(UMBILICAL_UART)) {
        uint8_t data = (uint8_t)MAP_UARTCharGet(UMBILICAL_UART);
        circ_bbuf_push(&umbilical_ring_buffer, data);
    }
}
}

err_t umbilicalBytesAvailable(uint8_t* bytes_available) {
    *bytes_available =
        ((umbilical_ring_buffer.head - umbilical_ring_buffer.tail) &
         (umbilical_ring_buffer.maxlen - 1));

    return 0;
}

err_t umbilicalReadPacket(uint8_t umbilical_buffer[255],
                    uint8_t* umbilical_buffer_len) {
    uint8_t sync_char;
    // Burn until we sync
    circ_bbuf_pop(&umbilical_ring_buffer, &sync_char);
    if (sync_char != 0xCA) {
        return 1;
    }

    circ_bbuf_pop(&umbilical_ring_buffer, &sync_char);
    if (sync_char != 0xFE) {
        return 1;
    }

    uint8_t counter = 0;
    while (counter < 3) {
        int32_t buffer_error =
            circ_bbuf_pop(&umbilical_ring_buffer, &umbilical_buffer[counter]);

        if (buffer_error == 0) {
            counter++;
        }
        // TODO Else timeout?
    }

    uint8_t payload_length = umbilical_buffer[0];

    counter = 0;
    // Read in the rest of the packet which is the payload
    while (counter < payload_length) {
        int32_t buffer_error =
            circ_bbuf_pop(&umbilical_ring_buffer, &umbilical_buffer[counter]);

        if (buffer_error == 0) {
            counter++;
        }
        // TODO Else timeout?
    }

    *umbilical_buffer_len = counter;

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
