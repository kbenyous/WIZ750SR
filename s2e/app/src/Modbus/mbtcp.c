#include <string.h>
#include <stdbool.h>

#include "socket.h"
#include "W7500x_wztoe.h"
#include "mbrtu.h"
#include "mbcrc.h"
#include "mbascii.h"
#include "common.h"
#include "ConfigData.h"

#define MB_TCP_BUF_SIZE     ( 256 + 7 ) /* Must hold a complete Modbus TCP frame. */

/* ----------------------- Defines ------------------------------------------*/

/* ----------------------- MBAP Header --------------------------------------*/
/*

    <------------------------ MODBUS TCP/IP ADU(1) ------------------------->
                <----------- MODBUS PDU (1') ---------------->
    +-----------+---------------+------------------------------------------+
    | TID | PID | Length | UID  |Code | Data                               |
    +-----------+---------------+------------------------------------------+
    |     |     |        |      |
    (2)   (3)   (4)      (5)    (6)

    (2)  ... MB_TCP_TID          = 0 (Transaction Identifier - 2 Byte)
    (3)  ... MB_TCP_PID          = 2 (Protocol Identifier - 2 Byte)
    (4)  ... MB_TCP_LEN          = 4 (Number of bytes - 2 Byte)
    (5)  ... MB_TCP_UID          = 6 (Unit Identifier - 1 Byte)
    (6)  ... MB_TCP_FUNC         = 7 (Modbus Function Code)

    (1)  ... Modbus TCP/IP Application Data Unit
    (1') ... Modbus Protocol Data Unit
*/
#define MB_TCP_TID1         0
#define MB_TCP_TID2         1
#define MB_TCP_PID          2
#define MB_TCP_LEN          4
#define MB_TCP_UID          6
#define MB_TCP_FUNC         7

#define MB_UDP_CRC16_SIZE   2

#define MB_TCP_PROTOCOL_ID  0   /* 0 = Modbus Protocol */

uint8_t mbTCPtid1, mbTCPtid2;
static uint8_t    aucTCPBuf[MB_TCP_BUF_SIZE];

volatile uint8_t* pucRTUBufferCur;
volatile uint16_t usRTUBufferPos;

extern volatile uint8_t* pucASCIIBufferCur;
extern volatile uint16_t usASCIIBufferPos;

static bool mbTCPGet(uint8_t sock, uint8_t ** ppucMBTCPFrame, uint16_t * usTCPLength) {
    struct __network_info *net = (struct __network_info *)get_DevConfig_pointer()->network_info;

    uint16_t len;
    uint16_t usTCPBufPos;
    uint8_t  peerip[4];
    uint16_t peerport;

    len = getSn_RX_RSR(sock);

    if (len > 0) {
        if (net->working_mode == UDP_MODE) {
            usTCPBufPos = recvfrom(SOCK_DATA, aucTCPBuf, len, peerip, &peerport);

            if (memcmp(peerip, net->remote_ip, sizeof(peerip)) || net->remote_port != peerport) {
                net->remote_ip[0] = peerip[0];
                net->remote_ip[1] = peerip[1];
                net->remote_ip[2] = peerip[2];
                net->remote_ip[3] = peerip[3];
                net->remote_port = peerport;
            }
        } else {
            usTCPBufPos = recv(sock, aucTCPBuf, len);
        }
        *ppucMBTCPFrame = &aucTCPBuf[0];
        *usTCPLength = usTCPBufPos;
        return true;
    }
    return false;
}

static bool mbTCPPackage(uint8_t sock, uint8_t* pucRcvAddress, uint8_t** ppucFrame, int32_t * pusLength) {
    uint8_t		*pucMBTCPFrame;
    uint16_t	usLength;
    uint16_t	usPID;

    if (mbTCPGet(sock, &pucMBTCPFrame, &usLength) != false) {
        usPID = pucMBTCPFrame[MB_TCP_PID] << 8U;
        usPID |= pucMBTCPFrame[MB_TCP_PID + 1];

        if (usPID == MB_TCP_PROTOCOL_ID) {
            /*  Modbus TCP does not use any addresses. Fake the source address such
                that the processing part deals with this frame.
            */
            *pucRcvAddress = pucMBTCPFrame[MB_TCP_UID];
            mbTCPtid1 = pucMBTCPFrame[MB_TCP_TID1];
            mbTCPtid2 = pucMBTCPFrame[MB_TCP_TID2];

            *ppucFrame = &pucMBTCPFrame[MB_TCP_FUNC];
						if(usLength < MB_TCP_FUNC)
						{
							return false;
						}
            *pusLength = usLength - MB_TCP_FUNC;

            return true;
        }
    }
    return false;
}

bool MBtcp2rtuFrame(uint8_t sock) {
    uint8_t pucRcvAddress;
    int32_t pusLength;
    uint8_t* ppucFrame;
    uint16_t usCRC16;

    if (mbTCPPackage(sock, &pucRcvAddress, &ppucFrame, &pusLength) != false) {
        pucRTUBufferCur = ppucFrame - 1;
        pucRTUBufferCur[MB_SER_PDU_ADDR_OFF] = (uint8_t)pucRcvAddress;
        usRTUBufferPos = (uint16_t)pusLength + MB_RTU_ADDR_SIZE;
        usCRC16 = usMBCRC16((uint8_t *) pucRTUBufferCur, usRTUBufferPos);
        pucRTUBufferCur[usRTUBufferPos++] = (uint8_t)(usCRC16 & 0xFF);
        pucRTUBufferCur[usRTUBufferPos++] = (uint8_t)(usCRC16 >> 8);
        return true;
    }

    return false;
}

bool MBtcp2asciiFrame(uint8_t sock) {
    uint8_t pucRcvAddress;
    int32_t pusLength;
    uint8_t* ppucFrame;
    uint8_t usLRC;

    if (mbTCPPackage(sock, &pucRcvAddress, &ppucFrame, &pusLength) != false) {
        pucASCIIBufferCur = ppucFrame - 1;
        pucASCIIBufferCur[MB_SER_PDU_ADDR_OFF] = pucRcvAddress;

        usASCIIBufferPos = (uint16_t)pusLength + MB_RTU_ADDR_SIZE;

        usLRC = prvucMBLRC((uint8_t *) pucASCIIBufferCur, usASCIIBufferPos);
        pucASCIIBufferCur[usASCIIBufferPos++] = usLRC;

        return true;
    }
    return false;
}

