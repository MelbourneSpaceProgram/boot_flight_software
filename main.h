#ifndef MAIN_H_
#define MAIN_H_

#include "data_types.h"
#include "source/updater/updater.h"

enum SystemState {
    SYSTEM_IDLE_STATE = 0,
    SYSTEM_HANDLE_LITHIUM_PACKET = 2,
    SYSTEM_HANDLE_UMBILICAL_PACKET = 3,
    SYSTEM_HANDLE_PAYLOAD = 4,
    SYSTEM_HANDLE_LITHIUM_COMMAND_RESPONSE = 5,
};

err_t init_peripherals(void);

#endif /* MAIN_H_ */
