#include <source/drivers/hal.h>
#include <ti/devices/msp432e4/driverlib/driverlib.h>

err_t init_clock() {
    system_clock_hz = MAP_SysCtlClockFreqSet(
        SYSCTL_OSC_INT | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480, 120000000);
    return 0;
}

err_t init_gpio() {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOC)) {
    }

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOQ);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOQ)) {
    }

    GPIOPinTypeGPIOOutput(GPIO_PORTC_BASE, sys_reset_pin | update_trigger_pin);
    GPIOPinTypeGPIOOutput(led_port, led_pin);

    // Keep the system MCU out of reset
    GPIOPinWrite(sys_reset_port, sys_reset_pin, sys_reset_pin);

    // Do not activate the bootloader firmware update trigger
    GPIOPinWrite(update_trigger_port, update_trigger_pin, update_trigger_pin);

    GPIOPinWrite(led_port, led_pin, led_pin);

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

    MAP_UARTConfigSetExpClk(
        UART0_BASE, system_clock_hz, 500000,
        UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE);

    // Double check what we get back

    uint32_t baud, config;
    UARTConfigGetExpClk(UART0_BASE, 120E6, &baud, &config);

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

err_t init_umbilical_uart() {
    /* Enable the GPIO Peripheral used by the UART */
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    while (!(MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB))) {
    }

    /* Enable UART0 */
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);

    /* Configure GPIO Pins for UART mode */
    MAP_GPIOPinConfigure(GPIO_PB0_U1RX);
    MAP_GPIOPinConfigure(GPIO_PB1_U1TX);
    MAP_GPIOPinTypeUART(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);

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

    MAP_SSIConfigSetExpClk(SSI1_BASE, system_clock_hz, SSI_FRF_MOTO_MODE_0,
                           SSI_MODE_MASTER, (system_clock_hz / 24), 8);
    MAP_SSIEnable(SSI1_BASE);

    /* Flush the Receive FIFO */
    uint32_t getResponseData;
    while (MAP_SSIDataGetNonBlocking(SSI1_BASE, &getResponseData))
        ;

    return 0;
}

err_t init_crc() {
    MAP_SysCtlPeripheralReset(SYSCTL_PERIPH_CCM0);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_CCM0);
    while (!(MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_CCM0))) {
    }

    MAP_CRCConfigSet(CCM0_BASE, (CRC_CFG_IBR | CRC_CFG_OBR | CRC_CFG_RESINV |
                                 CRC_CFG_INIT_1 | CRC_CFG_TYPE_P1EDC6F41 |
                                 CRC_CFG_SIZE_8BIT));

    return 0;
}
