#ifndef SOURCE_UPDATER_UPDATER_H_
#define SOURCE_UPDATER_UPDATER_H_

#include "data_types.h"

err_t sendCommand(uint8_t* buffer);

enum UpdaterError {
    NO_ERROR = 0,

};
enum State {
    IDLE_STATE = 0,
    PING_STATE = 1,
    RESET_STATE = 2,
    SYNC_STATE = 3,
    DOWNLOAD_STATE = 4,
    SEND_DATA_STATE = 5,
};

enum Response {
    ACK = 0x00,  // TODO
    NAK = 0x01,  // TODO
};

constexpr uint32_t program_size = 0xFFFFF;
constexpr uint32_t program_start_address = 0x00;
constexpr uint32_t reset_failure_threshold = 100;

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

err_t setChecksum(uint8_t* command);
err_t updateFirmware(ImageBaseAddress image_address);
err_t beginFirmwareUpdate(ImageBaseAddress image_address);

#endif /* SOURCE_UPDATER_UPDATER_H_ */
