#include "flash.cpp"

/* DriverLib Includes */
#include "ti/devices/msp432e4/driverlib/driverlib.h"

/* Standard Includes */
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

err_t configureFlashSpi() {
    /* The SSI1 peripheral must be disabled, reset and re enabled for use
     * Wait till the Peripheral ready is not asserted */
    MAP_SysCtlPeripheralDisable(SYSCTL_PERIPH_SSI1);
    MAP_SysCtlPeripheralReset(SYSCTL_PERIPH_SSI1);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI1);
    while (!(MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_SSI1))) {
    }

    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    while (!(MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOE))) {
    }

    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOQ);
    while (!(MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOQ))) {
    }

    MAP_GPIOPinConfigure(GPIO_PB5_SSI1CLK);
    MAP_GPIOPinConfigure(
        GPIO_PB4_SSI1FSS);  // TODO(akremor): Might need to unconfigure
    MAP_GPIOPinConfigure(GPIO_PE4_SSI1XDAT0);
    MAP_GPIOPinConfigure(GPIO_PE5_SSI1XDAT1);

    /* Configure the GPIO settings for the SSI pins.  This function also gives
     * control of these pins to the SSI hardware.  Consult the data sheet to
     * see which functions are allocated per pin */
    MAP_GPIOPinTypeSSI(GPIO_PORTB_BASE, GPIO_PIN_5 | GPIO_PIN_4);
    MAP_GPIOPinTypeSSI(GPIO_PORTE_BASE, GPIO_PIN_5 | GPIO_PIN_4);

    /* No driverlib API to only set the GPIO high drive setting, so we use
     * bare metal */
    GPIOB->DR8R |= (GPIO_PIN_5 | GPIO_PIN_4);
    GPIOE->DR8R |= (GPIO_PIN_5 | GPIO_PIN_4);

    /* Configure and enable the SSI port for SPI master mode.  Use SSI1,
     * system clock supply, idle clock level low and active low clock in
     * freescale SPI mode, master mode, 60MHz SSI frequency, and 8-bit data.
     * For SPI mode, you can set the polarity of the SSI clock when the SSI
     * unit is idle.  You can also configure what clock edge you want to
     * capture data on.  Please reference the datasheet for more information on
     * the different SPI modes */
    MAP_SSIConfigSetExpClk(SSI1_BASE, 120E6, SSI_FRF_MOTO_MODE_0,
                           SSI_MODE_MASTER, (120E6 / 2), 8);

    /* No driverlib API to enable the SSI High Speed Clock, so we use
     * bare metal */
    SSI1->CR1 |= SSI_CR1_HSCLKEN;
    MAP_SSIEnable(SSI1_BASE);

    /* Read any residual data from the SSI port.  This makes sure the receive
     * FIFOs are empty, so we don't read any unwanted junk.  This is done here
     * because the SPI SSI mode is full-duplex, which allows you to send and
     * receive at the same time.  The SSIDataGetNonBlocking function returns
     * "true" when data was returned, and "false" when no data was returned.
     * The "non-blocking" function checks if there is any data in the receive
     * FIFO and does not "hang" if there isn't */
    while (MAP_SSIDataGetNonBlocking(SSI1_BASE, &dummyRead[0])) {
    }

    flash4ByteMode();

    return 0;
}

err_t flash4ByteMode() {
    // TODO Configure the gpio in hal
    flashSelectChip();

    MAP_SSIDataPut(SSI1_BASE, EN4B);

    flashDeselectChip();
}

err_t flashSelectChip() {
    GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_2, 0x00);

    return 0;
}

err_t flashDeselectChip() {
    GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_2, GPIO_PIN_2);

    return 0;
}

err_t flashRead(uint32_t base_address, uint32_t* read_buffer,
                uint32_t buffer_length) {
    // TODO(akremor): Place timeouts on this function
    flashSelectChip();

    uint8_t dummy_buffer[1];

    MAP_SSIDataPut(SSI1_BASE, READ4B);
    MAP_SSIDataGet(SSI1_BASE, dummy_buffer);

    MAP_SSIDataPut(SSI1_BASE, base_address >> 24 & 0xFF);
    MAP_SSIDataGet(SSI1_BASE, dummy_buffer);

    MAP_SSIDataPut(SSI1_BASE, base_address >> 16 & 0xFF);
    MAP_SSIDataGet(SSI1_BASE, dummy_buffer);

    MAP_SSIDataPut(SSI1_BASE, base_address >> 8 & 0xFF);
    MAP_SSIDataGet(SSI1_BASE, dummy_buffer);

    MAP_SSIDataPut(SSI1_BASE, base_address >> 0 & 0xFF);
    MAP_SSIDataGet(SSI1_BASE, dummy_buffer);

    for (int i = 0; i < buffer_length; i++) {
        // Needed to clock the RX line

        uint8_t temp_buffer[4];

        for (int j = 0; j < 4; j++) {
            MAP_SSIDataPut(SSI1_BASE, DUMMY_BYTE);
            MAP_SSIDataGet(SSI1_BASE, temp_buffer[j]);
        }

        read_buffer[i] = temp_buffer[0] << 24 | temp_buffer[1] << 16 |
                         temp_buffer[2] << 8 | temp_buffer[3] << 0;
    }

    flashDeselectChip();

    return 0;
}
