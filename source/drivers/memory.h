#ifndef SOURCE_DRIVERS_MEMORY_H_
#define SOURCE_DRIVERS_MEMORY_H_

#include "data_types.h"
#include "source/updater/updater.h"

constexpr uint32_t kSectorSizeBytes = 16384;

enum MemoryError {
    MEMORY_NO_ERROR = 0x00,
    MEMORY_COULD_NOT_ERASE = 0x01,
    MEMORY_COULD_NOT_PROGRAM = 0x02,
};

err_t writeBytesToMemory(ImageBaseAddress image, uint32_t start_address,
                         uint8_t* data, uint32_t data_size);
err_t getProgramBytes(ImageBaseAddress image_base_address,
                      uint32_t program_counter, uint8_t* buffer,
                      uint8_t* buffer_len);

#endif /* SOURCE_DRIVERS_MEMORY_H_ */
