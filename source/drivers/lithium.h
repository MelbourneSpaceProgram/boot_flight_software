
#ifndef SOURCE_DRIVERS_LITHIUM_H_
#define SOURCE_DRIVERS_LITHIUM_H_

#include <data_types.h>
#include <stdint.h>

constexpr uint32_t lithium_buffer_max_len = 255;

err_t lithiumBytesAvailable(uint8_t* bytes_available);
err_t lithiumReadPacket(uint8_t lithium_buffer[255],
                        uint8_t* lithium_buffer_len);

#endif /* SOURCE_DRIVERS_LITHIUM_H_ */
