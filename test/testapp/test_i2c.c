/*
 * Copyright 2014, 2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * NXP USBSIO Library: USB serial I/O test application - I2C test code
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <pthread.h>
#endif

#include "lpcusbsio.h"

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/*****************************************************************************
 * Public functions
 ****************************************************************************/

int RunI2CTest(LPC_HANDLE hSIOPort)
{
    int res;
    int err_code = LPCUSBSIO_OK;
    LPC_HANDLE hI2CPort = NULL;

    I2C_PORTCONFIG_T cfgParam;
    I2C_FAST_XFER_T xfer;
    uint8_t rx_buff[1024];
    uint8_t tx_buff[1024];
    uint16_t i;
    uint16_t tx_length;
    uint16_t rx_length;
    uint16_t slaveAddr;


    /*Init the I2C port for standard speed communication */
    cfgParam.ClockRate = I2C_CLOCK_STANDARD_MODE;
    cfgParam.Options = 0;
    printf("Enter the I2C Slave Address (0 - 127):  ");
#ifdef _WIN32
    scanf_s("%hu", &slaveAddr);
#else
    scanf("%hu", &slaveAddr);
#endif

    printf("Enter the number of bytes to transmit over I2C (Max of %d):  ", LPCUSBSIO_GetMaxDataSize(hSIOPort));
#ifdef _WIN32
    scanf_s("%hu", &tx_length);
#else
    scanf("%hu", &tx_length);
#endif

    if (tx_length > 0) {
        printf("Enter the data bytes to be transmitted:  ");
        for (i = 0; i < tx_length; i++) {
#ifdef _WIN32
            scanf_s("%hhu", &tx_buff[i]);
#else
            scanf("%hhu", &tx_buff[i]);
#endif
        }
    }

    printf("Enter the number of bytes to receive over I2C (Max of %d):  ", LPCUSBSIO_GetMaxDataSize(hSIOPort));
#ifdef _WIN32
    scanf_s("%hu", &rx_length);
#else
    scanf("%hu", &rx_length);
#endif

    /* open I2C0 port */
    hI2CPort = I2C_Open(hSIOPort, &cfgParam, 0);
    if (hI2CPort != NULL) {
        xfer.options = 0;
        xfer.txSz = tx_length;
        xfer.rxSz = rx_length;
        xfer.txBuff = &tx_buff[0];
        xfer.rxBuff = &rx_buff[0];
        xfer.slaveAddr = slaveAddr;
        res = I2C_FastXfer(hI2CPort, &xfer);
        if (res > 0) {
            if (rx_length > 0) {
                printf("I2C received %d number of bytes: \r\n", res);
                /* Print Received data */
                for (i = 0; i < res; i++) {
                    printf("%02X  ", rx_buff[i]);
                    if (!((i + 1) & 0x0F)) {
                        printf("\r\n");
                    }
                }
                printf("\r\n");
            }
            else {
                printf("I2C transmitted %d number of bytes: \r\n", res);
            }

        }
        else {
            printf("I2C transfer error:  ");
            printf("%ls\r\n", LPCUSBSIO_Error(hSIOPort));
            err_code = res;
            if (res == LPCUSBSIO_ERR_TIMEOUT) {
                /* issue reset to break loops */
                I2C_Reset(hI2CPort);
            }
        }
        I2C_Close(hI2CPort);
    }
    else {
        printf("Unable to open I2C port.\r\n");
        printf("%ls", LPCUSBSIO_Error(hSIOPort));
        printf("\r\n");
        err_code = LPCUSBSIO_GetLastError();
    }

    return err_code;
}


