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
    uint32_t buffer_len;

    beginFirmwareUpdate(Image11InMemory);

    SystemState state = SYSTEM_IDLE_STATE;
    while (1) {
        switch (state) {
            case SYSTEM_IDLE_STATE: {
                // Mock the packet

                buffer[0] = 0xCA;
                buffer[1] = 0xFE;
                buffer[2] = 15;
                buffer[3] = PAYLOAD_FIRMWARE_PART_PACKET;
                buffer[4] = Image11InMemory >> 24 & 0xFF;
                buffer[5] = Image11InMemory >> 16 & 0xFF;
                buffer[6] = Image11InMemory >> 8 & 0xFF;
                buffer[7] = Image11InMemory >> 0 & 0xFF;
                buffer[8] = 0x10000 >> 24 & 0xFF;
                buffer[9] = 0x10000 >> 16 & 0xFF;
                buffer[10] = 0x10000 >> 8 & 0xFF;
                buffer[11] = 0x10000 >> 0 & 0xFF;
                buffer[12] = 0x08;
                buffer[13] = 0x09;
                buffer[14] = 0x0A;
                buffer[15] = 0x0B;

                buffer_len = 16;

                state = SYSTEM_HANDLE_UMBILICAL_PACKET;

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

                state = SYSTEM_HANDLE_PAYLOAD;
                break;
            }
            case SYSTEM_HANDLE_PAYLOAD: {
                handlePayload(buffer, buffer_len);
                break;
            }
        }
    }
}
