/*
 * Copyright 2014, 2021-2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * NXP USBSIO Library to control SPI, I2C and GPIO bus over USB
 */

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#if defined(__FreeBSD__)
#include "hidapi_mock.h"
#else
#include "hidapi.h"
#endif
#include "lpcusbsio.h"
#include "lpcusbsio_protocol.h"
#ifdef _WIN32
#include <Windows.h>
#else
#include <pthread.h>
#endif

/* enable debug logging */
#define SIO_DEBUG 0

#ifdef _WIN32
#define SIO_DEBUG_LOG "C:\\Temp\\libusbsio.log"
#else
#define SIO_DEBUG_LOG "/tmp/libusbsio.log"
#endif

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/
/* On windows the HID report data starts after first byte since this byte is used for reportID. */
#define HID_REPORT_DATA_OFFSET		1

#define NUM_LIB_ERR_STRINGS         6
#define NUM_FW_ERR_STRINGS          6
#define NUM_BRIDGE_ERR_STRINGS      4
#define MAX_FWVER_STRLEN			60

#define MAX_I2C_PORTS				8
#define MAX_SPI_PORTS				8

typedef struct LPCUSBSIO_Port_Ctrl {
    LPC_HANDLE hUsbSio;
    uint8_t portNum;
} LPCUSBSIO_PortCtrl_t;

typedef struct LPCUSBSIO_Ctrl {

    struct hid_device_info *hidInfo;
    hid_device *hidDev;
    uint8_t peripheralId[8];
    uint8_t transId;
    uint8_t maxI2CPorts;
    uint8_t maxSPIPorts;
    uint8_t maxGPIOPorts;
    uint32_t maxDataSize;
    uint32_t fwVersion;
    char fwBuild[MAX_FWVER_STRLEN];
    uint8_t outPacket[HID_SIO_PACKET_SZ + 1];
    uint8_t inPacket[HID_SIO_PACKET_SZ + 1];

    LPCUSBSIO_PortCtrl_t i2cPorts[MAX_I2C_PORTS];
    LPCUSBSIO_PortCtrl_t spiPorts[MAX_SPI_PORTS];
#ifdef _WIN32
    HANDLE sioMutex;
#else
    pthread_mutex_t sioMutex;
#endif

    struct LPCUSBSIO_Ctrl *next;

} LPCUSBSIO_Ctrl_t;

struct LPCSIO_Ctrl {
    struct hid_device_info *devInfoList;

    LPCUSBSIO_Ctrl_t *devList;
};


#ifdef _DEBUG
#define LIB_VERSION_DBG "DEBUG "
#else
#define LIB_VERSION_DBG ""
#endif

static const char *g_LibVersion = "NXP LIBUSBSIO v2.1c " LIB_VERSION_DBG "(" __DATE__ " " __TIME__ ")";
static const char *g_fwInitVer = "FW Ver Unavailable";
static char g_Version[128];

static struct LPCSIO_Ctrl g_Ctrl = {0, };
static int32_t g_lastError = LPCUSBSIO_OK;

static const wchar_t *g_LibErrMsgs[NUM_LIB_ERR_STRINGS] = {
    L"No errors are recorded.",
    L"HID library error.",							/* LPCUSBSIO_ERR_HID_LIB */
    L"Handle passed to the function is invalid.",	/* LPCUSBSIO_ERR_BAD_HANDLE */
    L"Mutex Calls failed.",	/* LPCUSBSIO_ERR_SYNCHRONIZATION */
    L"Memory Allocation Error.",	/* LPCUSBSIO_ERR_MEM_ALLOC */
    L"Mutex Creation Error.",	/* LPCUSBSIO_ERR_MUTEX_CREATE */
};

static const wchar_t *g_fwErrMsgs[NUM_FW_ERR_STRINGS] = {
    L"Firmware error.",								/* catch-all firmware error */
    L"Fatal error happened",							/* LPCUSBSIO_ERR_FATAL */
    L"Transfer aborted due to NAK",					/* LPCUSBSIO_ERR_I2C_NAK */
    L"Transfer aborted due to bus error",			/* LPCUSBSIO_ERR_I2C_BUS */
    L"No acknowledgement received from slave address",	/* LPCUSBSIO_ERR_I2C_SLAVE_NAK */
    L"I2C bus arbitration lost to other master",	/* LPCUSBSIO_ERR_I2C_SLAVE_NAK */
};

static const wchar_t *g_BridgeErrMsgs[NUM_BRIDGE_ERR_STRINGS + 1] = {
    L"Transaction timed out.",						/* LPCUSBSIO_ERR_TIMEOUT */
    L"Invalid HID_SIO Request or Request not supported in this version.",	/* LPCUSBSIO_ERR_INVALID_CMD */
    L"Invalid parameters are provided for the given Request.",	/* LPCUSBSIO_ERR_INVALID_PARAM */
    L" Partial transfer completed.",						/* LPCUSBSIO_ERR_PARTIAL_DATA */
    L" Unsupported Error Code",						/* Error code not supported by library */
};

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Private functions
 ****************************************************************************/

static int32_t LibCleanup();

#if SIO_DEBUG>0

#ifdef _DEBUG
#warning LIBUSBSIO Debug logging is enabled
#else
#error LIBUSBSIO Debug logging should not be enabled in non-debug build
#endif

void LogText(const char* text)
{
    static FILE* f = NULL;

    if(!f)
        f = fopen(SIO_DEBUG_LOG, "a");

    if(f)
    {
        fputs(text, f);
        fflush(f);
    }
}

void Log(const char* format, ...)
{
    char buff[1024];

    va_list args;
    va_start(args, format);

    vsnprintf(buff, sizeof(buff), format, args);
    LogText(buff);

    va_end(args);
}
#else
#define LogText(x)
#define Log(x, ...)
#endif

static struct hid_device_info *GetDevAtIndex(uint32_t index)
{
    struct hid_device_info *cur_dev = g_Ctrl.devInfoList;
    int32_t count = 0;

    while (cur_dev) {
        if (count++ == index) {
            break;
        }

        cur_dev = cur_dev->next;
    }
    return cur_dev;
}

static int32_t validHandle(LPCUSBSIO_Ctrl_t *dev)
{
    LPCUSBSIO_Ctrl_t *curDev = g_Ctrl.devList;

    while (dev != curDev) {
        curDev = curDev->next;
    }

    return (curDev == NULL) ? 0 : 1;
}

