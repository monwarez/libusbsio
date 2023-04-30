/*
 * Copyright 2014, 2021-2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * NXP USBSIO Library: USB serial I/O test application
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

extern int RunI2CTest(LPC_HANDLE hSIOPort);
extern int RunSPITest(LPC_HANDLE hSIOPort);
extern int RunGPIOTest(LPC_HANDLE hSIOPort);
extern int RunI2CRWTest(LPC_HANDLE hSIOPort);
extern int RunI2CDataTest(LPC_HANDLE hSIOPort);

/*****************************************************************************
 * Private functions
 ****************************************************************************/

static void print_menu(void)
{
    printf("\n");
    printf("Press '1' to run I2C Transfer test \n");
    printf("Press '2' to run SPI Transfer test \n");
    printf("Press '3' to run GPIO test \n");
    printf("Press '4' to run I2C Read Write test \n");
    printf("Press '5' to run I2C Large data transfer test \n");
    printf("Press 'q' to exit \n");
}

/* use our own wchar_t puts routine to avoid mixing of char/wchar_t in stdout */
static void wputs(wchar_t* p)
{
    wchar_t c;

    if(p)
    {
        while((c=*p++) != 0)
            putc(c >> 8 ? '?' : c, stdout);
    }
}

/*****************************************************************************
 * Public functions
 ****************************************************************************/

int main(int argc, char *argv[])
{
    LPC_HANDLE hSIOPort;
    int res;
    char ch;

    if((res = LPCUSBSIO_GetNumPorts(LPCUSBSIO_VID, LPCUSBSIO_PID)) > 0)
    {
        printf("Total LPCLink2 devices: %d\n", res);
    }
    else if((res = LPCUSBSIO_GetNumPorts(LPCUSBSIO_VID, MCULINKSIO_PID)) > 0)
    {
        printf("Total MCULink devices: %d\n", res);
    }
    else
    {
        printf("No USBSIO bridge device found\n");
    }

    if (res > 0) 
    {
        HIDAPI_DEVICE_INFO_T info;
        memset(&info, 0, sizeof(info));

        printf("Using device #0 ");
        if (LPCUSBSIO_GetDeviceInfo(0, &info) == LPCUSBSIO_OK)
        {
            wputs(info.manufacturer_string); 
            printf(" ");
            wputs(info.product_string);
            printf(" ");
            wputs(info.serial_number);
            printf("\n"); 
        }
        else
        {
            printf(" (no HID_API information)\n");
        }

        /*open device at index 0 */
        hSIOPort = LPCUSBSIO_Open(0);

        if(!hSIOPort)
        {
            /* This couold be an issue of /dev/hidrawX access rights */
            printf("Could not open HID device (check access rights)\n");
            return 1;
        }

        printf("Device version: %s \n ", LPCUSBSIO_GetVersion(hSIOPort));

        printf("\nTestApp options menu:  \n");
        print_menu();

        while ((ch = getchar()) != 'q') 
        {
            switch (ch) 
            {
            case '1':
                res = RunI2CTest(hSIOPort);
                break;
            case '2':
                res = RunSPITest(hSIOPort);
                break;
            case '3':
                res = RunGPIOTest(hSIOPort);
                break;
            case '4':
                res = RunI2CRWTest(hSIOPort);
                break;
            case '5':
                res = RunI2CDataTest(hSIOPort);
                break;
            default:
                continue;
            }
            if (res == LPCUSBSIO_ERR_HID_LIB) 
            {
                printf("HID Library Error, exiting...\n");
                LPCUSBSIO_Close(hSIOPort);
                return 1;
            }

            print_menu();
        }
        LPCUSBSIO_Close(hSIOPort);
    }
    else 
    {
        printf("Error: No free ports. \n");
    }
    printf("Exiting \n");
    return 0;
}
