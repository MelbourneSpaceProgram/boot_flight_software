#include <data_types.h>
#include <source/drivers/flash.h>
#include <source/drivers/hal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ti/devices/msp432e4/driverlib/driverlib.h>

void clearSpiFifo();
err_t flashWrite32Bit(uint32_t* data, uint32_t length);

err_t flashWrite32Bit(uint32_t* data, uint32_t length) {
    uint32_t dummy_buffer[1];

    for (uint32_t i = 0; i < length; i++) {
        clearSpiFifo();
        MAP_SSIDataPut(SSI1_BASE, data[i] >> 24 & 0xFF);
        MAP_SSIDataPut(SSI1_BASE, data[i] >> 16 & 0xFF);
        MAP_SSIDataPut(SSI1_BASE, data[i] >> 8 & 0xFF);
        MAP_SSIDataPut(SSI1_BASE, data[i] >> 0 & 0xFF);

        while (SSIBusy(SSI1_BASE))
            ;
        clearSpiFifo();
    }

    return 0;
}
err_t configureFlashSpi() {
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI1);
    while (!(MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_SSI1))) {
    }

    while (!(MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_SSI1))) {
    }

    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    while (!(MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOE))) {
    }

    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOQ);
    while (!(MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOQ))) {
    }

    GPIOPinTypeGPIOOutput(GPIO_PORTQ_BASE, GPIO_PIN_2);
    flashDeselectChip();

    MAP_GPIOPinConfigure(GPIO_PB5_SSI1CLK);
    MAP_GPIOPinConfigure(GPIO_PB4_SSI1FSS);
    MAP_GPIOPinConfigure(GPIO_PE4_SSI1XDAT0);
    MAP_GPIOPinConfigure(GPIO_PE5_SSI1XDAT1);

    MAP_GPIOPinTypeSSI(GPIO_PORTB_BASE, GPIO_PIN_5 | GPIO_PIN_4);
    MAP_GPIOPinTypeSSI(GPIO_PORTE_BASE, GPIO_PIN_5 | GPIO_PIN_4);

    GPIOB->DR8R |= (GPIO_PIN_5 | GPIO_PIN_4);
    GPIOE->DR8R |= (GPIO_PIN_5 | GPIO_PIN_4);

    MAP_SSIConfigSetExpClk(SSI1_BASE, system_clock_hz, SSI_FRF_MOTO_MODE_0,
                           SSI_MODE_MASTER, system_clock_hz / 100, 8);

    SSI1->CR1 |= SSI_CR1_HSCLKEN;
    MAP_SSIEnable(SSI1_BASE);

    flash4ByteMode();

    clearSpiFifo();

    return 0;
}

err_t flash4ByteMode() {
    flashSelectChip();

    SSIDataPut(SSI1_BASE, EN4B);
    while (SSIBusy(SSI1_BASE))
        ;

    flashDeselectChip();

    return 0;
}

err_t flashSelectChip() {
    GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_2, 0x00);

    return 0;
}

err_t flashDeselectChip() {
    GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_2, GPIO_PIN_2);

    return 0;
}

err_t flashRead(uint32_t base_address, byte* read_buffer,
                uint32_t buffer_length) {
    flashSelectChip();

    uint32_t dummy_buffer[1];

    MAP_SSIDataPut(SSI1_BASE, READ4B);
    while (SSIBusy(SSI1_BASE))
        ;
    clearSpiFifo();

    flashWrite32Bit(&base_address, 1);

    for (uint16_t i = 0; i < buffer_length; i++) {
        // Needed to clock the RX line
        MAP_SSIDataPut(SSI1_BASE, DUMMY_BYTE);
        MAP_SSIDataGet(SSI1_BASE, (uint32_t*)&read_buffer[i]);
    }

    flashDeselectChip();

    return 0;
}

err_t flashWriteEnable() {
    flashSelectChip();

    clearSpiFifo();
    MAP_SSIDataPut(SSI1_BASE, WREN);
    clearSpiFifo();

    flashDeselectChip();

    return 0;
}

// Ensure this isn't used to write outside a page
err_t flashWrite(uint32_t base_address, byte* write_buffer,
                 uint32_t buffer_length) {
    flashSelectChip();

    byte sector_buffer[kSectorLength];
    // This rounds to the base page address
    uint32_t sector_address = base_address & 0xFFFFFF00;  // Last 8 bits zeroed

    if (flashRead(sector_address, sector_buffer, kSectorLength) != 0) {
        // TODO(dingbenjamin): Error code
        flashDeselectChip();
        return 1;
    }

    // Now merge in the new data to be written
    memcpy(sector_buffer + (base_address - sector_address), write_buffer,
           buffer_length);

    flashWriteEnable();

    // Erase the sector
    clearSpiFifo();
    MAP_SSIDataPut(SSI1_BASE, SE);
    MAP_SSIDataPut(SSI1_BASE, sector_address >> 24 & 0xFF);
    MAP_SSIDataPut(SSI1_BASE, sector_address >> 16 & 0xFF);
    MAP_SSIDataPut(SSI1_BASE, sector_address >> 8 & 0xFF);
    MAP_SSIDataPut(SSI1_BASE, sector_address >> 0 & 0xFF);
    clearSpiFifo();

    clearSpiFifo();
    MAP_SSIDataPut(SSI1_BASE, PP4);
    MAP_SSIDataPut(SSI1_BASE, sector_address >> 24 & 0xFF);
    MAP_SSIDataPut(SSI1_BASE, sector_address >> 16 & 0xFF);
    MAP_SSIDataPut(SSI1_BASE, sector_address >> 8 & 0xFF);
    MAP_SSIDataPut(SSI1_BASE, sector_address >> 0 & 0xFF);
    clearSpiFifo();

    for (uint32_t i = 0; i < kPageLength; i++) {
        MAP_SSIDataPut(SSI1_BASE, sector_buffer[i]);
        while (SSIBusy(SSI1_BASE))
            ;
    }

    flashDeselectChip();

    return 0;
}

void clearSpiFifo() {
    uint32_t dummyRead[1];
    while (MAP_SSIDataGetNonBlocking(SSI1_BASE, &dummyRead[0])) {
    }
}
