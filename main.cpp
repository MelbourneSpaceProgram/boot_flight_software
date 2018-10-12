#include "main.h"
#include <source/drivers/hal.h>
//#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <ti/devices/msp432e4/driverlib/driverlib.h>
#include "source/drivers/lithium.h"
#include "source/drivers/payload.h"
#include "source/drivers/umbilical.h"
#include "source/updater/updater.h"

int main(void) {
    init_clock();
    init_gpio();
    init_system_uart();
    init_lithium_uart();
    init_umbilical_uart();
    init_flash_spi();

    SystemState state = SYSTEM_IDLE_STATE;
    while (1) {
        switch (state) {
            case SYSTEM_IDLE_STATE: {
                // TODO
                // If something of interest in a buffer, call the packet handler
                state = SYSTEM_IDLE_STATE;
                break;
            }
            case SYSTEM_HANDLE_PAYLOAD: {
                handlePayload(payload_buffer, payload_buffer_len);
                break;
            }
        }
    }
}
