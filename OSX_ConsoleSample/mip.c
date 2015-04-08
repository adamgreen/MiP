/* Copyright (C) 2015  Adam Green (https://github.com/adamgreen)

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
/* Implementation of MiP C API. */
#include <assert.h>
#include <mach/mach_time.h>
#include <stdio.h> // UNDONE: Just here for some temporary debug printf().
#include <stdlib.h>
#include "mip.h"
#include "mip-transport.h"


// MiP Protocol Commands.
// These command codes are placed in the first byte of requests sent to the MiP and responses sent back from the MiP.
// See https://github.com/WowWeeLabs/MiP-BLE-Protocol/blob/master/MiP-Protocol.md for more information.
#define MIP_CMD_SET_GESTURE_RADAR_MODE  0x0C
#define MIP_CMD_GET_RADAR_RESPONSE      0x0C
#define MIP_CMD_GET_GESTURE_RADAR_MODE  0x0D
#define MIP_CMD_GET_SOFTWARE_VERSION    0x14
#define MIP_CMD_TURN_LEFT               0x73
#define MIP_CMD_TURN_RIGHT              0x74
#define MIP_CMD_STOP                    0x77
#define MIP_CMD_CONTINUOUS_DRIVE        0x78
#define MIP_CMD_SET_CHEST_LED           0x84


// MiP::flags bits
#define MIP_FLAG_RADAR_VALID 0x01


struct MiP
{
    MiPTransport*             pTransport;
    mach_timebase_info_data_t machTimebaseInfo;
    MiPRadarNotification      lastRadar;
    uint32_t                  flags;
};


// Forward Function Declarations.
static void readNotifications(MiP* pMiP);
static uint32_t milliseconds(MiP* pMiP);


MiP* mipInit(const char* pInitOptions)
{
    MiP* pMiP = NULL;

    pMiP = calloc(1, sizeof(*pMiP));
    if (!pMiP)
        goto Error;
    pMiP->pTransport = mipTransportInit(pInitOptions);
    if (!pMiP->pTransport)
        goto Error;
    mach_timebase_info(&pMiP->machTimebaseInfo);

    return pMiP;

Error:
    if (pMiP)
    {
        mipTransportUninit(pMiP->pTransport);
        free(pMiP);
    }
    return NULL;
}

void mipUninit(MiP* pMiP)
{
    if (!pMiP)
        return;
    mipTransportUninit(pMiP->pTransport);
}

int mipConnectToRobot(MiP* pMiP, const char* pRobotName)
{
    assert( pMiP );
    return mipTransportConnectToRobot(pMiP->pTransport, pRobotName);
}

int mipDisconnectFromRobot(MiP* pMiP)
{
    assert( pMiP );
    return mipTransportDisconnectFromRobot(pMiP->pTransport);
}

int mipStartRobotDiscovery(MiP* pMiP)
{
    assert( pMiP );
    return mipTransportStartRobotDiscovery(pMiP->pTransport);
}

int mipGetDiscoveredRobotCount(MiP* pMiP, size_t* pCount)
{
    assert( pMiP );
    return mipTransportGetDiscoveredRobotCount(pMiP->pTransport, pCount);
}

int mipGetDiscoveredRobotName(MiP* pMiP, size_t robotIndex, const char** ppRobotName)
{
    assert( pMiP );
    return mipTransportGetDiscoveredRobotName(pMiP->pTransport, robotIndex, ppRobotName);
}

int mipStopRobotDiscovery(MiP* pMiP)
{
    assert( pMiP );
    return mipTransportStopRobotDiscovery(pMiP->pTransport);
}

int mipSetGestureRadarMode(MiP* pMiP, MiPGestureRadarMode mode)
{
    uint8_t command[1+1];

    assert( pMiP );
    assert( mode == MIP_GESTURE_RADAR_DISABLED || mode == MIP_GESTURE || mode == MIP_RADAR );

    command[0] = MIP_CMD_SET_GESTURE_RADAR_MODE;
    command[1] = mode;
    return mipRawSend(pMiP, command, sizeof(command));
}

int mipGetGestureRadarMode(MiP* pMiP, MiPGestureRadarMode* pMode)
{
    static const uint8_t getGestureRadarMode[1] = { MIP_CMD_GET_GESTURE_RADAR_MODE };
    uint8_t              response[1+1];
    size_t               responseLength;
    int                  result;

    result = mipRawReceive(pMiP, getGestureRadarMode, sizeof(getGestureRadarMode), response, sizeof(response), &responseLength);
    if (result)
        return result;
    if (responseLength != 2 ||
        response[0] != MIP_CMD_GET_GESTURE_RADAR_MODE ||
        (response[1] != MIP_GESTURE_RADAR_DISABLED &&
         response[1] != MIP_GESTURE &&
         response[1] != MIP_RADAR))
    {
        return MIP_ERROR_BAD_RESPONSE;
    }

    *pMode = response[1];
    return result;
}

int mipSetChestLED(MiP* pMiP, uint8_t red, uint8_t green, uint8_t blue)
{
    uint8_t command[1+3];

    assert( pMiP );

    command[0] = MIP_CMD_SET_CHEST_LED;
    command[1] = red;
    command[2] = green;
    command[3] = blue;
    return mipRawSend(pMiP, command, sizeof(command));
}