static int32_t validPortHandle(LPC_HANDLE hPort)
{
    uintptr_t portAdr = (uintptr_t)hPort;
    uintptr_t lowAdr, highAdr;
    LPCUSBSIO_Ctrl_t *curDev = g_Ctrl.devList;
    int32_t ret = 0;

    while (NULL != curDev) {
        lowAdr = (uintptr_t)&curDev->i2cPorts[0];
        highAdr = lowAdr + ((MAX_I2C_PORTS + MAX_SPI_PORTS)* sizeof(LPCUSBSIO_PortCtrl_t));

        if ((portAdr >= lowAdr) && (portAdr <= highAdr)) {
            ret = 1;
            break;
        }
        curDev = curDev->next;
    }

    return ret;
}

static void freeDevice(LPCUSBSIO_Ctrl_t *dev)
{
    LPCUSBSIO_Ctrl_t *curDev = g_Ctrl.devList;

    if (curDev == dev) {
        g_Ctrl.devList = dev->next;
    }
    else {
        while (curDev) {
            if (curDev->next == dev) {
                /* update linked list */
                curDev->next = dev->next;
                break;
            }
            curDev = curDev->next;
        }
    }
    free(dev);

    /* unload HID library if all devices are closed. */
    if (g_Ctrl.devList == NULL) {
        hid_free_enumeration(g_Ctrl.devInfoList);
        g_Ctrl.devInfoList = NULL;

        // potential place to unload HID library
        LibCleanup();
    }
}

static const wchar_t *GetErrorString(int32_t err)
{
    const wchar_t *retStr = g_LibErrMsgs[0];
    int index = abs(err);

    if (index < 0x10) {
        retStr = (index < NUM_LIB_ERR_STRINGS) ? g_LibErrMsgs[index] : g_LibErrMsgs[0];
    }
    else if (index < 0x20) {
        index -= 0x10;
        retStr = (index < NUM_FW_ERR_STRINGS) ? g_fwErrMsgs[index] : g_fwErrMsgs[0];
    }
    else if (index < 0x30) {
        index -= 0x20;
        retStr = (index < NUM_BRIDGE_ERR_STRINGS) ? g_BridgeErrMsgs[index] : g_BridgeErrMsgs[NUM_BRIDGE_ERR_STRINGS];
    }

    return retStr;
}

static int32_t ConvertResp(int32_t res)
{
    int ret;

    if (res == HID_SIO_RES_OK) {
        ret = LPCUSBSIO_OK;
    }
    else {
        ret = -(res + 0x10);
    }
    return ret;
}

static int32_t SIO_SendRequest(LPCUSBSIO_Ctrl_t *dev, uint8_t portNum, uint8_t req, uint8_t *outData, uint32_t outDataLen, uint8_t *inData, uint32_t *inLen)
{
    HID_SIO_OUT_REPORT_T *pOut;
    HID_SIO_IN_REPORT_T *pIn;
    int32_t res = 0;
    uint8_t read_pending;
    uint32_t outLen = outDataLen;
    uint32_t oneTx;

    Log("SIO_SendRequest(dev, portNum=%d, req=0x%x, outData, outLen=%d, inData, inLen)\n", portNum, req, outLen);

    if (((outLen > 0) && (outData == NULL)) || ((inLen != NULL) && (inData == NULL))) {
        /* Param Error */
        return g_lastError = LPCUSBSIO_ERR_INVALID_PARAM;
    }

#if SIO_DEBUG>0
    if(outDataLen)
    {
        uint32_t i;
        Log("  outData[%d]: ", outDataLen);
        for(i=0; i<outDataLen; i++)
            Log("%02X ", outData[i]);

        Log("\n              ");
        for(i=0; i<outDataLen; i++)
            Log("%c", isprint(outData[i]) ? outData[i] : '.');

        Log("\n");
    }
#endif

    /* construct SIO request and send to device. */
#ifdef _WIN32
    if (WaitForSingleObject(dev->sioMutex, INFINITE) != WAIT_OBJECT_0) {
        return g_lastError = LPCUSBSIO_ERR_SYNCHRONIZATION;
    }
#else
    if (pthread_mutex_lock(&dev->sioMutex) != 0) {
        return g_lastError = LPCUSBSIO_ERR_SYNCHRONIZATION;
    }
#endif
    dev->outPacket[0] = 0;
    g_lastError = LPCUSBSIO_OK;
    pOut = (HID_SIO_OUT_REPORT_T *)&dev->outPacket[HID_REPORT_DATA_OFFSET];
    pOut->transId = dev->transId++;
    pOut->sesId = portNum;
    pOut->req = req;
    pOut->transfer_len = HID_SIO_CALC_TRANSFER_LEN(outLen);
    pOut->packet_num = 0;
    do {
        if (outLen > HID_SIO_PACKET_DATA_SZ) {
            oneTx = HID_SIO_PACKET_DATA_SZ;
        }
        else {
            oneTx = outLen;
        }

        pOut->packet_len = oneTx + HID_SIO_PACKET_HEADER_SZ;

        Log("SIO_SendRequest: transId=%d, packet_num=%d, packet_len=%d, transfer_len=%d\n", pOut->transId, pOut->packet_num, pOut->packet_len, pOut->transfer_len);

        memset(&pOut->data[0], 0, HID_SIO_PACKET_DATA_SZ);
        memcpy(&pOut->data[0], outData, oneTx);

        /* the +1 is for HID_REPORT_DATA_OFFSET */
        res = hid_write(dev->hidDev, &dev->outPacket[0], HID_SIO_PACKET_SZ + 1);

        outLen -= oneTx;
        outData += oneTx;
        pOut->packet_num++;

        Log("SIO_SendRequest: result=%d, outLen remaining=%d\n", res, outLen);

    } while ((res > 0) && ((outLen > 0)));

    /* Start Read */
    if (inLen != NULL) {
        *inLen = 0;
    }
    read_pending = 1;
    while ((res> 0) && (read_pending)) {
        res = hid_read_timeout(dev->hidDev, &dev->inPacket[0], HID_SIO_PACKET_SZ + 1, LPCUSBSIO_READ_TMO);

        Log("SIO_SendRequest: hid_read_timeout result=%d\n", res);

        if (res > 0) {
            /* check reponse received from LPC */
            pIn = (HID_SIO_IN_REPORT_T *)&dev->inPacket[0];

            Log("SIO_SendRequest: input packet: resp=%d, transId=%d, packet_len=%d, packet_num=%d, transfer_len=%d\n", pIn->resp, pIn->transId, pIn->packet_len, pIn->packet_num, pIn->transfer_len);

            if (pIn->transId != pOut->transId) {
                /* May be previous response discard it. */
                Log("SIO_SendRequest: pIn->transId != pOut->transId (%d != %d), discard\n", pIn->transId, pOut->transId);
                continue;
            }
            else {
                if (pIn->resp == HID_SIO_RES_OK) {
                    if (inLen != NULL) {
                        memcpy(inData, &pIn->data[0], pIn->packet_len - HID_SIO_PACKET_HEADER_SZ);
                        inData += pIn->packet_len - HID_SIO_PACKET_HEADER_SZ;
                        *inLen += pIn->packet_len - HID_SIO_PACKET_HEADER_SZ;
                    }
                    if ((pIn->packet_num * HID_SIO_PACKET_SZ + pIn->packet_len) == pIn->transfer_len) {
                        Log("SIO_SendRequest: finished\n");
                        read_pending = 0;
                        res = LPCUSBSIO_OK;
                    }
                    else {
                        Log("SIO_SendRequest: not finished\n");
                    }
                }
                else {
                    /* update status */
                    res = ConvertResp(pIn->resp);
                    Log("SIO_SendRequest: ConvertResp res=%d\n", res);
                    read_pending = 0;
                }
            }
        }
        else if (res == 0) {
            Log("SIO_SendRequest: wait timeout!\n");
            res = LPCUSBSIO_ERR_TIMEOUT;
            read_pending = 0;
        }
    }
#ifdef _WIN32
    if (!ReleaseMutex(dev->sioMutex)) {
        res = LPCUSBSIO_ERR_SYNCHRONIZATION;
    }
#else
    if (pthread_mutex_unlock(&dev->sioMutex) != 0) {
        res = LPCUSBSIO_ERR_SYNCHRONIZATION;
    }
#endif
    Log("SIO_SendRequest: returning %d\n", res);
    return g_lastError = res;
}

