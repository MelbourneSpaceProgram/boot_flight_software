#include "memory.h"
#include <string.h>
#include "source/internal_image.h"
#include "ti/devices/msp432e4/driverlib/driverlib.h"

uint8_t sector[kSectorSizeBytes];

err_t writeBytesToMemory(ImageBaseAddress image, uint32_t start_address_temp,
                         uint8_t* data, uint32_t data_size) {
    uint32_t start_address = start_address_temp - image;  // TODO Adam
    if (image == Image11InMemory) {
        // TODO Assert valid actual_address
        const uint8_t* actual_address = &flight_software[start_address];
        // Now find a pointer to the lower sector boundary
        uint32_t sector_boundary_temp =
            (uint32_t)actual_address / kSectorSizeBytes;
        uint32_t sector_boundary = sector_boundary_temp * kSectorSizeBytes;
        uint32_t sector_offset = (uint32_t)actual_address - sector_boundary;

        uint32_t actual_sector_size = SysCtlFlashSectorSizeGet();
        tFlashProtection proc = FlashProtectGet(sector_boundary);

        memcpy(sector, (void*)sector_boundary, kSectorSizeBytes);

        // Apply the patch
        memcpy(sector + sector_offset, data, data_size);

        // Flash memory can only be written 1->0, not 0->1. Only an erase
        // operation can go 0->1 but an erase must operate on the entire sector.
        int32_t erase_error = FlashErase(sector_boundary);

        if (erase_error != 0) {
            return MEMORY_COULD_NOT_ERASE;
        }

        int32_t program_error =
            FlashProgram((uint32_t*)sector, sector_boundary, kSectorSizeBytes);

        if (program_error != 0) {
            return MEMORY_COULD_NOT_PROGRAM;
        }
    } else {
        // TOOD(akremor): Integrate in the flash memory API (physical chip)
    }

    return MEMORY_NO_ERROR;
}

err_t getProgramBytes(ImageBaseAddress image_base_address,
                      uint32_t program_counter, uint8_t* buffer,
                      uint8_t* buffer_len) {
    // Read from memory using a switch state

    if (image_base_address == Image11InMemory) {
        uint8_t bytes_to_read = 32;

        if (program_size - program_counter < 32) {
            bytes_to_read = program_size - program_counter;
        }

        *buffer_len = bytes_to_read;
        // Max buffer size will be 32; TODO Check this
        for (int i = 0; i < bytes_to_read; i++) {
            buffer[i] = flight_software[program_counter + i];
        }
    }

    return MEMORY_NO_ERROR;
}
