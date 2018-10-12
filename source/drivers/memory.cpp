#include "memory.h"
#include <string.h>
#include "source/internal_image.h"
#include "ti/devices/msp432e4/driverlib/driverlib.h"

err_t writeBytesToMemory(ImageBaseAddress image, uint32_t start_address,
                         uint8_t* data, uint32_t data_size) {
    if (image == Image11InMemory) {
        const uint8_t* actual_address = &flight_software[start_address];
        // Now find a pointer to the lower sector boundary
        uint32_t sector_boundary = (uint32_t)actual_address / kSectorSizeBytes;
        uint32_t sector_offset = (uint32_t)actual_address - sector_boundary;

        uint8_t sector[kSectorSizeBytes];

        memcpy(sector, &sector_boundary, kSectorSizeBytes);

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

    return 0;
}
