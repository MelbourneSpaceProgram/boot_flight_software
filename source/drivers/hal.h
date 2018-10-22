#ifndef SOURCE_DRIVERS_HAL_H_
#define SOURCE_DRIVERS_HAL_H_

#include <ti/devices/msp432e4/driverlib/driverlib.h>
#include "data_types.h"

constexpr uint32_t update_trigger_port = GPIO_PORTC_BASE;
constexpr uint32_t update_trigger_pin = GPIO_PIN_7;
constexpr uint32_t sys_reset_port = GPIO_PORTC_BASE;
constexpr uint32_t sys_reset_pin = GPIO_PIN_4;
constexpr uint32_t led_port = GPIO_PORTQ_BASE;
constexpr uint32_t led_pin = GPIO_PIN_4;

static uint32_t system_clock_hz;

err_t init_gpio();
err_t init_system_uart();
err_t init_lithium_uart();
err_t init_umbilical_uart();
err_t init_flash_spi();
err_t init_clock();
err_t init_crc();
init_hibernate();

#endif /* SOURCE_DRIVERS_HAL_H_ */
