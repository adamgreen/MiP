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
#define MIP_ERROR_NONE          0 // Successful.
#define MIP_ERROR_CONNECT       1 // Connection to MiP failed.
#define MIP_ERROR_PARAM         2 // Invalid parameter passed to API.
#define MIP_ERROR_MEMORY        3 // Out of memory.
#define MIP_ERROR_NOT_CONNECTED 4 // No MiP robot connected.
#define MIP_ERROR_NO_REQUEST    5 // Not waiting for a response from a request.
#define MIP_ERROR_TIMEOUT       6 // Timed out waiting for response.
#define MIP_ERROR_EMPTY         7 // The queue was empty.

// expectResponse parameter values for mipTransportSendRequest() parameter.
#define MIP_EXPECT_NO_RESPONSE 0
#define MIP_EXPECT_RESPONSE    1

// MiP Protocol Commands.
// These command codes are placed in the first byte of requests sent to the MiP and responses sent back from the MiP.
// See https://github.com/WowWeeLabs/MiP-BLE-Protocol/blob/master/MiP-Protocol.md for more information.
// UNDONE: I will flesh these out later.
#define MIP_CMD_GET_SOFTWARE_VERSION 0x14

// Maximum length of MiP request and response buffer lengths.
#define MIP_REQUEST_MAX_LEN     (17 + 1)    // Longest request is MPI_CMD_PLAY_SOUND.
#define MIP_RESPONSE_MAX_LEN    (5 + 1)     // Longest response is MPI_CMD_REQUEST_CHEST_LED.


// UNDONE: The following declarations are actually specific to the transport being used, currently BLE on OSX.
//         They should be moved to a mip-transport.h later since an application will actually use a higher level API
//         in the future which is transport agnostic.

// An abstract object type used by the MiP API to provide transport specific information to each transport function.
// It will be initially created by a call to mipTransportInit() and then passed in as the first parameter to each of the
// other mipTransport*() functions.  It can be freed at the end with a call to mipTransportUninit;
typedef struct MiPTransport MiPTransport;

// Initialize a MiPTransport object.
// Will be the first mipTransport*() function called so it can be used for any setup that the transport needs to take
// care of.  Transport specific data can be stored in the returned in the object pointed to by the returned pointer.
//
//   pInitOptions: A character string which originates with the user.  It could be used for things like serial port,
//                 etc.
//   Returns: NULL on error.
//            A valid pointer to a transport specific object otherwise.
MiPTransport* mipTransportInit(const char* pInitOptions);

// Cleanup a MiPTransport object.
// Will be the last mipTransport*() function called so it can be used to cleanly shutdown any transport resources.
//
//   pTransport: An object that was previously returned from the mipTransportInit() call.
void          mipTransportUninit(MiPTransport* pTransport);

// Connect to a MiP robot.
//   pTransport: An object that was previously returned from the mipTransportInit() call.
//   pRobotName: The name of the robot to which a connection should be made.  This parameter can be NULL to indicate
//               that the first robot discovered.  A list of valid names can be found through the use of the
//               mipTransportStartRobotDiscovery(), mipTransportGetDiscoveredRobotCount(),
//               mipTransportGetDiscoveredRobotName(), and mipTransportStopRobotDiscovery() functions.
//   Returns: MIP_ERROR_NONE on success and a non-zero MIP_ERROR_* code otherwise.
int mipTransportConnectToRobot(MiPTransport* pTransport, const char* pRobotName);

// Disconnect from MiP robot.
//   pTransport: An object that was previously returned from the mipTransportInit() call.
//   Returns: MIP_ERROR_NONE on success and a non-zero MIP_ERROR_* code otherwise.
int mipTransportDisconnectFromRobot(MiPTransport* pTransport);

// Start the process of discovering MiP robots to which a connection can be made.
// This discovery process will continue until mipTransportStopRobotDiscovery() is called.  Once the discovery process
// has started, the mipTransportGetDiscoveredRobotCount() and mipTransportGetDiscoveredRobotName() functions called be
// called to query the current list of robots.  Those functions can still be called after calling
// mipTransportStopRobotDiscovery() but no new robots will be added to the list.
//
//   pTransport: An object that was previously returned from the mipTransportInit() call.
//   Returns: MIP_ERROR_NONE on success and a non-zero MIP_ERROR_* code otherwise.
int mipTransportStartRobotDiscovery(MiPTransport* pTransport);

// Query how many MiP robots the discovery process has found so far.
// The discovery process is started by calling mipTransportStartRobotDiscovery().  The count returned by this function
// can increase (if more and more robots are discovered over time) until mipTransportStopRobotDiscovery() is called.
//
//   pTransport: An object that was previously returned from the mipTransportInit() call.
//   pCount: A pointer to where the current count of robots should be placed.  Shouldn't be NULL.
//   Returns: MIP_ERROR_NONE on success and a non-zero MIP_ERROR_* code otherwise.
int mipTransportGetDiscoveredRobotCount(MiPTransport* pTransport, size_t* pCount);

