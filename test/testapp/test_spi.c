/*
 * Copyright 2014, 2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * NXP USBSIO Library: USB serial I/O test application - SPI test code
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

int RunSPITest(LPC_HANDLE hSIOPort)
{
    int res;
    int err_code = LPCUSBSIO_OK;
    LPC_HANDLE hSPIPort = NULL;

    HID_SPI_PORTCONFIG_T cfgParam;
    SPI_XFER_T xfer;
    uint8_t rx_buff[1024];
    uint8_t tx_buff[1024];
    uint16_t i;
    uint16_t length;
    uint16_t ssel_port;
    uint16_t ssel_pin;

    /*Init the SPI port for 1MHz communication */
    cfgParam.busSpeed = 1000000;
    cfgParam.Options = HID_SPI_CONFIG_OPTION_DATA_SIZE_8 | HID_SPI_CONFIG_OPTION_POL_0 | HID_SPI_CONFIG_OPTION_PHA_0;
    printf("Enter the GPIO port number used for the SPI device select:  ");
#ifdef _WIN32
    scanf_s("%hu", &ssel_port);
#else
    scanf("%hu", &ssel_port);
#endif
    printf("Enter the GPIO pin number used for the SPI device select:  ");
#ifdef _WIN32
    scanf_s("%hu", &ssel_pin);
#else
    scanf("%hu", &ssel_pin);
#endif
    printf("Enter Number of bytes for SPI transfer (Max of %d):  ", LPCUSBSIO_GetMaxDataSize(hSIOPort));
    #ifdef _WIN32
    scanf_s("%hu", &length);
    #else
    scanf("%hu", &length);
    #endif
    printf("Enter the data bytes to be transmitted \r\n");
    for (i = 0; i < length; i++) {
#ifdef _WIN32
        scanf_s("%hhu", &tx_buff[i]);
#else
        scanf("%hhu", &tx_buff[i]);
#endif
    }
    /* open SPI port 0 */
    hSPIPort = SPI_Open(hSIOPort, &cfgParam, 0);

    if (hSPIPort != NULL) {

        xfer.options = 0;
        xfer.length = length;
        xfer.txBuff = &tx_buff[0];
        xfer.rxBuff = &rx_buff[0];
        xfer.device = (uint8_t)LPCUSBSIO_GEN_SPI_DEVICE_NUM(ssel_port, ssel_pin);
        res = SPI_Transfer(hSPIPort, &xfer);
        if (res > 0) {
            printf("SPI received %d number of bytes: \r\n", res);
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
            printf("SPI transfer error:  ");
            printf("%ls\r\n", LPCUSBSIO_Error(hSIOPort));
            err_code = res;
            if (res == LPCUSBSIO_ERR_TIMEOUT) {
                /* issue reset to break loops */
                SPI_Reset(hSPIPort);
            }
        }
        SPI_Close(hSPIPort);
    }
    else {
        printf("Unable to open SPI port.\r\n");
        printf("%ls", LPCUSBSIO_Error(hSIOPort));
        printf("\r\n");
        err_code = LPCUSBSIO_GetLastError();
    }

    return err_code;

}

