#include <source/drivers/lithium.h>
#include <source/drivers/ring_buffer.h>
#include <string.h>
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

void LithiumUARTSend(const uint8_t* buffer, uint8_t buffer_len) {
    while (buffer_len--) {
        MAP_UARTCharPut(LITHIUM_UART, *buffer++);
    }
}

void calcLithiumChecksum(uint8_t* checksum, const uint8_t* data,
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

    // TODO Finish
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
    calcLithiumChecksum(checksum, buffer + 2, 4);
    if (!(checksum[0] == buffer[6] && checksum[1] == buffer[7])) {
        return LITHIUM_BAD_HEADER_CHECKSUM;
    }

    return LITHIUM_NO_ERROR;
}

err_t authenticateLithiumPacket(const uint8_t* buffer, uint8_t buffer_len) {
    uint32_t* signature_bytes =
        (uint32_t*)(buffer + kLithiumHeaderSize + kLithiumAX25HeaderSize);
    uint32_t* payload_to_authenticate =
        (uint32_t*)(buffer + kLithiumHeaderSize + kLithiumAX25HeaderSize +
                    kLithiumSignatureSize);
    uint8_t buffer_len_32;

    // Zero pad the last word if required
    uint8_t remainder = buffer_len % sizeof(uint32_t);
    if (remainder != 0) {
        buffer_len_32 = buffer_len / sizeof(uint32_t) + 1;
        // Get pointer to last word. Cast to look at word as 4 bytes.
        uint8_t* word_to_zero_pad =
            (uint8_t*)(payload_to_authenticate + buffer_len_32 - 1);
        for (uint8_t i = 0; i < remainder; i++) {
            word_to_zero_pad[3 - i] = 0;
        }
    } else {
        // No zero padding required
        buffer_len_32 = buffer_len / sizeof(uint32_t);
    }

    uint32_t hash_result[5] = {0};  // Hash algorithm returns 5 words

    // Configure the SHA/MD5 module for the SHA1 algorithm.
    MAP_SHAMD5Reset(SHAMD5_BASE);
    MAP_SHAMD5ConfigSet(SHAMD5_BASE, SHAMD5_ALGO_HMAC_SHA1);

    // Copy the Key to SHA/MD5 module.
    MAP_SHAMD5HMACKeySet(SHAMD5_BASE, (uint32_t*)kHmacKey);

    // Copy the data to SHA/MD5 module and begin the HMAC generation.
    MAP_SHAMD5HMACProcess(SHAMD5_BASE, payload_to_authenticate, buffer_len,
                          hash_result);

    if (signature_bytes[0] != hash_result[0]) {
        return LITHIUM_BAD_HASH;
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

err_t buildLithiumHeader(uint8_t* buffer, uint8_t payload_size,
                         LithiumCommandCodes command_code) {
    if (command_code == kTransmitCommandCode) {
        memcpy(buffer, kLithiumTransmitHeaderPrototype, kLithiumHeaderSize);
        buffer[kLithiumPayloadSizeByte] = payload_size;
        calcLithiumChecksum(buffer + 5, buffer, 6);
    }
    return LITHIUM_NO_ERROR;
}

err_t lithiumReadPacket(uint8_t lithium_buffer[255],
                        uint8_t* lithium_buffer_len) {
    uint32_t timeout_counter = 10000;

    uint8_t sync_char;
    // Burn until we sync
    circ_bbuf_pop(&lithium_ring_buffer, &sync_char);
    if (sync_char != kLithiumSyncCharOne) {
        return 1;
    }

    circ_bbuf_pop(&lithium_ring_buffer, &sync_char);
    if (sync_char != kLithiumSyncCharTwo) {
        return 1;
    }

    uint8_t counter = 1;
    while (counter < 8) {
        if (timeout_counter == 0) {
            return 1;
        }

        int32_t buffer_error =
            circ_bbuf_pop(&lithium_ring_buffer, &lithium_buffer[counter]);

        if (buffer_error == 0) {
            counter++;
        } else {
            timeout_counter--;
        }
    }

    validateLithiumPacket(lithium_buffer, 8);

    uint8_t payload_length = lithium_buffer[0];  // TODO

    counter = 0;
    // Read in the rest of the packet which is the payload
    while (counter < payload_length) {
        if (timeout_counter == 0) {
            return 1;
        }
        int32_t buffer_error =
            circ_bbuf_pop(&lithium_ring_buffer, &lithium_buffer[counter]);

        if (buffer_error == 0) {
            counter++;
        } else {
            timeout_counter--;
        }
    }

    err_t packet_authenticated =
        authenticateLithiumPacket(lithium_buffer, *lithium_buffer_len);

    if (packet_authenticated != LITHIUM_NO_ERROR) {
        return 1;  // TODO
    }

    *lithium_buffer_len = counter;

    return LITHIUM_NO_ERROR;
}

err_t sendLithiumPacket(uint8_t* payload, uint8_t payload_size,
                        LithiumCommandCodes command_code) {
    if (payload_size > lithium_buffer_max_len) {
        return LITHIUM_BAD_PAYLOAD_LENGTH;
    }

    err_t error =
        buildLithiumHeader(lithium_buffer, payload_size, command_code);

    if (error != LITHIUM_NO_ERROR) {
        return error;
    }
    memcpy(lithium_buffer + 8, payload, payload_size);
    // Add tail checksum
    calcLithiumChecksum(lithium_buffer + payload_size + kLithiumHeaderSize,
                        lithium_buffer + 2,
                        kLithiumHeaderSize - 2 + payload_size);

    LithiumUARTSend(lithium_buffer,
                    kLithiumHeaderSize + payload_size + kLithiumTailSize);
    return LITHIUM_NO_ERROR;
}