int RunI2CRWTest(LPC_HANDLE hSIOPort)
{
    int res;
    int err_code = LPCUSBSIO_OK;
    LPC_HANDLE hI2CPort = NULL;

    I2C_PORTCONFIG_T cfgParam;
    uint8_t rx_buff[1024];
    uint8_t tx_buff[1024];
    uint16_t i;
    uint16_t tx_length;
    uint16_t rx_length;
    uint16_t slaveAddr;


    /*Init the I2C port for standard speed communication */
    cfgParam.ClockRate = I2C_CLOCK_STANDARD_MODE;
    cfgParam.Options = 0;
    printf("Enter the I2C Slave Address (0 - 127):  ");
#ifdef _WIN32
    scanf_s("%hu", &slaveAddr);
#else
    scanf("%hu", &slaveAddr);
#endif

    printf("Enter the number of bytes to transmit over I2C (Max of %d):  ", LPCUSBSIO_GetMaxDataSize(hSIOPort));
#ifdef _WIN32
    scanf_s("%hu", &tx_length);
#else
    scanf("%hu", &tx_length);
#endif

    if (tx_length > 0) {
        printf("Enter the data bytes to be transmitted:  ");
        for (i = 0; i < tx_length; i++) {
#ifdef _WIN32
            scanf_s("%hhu", &tx_buff[i]);
#else
            scanf("%hhu", &tx_buff[i]);
#endif
        }
    }

    printf("Enter the number of bytes to receive over I2C (Max of %d):  ", LPCUSBSIO_GetMaxDataSize(hSIOPort));
#ifdef _WIN32
    scanf_s("%hu", &rx_length);
#else
    scanf("%hu", &rx_length);
#endif
    /* open I2C0 port */
    hI2CPort = I2C_Open(hSIOPort, &cfgParam, 0);
    if (hI2CPort != NULL) {
        if (tx_length > 0) {
            res = I2C_DeviceWrite(hI2CPort, (uint8_t)slaveAddr, &tx_buff[0], tx_length, (I2C_TRANSFER_OPTIONS_START_BIT | I2C_TRANSFER_OPTIONS_STOP_BIT | I2C_TRANSFER_OPTIONS_BREAK_ON_NACK));
            if (res > 0) {
                printf("I2C transmitted %d number of bytes: \r\n", res);
            }
            else {
                printf("I2C write error:  ");
                printf("%ls\r\n", LPCUSBSIO_Error(hSIOPort));
                if (res == LPCUSBSIO_ERR_TIMEOUT) {
                    /* issue reset to break loops */
                    I2C_Reset(hI2CPort);
                }
                I2C_Close(hI2CPort);
                return res;
            }
        }
        if (rx_length > 0) {
            res = I2C_DeviceRead(hI2CPort, (uint8_t)slaveAddr, &rx_buff[0], rx_length, (I2C_TRANSFER_OPTIONS_START_BIT | I2C_TRANSFER_OPTIONS_STOP_BIT | I2C_TRANSFER_OPTIONS_NACK_LAST_BYTE));
            if (res > 0) {
                printf("I2C received %d number of bytes: \r\n", res);
                /* Print Received data */
                for (i = 0; i < res; i++) {
                    printf("%02X  ", rx_buff[i]);
                    if (!((i + 1) & 0x0F)) {
                        printf("\r\n");
                    }
                }
                printf("\r\n");
            }
            else {
                printf("I2C read error:  ");
                printf("%ls\r\n", LPCUSBSIO_Error(hSIOPort));
                err_code = res;
                if (res == LPCUSBSIO_ERR_TIMEOUT) {
                    /* issue reset to break loops */
                    I2C_Reset(hI2CPort);
                }
            }
        }
        I2C_Close(hI2CPort);
    }
    else {
        printf("Unable to open I2C port.\r\n");
        printf("%ls", LPCUSBSIO_Error(hSIOPort));
        printf("\r\n");
        err_code = LPCUSBSIO_GetLastError();
    }
    return err_code;
}

