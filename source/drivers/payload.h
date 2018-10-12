#ifndef SOURCE_DRIVERS_PAYLOAD_H_
#define SOURCE_DRIVERS_PAYLOAD_H_

#include "data_types.h"

constexpr uint32_t payload_buffer_max_len = 255;

// Anyone copying to or from these buffers may need to disable interrupts to
// ensure integrity, or we could implement a ping-pong buffer or similar
static uint8_t payload_buffer[payload_buffer_max_len] = {0};
static uint32_t payload_buffer_len = 0;

enum PayloadType {
    PAYLOAD_FIRMWARE_PIECE = 0x00C1,
    PAYLOAD_COPY_IMAGE = 0x00C3,
    PAYLOAD_INITIATE_UPDATE = 0x00C2,
    PAYLOAD_BAD_PAYLOAD = 0x05,
    PAYLOAD_IMAGE_FAILED_CHECKSUM = 0x06
};

/**
 * Takes in the core payload, which includes the payload type followed by the
 * bytes of it. Length refers to the size of the buffer going in
 */
err_t handlePayload(uint8_t* buffer, uint8_t buffer_len);

#endif /* SOURCE_DRIVERS_PAYLOAD_H_ */
