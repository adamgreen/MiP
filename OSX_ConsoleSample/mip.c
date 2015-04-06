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
#include <stdlib.h>
#include "mip.h"
#include "mip-transport.h"

// MiP Protocol Commands.
// These command codes are placed in the first byte of requests sent to the MiP and responses sent back from the MiP.
// See https://github.com/WowWeeLabs/MiP-BLE-Protocol/blob/master/MiP-Protocol.md for more information.
// UNDONE: I will flesh these out later.
#define MIP_CMD_GET_SOFTWARE_VERSION 0x14


struct MiP
{
    MiPTransport* pTransport;
};


MiP* mipInit(const char* pInitOptions)
{
    MiP* pMiP = NULL;

    pMiP = calloc(1, sizeof(*pMiP));
    if (!pMiP)
        goto Error;
    pMiP->pTransport = mipTransportInit(pInitOptions);
    if (!pMiP->pTransport)
        goto Error;
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
