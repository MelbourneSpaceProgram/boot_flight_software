#include "data_types.h"

enum PayloadType {
    PAYLOAD_FIRMWARE_PIECE = 0x00C1,
    PAYLOAD_COPY_IMAGE = 0x00C3,
    PAYLOAD_INITIATE_UPDATE = 0x00C2,
    PAYLOAD_BAD_PAYLOAD = 0x05,
};

/**
 * Takes in the core payload, which includes the payload type followed by the
 * bytes of it. Length refers to the size of the buffer going in
 */
err_t handlePayload(uint8_t* buffer, uint8_t buffer_len);
