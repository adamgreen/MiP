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
/* This header file describes the public API that an application can use to communicate with the WowWee MiP
   self-balancing robot.
*/
#ifndef MIP_H_
#define MIP_H_

#include <stdint.h>
#include <stdlib.h>

// Integer error codes that can be returned from most of these MiP API functions.
#define MIP_ERROR_NONE          0 // Success
#define MIP_ERROR_CONNECT       1 // Connection to MiP failed.
#define MIP_ERROR_PARAM         2 // Invalid parameter passed to API.
#define MIP_ERROR_MEMORY        3 // Out of memory.
#define MIP_ERROR_NOT_CONNECTED 4 // No MiP robot connected.
#define MIP_ERROR_NO_REQUEST    5 // Not waiting for a response from a request.
#define MIP_ERROR_TIMEOUT       6 // Timed out waiting for response.
#define MIP_ERROR_EMPTY         7 // The queue was empty.

// Maximum length of MiP request and response buffer lengths.
#define MIP_REQUEST_MAX_LEN     (17 + 1)    // Longest request is MPI_CMD_PLAY_SOUND.
#define MIP_RESPONSE_MAX_LEN    (5 + 1)     // Longest response is MPI_CMD_REQUEST_CHEST_LED.


// Abstraction of the pointer type returned by mipInit() and subsequently passed into all other mip*() functions.
typedef struct MiP MiP;


// The documentation for these functions can be found at the following link:
//  https://github.com/adamgreen/MiP/tree/master/OSX_ConsoleSample#readme
MiP* mipInit(const char* pInitOptions);
void mipUninit(MiP* pMiP);

int mipConnectToRobot(MiP* pMiP, const char* pRobotName);
int mipDisconnectFromRobot(MiP* pMiP);

int mipStartRobotDiscovery(MiP* pMiP);
int mipGetDiscoveredRobotCount(MiP* pMiP, size_t* pCount);
int mipGetDiscoveredRobotName(MiP* pMiP, size_t robotIndex, const char** ppRobotName);
int mipStopRobotDiscovery(MiP* pMiP);

int mipRawSend(MiP* pMiP, const uint8_t* pRequest, size_t requestLength);
int mipRawReceive(MiP* pMiP, const uint8_t* pRequest, size_t requestLength,
                             uint8_t* pResponseBuffer, size_t responseBufferSize, size_t* pResponseLength);
int mipRawReceiveNotification(MiP* pMiP, uint8_t* pNotifyBuffer, size_t notifyBufferSize, size_t* pNotifyLength);


#endif // MIP_H_
