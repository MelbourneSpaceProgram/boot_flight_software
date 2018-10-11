#include "payload.h"
#include "source/drivers/memory.h"
#include "source/updater/updater.h"

err_t handlePayload(uint8_t* buffer, uint8_t buffer_len) {
    if (buffer[0] == PAYLOAD_FIRMWARE_PART_PACKET) {
        // payload[0:3] = image_base_address
        // payload[4:7] = image_patch_address
        // payload[8] = patch_length
        // payload[9:9+patch_length-1] = patch, applied sequentially from
        // image_patch_address
        uint32_t image_base_address =
            buffer[1] << 24 | buffer[2] << 16 | buffer[3] << 8 | buffer[4] << 0;
        uint32_t image_patch_address =
            buffer[5] << 24 | buffer[6] << 16 | buffer[7] << 8 | buffer[8] << 0;
        uint8_t patch_length_bytes = buffer[9];

        // Patch length bytes and address must be 4-aligned
        uint32_t program_bytes[patch_length_bytes];
        for (int i = 0; i < patch_length_bytes / 4; i++) {
            program_bytes[i] = buffer[4 * i] << 24 | buffer[4 * i + 1] << 16 |
                               buffer[4 * i + 2] << 8 | buffer[4 * i + 3] << 0;
        }

        writeBytesToMemory((ImageBaseAddress)image_base_address,
                           image_patch_address, program_bytes,
                           patch_length_bytes / 4);
    } else if (buffer[0] == PAYLOAD_INITIATE_UPDATE_PACKET) {
        uint32_t image_base_address =
            buffer[1] << 24 | buffer[2] << 16 | buffer[3] << 8 | buffer[4] << 0;
        uint32_t image_checksum __attribute__ ((unused));
        image_checksum = buffer[5] << 24 | buffer[6] << 16 | buffer[7] << 8 | buffer[8] << 0;

        // TODO Validate image base address?
        err_t firmware_update_error =
            beginFirmwareUpdate((ImageBaseAddress)image_base_address);
        return firmware_update_error;
    } else if (buffer[0] == PAYLOAD_GET_STATUS_PACKET) {
        // TODO Send a listing of packets or something?
    } else {
        return PAYLOAD_BAD_PAYLOAD;
    }

    return 0;
}
