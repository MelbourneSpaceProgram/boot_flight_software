#include "data_types.h"

enum PayloadType {
    PAYLOAD_FIRMWARE_PART_PACKET = 0x01,
    PAYLOAD_GET_STATUS_PACKET = 0x02,
    PAYLOAD_COPY_IMAGE_PACKET = 0x03,
    PAYLOAD_INITIATE_UPDATE_PACKET = 0x04,
    PAYLOAD_BAD_PAYLOAD = 0x05,
};

/**
 * Takes in the core payload, which includes the payload type followed by the
 * bytes of it. Length refers to the size of the buffer going in
 */
err_t handlePayload(uint8_t* buffer, uint8_t buffer_len);
