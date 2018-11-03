#include "main.h"
#include <main.h>
#include <source/drivers/flash.h>
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
        GPIOPinWrite(sys_reset_port, low_power_trigger_pin,
                     low_power_trigger_pin);
        SysCtlDelay(10 * system_clock_hz);  // 10 second wait before hibernating
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

    static uint32_t payloads_received = 0;
    static uint32_t payload_errors = 0;

    SystemState state = SYSTEM_IDLE_STATE;
    while (1) {
        switch (state) {
            case SYSTEM_IDLE_STATE: {
                // If something of interest in a buffer, call the packet handler
                uint8_t n_bytes_available = 0;
                umbilicalBytesAvailable(&n_bytes_available);
                if (n_bytes_available > 5) {
                    state = SYSTEM_HANDLE_UMBILICAL_PACKET;
                    break;
                }

                lithiumBytesAvailable(&n_bytes_available);
                if (n_bytes_available > 5) {
                    state = SYSTEM_HANDLE_LITHIUM_PACKET;
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
                if (read_successful != LITHIUM_NO_ERROR) {
                    state = SYSTEM_IDLE_STATE;
                    break;
                }

                state = SYSTEM_HANDLE_PAYLOAD;
                break;
            }
            case SYSTEM_HANDLE_PAYLOAD: {
                payloads_received++;
                err_t err =
                    handlePayload(payload_buffer, (uint8_t)payload_buffer_len);
                if (err != 0) {
                    payload_errors++;
                }
                state = SYSTEM_IDLE_STATE;
                break;
            }
        }
    }
}
