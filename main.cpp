#include "main.h"
#include <main.h>
#include <source/drivers/hal.h>
#include <source/drivers/lithium.h>
#include <source/drivers/payload.h>
#include <source/drivers/ring_buffer.h>
#include <source/drivers/umbilical.h>
#include <source/globals.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <ti/devices/msp432e4/driverlib/driverlib.h>
#include "source/drivers/lithium.h"
#include "source/drivers/payload.h"
#include "source/drivers/umbilical.h"
#include "source/updater/updater.h"

uint8_t SYSTEM_FLAGS;

int main(void) {
    init_clock();
    init_gpio();
    init_hibernate();

    bool hibernate_on_boot;
    should_hibernate_on_boot(&hibernate_on_boot);
    if (hibernate_on_boot) {
        SysCtlDelay(30 * system_clock_hz);  // 30 second wait before hibernating
        GPIOPinWrite(sys_reset_port, sys_reset_pin, 0x00);
        MAP_HibernateRTCMatchSet(0, (MAP_HibernateRTCGet() + hibernate_time_s));
        MAP_HibernateRequest();

        while (1) {
        }
    }

    GPIOPinWrite(sys_reset_port, sys_reset_pin, sys_reset_pin);

    init_system_uart();
    init_lithium_uart();
    init_umbilical_uart();
    init_flash_spi();
    init_crc();
    MAP_IntMasterEnable();

    SYSTEM_FLAGS = 0;

    uint8_t payload_buffer_len = 0;
    uint8_t payload_buffer[payload_buffer_max_len] = {0};

    SystemState state = SYSTEM_IDLE_STATE;
    while (1) {
        switch (state) {
            case SYSTEM_IDLE_STATE: {
                // If something of interest in a buffer, call the packet handler
                uint8_t are_bytes_available;
                umbilicalBytesAvailable(&are_bytes_available);
                if (are_bytes_available > 5) {
                    state = SYSTEM_HANDLE_UMBILICAL_PACKET;
                    break;
                }

                state = SYSTEM_IDLE_STATE;
                break;
            }
            case SYSTEM_HANDLE_UMBILICAL_PACKET: {
                err_t read_successful =
                    umbilicalReadPacket(payload_buffer, &payload_buffer_len);
                if (read_successful != UMB_NO_ERROR) {
                    state = SYSTEM_IDLE_STATE;
                    break;
                }
                state = SYSTEM_HANDLE_PAYLOAD;
                break;
            }
            case SYSTEM_HANDLE_LITHIUM_PACKET: {
                err_t read_successful =
                    lithiumReadPacket(payload_buffer, &payload_buffer_len);
                if (read_successful != UMB_NO_ERROR) {
                    state = SYSTEM_IDLE_STATE;
                    break;
                }

                state = SYSTEM_HANDLE_PAYLOAD;
                break;
            }
            case SYSTEM_HANDLE_PAYLOAD: {
                handlePayload(payload_buffer, (uint8_t)payload_buffer_len);
                state = SYSTEM_IDLE_STATE;
                break;
            }
        }
    }
}