// Query the name of a specific MiP robot which the discovery process has found.
// The discovery process is started by calling mipTransportStartRobotDiscovery().  This function is used to index into
// the list of discovered robots to obtain its name.  This name can be later used as the pRobotName parameter of the
// mipTransportConnectToRobot() function.
//
//   pTransport: An object that was previously returned from the mipTransportInit() call.
//   robotIndex: The index of the robot for which the name should be obtained.  It must be >= 0 and < the count returned
//               by mipTransportGetDiscoveredRobotCount().
//   pCount: A pointer to where the robot name should be placed.  Shouldn't be NULL.
//   Returns: MIP_ERROR_NONE on success and a non-zero MIP_ERROR_* code otherwise.
int mipTransportGetDiscoveredRobotName(MiPTransport* pTransport, size_t robotIndex, const char** ppRobotName);

// Stops the process of discovering MiP robots to which a connection can be made.
// The discovery process is started with a call to mipTransportStartRobotDiscovery() and stops when this function is
// called.  MiP robots which were found between these two calls can be listed through the use of the
// mipTransportGetDiscoveredRobotCount() and mipTransportGetDiscoveredRobotName() functions.
//
//   pTransport: An object that was previously returned from the mipTransportInit() call.
//   Returns: MIP_ERROR_NONE on success and a non-zero MIP_ERROR_* code otherwise.
int mipTransportStopRobotDiscovery(MiPTransport* pTransport);

// Send a request to the MiP robot.
//
//   pTransport: An object that was previously returned from the mipTransportInit() call.
//   pRequest: Is a pointer to the array of bytes to be sent to the robot.
//   requestLength: Is the number of bytes in the pRequest buffer to be sent to the robot.
//   expectResponse: Set to 0 if the robot is not expected to send a response to this request.  Set to non-zero if the
//                   robot will send a response to this request - a response which can be read by a subsequent call to
//                   mipTransportGetResponse().
//   Returns: MIP_ERROR_NONE on success and a non-zero MIP_ERROR_* code otherwise.
int mipTransportSendRequest(MiPTransport* pTransport, const uint8_t* pRequest, size_t requestLength, int expectResponse);

// Retrieve the response from the MiP robot for the last request made.
//
//   pTransport: An object that was previously returned from the mipTransportInit() call.
//   pResponseBuffer: Is a pointer to the array of bytes into which the response should be copied.
//   responseBufferSize: Is the number of bytes in the pResponseBuffer.
//   pResponseLength: Is a pointer to where the actual number of bytes in the response should be placed.  This value
//                    may be truncated to responseBufferSize if the actual response was > responseBufferSize.
//   Returns: MIP_ERROR_NONE on success and a non-zero MIP_ERROR_* code otherwise.
int mipTransportGetResponse(MiPTransport* pTransport,
                            uint8_t* pResponseBuffer,
                            size_t responseBufferSize,
                            size_t* pResponseLength);

// Has the robot yet responded to the last request made?
//
//   Returns: 0 if still waiting for the response which means that a call to mipTransportGetResponse() would block
//                waiting for the response to arrive.
//            non-zero if the response has been received.
int mipTransportIsResponseAvailable(MiPTransport* pTransport);


// Get an out of band response sent by the MiP robot.
// Sometimes the MiP robot sends notifications which aren't in direct response to the last request made.  This
// function will return one of these responses/notifications.
//
//   pTransport: An object that was previously returned from the mipTransportInit() call.
//   pResponseBuffer: Is a pointer to the array of bytes into which the response should be copied.
//   responseBufferSize: Is the number of bytes in the pResponseBuffer.
//   pResponseLength: Is a pointer to where the actual number of bytes in the response should be placed.  This value
//                    may be truncated to responseBufferSize if the actual response was > responseBufferSize.
//   Returns: MIP_ERROR_NONE on success and a non-zero MIP_ERROR_* code otherwise.
int mipTransportGetOutOfBandResponse(MiPTransport* pTransport,
                                     uint8_t* pResponseBuffer,
                                     size_t responseBufferSize,
                                     size_t* pResponseLength);


// UNDONE: The following is specific to OS X BLE transport implementation and should be moved at a later time.
// Initialize the MiP transport on OS X to use BLE (Bluetooth Low Energy).
// * It should be called from a console application's main().
// * It initializes the low level transport layer and starts a separate thread to run the developer's robot code.  The
//   developer provides this code in their implementation of the robotMain() function.
void osxMiPInitAndRun(void);

// This is the API that the developer must provide to run their robot code.  It will be run on a separate thread while
// the main thread is used for handling OS X events via a NSApplicationDelegate.
void robotMain(void);

#endif // MIP_H_
