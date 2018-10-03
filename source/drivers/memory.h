#include "data_types.h"
#include "Source/updater/updater.h"

err_t writeBytesToMemory(ImageBaseAddress image, uint32_t start_address,
                         uint32_t* data, uint32_t data_size);