static int32_t GPIO_SendCmd(LPC_HANDLE hUsbSio, uint8_t port, uint32_t cmd, uint32_t setPins, uint32_t clrPins, uint32_t* status)
{
    LPCUSBSIO_Ctrl_t *dev = (LPCUSBSIO_Ctrl_t *)hUsbSio;
    int32_t res;
    uint8_t *outData;
    uint8_t *inData;
    uint32_t inLen;

    if (validHandle(hUsbSio) == 0) {
        return g_lastError = LPCUSBSIO_ERR_BAD_HANDLE;
    }
    /* construct req packet */
    outData = (uint8_t *)malloc(8);
    inData = (uint8_t *)malloc(4);
    if ((outData != NULL) && (inData != NULL)) {
        memcpy(outData, &setPins, sizeof(uint32_t));
        memcpy(outData + 4, &clrPins, sizeof(uint32_t));

        res = SIO_SendRequest(dev, port, cmd, outData, 8, inData, &inLen);
        if (res == LPCUSBSIO_OK) {
            /* parse response */
            res = inLen;
            if (res != 0) {
                /* copy data back to user buffer */
                *status = *((uint32_t*)inData);
            }
        }
        free(outData);
        free(inData);
    }
    else {
        g_lastError = LPCUSBSIO_ERR_MEM_ALLOC;
        res = LPCUSBSIO_ERR_MEM_ALLOC;
    }
    return res;
}

void free_hid_dev(struct hid_device_info *dev)
{
    dev->next = NULL;
    hid_free_enumeration(dev);
}

/*****************************************************************************
 * Public functions
 ****************************************************************************/

