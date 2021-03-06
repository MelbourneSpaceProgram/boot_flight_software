#ifndef SOURCE_UPDATER_UPDATER_H_
#define SOURCE_UPDATER_UPDATER_H_

#include "data_types.h"
#include "source/internal_image.h"

const uint32_t program_size = flight_software_length;
constexpr uint32_t program_start_address = 0x00;
constexpr uint32_t reset_failure_threshold = 100;
constexpr uint32_t uart_read_timeout_ms = 100;

enum UpdaterError {
    UPDATER_NO_ERROR = 0,
    UPDATER_ERROR_COUNT_EXCEEDED = 0x01,
    UPDATER_FAIL_SEND_SYNC_CHAR = 0x02,
    UPDATER_TYPE_1_RESPONSE_TIMEOUT = 0x03,
    UPDATER_TYPE_2_RESPONSE_TIMEOUT = 0x04,
};

enum State {
    IDLE_STATE = 0,
    PING_STATE = 1,
    RESET_STATE = 2,
    SYNC_STATE = 3,
    DOWNLOAD_STATE = 4,
    SEND_DATA_STATE = 5,
    SUCCESS_STATE = 6,
};

enum Response {
    ACK = 0xCC,
    NAK = 0x33,
    COMMAND_RET_SUCCESS = 0x40,
    COMMAND_RET_UNKNOWN_CMD = 0x41,
    COMMAND_RET_INVALID_CMD = 0x42,
    COMMAND_INVALID_ADR = 0x43,
    COMMAND_RET_FLASH_FAIL = 0x44,
    COMMAND_RET_CRC_FAIL = 0x45,
};

enum ImageBaseAddress {
    Golden1 = 0x100000,
    Golden2 = 0x200000,
    Golden3 = 0x300000,
    Golden4 = 0x400000,
    Golden5 = 0x500000,
    Image1 = 0x600000,
    Image2 = 0x700000,
    Image3 = 0x800000,
    Image4 = 0x900000,
    Image5 = 0xA00000,
    Image6 = 0xB00000,
    Image7 = 0xC00000,
    Image8 = 0xD00000,
    Image9 = 0xE00000,
    Image10 = 0xF00000,
    Image11InMemory = 0x1000000,
};

err_t beginFirmwareUpdate(ImageBaseAddress image_base_address);

#endif /* SOURCE_UPDATER_UPDATER_H_ */