int mipContinuousDrive(MiP* pMiP, int8_t velocity, int8_t turnRate)
{
    uint8_t command[1+2];

    assert( pMiP );
    assert( velocity >= -32 && velocity <= 32 );
    assert( turnRate >= -32 && turnRate <= 32 );

    command[0] = MIP_CMD_CONTINUOUS_DRIVE;

    if (velocity == 0)
        command[1] = 0x00;
    else if (velocity < 0)
        command[1] = 0x20 + (-velocity);
    else
        command[1] = velocity;

    if (turnRate == 0)
        command[2] = 0x00;
    else if (turnRate < 0)
        command[2] = 0x60 + (-turnRate);
    else
        command[2] = 0x40 + turnRate;

    return mipRawSend(pMiP, command, sizeof(command));
}

int mipTurnLeft(MiP* pMiP, uint16_t degrees, uint8_t speed)
{
    // The turn command is in units of 5 degrees.
    uint8_t angle = degrees / 5;
    uint8_t command[1+2];

    assert( pMiP );
    assert( degrees <= 255 * 5 );
    assert( speed <= 24 );

    command[0] = MIP_CMD_TURN_LEFT;
    command[1] = angle;
    command[2] = speed;

    return mipRawSend(pMiP, command, sizeof(command));
}

int mipTurnRight(MiP* pMiP, uint16_t degrees, uint8_t speed)
{
    // The turn command is in units of 5 degrees.
    uint8_t angle = degrees / 5;
    uint8_t command[1+2];

    assert( pMiP );
    assert( degrees <= 255 * 5 );
    assert( speed <= 24 );

    command[0] = MIP_CMD_TURN_RIGHT;
    command[1] = angle;
    command[2] = speed;

    return mipRawSend(pMiP, command, sizeof(command));
}

int mipStop(MiP* pMiP)
{
    uint8_t command[1];

    assert( pMiP );

    command[0] = MIP_CMD_STOP;
    return mipRawSend(pMiP, command, sizeof(command));
}

int mipGetLatestRadarNotification(MiP* pMiP, MiPRadarNotification* pNotification)
{
    MiPRadar radar;

    readNotifications(pMiP);

    if ((pMiP->flags & MIP_FLAG_RADAR_VALID) == 0)
        return MIP_ERROR_EMPTY;
    radar = pMiP->lastRadar.radar;
    if (radar != MIP_RADAR_NONE && radar != MIP_RADAR_10CM_30CM && radar != MIP_RADAR_0CM_10CM)
        return MIP_ERROR_BAD_RESPONSE;

    *pNotification = pMiP->lastRadar;
    return MIP_ERROR_NONE;
}

static void readNotifications(MiP* pMiP)
{
    size_t               responseLength = 0;
    uint8_t              response[MIP_RESPONSE_MAX_LEN];

    while (MIP_ERROR_NONE == mipRawReceiveNotification(pMiP, response, sizeof(response), &responseLength))
    {
        // Must have at least one byte to indicate which response is being given.
        if (responseLength < 1)
            continue;
        switch (response[0])
        {
        case MIP_CMD_GET_RADAR_RESPONSE:
            if (responseLength == 2)
            {
                pMiP->lastRadar.millisec = milliseconds(pMiP);
                pMiP->lastRadar.radar = response[1];
                pMiP->flags |= MIP_FLAG_RADAR_VALID;
            }
            break;
        default:
            printf("notification -> ");
            for (int i = 0 ; i < responseLength ; i++)
                printf("%02X", response[i]);
            printf("\n");
            break;
        }
    }
}

static uint32_t milliseconds(MiP* pMiP)
{
    static const uint64_t nanoPerMilli = 1000000;

    return (uint32_t)((mach_absolute_time() * pMiP->machTimebaseInfo.numer) /
                      (nanoPerMilli * pMiP->machTimebaseInfo.denom));
}

int mipRawSend(MiP* pMiP, const uint8_t* pRequest, size_t requestLength)
{
    assert( pMiP );
    return mipTransportSendRequest(pMiP->pTransport, pRequest, requestLength, MIP_EXPECT_NO_RESPONSE);
}

int mipRawReceive(MiP* pMiP, const uint8_t* pRequest, size_t requestLength,
                             uint8_t* pResponseBuffer, size_t responseBufferSize, size_t* pResponseLength)
{
    int result = -1;

    assert( pMiP );

    result = mipTransportSendRequest(pMiP->pTransport, pRequest, requestLength, MIP_EXPECT_RESPONSE);
    if (result)
        return result;
    return mipTransportGetResponse(pMiP->pTransport, pResponseBuffer, responseBufferSize, pResponseLength);
}

int mipRawReceiveNotification(MiP* pMiP, uint8_t* pNotifyBuffer, size_t notifyBufferSize, size_t* pNotifyLength)
{
    assert( pMiP );
    return mipTransportGetOutOfBandResponse(pMiP->pTransport, pNotifyBuffer, notifyBufferSize, pNotifyLength);
}
