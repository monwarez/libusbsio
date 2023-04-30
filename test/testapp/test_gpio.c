/*
 * Copyright 2014, 2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * NXP USBSIO Library: USB serial I/O test application - GPIO test code
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

int RunGPIOTest(LPC_HANDLE hSIOPort)
{
    int res;
    uint16_t port;
    uint16_t pin;
    uint32_t value;

    printf("IOConfig for GPIO - Enter IO port, pin and config value: \r\n");
#ifdef _WIN32
    scanf_s("%hu%hu%u", &port, &pin, &value);
#else
    scanf("%hu%hu%u", &port, &pin, &value);
#endif
    /* Set Pin as GPIO */
    res = GPIO_ConfigIOPin(hSIOPort, (uint8_t)port, (uint8_t)pin, value);
    if (res < 0) {
        printf("GPIO IOConfig Error:  ");
        printf("%ls\r\n", LPCUSBSIO_Error(hSIOPort));
        return res;
    }

    printf("Enter GPIO port and pin number: \r\n");
#ifdef _WIN32
    scanf_s("%hu%hu", &port, &pin);
#else
    scanf("%hu%hu", &port, &pin);
#endif
    value = 0;
    /*Set pin as output port */
    res = GPIO_SetPortOutDir(hSIOPort, (uint8_t)port, (uint8_t)pin);
    if (res >= 0) {
        printf(" GPIO Output direction set \r\n");
    }
    else {
        printf("GPIO Set PortDir Error:  ");
        printf("%ls\r\n", LPCUSBSIO_Error(hSIOPort));
        return res;
    }

    GPIO_ReadPort(hSIOPort, (uint8_t)port, &value);
    if (res > 0) {
        printf("Port Value before update is %x\r\n", value);
    }
    else {
        printf("GPIO Read Port Error:  ");
        printf("%ls\r\n", LPCUSBSIO_Error(hSIOPort));
        return res;
    }

    GPIO_TogglePin(hSIOPort, (uint8_t)port, (uint8_t)pin);
    if (res >= 0) {
        printf(" GPIOPin Toggled \r\n");
    }
    else {
        printf("GPIO Pin Toggle Error:  ");
        printf("%ls\r\n", LPCUSBSIO_Error(hSIOPort));
        return res;
    }

    GPIO_ReadPort(hSIOPort, (uint8_t)port, &value);
    if (res > 0) {
        printf("Port Value before update is %x\r\n", value);
    }
    else {
        printf("GPIO Read Port Error:  ");
        printf("%ls\r\n", LPCUSBSIO_Error(hSIOPort));
        return res;
    }
    return LPCUSBSIO_OK;
}
