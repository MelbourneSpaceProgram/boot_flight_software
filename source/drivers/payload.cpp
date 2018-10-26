#include <source/drivers/hal.h>
#include <source/drivers/memory.h>
#include <source/drivers/payload.h>
#include <source/updater/updater.h>

err_t handlePayload(uint8_t* buffer, uint8_t buffer_len) {
    uint8_t packet_length = buffer[0];
    uint16_t packet_checksum = buffer[1] << 8 | buffer[2];
    uint16_t command_id = buffer[3] << 8 | buffer[4];

    if (command_id == PAYLOAD_FIRMWARE_PIECE) {
        uint32_t image_base_address =
            buffer[5] << 0 | buffer[6] << 8 | buffer[7] << 16 | buffer[8] << 24;
        uint32_t image_patch_address = buffer[9] << 0 | buffer[10] << 8 |
                                       buffer[11] << 16 | buffer[12] << 24;

        uint32_t patch_length_bytes = packet_length - 13;

        writeBytesToMemory((ImageBaseAddress)image_base_address,
                           image_patch_address, &buffer[13],
                           patch_length_bytes);

    } else if (command_id == PAYLOAD_INITIATE_UPDATE) {
        uint32_t image_base_address =
            buffer[5] << 0 | buffer[6] << 8 | buffer[7] << 16 | buffer[8] << 24;
        uint32_t image_checksum __attribute__((unused));
        image_checksum = buffer[9] << 0 | buffer[10] << 8 | buffer[11] << 16 |
                         buffer[12] << 24;

        // Confirm the CRC is valid
        // Reset the CRC module to clear out any previous calculations
        init_crc();

        uint32_t program_counter = 0;

        if (0) {
            while (program_counter < flight_software_length) {
                uint8_t image_bytes[32];

                uint8_t bytes_read = 0;
                err_t bytes_error =
                    getProgramBytes((ImageBaseAddress)image_base_address,
                                    program_counter, image_bytes, &bytes_read);

                for (uint8_t i = 0; i < 32; i++) {
                    CRCDataWrite(CCM0_BASE, image_bytes[i]);
                }
            }

            uint32_t crc_result_raw = CRCResultRead(CCM0_BASE, true);
            // Endianess needs to be corrected to match the golden test set.
            // Likely a config issue but this 'works'. Given: 0x31 0x32 0x33
            // 0x34 0x35 0x36 0x37 0x38 0x39 Expect: 0xE3069283

            uint32_t crc_result = ((crc_result_raw >> 24) & 0xff) |
                                  ((crc_result_raw << 8) & 0xff0000) |
                                  ((crc_result_raw >> 8) & 0xff00) |
                                  ((crc_result_raw << 24) & 0xff000000);

            if (crc_result != image_checksum) {
                return PAYLOAD_IMAGE_FAILED_CHECKSUM;
            }
        }

        err_t firmware_update_error =
            beginFirmwareUpdate((ImageBaseAddress)image_base_address);
        return firmware_update_error;
    } else if (command_id == PAYLOAD_COPY_IMAGE) {
        uint32_t image_base_address =
            buffer[5] << 0 | buffer[6] << 8 | buffer[7] << 16 | buffer[8] << 24;
        uint32_t image_dest_address = buffer[9] << 0 | buffer[10] << 8 |
                                      buffer[11] << 16 | buffer[12] << 24;

        // TODO

    } else {
        return PAYLOAD_BAD_PAYLOAD;
    }

    return 0;
}
