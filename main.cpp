#include <main.h>
#include <source/drivers/hal.h>
#include <source/drivers/payload.h>
#include <source/drivers/umbilical.h>
#include <source/globals.h>

uint8_t SYSTEM_FLAGS;

extern "C" {
volatile bool hibernate_on_boot = true;
void HIBERNATE_IRQHandler(void) {
    uint32_t getIntStatus;

    /* Get the Hibernate Interrupt Status*/
    getIntStatus = MAP_HibernateIntStatus(true);

    if (getIntStatus == HIBERNATE_INT_RTC_MATCH_0) {
        MAP_HibernateIntClear(HIBERNATE_INT_RTC_MATCH_0);
        hibernate_on_boot = false;
    }
}
}

int main(void) {
    init_clock();
    init_gpio();
    init_hibernate();

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
                    state = SYSTEM_IDLE_STATE;
                    break;
                }
                err_t umb_packet_error =
                    getUmbilicalPacket(payload_buffer, &payload_buffer_len);
                // TODO(wschuetz) change to SYSTEM_HANDLE_PAYLOAD when needed
                state = SYSTEM_IDLE_STATE;
                break;
            }
            case SYSTEM_HANDLE_PAYLOAD: {
                handlePayload(payload_buffer, (uint8_t)payload_buffer_len);
                break;
            }
        }
    }
}
