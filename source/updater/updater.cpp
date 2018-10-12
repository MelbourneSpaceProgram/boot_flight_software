#include "updater.h"
#include <stdio.h>
#include "assert.h"
#include "data_types.h"
#include "source/drivers/init_hal.h"
#include "source/internal_image.h"
#include "ti/devices/msp432e4/driverlib/driverlib.h"

#define SYS_MCU_UART UART0_BASE

err_t sendCommand(uint8_t* buffer);
err_t setChecksum(uint8_t* command);

err_t getProgramBytes(ImageBaseAddress image_base_address,
                      uint32_t program_counter, uint8_t* buffer,
                      uint8_t* buffer_len) {
    // Read from memory using a switch state

    if (image_base_address == Image11InMemory) {
        uint8_t bytes_to_read = 32;

        if (program_size - program_counter < 32) {
            bytes_to_read = program_size - program_counter;
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

err_t firmwareStateMachine(ImageBaseAddress image_address) {
    uint32_t reset_counter = 0;
    bool led_flag = 1;
    uint32_t program_counter = 0;
    State state = IDLE_STATE;

    while (true) {
        if (led_flag) {
            led_flag = 0;
            GPIOPinWrite(led_port, led_pin, 0);
        } else {
            led_flag = 1;
            GPIOPinWrite(led_port, led_pin, led_pin);
        }

        clearFifo();
        if (reset_counter > reset_failure_threshold) {
            return -1;  // TODO
        }

        switch (state) {
            case IDLE_STATE: {
                state = SYNC_STATE;
                break;
            }
            case SYNC_STATE: {
                err_t sync_error = sendSync();
                if (sync_error != NO_ERROR) {
                    return sync_error;
                }

                // Am I receiving bytes here and then not dealing with them?

                SysCtlDelay(120E6 * 0.001);
                state = PING_STATE;
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
                    state = DOWNLOAD_STATE;
                    break;
                } else {
                    state = RESET_STATE;
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
                    state = RESET_STATE;
                    break;
                }

                err_t status_error = sendGetStatus();
                if (status_error != NO_ERROR) {
                    return status_error;
                }

                Response status_response;
                err_t status_response_error =
                    getType2Response(&status_response);

                if (status_response_error != NO_ERROR) {
                    state = RESET_STATE;
                    break;
                }

                sendAckResponse();

                state = SEND_DATA_STATE;
                break;
            }
            case SEND_DATA_STATE: {
                uint8_t programData[32];
                uint8_t length;

                getProgramBytes(image_address, program_counter, programData,
                                &length);

                err_t data_error = sendSendData(programData, length);

                if (data_error != NO_ERROR) {
                    return data_error;
                }

                Response command_response = NAK;
                err_t response_error = getType1Response(&command_response);

                if (response_error != NO_ERROR || command_response != ACK) {
                    state = RESET_STATE;
                    break;
                }

                err_t status_error = sendGetStatus();
                if (status_error != NO_ERROR) {
                    return status_error;
                }

                Response status_response;
                err_t status_response_error =
                    getType2Response(&status_response);

                if (status_response_error != NO_ERROR) {
                    state = RESET_STATE;
                    break;
                }

                sendAckResponse();

                program_counter += length;
                if (program_counter == program_size) {
                    state = RESET_STATE;
                    break;
                } else {
                    state = SEND_DATA_STATE;
                    break;
                }
                break;
            }
            case RESET_STATE: {
                reset_counter++;
                err_t reset_error = sendReset();

                if (reset_error != NO_ERROR) {
                    return reset_error;
                }

                Response command_response = NAK;
                err_t response_error = getType1Response(&command_response);

                if (response_error != NO_ERROR || command_response != ACK) {
                    state = RESET_STATE;
                    break;
                }

                state = SUCCESS_STATE;
                break;
            }
            case SUCCESS_STATE: {
                return NO_ERROR;
            }
        }
    }
    return NO_ERROR;
}

err_t beginFirmwareUpdate(ImageBaseAddress image_address) {
    // Signal that the system MCU should enter the bootloader by
    // flagging these GPIO
    GPIOPinWrite(led_port, led_pin, 0x00);
    GPIOPinWrite(update_trigger_port, update_trigger_pin, 0x00);

    // Trigger a reset to get it back into the bootloader by pulling nRESET low
    GPIOPinWrite(sys_reset_port, sys_reset_pin, 0);
    SysCtlDelay(120E6 / 8);
    GPIOPinWrite(sys_reset_port, sys_reset_pin, sys_reset_pin);

    err_t update_error = firmwareStateMachine(image_address);

    GPIOPinWrite(led_port, led_pin, led_pin);
    GPIOPinWrite(update_trigger_port, update_trigger_pin, update_trigger_pin);

    return update_error;
}
