#include "updater.h"
#include "assert.h"
#include "data_types.h"
#include "source/internal_image.h"
#include "ti/devices/msp432e4/driverlib/driverlib.h"

#define SYS_MCU_UART UART0_BASE
#define FULL_PROGRAM_LENGTH 0

err_t getProgramBytes(ImageBaseAddress image_base_address,
                      uint32_t program_counter, uint8_t* buffer,
                      uint8_t* buffer_len) {
    // Read from memory using a switch state

    if (image_base_address == Image11InMemory) {
        // Max buffer size will be 32; TODO Check this
        for (int i = 0; i < 32; i++) {
            buffer[i] = flight_software[program_counter + i];
            *buffer_len = i;
        }
    }

    return 0;
}

err_t sendCommand(uint8_t* buffer) {
    setChecksum(buffer);

    for (int i = 0; i < buffer[0]; i++) {
        bool success = UARTCharPutNonBlocking(SYS_MCU_UART, buffer[i]);
        if (success != true) {
            return 1;
        }
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
    buffer[3] = programAddress << 24 & 0xFF;
    buffer[4] = programAddress << 16 & 0xFF;
    buffer[5] = programAddress << 8 & 0xFF;
    buffer[6] = programAddress << 0 & 0xFF;
    buffer[7] = programSize << 24 & 0xFF;
    buffer[8] = programSize << 16 & 0xFF;
    buffer[9] = programSize << 8 & 0xFF;
    buffer[10] = programSize << 0 & 0xFF;

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
    uint32_t timeout = 10;  // TODO This value, like 5 seconds?

    while (timeout > 0) {
        int32_t value = UARTCharGet(SYS_MCU_UART);

        if (value == -1) {
            return 1;
        }

        if (value == 0) {
            continue;
        }

        if (value != 0) {
            *command_response = (Response)value;
            return 0;
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
    }
    // TODO Error checking on the value read back?
    return -1;
}

err_t firmwareStateMachine(State* state, ImageBaseAddress image_address,
                           uint32_t* program_counter) {
    switch (*state) {
        case IDLE_STATE: {
            *state = SYNC_STATE;
            break;
        }
        case SYNC_STATE: {
            err_t sync_error = sendSync();
            if (sync_error != NO_ERROR) {
                return sync_error;
            }

            SysCtlDelay(120E6 * 0.001);
            *state = PING_STATE;
            break;
        }
        case PING_STATE: {
            err_t ping_error = sendPing();
            if (ping_error != NO_ERROR) {
                return ping_error;
            }

            Response command_response = NAK;
            err_t response_error = getType1Response(&command_response);
            SysCtlDelay(120E6 * 0.001);
            if (response_error == NO_ERROR && command_response == ACK) {
                *state = DOWNLOAD_STATE;
                break;
            } else {
                *state = RESET_STATE;
                break;
            }
        }
        case DOWNLOAD_STATE: {
            err_t download_error =
                sendDownload(program_start_address, program_size);
            if (download_error != NO_ERROR) {
                return download_error;
            }

            Response command_response = NAK;
            err_t response_error = getType1Response(&command_response);

            if (response_error != NO_ERROR || command_response != ACK) {
                *state = RESET_STATE;
                break;
            }

            err_t status_error = sendGetStatus();
            if (status_error != NO_ERROR) {
                return status_error;
            }

            Response status_response;
            err_t status_response_error = getType2Response(&status_response);

            if (status_response_error != NO_ERROR) {
                *state = RESET_STATE;
                break;
            }

            sendAckResponse();

            *state = SEND_DATA_STATE;
            break;
        }
        case SEND_DATA_STATE: {
            uint8_t programData[32];
            uint8_t length;

            getProgramBytes(image_address, *program_counter, programData,
                            &length);

            err_t data_error = sendSendData(programData, length);

            if (data_error != NO_ERROR) {
                return data_error;
            }

            Response command_response = NAK;
            err_t response_error = getType1Response(&command_response);

            if (response_error != NO_ERROR || command_response != ACK) {
                *state = RESET_STATE;
                break;
            }

            err_t status_error = sendGetStatus();
            if (status_error != NO_ERROR) {
                return status_error;
            }

            Response status_response;
            err_t status_response_error = getType2Response(&status_response);

            if (status_response_error != NO_ERROR) {
                *state = RESET_STATE;
                break;
            }

            sendAckResponse();

            *program_counter += length;
            // TODO Check this is not comparing the pointer
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
    return NO_ERROR;
}

err_t updateFirmware(ImageBaseAddress image_address) {
    State state = IDLE_STATE;
    uint32_t reset_counter = 0;
    uint32_t program_counter = 0;

    while (1) {
        firmwareStateMachine(&state, image_address, &program_counter);

        if (state == RESET_STATE) {
            reset_counter++;
        }

        if (reset_counter > reset_failure_threshold) {
            return -1;  // TODO
        }
    }
}

err_t beginFirmwareUpdate(ImageBaseAddress image_address) {
    // Signal that the system MCU should enter the bootloader by
    // flagging these GPIO
    GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_7, 0x00);

    // Trigger a reset to get it back into the bootloader by pulling nRESET low
    GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_4, 0);
    // Wait? TODO
    GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_4, GPIO_PIN_4);

    err_t update_error = updateFirmware(image_address);

    GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_7, GPIO_PIN_7);

    return update_error;
}
