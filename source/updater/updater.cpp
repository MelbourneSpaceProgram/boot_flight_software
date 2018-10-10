#include "updater.h"
#include <stdio.h>
#include "assert.h"
#include "data_types.h"
#include "source/internal_image.h"
#include "ti/devices/msp432e4/driverlib/driverlib.h"

#define SYS_MCU_UART UART0_BASE
#define FULL_PROGRAM_LENGTH 0xFFFFF

err_t getProgramBytes(ImageBaseAddress image_base_address,
                      uint32_t program_counter, uint8_t* buffer,
                      uint8_t* buffer_len) {
    // Read from memory using a switch state

    if (image_base_address == Image11InMemory) {
        uint8_t bytes_to_read = 32;

        if (FULL_PROGRAM_LENGTH - program_counter < 32) {
            bytes_to_read = FULL_PROGRAM_LENGTH - program_counter;
        }

        *buffer_len = bytes_to_read;
        // Max buffer size will be 32; TODO Check this
        for (int i = 0; i < bytes_to_read; i++) {
            buffer[i] = flight_software[program_counter + i];
        }
    }


    return 0;
}

err_t sendCommand(uint8_t* buffer) {
    setChecksum(buffer);

    for (int i = 0; i < buffer[0]; i++) {
        UARTCharPut(SYS_MCU_UART, buffer[i]);
    }

    return 0;
}

err_t sendPing() {
    uint8_t buffer[] = {0x03, 0x20, 0x20};
    return sendCommand(buffer);
}

err_t setChecksum(uint8_t* command) {
    int32_t sum_of_bytes = 0;

    // Checksum is only calculated over field 3, which begins from byte 2
    // (0-indexed)
    for (int16_t i = 2; i < command[0]; i++) {
        sum_of_bytes += command[i];
    }

    command[1] = sum_of_bytes & 0xFF;

    return 0;
}

err_t sendSync() {
    bool sync_char_1 = UARTCharPutNonBlocking(SYS_MCU_UART, 0x55);
    bool sync_char_2 = UARTCharPutNonBlocking(SYS_MCU_UART, 0x55);

    if (sync_char_1 != true || sync_char_2 != true) {
        return 1;
    }
    return 0;
}

err_t sendDownload(uint32_t programAddress, uint32_t programSize) {
    uint8_t buffer[11] = {};

    buffer[0] = 0x0B;
    buffer[1] = 0x00;
    buffer[2] = 0x21;
    buffer[3] = programAddress >> 24 & 0xFF;
    buffer[4] = programAddress >> 16 & 0xFF;
    buffer[5] = programAddress >> 8 & 0xFF;
    buffer[6] = programAddress >> 0 & 0xFF;
    buffer[7] = programSize >> 24 & 0xFF;
    buffer[8] = programSize >> 16 & 0xFF;
    buffer[9] = programSize >> 8 & 0xFF;
    buffer[10] = programSize >> 0 & 0xFF;

    return sendCommand(buffer);
}

err_t sendGetStatus() {
    uint8_t buffer[] = {0x03, 0x23, 0x23};
    return sendCommand(buffer);
}

err_t sendAckResponse() {
    UARTCharPutNonBlocking(SYS_MCU_UART, ACK);
    return 0;
}

err_t sendReset() {
    uint8_t buffer[] = {0x03, 0x25, 0x25};
    return sendCommand(buffer);
}

err_t sendSendData(uint8_t* programData, uint8_t length) {
    ASSERT(length % 4 == 0);

    uint8_t buffer[4 * 16 + 3] = {};

    buffer[0] = 3 + length;
    buffer[1] = 0x00;
    buffer[2] = 0x24;

    for (int i = 0; i < length; i++) {
        buffer[3 + i] = programData[i];
    }

    return sendCommand(buffer);
}

err_t getType1Response(Response* command_response) {
    // Wait for packets available with some reset timer

    // 100 ms
    uint32_t timeout = 100000;  // TODO This value, like 5 seconds?

    while (timeout > 0) {
        if (UARTCharsAvail(SYS_MCU_UART)) {
            int32_t value = UARTCharGetNonBlocking(SYS_MCU_UART);
            value = value & 0xFF;
            if (value == 0) {
                continue;
            }

            if (value != 0) {
                if (value == ACK || value == NAK) {
                    *command_response = (Response)value;
                }
                return 0;
            }
        } else {
            SysCtlDelay(120E6 * 0.001);
            timeout--;
        }
    }
    // TODO Error checking on the value read back?
    return 1;
}

err_t getType2Response(Response* command_response) {
    uint32_t timeout = 5000000;  // TODO This value, like 5 seconds?

    uint8_t received_value_count = 0;

    while (timeout > 0) {
        int32_t value = UARTCharGet(SYS_MCU_UART);

        if (value != 0) {
            received_value_count++;
        }

        if (received_value_count == 3) {
            // The response, who cares about size etc
            // TODO
            *command_response = (Response)value;
            return 0;
        }
        SysCtlDelay(120E6 * 0.001);
        timeout--;
    }
    // TODO Error checking on the value read back?
    return -1;
}
err_t clearFifo() {
    while (UARTCharsAvail(SYS_MCU_UART)) {
        UARTCharGetNonBlocking(SYS_MCU_UART);
    }

    return 0;
}