LPCUSBSIO_API int32_t LPCUSBSIO_GetNumPorts(uint32_t vid, uint32_t pid)
{
    struct hid_device_info *cur_dev;
    struct hid_device_info *temp_dev;
    struct hid_device_info *prev_dev = NULL;
    int32_t count = 0;

    Log("LPCUSBSIO_GetNumPorts(vid=0x%x, pid=0x%x)\n", vid, pid);

    /* free any HID device structures if we were called previously */
    if (g_Ctrl.devInfoList != NULL) {
        hid_free_enumeration(g_Ctrl.devInfoList);
        g_Ctrl.devInfoList = NULL;
    }

    cur_dev = g_Ctrl.devInfoList = hid_enumerate(vid, pid);

    Log("hid_enumerate returns %p\n", cur_dev);

    while (cur_dev)
    {
#if SIO_DEBUG
        char ps[512];
        wcstombs(ps, cur_dev->product_string, sizeof(ps)-1);
        Log("    #if=%d product_string=%s ...", cur_dev->interface_number, ps);
#endif

        /* iterate through the list and remove non-SIO devices */
#ifdef __MACH__
        /* usage_page only usable on Win/Mac */
        if (cur_dev->usage_page != (0xFF00| HID_USAGE_PAGE_SERIAL_IO))
        {
#else
        /* interface name used instead of usage_page indication */
        if (wcsncmp(cur_dev->product_string, L"LPCSIO", 6) != 0 && wcsncmp(cur_dev->product_string, L"MCUSIO", 6) != 0)
        {
#endif
            temp_dev = cur_dev->next;
            /* Update head pointer if the head is removed */
            if (g_Ctrl.devInfoList == cur_dev) {
                g_Ctrl.devInfoList = temp_dev;
            }
            /*If previously valid device found then point it to next node */
            if (prev_dev != NULL) {
                prev_dev->next = temp_dev;
            }
            free_hid_dev(cur_dev);
            cur_dev = temp_dev;
            Log("skipping\n");
            continue;
        }
        Log("using as device %d\n", count);
        count++;
        prev_dev = cur_dev;
        cur_dev = cur_dev->next;
    }

    Log("LPCUSBSIO_GetNumPorts returns %d\n", count);

    return count;
}

LPCUSBSIO_API int32_t LPCUSBSIO_GetDeviceInfo(uint32_t index, HIDAPI_DEVICE_INFO_T* pInfo)
{
    struct hid_device_info* dev = GetDevAtIndex(index);

    if (dev)
    {
        memset(pInfo, 0, sizeof(*pInfo));
        pInfo->path = dev->path;
        pInfo->vendor_id = dev->vendor_id;
        pInfo->product_id = dev->product_id;
        pInfo->serial_number = dev->serial_number;
        pInfo->release_number = dev->release_number;
        pInfo->manufacturer_string = dev->manufacturer_string;
        pInfo->product_string = dev->product_string;
        pInfo->interface_number = dev->interface_number;

        return LPCUSBSIO_OK;
    }
    else
    {
        return LPCUSBSIO_ERR_BAD_HANDLE;
    }
}

LPCUSBSIO_API LPC_HANDLE LPCUSBSIO_Open(uint32_t index)
{
    hid_device *pHid = NULL;
    LPCUSBSIO_Ctrl_t *dev = NULL;
    struct hid_device_info *cur_dev = GetDevAtIndex(index);
    int32_t res;
    uint8_t *inData;
    uint32_t inLen;

    Log("LPCUSBSIO_Open(index=%d, dev_path=%s)\n", index, (cur_dev && cur_dev->path) ? cur_dev->path : "nil");

    if (cur_dev) {
        pHid = hid_open_path(cur_dev->path);

        Log("LPCUSBSIO_Open: hid_open_path returns %p\n", pHid);

        if (pHid) {
            dev = malloc(sizeof(LPCUSBSIO_Ctrl_t));
            if (dev != NULL) {
                memset(dev, 0, sizeof(LPCUSBSIO_Ctrl_t));
                dev->hidDev = pHid;
                dev->hidInfo = cur_dev;
                g_lastError = LPCUSBSIO_OK;

                /* insert at top */
                dev->next = g_Ctrl.devList;
                g_Ctrl.devList = dev;
                /* Set all calls to this hid device as blocking. */
                // hid_set_nonblocking(dev->hidDev, 0);
                inData = (uint8_t *)malloc(12 + MAX_FWVER_STRLEN);
#ifdef _WIN32
                dev->sioMutex = CreateMutex(NULL, FALSE, NULL);
                if (dev->sioMutex == NULL) {
                    g_lastError = LPCUSBSIO_ERR_MUTEX_CREATE;
                    if (inData != NULL) {
                        free(inData);
                    }
                    return NULL;
                }
#else
                res = pthread_mutex_init(&dev->sioMutex, NULL);
                if (res != 0) {
                    g_lastError = LPCUSBSIO_ERR_MUTEX_CREATE;
                    if (inData != NULL) {
                        free(inData);
                    }
                    return NULL;
                }
#endif
                if (inData != NULL) {
                    memset(inData, 0, 12 + MAX_FWVER_STRLEN);
                    /* Send HID_SIO_REQ_DEV_INFO */
                    res = SIO_SendRequest(dev, 0, HID_SIO_REQ_DEV_INFO, NULL, 0, inData, &inLen);
                    if (res == LPCUSBSIO_OK) {
                        /* parse response */
                        if (inLen >= 12)	{
                            dev->maxI2CPorts = inData[0];
                            dev->maxSPIPorts = inData[1];
                            dev->maxGPIOPorts = inData[2];
                            dev->maxDataSize = *((uint32_t*)(inData + 4));
                            dev->fwVersion = *((uint32_t*)(inData + 8));
                            /* copy data back to user buffer */
                            #ifdef _WIN32
                            sprintf_s(&dev->fwBuild[0], MAX_FWVER_STRLEN, "FW %d.%d %s",
                                (dev->fwVersion >> 16),
                                (dev->fwVersion & 0xFFFF),
                                inData + 12);
                            #else
                            sprintf(&dev->fwBuild[0], "FW %d.%d %s",
                                (dev->fwVersion >> 16),
                                (dev->fwVersion & 0xFFFF),
                                inData + 12);
                            #endif
                        }

                    }
                    else {
                        memcpy(&dev->fwBuild[0], &g_fwInitVer[0], strlen(g_fwInitVer));
                    }
                    free(inData);
                }
            }
        }
    }
    Log("LPCUSBSIO_Open: returning %p\n", dev);
    return (LPC_HANDLE) dev;
}

LPCUSBSIO_API int32_t LPCUSBSIO_Close(LPC_HANDLE hUsbSio)
{
    LPCUSBSIO_Ctrl_t *dev = (LPCUSBSIO_Ctrl_t *)hUsbSio;
    int32_t res;
    uint8_t i;

    Log("LPCUSBSIO_Close(hUsbSio=%p)\n", hUsbSio);

    if (validHandle(hUsbSio) == 0) {
        return g_lastError = LPCUSBSIO_ERR_BAD_HANDLE;
    }

    for (i = 0; i < dev->maxI2CPorts; i++) {
        /* For each I2C port, check if it is open */
        if (dev->i2cPorts[i].hUsbSio == dev) {
            /* If I2C port is open, then close it */
            res = I2C_Close(&dev->i2cPorts[i]);
        }
    }

    for (i = 0; i < dev->maxSPIPorts; i++) {
        if (dev->spiPorts[i].hUsbSio == dev) {
            /* Valid SPI port found, so close it */
            res = SPI_Close(&dev->spiPorts[i]);
        }
    }
#ifdef _WIN32
    if (dev->sioMutex != NULL) {
        CloseHandle(dev->sioMutex);
        dev->sioMutex = NULL;
    }
#else
    pthread_mutex_destroy(&dev->sioMutex);
#endif
    hid_close(dev->hidDev);
    freeDevice(dev);

    (void)(res);
    return LPCUSBSIO_OK;
}

LPCUSBSIO_API const wchar_t *LPCUSBSIO_Error(LPC_HANDLE hUsbSio)
{
    LPCUSBSIO_Ctrl_t *dev = (LPCUSBSIO_Ctrl_t *)hUsbSio;
    const wchar_t *retStr = NULL;

    if (LPCUSBSIO_ERR_HID_LIB == g_lastError) {
        retStr = hid_error(dev->hidDev);
    } else {
            retStr = GetErrorString(g_lastError);
    }

    return retStr;
}

LPCUSBSIO_API const char *LPCUSBSIO_GetVersion(LPC_HANDLE hUsbSio)
{
    LPCUSBSIO_Ctrl_t *dev = (LPCUSBSIO_Ctrl_t *)hUsbSio;
    uint32_t index = 0;

    /* copy library version */
    memcpy(&g_Version[index], &g_LibVersion[0], strlen(g_LibVersion));
    index += (uint32_t)strlen(g_LibVersion);

    /* if handle is good copy firmware version */
    if (validHandle(hUsbSio) != 0) {
        g_Version[index] = '/';
        index++;
        /* copy firmware version */
        memcpy(&g_Version[index], &dev->fwBuild[0], strlen(dev->fwBuild));
        index += (uint32_t)strlen(dev->fwBuild);
    }

    return &g_Version[0];
}

LPCUSBSIO_API uint32_t LPCUSBSIO_GetNumI2CPorts(LPC_HANDLE hUsbSio)
{
    LPCUSBSIO_Ctrl_t *dev = (LPCUSBSIO_Ctrl_t *)hUsbSio;

    if (validHandle(hUsbSio) == 0) {
        return g_lastError = LPCUSBSIO_ERR_BAD_HANDLE;
    }
    return dev->maxI2CPorts;
}

LPCUSBSIO_API uint32_t LPCUSBSIO_GetNumSPIPorts(LPC_HANDLE hUsbSio)
{
    LPCUSBSIO_Ctrl_t *dev = (LPCUSBSIO_Ctrl_t *)hUsbSio;

    if (validHandle(hUsbSio) == 0) {
        return g_lastError = LPCUSBSIO_ERR_BAD_HANDLE;
    }
    return dev->maxSPIPorts;
}

LPCUSBSIO_API uint32_t LPCUSBSIO_GetNumGPIOPorts(LPC_HANDLE hUsbSio)
{
    LPCUSBSIO_Ctrl_t *dev = (LPCUSBSIO_Ctrl_t *)hUsbSio;

    if (validHandle(hUsbSio) == 0) {
        return g_lastError = LPCUSBSIO_ERR_BAD_HANDLE;
    }
    return dev->maxGPIOPorts;
}

LPCUSBSIO_API uint32_t LPCUSBSIO_GetMaxDataSize(LPC_HANDLE hUsbSio)
{
    LPCUSBSIO_Ctrl_t *dev = (LPCUSBSIO_Ctrl_t *)hUsbSio;

    if (validHandle(hUsbSio) == 0) {
        return g_lastError = LPCUSBSIO_ERR_BAD_HANDLE;
    }
    return dev->maxDataSize;
}

LPCUSBSIO_API int32_t LPCUSBSIO_GetLastError()
{
    return g_lastError;
}
/********************************  I2C functions *****************************************/

LPCUSBSIO_API LPC_HANDLE I2C_Open(LPC_HANDLE hUsbSio, I2C_PORTCONFIG_T *config, uint8_t portNum)
{
    LPCUSBSIO_Ctrl_t *dev = (LPCUSBSIO_Ctrl_t *) hUsbSio;
    int32_t res;
    uint8_t *outData;
    LPC_HANDLE retHandle = NULL;


    if ((validHandle(hUsbSio) == 0) || (config == NULL) || (portNum >= dev->maxI2CPorts)) {
        g_lastError = LPCUSBSIO_ERR_INVALID_PARAM;
        return NULL;
    }

    /* construct req packet */
    outData = (uint8_t *)malloc(sizeof(I2C_PORTCONFIG_T));
    if (outData != NULL) {
        memcpy(outData, config, sizeof(I2C_PORTCONFIG_T));
        res = SIO_SendRequest(dev, portNum, HID_I2C_REQ_INIT_PORT, outData, sizeof(I2C_PORTCONFIG_T), NULL, NULL);
        if (res == LPCUSBSIO_OK) {
            dev->i2cPorts[portNum].portNum = portNum;
            dev->i2cPorts[portNum].hUsbSio = (LPC_HANDLE)dev;
            retHandle = (LPC_HANDLE)&dev->i2cPorts[portNum];
        }
        free(outData);
    }

    return retHandle;
}

LPCUSBSIO_API int32_t I2C_Close(LPC_HANDLE hI2C)
{
    LPCUSBSIO_PortCtrl_t *devI2c = (LPCUSBSIO_PortCtrl_t *)hI2C;
    int32_t res;
    if (validPortHandle(hI2C) == 0) {
        return g_lastError = LPCUSBSIO_ERR_BAD_HANDLE;
    }
    res = SIO_SendRequest(devI2c->hUsbSio, devI2c->portNum, HID_I2C_REQ_DEINIT_PORT, NULL, 0, NULL, NULL);
    if (res == LPCUSBSIO_OK) {
        devI2c->portNum = 0;
        devI2c->hUsbSio = NULL;
    }
    return res;
}


LPCUSBSIO_API int32_t I2C_DeviceRead(LPC_HANDLE hI2C,
                                     uint8_t deviceAddress,
                                     uint8_t *buffer,
                                     uint16_t sizeToTransfer,
                                     uint8_t options)
{
    LPCUSBSIO_PortCtrl_t *devI2c = (LPCUSBSIO_PortCtrl_t *)hI2C;
    LPCUSBSIO_Ctrl_t *dev;
    int32_t res;
    HID_I2C_RW_PARAMS_T param;
    uint8_t *outData;
    uint8_t *inData;
    uint32_t inLen;

    if (validPortHandle(hI2C) == 0) {
        return g_lastError = LPCUSBSIO_ERR_BAD_HANDLE;
    }
    /* get the SIO Device*/
    dev = (LPCUSBSIO_Ctrl_t *)devI2c->hUsbSio;

    /* do parameter check */
    if ((sizeToTransfer > dev->maxDataSize) ||
        ((sizeToTransfer > 0) && (buffer == NULL)) ||
        (deviceAddress > 127)) {

        return g_lastError = LPCUSBSIO_ERR_INVALID_PARAM;
    }
    param.length = sizeToTransfer;
    param.options = options;
    param.slaveAddr = deviceAddress;
    /* construct req packet */
    outData = (uint8_t *)malloc(sizeof(HID_I2C_RW_PARAMS_T));
    inData = (uint8_t *)malloc(sizeToTransfer);
    if ((outData != NULL) && (inData != NULL)) {
        memcpy(outData, &param, sizeof(HID_I2C_RW_PARAMS_T));

        res = SIO_SendRequest(dev, devI2c->portNum, HID_I2C_REQ_DEVICE_READ, outData, sizeof(HID_I2C_RW_PARAMS_T), inData, &inLen);
        if (res == LPCUSBSIO_OK) {
            /* copy data back to user buffer */
            memcpy(buffer, inData, inLen);
            res = inLen;
        }
        free(outData);
        free(inData);
    }
    else {
        g_lastError = LPCUSBSIO_ERR_MEM_ALLOC;
        res = LPCUSBSIO_ERR_MEM_ALLOC;
    }

    return res;
}

LPCUSBSIO_API int32_t I2C_DeviceWrite(LPC_HANDLE hI2C,
                                      uint8_t deviceAddress,
                                      uint8_t *buffer,
                                      uint16_t sizeToTransfer,
                                      uint8_t options)
{
    LPCUSBSIO_PortCtrl_t *devI2c = (LPCUSBSIO_PortCtrl_t *)hI2C;
    LPCUSBSIO_Ctrl_t *dev;
    int32_t res;
    HID_I2C_RW_PARAMS_T param;
    uint8_t *outData;

    if (validPortHandle(hI2C) == 0) {
        return g_lastError = LPCUSBSIO_ERR_BAD_HANDLE;
    }
    /* get the SIO Device*/
    dev = (LPCUSBSIO_Ctrl_t *)devI2c->hUsbSio;

    /* do parameter check */
    if ((sizeToTransfer > dev->maxDataSize) ||
        ((sizeToTransfer > 0) && (buffer == NULL)) ||
        (deviceAddress > 127)) {

        return g_lastError = LPCUSBSIO_ERR_INVALID_PARAM;
    }

    param.length = sizeToTransfer;
    param.options = options;
    param.slaveAddr = deviceAddress;
    /* construct req packet */
    outData = (uint8_t *)malloc(sizeToTransfer + sizeof(HID_I2C_RW_PARAMS_T));
    if (outData != NULL) {
        /* copy params */
        memcpy(outData, &param, sizeof(HID_I2C_RW_PARAMS_T));
        /* copy data buffer now */
        memcpy(outData + sizeof(HID_I2C_RW_PARAMS_T), buffer, sizeToTransfer);

        res = SIO_SendRequest(dev, devI2c->portNum, HID_I2C_REQ_DEVICE_WRITE, outData, sizeof(HID_I2C_RW_PARAMS_T)+sizeToTransfer, NULL, NULL);

        if (res == LPCUSBSIO_OK) {
            /* update user on transfered size */
            res = sizeToTransfer;
        }
        free(outData);
    }
    else {
        g_lastError = LPCUSBSIO_ERR_MEM_ALLOC;
        res = LPCUSBSIO_ERR_MEM_ALLOC;
    }

    return res;
}

LPCUSBSIO_API int32_t I2C_FastXfer(LPC_HANDLE hI2C, I2C_FAST_XFER_T *xfer)
{
    LPCUSBSIO_PortCtrl_t *devI2c = (LPCUSBSIO_PortCtrl_t *)hI2C;
    LPCUSBSIO_Ctrl_t *dev;
    int32_t res;
    HID_I2C_XFER_PARAMS_T param;
    uint8_t *outData;
    uint8_t *inData;
    uint32_t inLen;

    if (validPortHandle(hI2C) == 0) {
        return g_lastError = LPCUSBSIO_ERR_BAD_HANDLE;
    }
    /* get the SIO Device*/
    dev = (LPCUSBSIO_Ctrl_t *)devI2c->hUsbSio;

    /* do parameter check */
    if ((xfer->txSz > dev->maxDataSize) || (xfer->rxSz > dev->maxDataSize) ||
        ((xfer->txSz > 0) && (xfer->txBuff == NULL)) ||
        ((xfer->rxSz > 0) && (xfer->rxBuff == NULL)) ||
        (xfer->slaveAddr > 127) ) {

        return g_lastError = LPCUSBSIO_ERR_INVALID_PARAM;
    }
    param.txLength = xfer->txSz;
    param.rxLength = xfer->rxSz;
    param.options = xfer->options;
    param.slaveAddr = xfer->slaveAddr;
    /* construct req packet */
    outData = (uint8_t *)malloc(sizeof(HID_I2C_XFER_PARAMS_T)+xfer->txSz);
    inData = (uint8_t *)malloc(xfer->rxSz);
    if ((outData != NULL) && (inData != NULL)) {
        /* copy params */
        memcpy(outData, &param, sizeof(HID_I2C_XFER_PARAMS_T));
        /* copy data buffer now */
        memcpy(outData + sizeof(HID_I2C_XFER_PARAMS_T), &xfer->txBuff[0], xfer->txSz);

        res = SIO_SendRequest(dev, devI2c->portNum, HID_I2C_REQ_DEVICE_XFER, outData, sizeof(HID_I2C_XFER_PARAMS_T)+xfer->txSz, inData, &inLen);

        if (res == LPCUSBSIO_OK) {
            /* parse response */
            res = inLen;
            if (res != 0) {
                /* copy data back to user buffer */
                memcpy(&xfer->rxBuff[0], inData, res);
            }
            else {
                /* else it should be Tx only transfer. Update transferred size. */
                res = xfer->txSz;
            }
        }
        free(outData);
        free(inData);
    }
    else {
        g_lastError = LPCUSBSIO_ERR_MEM_ALLOC;
        res = LPCUSBSIO_ERR_MEM_ALLOC;
    }

    return res;
}

LPCUSBSIO_API int32_t I2C_Reset(LPC_HANDLE hI2C)
{
    LPCUSBSIO_PortCtrl_t *devI2c = (LPCUSBSIO_PortCtrl_t *)hI2C;
    LPCUSBSIO_Ctrl_t *dev;
    int32_t res;

    if (validPortHandle(hI2C) == 0) {
        return g_lastError = LPCUSBSIO_ERR_BAD_HANDLE;
    }

    /* get the SIO Device*/
    dev = (LPCUSBSIO_Ctrl_t *)devI2c->hUsbSio;

    res = SIO_SendRequest(dev, devI2c->portNum, HID_I2C_REQ_RESET, NULL, 0, NULL, NULL);

    return res;
}

/********************************  SPI functions *****************************************/

LPCUSBSIO_API LPC_HANDLE SPI_Open(LPC_HANDLE hUsbSio, HID_SPI_PORTCONFIG_T *config, uint8_t portNum)
{
    LPCUSBSIO_Ctrl_t *dev = (LPCUSBSIO_Ctrl_t *)hUsbSio;
    int32_t res;
    LPC_HANDLE retHandle = NULL;
    uint8_t *outData;

    if ((validHandle(hUsbSio) == 0) || (config == NULL) || (portNum >= dev->maxSPIPorts)) {
        g_lastError = LPCUSBSIO_ERR_INVALID_PARAM;
        return NULL;
    }

    Log("SPI_Open(hUsbSio=%p, cfg->busSpeed=%d, cfg->Options=%d, portNum=%d)\n", hUsbSio, config->busSpeed, config->Options);

    /* construct req packet */
    outData = (uint8_t *)malloc(sizeof(HID_SPI_PORTCONFIG_T));
    if (outData != NULL) {
        memcpy(outData, config, sizeof(HID_SPI_PORTCONFIG_T));
        res = SIO_SendRequest(dev, portNum, HID_SPI_REQ_INIT_PORT, outData, sizeof(HID_SPI_PORTCONFIG_T), NULL, NULL);
        if (res == LPCUSBSIO_OK) {
            dev->spiPorts[portNum].portNum = portNum;
            dev->spiPorts[portNum].hUsbSio = (LPC_HANDLE)dev;
            retHandle = (LPC_HANDLE)&dev->spiPorts[portNum];
        }
        free(outData);
    }

    Log("SPI_Open: returning %p)\n", retHandle);
    return retHandle;
}

LPCUSBSIO_API int32_t SPI_Close(LPC_HANDLE hSPI)
{
    LPCUSBSIO_PortCtrl_t *devSPI = (LPCUSBSIO_PortCtrl_t *)hSPI;
    int32_t res;

    if (validPortHandle(hSPI) == 0) {
        return g_lastError = LPCUSBSIO_ERR_BAD_HANDLE;
    }

    Log("SPI_Close(hSPI=%p\n", hSPI);

    res = SIO_SendRequest(devSPI->hUsbSio, devSPI->portNum, HID_SPI_REQ_DEINIT_PORT, NULL, 0, NULL, NULL);
    if (res == LPCUSBSIO_OK) {
        devSPI->portNum = 0;
        devSPI->hUsbSio = NULL;
    }
    return res;
}

LPCUSBSIO_API int32_t SPI_Transfer(LPC_HANDLE hSPI, SPI_XFER_T *xfer)
{
    LPCUSBSIO_PortCtrl_t *devSPI = (LPCUSBSIO_PortCtrl_t *)hSPI;
    LPCUSBSIO_Ctrl_t *dev;
    int32_t res;
    HID_SPI_XFER_PARAMS_T param;
    uint8_t *outData;
    uint8_t *inData;
    uint32_t inLen;

    if (validPortHandle(hSPI) == 0) {
        return g_lastError = LPCUSBSIO_ERR_BAD_HANDLE;
    }

    /* get the SIO Device*/
    dev = (LPCUSBSIO_Ctrl_t *)devSPI->hUsbSio;

    /* do parameter check */
    if ((xfer->length > dev->maxDataSize) ||
        ((xfer->length > 0) && ((xfer->txBuff == NULL) || (xfer->rxBuff == NULL)))) {

        return g_lastError = LPCUSBSIO_ERR_INVALID_PARAM;
    }

    Log("SPI_Transfer(hSPI=%p, xfer->device=%d, xfer->length=%d, xfer->options=%d)\n", hSPI, xfer->device, xfer->length, xfer->options);

    param.length = xfer->length;
    param.options = xfer->options;
    param.device = xfer->device;
    /* construct req packet */
    outData = (uint8_t *)malloc(sizeof(HID_SPI_XFER_PARAMS_T) + xfer->length);
    inData = (uint8_t *)malloc(xfer->length);
    if ((outData != NULL) && (inData != NULL)) {
        /* copy params */
        memcpy(outData, &param, sizeof(HID_SPI_XFER_PARAMS_T));
        /* copy data buffer now */
        /* Note that the for 16 bit data transfer the bytes are transferred in Little Endian Format */
        memcpy(outData + sizeof(HID_SPI_XFER_PARAMS_T), &xfer->txBuff[0], xfer->length);

        res = SIO_SendRequest(dev, devSPI->portNum, HID_SPI_REQ_DEVICE_XFER, outData, sizeof(HID_SPI_XFER_PARAMS_T)+xfer->length, inData, &inLen);

        if (res == LPCUSBSIO_OK) {
            /* parse response */
            res = inLen;
            /* copy data back to user buffer */
            memcpy(&xfer->rxBuff[0], inData, res);
        }
        free(outData);
        free(inData);
    }
    else {
        g_lastError = LPCUSBSIO_ERR_MEM_ALLOC;
        res = LPCUSBSIO_ERR_MEM_ALLOC;
    }

    Log("SPI_Transfer: returning %d\n", res);
    return res;
}

LPCUSBSIO_API int32_t SPI_Reset(LPC_HANDLE hSPI)
{
    LPCUSBSIO_PortCtrl_t *devSPI = (LPCUSBSIO_PortCtrl_t *)hSPI;
    LPCUSBSIO_Ctrl_t *dev;
    int32_t res;

    if (validPortHandle(hSPI) == 0) {
        return g_lastError = LPCUSBSIO_ERR_BAD_HANDLE;
    }

    Log("SPI_Reset(hSPI=%p)\n", hSPI);

    /* get the SIO Device*/
    dev = (LPCUSBSIO_Ctrl_t *)devSPI->hUsbSio;

    res = SIO_SendRequest(dev, devSPI->portNum, HID_SPI_REQ_RESET, NULL, 0, NULL, NULL);

    Log("SPI_Reset: returning %d\n", res);
    return res;
}

/********************************  GPIO functions *****************************************/
LPCUSBSIO_API int32_t GPIO_ReadPort(LPC_HANDLE hUsbSio, uint8_t port, uint32_t* status)
{
    return GPIO_SendCmd(hUsbSio, port, HID_GPIO_REQ_PORT_VALUE, 0, 0, status);
}


LPCUSBSIO_API int32_t GPIO_WritePort(LPC_HANDLE hUsbSio, uint8_t port, uint32_t* status)
{
    uint32_t setPins = *status;

    return GPIO_SendCmd(hUsbSio, port, HID_GPIO_REQ_PORT_VALUE, setPins, ~setPins, status);
}

LPCUSBSIO_API int32_t GPIO_SetPort(LPC_HANDLE hUsbSio, uint8_t port, uint32_t pins)
{
    uint32_t status = 0;
    return GPIO_SendCmd(hUsbSio, port, HID_GPIO_REQ_PORT_VALUE, pins, 0, &status);
}

LPCUSBSIO_API int32_t GPIO_ClearPort(LPC_HANDLE hUsbSio, uint8_t port, uint32_t pins)
{
    uint32_t status = 0;
    return GPIO_SendCmd(hUsbSio, port, HID_GPIO_REQ_PORT_VALUE, 0, pins, &status);
}

LPCUSBSIO_API int32_t GPIO_GetPortDir(LPC_HANDLE hUsbSio, uint8_t port, uint32_t* pPins)
{
    return GPIO_SendCmd(hUsbSio, port, HID_GPIO_REQ_PORT_DIR, 0, 0, pPins);
}

LPCUSBSIO_API int32_t GPIO_SetPortOutDir(LPC_HANDLE hUsbSio, uint8_t port, uint32_t pins)
{
    uint32_t status = 0;
    return GPIO_SendCmd(hUsbSio, port, HID_GPIO_REQ_PORT_DIR, pins, 0, &status);
}

LPCUSBSIO_API int32_t GPIO_SetPortInDir(LPC_HANDLE hUsbSio, uint8_t port, uint32_t pins)
{
    uint32_t status;
    return GPIO_SendCmd(hUsbSio, port, HID_GPIO_REQ_PORT_DIR, 0, pins, &status);
}

LPCUSBSIO_API int32_t GPIO_SetPin(LPC_HANDLE hUsbSio, uint8_t port, uint8_t pin)
{
    uint32_t status = 0;
    return GPIO_SendCmd(hUsbSio, port, HID_GPIO_REQ_PORT_VALUE, (1 << pin), 0, &status);
}

LPCUSBSIO_API int32_t GPIO_GetPin(LPC_HANDLE hUsbSio, uint8_t port, uint8_t pin)
{
    uint32_t status = 0;
    int32_t res = GPIO_SendCmd(hUsbSio, port, HID_GPIO_REQ_PORT_VALUE, 0, 0, &status);

    if (res > 0) {
        res = (status & (1 << pin)) ? 1 : 0;
    }
    return res;
}

LPCUSBSIO_API int32_t GPIO_ClearPin(LPC_HANDLE hUsbSio, uint8_t port, uint8_t pin)
{
    uint32_t status = 0;
    return GPIO_SendCmd(hUsbSio, port, HID_GPIO_REQ_PORT_VALUE, 0, (1 << pin), &status);
}

LPCUSBSIO_API int32_t GPIO_TogglePin(LPC_HANDLE hUsbSio, uint8_t port, uint8_t pin)
{
    LPCUSBSIO_Ctrl_t *dev = (LPCUSBSIO_Ctrl_t *)hUsbSio;
    int32_t res;
    uint8_t outData;

    if (validHandle(hUsbSio) == 0) {
        return g_lastError = LPCUSBSIO_ERR_BAD_HANDLE;
    }
    /* construct req packet */
    outData = pin;

    res = SIO_SendRequest(dev, port, HID_GPIO_REQ_TOGGLE_PIN, &outData, 1, NULL, NULL);

    return res;
}

LPCUSBSIO_API int32_t GPIO_ConfigIOPin(LPC_HANDLE hUsbSio, uint8_t port, uint8_t pin, uint32_t mode)
{
    LPCUSBSIO_Ctrl_t *dev = (LPCUSBSIO_Ctrl_t *)hUsbSio;
    int32_t res;
    uint8_t outData[5];

    if (validHandle(hUsbSio) == 0) {
        return g_lastError = LPCUSBSIO_ERR_BAD_HANDLE;
    }
    /* construct req packet */
    outData[0] = (uint8_t)((mode >> 0) & 0xff);
    outData[1] = (uint8_t)((mode >> 8) & 0xff);
    outData[2] = (uint8_t)((mode >> 16) & 0xff);
    outData[3] = (uint8_t)((mode >> 24) & 0xff);
    outData[4] = pin;

    res = SIO_SendRequest(dev, port, HID_GPIO_REQ_IOCONFIG, &outData[0], 5, NULL, NULL);

    return res;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// new HID low-level functions used to simplify direct HID access from LIBUSBSIO Python wrapper

HIDAPI_ENUM_T* g_hidapiEnums = NULL;

LPCUSBSIO_API HIDAPI_ENUM_HANDLE HIDAPI_Enumerate(uint32_t vid, uint32_t pid, int32_t read_ex_info)
{
    HIDAPI_ENUM_T* enm = NULL;

    struct hid_device_info* devs = hid_enumerate(vid, pid);

    enm = (HIDAPI_ENUM_T*)calloc(1, sizeof(HIDAPI_ENUM_T));
    if(!enm)
    {
        hid_free_enumeration(devs);
        return NULL;
    }

    // note that devs may be NULL
    enm->head = devs;
    enm->pos = devs;
    enm->ex_info = read_ex_info;

    // insert enum object to global list
    enm->next_enum = g_hidapiEnums;
    g_hidapiEnums = enm;
    return (HIDAPI_ENUM_HANDLE) enm;
}

LPCUSBSIO_API int32_t HIDAPI_EnumerateNext(HIDAPI_ENUM_HANDLE hHidEnum, HIDAPI_DEVICE_INFO_T* pInfo)
{
    HIDAPI_ENUM_T* enm = (HIDAPI_ENUM_T*)hHidEnum;
    struct hid_device_info* dev = NULL;

    if(!enm || !enm->head || !enm->pos)
        return 0;

    // get next device
    dev = enm->pos;
    enm->pos = dev->next;
    
    memset(pInfo, 0, sizeof(*pInfo));
    pInfo->path = dev->path;
    pInfo->vendor_id = dev->vendor_id;
    pInfo->product_id = dev->product_id;
    pInfo->serial_number = dev->serial_number;
    pInfo->release_number = dev->release_number;
    pInfo->manufacturer_string = dev->manufacturer_string;
    pInfo->product_string = dev->product_string;
    pInfo->interface_number = dev->interface_number;

    if(enm->ex_info && dev->path)
    {
        hid_device* dd = hid_open_path(dev->path);
        if (dd != NULL)
        {
            hid_get_report_lengths(dd, &pInfo->ex.output_report_length, &pInfo->ex.input_report_length);
            hid_get_usage(dd, &pInfo->ex.usage_page, &pInfo->ex.usage);
            pInfo->ex.is_valid = 1;

            hid_close(dd);
        }
    }

    return 1;
}

LPCUSBSIO_API int32_t HIDAPI_EnumerateRewind(HIDAPI_ENUM_HANDLE hHidEnum)
{
    HIDAPI_ENUM_T* enm = (HIDAPI_ENUM_T*)hHidEnum;
    if(!enm || !enm->head)
        return 0;

     enm->pos = enm->head;
     return 1;
}

LPCUSBSIO_API int32_t HIDAPI_EnumerateFree(HIDAPI_ENUM_HANDLE hHidEnum)
{
    HIDAPI_ENUM_T* enm = (HIDAPI_ENUM_T*)hHidEnum;

    // find in global chain
    HIDAPI_ENUM_T** ppE = &g_hidapiEnums;
    int32_t found = 0;

    while(*ppE)
    {
        if(*ppE == enm)
        {
            *ppE = enm->next_enum;
            found = 1;
            break;
        }

        ppE = &((*ppE)->next_enum);
    }

    if (found)
    {
        hid_free_enumeration(enm->head);
        free(enm);
    }

    return found;
}

LPCUSBSIO_API HIDAPI_DEVICE_HANDLE HIDAPI_DeviceOpen(char* pDevicePath)
{
    hid_device* dd = hid_open_path(pDevicePath);
    return (HIDAPI_DEVICE_HANDLE)dd;
}

LPCUSBSIO_API int32_t HIDAPI_DeviceClose(HIDAPI_DEVICE_HANDLE hDevice)
{
    hid_device* dd = (hid_device*)hDevice;

    if (!dd)
        return -1;

    hid_close(dd);
    return 0;
}

LPCUSBSIO_API int32_t HIDAPI_DeviceWrite(HIDAPI_DEVICE_HANDLE hDevice, const void* data, int32_t size, uint32_t timeout_ms)
{
    hid_device* dd = (hid_device*)hDevice;

    if (!dd)
        return -1;

    return hid_write_timeout(dd, (const unsigned char*)data, (size_t)size, timeout_ms);
}

LPCUSBSIO_API int32_t HIDAPI_DeviceRead(HIDAPI_DEVICE_HANDLE hDevice, void* data, int32_t size, uint32_t timeout_ms)
{
    hid_device* dd = (hid_device*)hDevice;

    if (!dd)
        return -1;

    return hid_read_timeout(dd, (unsigned char*)data, (size_t)size, timeout_ms);
}

// called whenever some device or enumeration is closed, to see if we can unload the HID library
static int32_t LibCleanup()
{
    if(g_Ctrl.devInfoList)
        return 0;

    if(g_hidapiEnums)
        return 0;

    hid_exit();
    return 1;
}


