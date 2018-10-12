#include "memory.h"
#include "assert.h"
#include "ti/devices/msp432e4/driverlib/driverlib.h"

err_t writeBytesToMemory(ImageBaseAddress image, uint32_t start_address,
                         uint8_t* data, uint32_t data_size) {
    if (image == Image11InMemory) {
        ASSERT(start_address % 4 == 0);
        ASSERT(data_size % 4 == 0);
        // ASSUME deltas are 4-byte aligned

        // TODO Modify this to take bytes
        // Where did my read stuff go

        /*
        int32_t protec_error =  FlashProtectSet (start_address ,
        tFlashProtection    eProtect
        )*/

        int32_t error = 0;
        // int32_t error = FlashProgram(data, start_address, data_size);

        if (error != 0) {
            return error;
        }
    }

    return 0;
}
