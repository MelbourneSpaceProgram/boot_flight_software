#ifndef SOURCE_INTERNAL_IMAGE_H_
#define SOURCE_INTERNAL_IMAGE_H_

#include "data_types.h"

extern const uint8_t flight_software[] __attribute__((section(".image")));
extern const uint32_t flight_software_length;

#endif /* SOURCE_INTERNAL_IMAGE_H_ */
