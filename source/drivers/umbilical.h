
#ifndef SOURCE_DRIVERS_UMBILICAL_H_
#define SOURCE_DRIVERS_UMBILICAL_H_

#include "data_types.h"

#define UMBILICAL_BUFFER_MAX_LEN 255

enum UmbilicalStatus {
    UMB_NO_ERROR = 0x00,
    UMB_NO_PACKET = 0x01,
    UMB_BAD_PACKET = 0x02,
};

enum UmbilicalPacketType {
    UMB_FIRMWARE_PART_PACKET = 0x01,
    UMB_GET_STATUS_PACKET = 0x02,
    UMB_COPY_IMAGE_PACKET = 0x03,
    UMB_INITIATE_UPDATE_PACKET = 0x04,
};

#define UMB_PACKET_SYNC_CHAR_1 0xCA
#define UMB_PACKET_SYNC_CHAR_2 0xFE

err_t getUmbilicalPacket(uint8_t* destination, uint8_t* buffer_len);
err_t handleUmbilicalPacket(uint8_t* buffer, uint8_t buffer_len);
err_t validateUmbilicalPacket(uint8_t* buffer, uint8_t buffer_len);

#endif /* SOURCE_DRIVERS_UMBILICAL_H_ */
