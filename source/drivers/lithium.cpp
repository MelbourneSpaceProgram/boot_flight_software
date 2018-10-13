#include "lithium.h"
#include <string.h>
#include "ti/devices/msp432e4/driverlib/driverlib.h"

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

void LithiumUARTSend(const uint8_t* buffer, uint8_t buffer_len) {
    while (buffer_len--) {
        MAP_UARTCharPutNonBlocking(LITHIUM_UART, *buffer++);
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
        //Get pointer to last word. Cast to look at word as 4 bytes.
        uint8_t* word_to_zero_pad =
            (uint8_t*)(payload_to_authenticate + buffer_len_32 - 1);
        for (uint8_t i = 0; i < remainder; i++) {
            word_to_zero_pad[3-i] = 0;
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

err_t sendLithiumPacket(uint8_t* payload, uint8_t payload_size,
                        LithiumCommandCodes command_code) {
    if (payload_size > LITHIUM_BUFFER_MAX_LEN) {
        return LITHIUM_BAD_PAYLOAD_LENGTH;
    }
    if (lithium_buffer_len != 0) {
        // TODO(wschuetz) is this check necessary? Should not write to buffer
        // if in use
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
