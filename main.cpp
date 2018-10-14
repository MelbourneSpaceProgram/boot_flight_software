#include <main.h>
#include <source/drivers/hal.h>
#include <source/drivers/payload.h>
#include <source/drivers/umbilical.h>
#include <source/globals.h>

uint8_t SYSTEM_FLAGS;

int main(void) {
    init_clock();
    init_gpio();
    init_system_uart();
    init_lithium_uart();
    init_umbilical_uart();
    init_flash_spi();
    init_crc();
    MAP_IntMasterEnable();

    SYSTEM_FLAGS = 0;

    uint8_t umbilical_buffer_len = 0;
    uint8_t umbilical_buffer[umbilical_buffer_max_len] = {0};

    uint32_t payload_buffer_len = 0;
    uint8_t payload_buffer[payload_buffer_max_len] = {0};

    SystemState state = SYSTEM_IDLE_STATE;
    while (1) {
        switch (state) {
            case SYSTEM_IDLE_STATE: {
                // If something of interest in a buffer, call the packet handler
                if (SYSTEM_FLAGS & kUmbilicalReadBitMask) {
                    state = SYSTEM_HANDLE_UMBILICAL_PACKET;
                    break;
                }

                state = SYSTEM_IDLE_STATE;
                break;
            }
            case SYSTEM_HANDLE_UMBILICAL_PACKET: {
                err_t read_successful = umbilicalRead();
                SYSTEM_FLAGS &= !kUmbilicalReadBitMask;
                if (read_successful != UMB_NO_ERROR) {
                    umbilical_buffer_len = 0;
                    state = SYSTEM_IDLE_STATE;
                    break;
                }
                err_t umb_packet_error =
                    getUmbilicalPacket(umbilical_buffer, &umbilical_buffer_len);
                umbilical_buffer_len = 0;
                state = SYSTEM_IDLE_STATE;
                break;
            }
            case SYSTEM_HANDLE_PAYLOAD: {
                handlePayload(payload_buffer, umbilical_buffer_len);
                break;
            }
        }
    }
}
