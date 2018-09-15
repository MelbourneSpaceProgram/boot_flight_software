/* DriverLib Includes */
#include <ti/devices/msp432e4/driverlib/driverlib.h>

/* Standard Includes */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "main.h"
#include "source/updater/updater.h"

enum SystemState {
    SYSTEM_IDLE_STATE = 0,
    SYSTEM_UPDATE_BOOT = 1,
};

ImageBaseAddress image_address;
/* System clock rate in Hz */
uint32_t systemClock;

err_t init_gpio() {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOC)) {
    }

    GPIOPinTypeGPIOOutput(GPIO_PORTC_BASE,
                          GPIO_PIN_0 | GPIO_PIN_6 | GPIO_PIN_7);
    // Don't turn the system MCU off... yet
    GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_0, GPIO_PIN_0);

    // De-assert the trigger lines so we don't enter the bootloader
    // TODO Why am I running two?
    GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6 | GPIO_PIN_7, 0x00);

    return 0;
}

err_t init_system_uart() {
    /* Enable the GPIO Peripheral used by the UART */
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    while (!(MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA))) {
    }

    /* Enable UART0 */
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    /* Configure GPIO Pins for UART mode */
    MAP_GPIOPinConfigure(GPIO_PA0_U0RX);
    MAP_GPIOPinConfigure(GPIO_PA1_U0TX);
    MAP_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    return 0;
}

err_t init_lithium_uart() {
    /* Enable the GPIO Peripheral used by the UART */
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    while (!(MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA))) {
    }

    /* Enable UART0 */
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART2);

    /* Configure GPIO Pins for UART mode */
    MAP_GPIOPinConfigure(GPIO_PA6_U2RX);
    MAP_GPIOPinConfigure(GPIO_PA7_U2TX);
    MAP_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_6 | GPIO_PIN_7);

    return 0;
}

err_t init_flash_spi() {
    // TODO Confirm pinning
    /* Enable clocks to GPIO Port A and configure pins as SSI */
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    while (!(MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOE))) {
    }

    MAP_GPIOPinConfigure(GPIO_PB5_SSI1CLK);
    MAP_GPIOPinConfigure(GPIO_PB4_SSI1FSS);  // TODO
    MAP_GPIOPinConfigure(GPIO_PE4_SSI1XDAT0);
    MAP_GPIOPinConfigure(GPIO_PE5_SSI1XDAT1);
    MAP_GPIOPinTypeSSI(GPIO_PORTA_BASE,
                       (GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5));

    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI1);
    while (!(MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_SSI1))) {
    }

    MAP_SSIConfigSetExpClk(SSI1_BASE, systemClock, SSI_FRF_MOTO_MODE_0,
                           SSI_MODE_MASTER, (systemClock / 24), 8);
    MAP_SSIEnable(SSI1_BASE);

    /* Flush the Receive FIFO */
    uint32_t getResponseData;
    while (MAP_SSIDataGetNonBlocking(SSI1_BASE, &getResponseData))
        ;

    return 0;
}

int main(void) {
    /* Configure the system clock for 120 MHz */
    // TODO Confirm
    systemClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN |
                                          SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480),
                                         120000000);

    init_gpio();
    init_system_uart();
    init_lithium_uart();
    init_flash_spi();

    SystemState state = SYSTEM_IDLE_STATE;
    while (1) {
        switch (state) {
            case SYSTEM_IDLE_STATE: {
                break;
            }
            case SYSTEM_UPDATE_BOOT: {
                // Signal that the system MCU should enter the bootloader by
                // flagging these GPIO
                GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6 | GPIO_PIN_7,
                             GPIO_PIN_6 | GPIO_PIN_7);

                // Trigger a reset to get it back into the bootloader
                GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_0, 0);
                // Wait? TODO
                GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_0, GPIO_PIN_0);

                err_t update_error = updateFirmware(image_address);

                GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6 | GPIO_PIN_7, 0x00);
                break;
            }
        }
    }
}