int RunI2CDataTest(LPC_HANDLE hSIOPort)
{
    int res;
    int err_code = LPCUSBSIO_OK;
    LPC_HANDLE hI2CPort = NULL;

    I2C_PORTCONFIG_T cfgParam;
    uint8_t rx_buff[1024];
    uint8_t tx_buff[1024];
    uint16_t i;
    uint16_t xfer_length;
    uint16_t address;
    uint16_t slaveAddr;
    uint16_t xfer_type;
    static uint8_t seed = 1;


    /*Init the I2C port for standard speed communication */
    cfgParam.ClockRate = I2C_CLOCK_FAST_MODE_PLUS;
    cfgParam.Options = 0;
    printf("Enter the I2C Slave Address (0 - 127):  ");
#ifdef _WIN32
    scanf_s("%hu", &slaveAddr);
#else
    scanf("%hu", &slaveAddr);
#endif

    printf("Do you want to Write or Read Press 1 for write and 2 for Read:  ");
#ifdef _WIN32
    scanf_s("%hu", &xfer_type);
#else
    scanf("%hu", &xfer_type);
#endif

    printf("Enter the number of bytes to transfer over I2C (Max of %d):  ", LPCUSBSIO_GetMaxDataSize(hSIOPort) - 2);
#ifdef _WIN32
    scanf_s("%hu", &xfer_length);
#else
    scanf("%hu", &xfer_length);
#endif

    printf("Enter the EEPROM address to write or read:  ");
#ifdef _WIN32
    scanf_s("%hu", &address);
#else
    scanf("%hu", &address);
#endif

    /* open I2C0 port */
    hI2CPort = I2C_Open(hSIOPort, &cfgParam, 0);

    if (hI2CPort != NULL) {
        switch (xfer_type) {
        case 1:
            tx_buff[0] = (uint8_t) address;
            tx_buff[1] = (uint8_t)(address >> 8);
            for (i = 0; i < xfer_length; i++) {
                tx_buff[i + 2] = (seed + i) & 0xFF;
            }
            seed++;
            res = I2C_DeviceWrite(hI2CPort, (uint8_t)slaveAddr, &tx_buff[0], xfer_length + 2, (I2C_TRANSFER_OPTIONS_START_BIT | I2C_TRANSFER_OPTIONS_STOP_BIT | I2C_TRANSFER_OPTIONS_BREAK_ON_NACK));
            if (res > 0) {
                printf("I2C transmitted %d number of bytes: \r\n", res);
            }
            else {
                printf("I2C write error:  ");
                printf("%ls\r\n", LPCUSBSIO_Error(hSIOPort));
                if (res == LPCUSBSIO_ERR_TIMEOUT) {
                    /* issue reset to break loops */
                    I2C_Reset(hI2CPort);
                }
            }
            break;
        case 2:
            res = I2C_DeviceWrite(hI2CPort, (uint8_t)slaveAddr, (uint8_t *)&address, 2, (I2C_TRANSFER_OPTIONS_START_BIT | I2C_TRANSFER_OPTIONS_STOP_BIT | I2C_TRANSFER_OPTIONS_BREAK_ON_NACK));
            if (res <= 0) {
                printf("I2C write error:  ");
                printf("%ls\r\n", LPCUSBSIO_Error(hSIOPort));
                if (res == LPCUSBSIO_ERR_TIMEOUT) {
                    /* issue reset to break loops */
                    I2C_Reset(hI2CPort);
                }
                I2C_Close(hI2CPort);
                return res;
            }
            res = I2C_DeviceRead(hI2CPort, (uint8_t)slaveAddr, &rx_buff[0], xfer_length, (I2C_TRANSFER_OPTIONS_START_BIT | I2C_TRANSFER_OPTIONS_STOP_BIT | I2C_TRANSFER_OPTIONS_NACK_LAST_BYTE));
            if (res > 0) {
                printf("I2C received %d number of data bytes: \r\n", res);
                /* Print Received data */
                for (i = 0; i < res; i++) {
                    printf("%02X  ", rx_buff[i]);
                    if (!((i + 1) & 0x0F)) {
                        printf("\r\n");
                    }
                }
                printf("\r\n");
            }
            else {
                printf("I2C read error:  ");
                printf("%ls\r\n", LPCUSBSIO_Error(hSIOPort));
                err_code = res;
                if (res == LPCUSBSIO_ERR_TIMEOUT) {
                    /* issue reset to break loops */
                    I2C_Reset(hI2CPort);
                }
            }
            break;
        default:
            printf("Invalid transfer option \r\n");
            break;
        }
        I2C_Close(hI2CPort);
    }
    else {
        printf("Unable to open I2C port.\r\n");
        printf("%ls", LPCUSBSIO_Error(hSIOPort));
        printf("\r\n");
        err_code = LPCUSBSIO_GetLastError();
    }
    return err_code;
}

