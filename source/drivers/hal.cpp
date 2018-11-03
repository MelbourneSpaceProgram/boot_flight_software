#include <source/drivers/flash.h>
#include <source/drivers/hal.h>
#include <ti/devices/msp432e4/driverlib/driverlib.h>

extern "C" {
volatile bool hibernate_on_boot = true;
void HIBERNATE_IRQHandler(void) {
    uint32_t getIntStatus;

    getIntStatus = MAP_HibernateIntStatus(true);

    if (getIntStatus == HIBERNATE_INT_RTC_MATCH_0) {
        MAP_HibernateIntClear(HIBERNATE_INT_RTC_MATCH_0);
        hibernate_on_boot = false;
    }
}
}

err_t should_hibernate_on_boot(bool* should_hibernate) {
    *should_hibernate = hibernate_on_boot;
    return 0;
}

err_t init_clock() {
    MAP_SysCtlClockFreqSet(SYSCTL_OSC_INT | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480,
                           system_clock_hz);
    return 0;
}

err_t init_gpio() {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOC)) {
    }

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOQ);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOQ)) {
    }

    GPIOPinTypeGPIOOutput(GPIO_PORTC_BASE, sys_reset_pin | update_trigger_pin |
                                               low_power_trigger_pin);
    GPIOPinTypeGPIOOutput(led_port, led_pin);

    // Keep the system MCU out of reset
    GPIOPinWrite(sys_reset_port, sys_reset_pin, sys_reset_pin);

    // No low power mode yet
    GPIOPinWrite(sys_reset_port, low_power_trigger_pin, 0x00);

    // Do not activate the bootloader firmware update trigger
    GPIOPinWrite(update_trigger_port, update_trigger_pin, update_trigger_pin);

    GPIOPinWrite(led_port, led_pin, led_pin);

    return 0;
}

err_t init_hibernate() {
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_HIBERNATE);
    while (!(MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_HIBERNATE))) {
    }

    if (!MAP_HibernateIsActive()) {
        HibernateClockConfig(HIBERNATE_OSC_LFIOSC);
        MAP_HibernateEnableExpClk(system_clock_hz);

        MAP_HibernateWakeSet(HIBERNATE_WAKE_RTC);
        MAP_HibernateRTCEnable();
        MAP_HibernateRTCSet(0);

        HIB->CTL |= HIB_CTL_VDD3ON;
    }

    MAP_HibernateIntEnable(HIBERNATE_INT_RTC_MATCH_0);
    MAP_IntEnable(INT_HIBERNATE);

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
    UARTConfigGetExpClk(UART0_BASE, system_clock_hz, &baud, &config);

    return 0;
}

err_t init_lithium_uart() {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    while (!(SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA))) {
    }

    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART2);

    GPIOPinConfigure(GPIO_PA6_U2RX);
    GPIOPinConfigure(GPIO_PA7_U2TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_6 | GPIO_PIN_7);

    UARTConfigSetExpClk(
        UART2_BASE, system_clock_hz, 9600,
        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

    // Enable the UART interrupt.
    IntEnable(INT_UART2);
    UARTIntEnable(UART2_BASE, UART_INT_RX | UART_INT_RT);
    UARTEnable(UART2_BASE);

    UARTFIFOLevelSet(UART2_BASE, UART_FIFO_TX1_8, UART_FIFO_RX1_8);

    return 0;
}

err_t init_umbilical_uart() {
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    while (!(MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB))) {
    }

    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);

    MAP_GPIOPinConfigure(GPIO_PB0_U1RX);
    MAP_GPIOPinConfigure(GPIO_PB1_U1TX);
    MAP_GPIOPinTypeUART(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    MAP_UARTConfigSetExpClk(
        UART1_BASE, system_clock_hz, 9600,
        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

    // Enable the UART interrupt.
    MAP_IntEnable(INT_UART1);
    UARTIntEnable(UART1_BASE, UART_INT_RX | UART_INT_RT);
    MAP_UARTEnable(UART1_BASE);

    UARTFIFOLevelSet(UART1_BASE, UART_FIFO_TX1_8, UART_FIFO_RX1_8);

    return 0;
}

err_t init_flash_spi() {
    configureFlashSpi();

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
