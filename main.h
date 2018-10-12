#ifndef MAIN_H_
#define MAIN_H_

#include "data_types.h"
#include "source/updater/updater.h"

enum SystemState {
    SYSTEM_IDLE_STATE = 0,
    SYSTEM_HANDLE_PAYLOAD = 4,
};

err_t init_peripherals(void);

#endif /* MAIN_H_ */
