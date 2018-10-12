#ifndef SOURCE_DRIVERS_FLASH_H_
#define SOURCE_DRIVERS_FLASH_H_

#include "data_types.h"

// Dummy byte when padding or during receive operations
constexpr byte DUMMY_BYTE = 0x00;
constexpr byte EN4B = 0xB7;
constexpr byte READ4B = 0x13;
constexpr byte PP4 = 0x38;
constexpr byte SE = 0x20;
constexpr byte WREN = 0x06;
constexpr uint16_t kPageLength = 256;
constexpr uint16_t kSectorLength = 4096;

err_t configureFlashSpi();
err_t flash4ByteMode();
err_t flashSelectChip();
err_t flashDeselectChip();
err_t flashRead(uint32_t base_address, byte* read_buffer,
                uint32_t buffer_length);
err_t flashWrite(uint32_t base_address, byte* write_buffer,
                 uint32_t buffer_length);
void clearFifo();
err_t flashWriteEnable();

#endif  // SOURCE_DRIVERS_FLASH_H