err_t firmwareStateMachine(State* state, ImageBaseAddress image_address,
                           uint32_t* program_counter) {
    uint32_t reset_counter = 0;
    while (true) {
        clearFifo();
        if (reset_counter > reset_failure_threshold) {
            return -1;  // TODO
        }

        switch (*state) {
            case IDLE_STATE: {
                printf("Updater-IDLE_STATE");
                *state = SYNC_STATE;
                break;
            }
            case SYNC_STATE: {
                printf("Updater-SYNC_STATE");
                err_t sync_error = sendSync();
                if (sync_error != NO_ERROR) {
                    printf("Sync error %d", sync_error);
                    return sync_error;
                }

                // Am I receiving bytes here and then not dealing with them?

                SysCtlDelay(120E6 * 0.001);
                *state = PING_STATE;
                break;
            }
            case PING_STATE: {
                printf("Updater-PING_STATE");
                err_t ping_error = sendPing();
                if (ping_error != NO_ERROR) {
                    printf("Ping error %d", ping_error);
                    return ping_error;
                }

                Response command_response = NAK;
                err_t response_error = getType1Response(&command_response);
                SysCtlDelay(120E6 * 0.001);
                if (response_error == NO_ERROR && command_response == ACK) {
                    *state = DOWNLOAD_STATE;
                    break;
                } else {
                    printf("Response error %d, command_response %d",
                           response_error, command_response);
                    *state = RESET_STATE;
                    break;
                }
            }
            case DOWNLOAD_STATE: {
                printf("Updater-DOWNLOAD_STATE");
                err_t download_error =
                    sendDownload(program_start_address, program_size);
                if (download_error != NO_ERROR) {
                    printf("Download error %d", download_error);
                    return download_error;
                }

                Response command_response = NAK;
                err_t response_error = getType1Response(&command_response);

                if (response_error != NO_ERROR || command_response != ACK) {
                    printf("Response error %d, command_response %d",
                           response_error, command_response);
                    *state = RESET_STATE;
                    break;
                }

                err_t status_error = sendGetStatus();
                if (status_error != NO_ERROR) {
                    printf("Status error %d", status_error);
                    return status_error;
                }

                Response status_response;
                err_t status_response_error =
                    getType2Response(&status_response);

                if (status_response_error != NO_ERROR) {
                    printf("Type 2 status response error %d",
                           status_response_error);
                    *state = RESET_STATE;
                    break;
                }

                sendAckResponse();

                *state = SEND_DATA_STATE;
                break;
            }
            case SEND_DATA_STATE: {
                printf("Updater-SEND_DATA_STATE");
                uint8_t programData[32];
                uint8_t length;

                getProgramBytes(image_address, *program_counter, programData,
                                &length);

                err_t data_error = sendSendData(programData, length);

                if (data_error != NO_ERROR) {
                    printf("data error %d", data_error);
                    return data_error;
                }

                Response command_response = NAK;
                err_t response_error = getType1Response(&command_response);

                if (response_error != NO_ERROR || command_response != ACK) {
                    printf("Response error %d, command_response %d",
                           response_error, command_response);
                    *state = RESET_STATE;
                    break;
                }

                err_t status_error = sendGetStatus();
                if (status_error != NO_ERROR) {
                    printf("status error %d", status_error);
                    return status_error;
                }

                Response status_response;
                err_t status_response_error =
                    getType2Response(&status_response);

                if (status_response_error != NO_ERROR) {
                    printf("status response error %d", status_response_error);
                    *state = RESET_STATE;
                    break;
                }

                sendAckResponse();

                *program_counter += length;
                if (*program_counter == FULL_PROGRAM_LENGTH) {
                    *state = RESET_STATE;
                    break;
                } else {
                    *state = SEND_DATA_STATE;
                    break;
                }
                break;
            }
            case RESET_STATE: {
                printf("Updater-RESET_STATE");
                reset_counter++;
                err_t reset_error = sendReset();

                if (reset_error != NO_ERROR) {
                    return reset_error;
                }

                Response command_response = NAK;
                err_t response_error = getType1Response(&command_response);

                if (response_error != NO_ERROR || command_response != ACK) {
                    *state = RESET_STATE;
                    break;
                }

                *state = IDLE_STATE;
                break;
            }
        }
    }
    return NO_ERROR;
}

err_t updateFirmware(ImageBaseAddress image_address) {
    State state = IDLE_STATE;
    uint32_t reset_counter = 0;
    uint32_t program_counter = 0;

    firmwareStateMachine(&state, image_address, &program_counter);
}

err_t beginFirmwareUpdate(ImageBaseAddress image_address) {
    setbuf(stdout, NULL);

    // Take it out of reset
    GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_4, 0);
    SysCtlDelay(120E6 / 8);
    GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_4, GPIO_PIN_4);

    // Signal that the system MCU should enter the bootloader by
    // flagging these GPIO
    GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_7, 0x00);

    // Trigger a reset to get it back into the bootloader by pulling nRESET low
    GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_4, 0);
    SysCtlDelay(120E6 / 8);
    GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_4, GPIO_PIN_4);

    err_t update_error = updateFirmware(image_address);

    GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_7, GPIO_PIN_7);

    return update_error;
}
