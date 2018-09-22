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
    init_lithium_uart();
    init_umbilical_uart();
    init_flash_spi();

    uint8_t umbilical_buffer[UMBILICAL_BUFFER_MAX_LEN];
    uint8_t lithium_buffer[LITHIUM_BUFFER_MAX_LEN];
    uint8_t umbilical_buffer_len;

    uint8_t* buffer = umbilical_buffer + 10;  // TODO Made up offset

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

                // Check for Lithium packet TODO

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

                state = SYSTEM_HANDLE_PAYLOAD;
                break;
            }
            case SYSTEM_HANDLE_PAYLOAD: {
                // TODO
                handlePayload(buffer, 123);
                break;
            }
            case SYSTEM_HANDLE_LITHIUM_PACKET: {
                // TODO Decode the Lithium header where possible, then pass it
                // over to the payload processor There must be a valid packet in
                // lithium_buffer to be here

                // Then pass the packet to SYSTEM_HANDLE_PAYLOAD if it was
                // successfully decoded
                state = SYSTEM_IDLE_STATE;
                break;
            }
        }
    }
}


