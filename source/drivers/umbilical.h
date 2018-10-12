#ifndef SOURCE_DRIVERS_UMBILICAL_H_
#define SOURCE_DRIVERS_UMBILICAL_H_

#include "data_types.h"

constexpr uint32_t umbilical_buffer_max_len = 255;

constexpr uint8_t kUmbilicalSyncChar1 = 0xCA;
constexpr uint8_t kUmbilicalSyncChar2 = 0xFE;

enum UmbilicalStatus {
    UMB_NO_ERROR = 0x00,
    UMB_NO_PACKET = 0x01,
    UMB_BAD_PACKET = 0x02,
    UMB_BAD_PACKET_HEADER = 0x03,
    UMB_BAD_PACKET_LENGTH = 0x04,
    UMB_BAD_PACKET_COMMAND = 0x05,
};

err_t getUmbilicalPacket(uint8_t* destination, uint8_t* buffer_len);
err_t validateUmbilicalPacket(uint8_t* buffer, uint8_t buffer_len);

#endif /* SOURCE_DRIVERS_UMBILICAL_H_ */
