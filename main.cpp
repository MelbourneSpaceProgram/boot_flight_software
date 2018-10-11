#include <source/drivers/hal.h>
#include "main.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <ti/devices/msp432e4/driverlib/driverlib.h>
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

    uint8_t umbilical_buffer[umbilical_buffer_max_len] = {0};
    uint8_t umbilical_buffer_len = 0;

    uint8_t lithium_buffer[LITHIUM_BUFFER_MAX_LEN];
    uint8_t lithium_buffer_len;

    uint8_t buffer[UMBILICAL_BUFFER_MAX_LEN];
    uint32_t buffer_len;
    uint8_t payload_buffer[payload_buffer_max_len] = {0};
    uint32_t payload_buffer_len = 0;

    SystemState state = SYSTEM_IDLE_STATE;
    while (1) {
        switch (state) {
            case SYSTEM_IDLE_STATE: {
                // TODO
                // If something of interest in a buffer, call the packet handler
                err_t umb_packet_error =
                    getUmbilicalPacket(umbilical_buffer, &umbilical_buffer_len);

                if (umb_packet_error == UMB_NO_ERROR) {
                    state = SYSTEM_HANDLE_UMBILICAL_PACKET;
                    break;
                }

                err_t lithium_packet_error =
                    getLithiumPacket(lithium_buffer, &lithium_buffer_len);

                if (lithium_packet_error == LITHIUM_NO_ERROR) {
                    state = SYSTEM_HANDLE_LITHIUM_PACKET;
                    break;
                }

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

                // Place just the internal payload into the payload_buffer
                memcpy(payload_buffer, umbilical_buffer + 5,
                       umbilical_buffer_len - 5);
                payload_buffer_len = umbilical_buffer_len;

                state = SYSTEM_HANDLE_PAYLOAD;
                break;
            }
            case SYSTEM_HANDLE_LITHIUM_PACKET: {
                err_t packet_valid =
                    validateLithiumPacket(lithium_buffer, lithium_buffer_len);

                if (packet_valid != LITHIUM_NO_ERROR) {
                    state = SYSTEM_IDLE_STATE;
                    break;
                }

                if (lithium_buffer[kLithiumCommandCodeByte] !=
                    kReceivedDataCode) {
                    state = SYSTEM_HANDLE_LITHIUM_COMMAND_RESPONSE;
                    break;
                }
                memcpy(buffer, lithium_buffer, lithium_buffer_len);
                buffer_len = lithium_buffer_len;

                state = SYSTEM_HANDLE_PAYLOAD;
                break;
            }
            case SYSTEM_HANDLE_PAYLOAD: {
                handlePayload(payload_buffer, payload_buffer_len);
                break;
            }
            case SYSTEM_HANDLE_LITHIUM_COMMAND_RESPONSE: {
                // TODO(wschuetz) Handle responses from Lithium
            }
        }
    }
}
