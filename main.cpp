#include <main.h>
#include <source/drivers/hal.h>
#include <source/drivers/payload.h>

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
