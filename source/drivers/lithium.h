
#ifndef SOURCE_DRIVERS_LITHIUM_H_
#define SOURCE_DRIVERS_LITHIUM_H_

#include <data_types.h>

#define LITHIUM_BUFFER_MAX_LEN 255
#define LITHIUM_UART UART2_BASE

enum LithiumStatus {
    LITHIUM_NO_ERROR = 0x00,
    LITHIUM_NO_PACKET = 0x01,
    LITHIUM_BAD_SYNC_BITS = 0x02,
    LITHIUM_BAD_DIRECTION = 0x03,
    LITHIUM_BAD_HEADER_CHECKSUM = 0x04
};

enum LithiumCommandCodes {
    kNoOpCommandCode = 0x01,
    kResetSystemCommandCode = 0x02,
    kTransmitCommandCode = 0x03,
    kReceivedDataCode = 0x04,
    kGetConfigurationCommandCode = 0x05,
    kSetConfigurationCommandCode = 0x06,
    kGetTelemetryCommandCode = 0x07,
    kWriteFlashCommandCode = 0x08,
    kRfConfigureCommandCode = 0x09,
    kSetBeaconDataCommandCode = 0x10,
    kConfigureBeaconCommandCode = 0x11,
    kGetFirmwareRevCommandCode = 0x12,
    kDioKeyWriteCommandCode = 0x13,
    kFastSetPaCommandCode = 0x20,
};

constexpr uint8_t kLithiumSyncCharOne = 0x48;
constexpr uint8_t kLithiumSyncCharTwo = 0x65;
constexpr uint8_t kLithiumDirectionIn = 0x10;
constexpr uint8_t kLithiumDirectionOut = 0x20;

constexpr uint8_t kLithiumSyncCharOneByte = 0x00;
constexpr uint8_t kLithiumSyncCharTwoByte = 0x01;
constexpr uint8_t kLithiumDirectioneByte = 0x02;
constexpr uint8_t kLithiumCommandCodeByte = 0x03;

void CalcLithiumChecksum(uint8_t* checksum, const uint8_t* data,
                         const uint16_t data_size);
err_t validateLithiumPacket(const uint8_t* buffer, uint8_t buffer_len);
err_t getLithiumPacket(uint8_t* destination, uint8_t* buffer_len);

#endif /* SOURCE_DRIVERS_LITHIUM_H_ */
