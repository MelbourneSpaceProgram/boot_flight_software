/* DriverLib Includes */
#include <ti/devices/msp432e4/driverlib/driverlib.h>

/* Standard Includes */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "main.h"
#include "source/drivers/init_hal.h"
#include "source/drivers/lithium.h"
#include "source/drivers/payload.h"
#include "source/drivers/umbilical.h"
#include "source/updater/updater.h"

ImageBaseAddress image_address;

int main(void) {
    init_clock();
    init_gpio();
    init_system_uart();
    // init_lithium_uart();
    // init_umbilical_uart();
    // init_flash_spi();

    // while (1) {
    //}
    uint8_t umbilical_buffer[UMBILICAL_BUFFER_MAX_LEN];
    uint8_t umbilical_buffer_len;

    uint8_t buffer[UMBILICAL_BUFFER_MAX_LEN];
    uint32_t buffer_len;

    err_t error __attribute__((unused));
    error = beginFirmwareUpdate(Image11InMemory);

    while (1) {
    }
    SystemState state = SYSTEM_IDLE_STATE;
    while (1) {
        switch (state) {
            case SYSTEM_IDLE_STATE: {
                err_t umb_packet_error =
                    getUmbilicalPacket(umbilical_buffer, &umbilical_buffer_len);

                if (umb_packet_error == UMB_NO_ERROR) {
                    state = SYSTEM_HANDLE_UMBILICAL_PACKET;
                    break;
                }

                state = SYSTEM_IDLE_STATE;
                break;
            }
            case SYSTEM_UPDATE_BOOT: {
                beginFirmwareUpdate(image_address);
                state = SYSTEM_IDLE_STATE;
                break;
            }
            case SYSTEM_HANDLE_UMBILICAL_PACKET: {
                // There must be a valid packet in umbilical_buffer to be here
                err_t packet_valid = validateUmbilicalPacket(
                    umbilical_buffer, umbilical_buffer_len);

                if (packet_valid != UMB_NO_ERROR) {
                    state = SYSTEM_IDLE_STATE;
                    break;
                }

                // TODO Decode the Lithium header where possible, then pass it
                // over to the payload processor

                memcpy(buffer, umbilical_buffer, umbilical_buffer_len);
                buffer_len = umbilical_buffer_len;

                state = SYSTEM_HANDLE_PAYLOAD;
                break;
            }
            case SYSTEM_HANDLE_PAYLOAD: {
                handlePayload(buffer + 3, buffer_len - 3);
                break;
            }
            case SYSTEM_HANDLE_LITHIUM_PACKET: {
                // TODO
                break;
            }
        }
    }
}
