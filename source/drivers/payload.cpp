#include <source/drivers/memory.h>
#include <source/drivers/payload.h>
#include <source/updater/updater.h>

err_t handlePayload(uint8_t* buffer, uint8_t buffer_len) {
    uint8_t packet_length = buffer[0];
    uint16_t packet_checksum = buffer[1] << 8 | buffer[2];
    uint16_t command_id = buffer[3] << 8 | buffer[4];

    if (command_id == PAYLOAD_FIRMWARE_PIECE) {
        uint32_t image_base_address =
            buffer[5] << 24 | buffer[6] << 16 | buffer[7] << 8 | buffer[8] << 0;
        uint32_t image_patch_address = buffer[9] << 24 | buffer[10] << 16 |
                                       buffer[11] << 8 | buffer[12] << 0;

        uint32_t patch_length_bytes = packet_length - 13;

        writeBytesToMemory((ImageBaseAddress)image_base_address,
                           image_patch_address, &buffer[13],
                           patch_length_bytes);

    } else if (command_id == PAYLOAD_INITIATE_UPDATE) {
        uint32_t image_base_address =
            buffer[5] << 24 | buffer[6] << 16 | buffer[7] << 8 | buffer[8] << 0;
        uint32_t image_checksum __attribute__((unused));
        image_checksum = buffer[9] << 24 | buffer[10] << 16 | buffer[11] << 8 |
                         buffer[12] << 0;

        // TODO Validate image base address?
        err_t firmware_update_error =
            beginFirmwareUpdate((ImageBaseAddress)image_base_address);
        return firmware_update_error;
    } else if (command_id == PAYLOAD_COPY_IMAGE) {
        uint32_t image_base_address =
            buffer[5] << 24 | buffer[6] << 16 | buffer[7] << 8 | buffer[8] << 0;
        uint32_t image_dest_address = buffer[9] << 24 | buffer[10] << 16 |
                                      buffer[11] << 8 | buffer[12] << 0;

        // TODO

    } else {
        return PAYLOAD_BAD_PAYLOAD;
    }

    return 0;
}
