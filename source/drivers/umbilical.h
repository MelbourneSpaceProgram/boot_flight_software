#ifndef SOURCE_DRIVERS_UMBILICAL_H_
#define SOURCE_DRIVERS_UMBILICAL_H_

#include "data_types.h"

#define UMBILICAL_UART UART1_BASE
#define UMBILICAL_BUFFER_MAX_LEN 10000

constexpr uint32_t umbilical_buffer_max_len = 255;

constexpr uint8_t kUmbilicalSyncChar1 = 0xCA;
constexpr uint8_t kUmbilicalSyncChar2 = 0xFE;

constexpr uint8_t kUmbilicalReadBitMask = 0x01;

enum UmbilicalStatus {
    UMB_NO_ERROR = 0x00,
    UMB_NO_PACKET = 0x01,
    UMB_BAD_PACKET = 0x02,
    UMB_BAD_PACKET_HEADER = 0x03,
    UMB_BAD_SYNC_BITS = 0x04,
    UMB_BAD_PACKET_LENGTH = 0x05,
    UMB_BAD_PACKET_COMMAND = 0x06,
};

err_t umbilicalReadPacket(uint8_t *, uint8_t* len);
err_t umbilicalBytesAvailable(uint8_t* bytes_available);
err_t validateUmbilicalHeader(uint8_t* buffer, uint8_t buffer_len);

#endif /* SOURCE_DRIVERS_UMBILICAL_H_ */
