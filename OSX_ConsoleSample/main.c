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
#include <stdio.h>
#include <unistd.h>
#include "mip.h"


int main(int argc, char *argv[])
{
    // Initialize the Core Bluetooth stack on this the main thread and start the worker robot thread to run the
    // code found in robotMain() below.
    osxMiPInitAndRun();
    return 0;
}


void robotMain(void)
{
    // Connect to the first MiP robot discovered.
    MiPTransport* pTransport = mipTransportInit(NULL);
    mipTransportConnectToRobot(pTransport, NULL);

    // Set the chest LED to purple.
    static const uint8_t setChestPurple[] = "\x84\xff\x00\xff";
    mipTransportSendRequest(pTransport, setChestPurple, sizeof(setChestPurple)-1, MIP_EXPECT_NO_RESPONSE);

    // Read out the current settings for the chest LED.
    static const uint8_t getChestColor[] = "\x83";
    uint8_t response[5+1];
    size_t responseLength = 0;
    mipTransportSendRequest(pTransport, getChestColor, sizeof(getChestColor)-1, MIP_EXPECT_RESPONSE);
    mipTransportGetResponse(pTransport, response, sizeof(response), &responseLength);
    printf("Response Length: %lu\n", responseLength);
    printf("Command: %02X\n", response[0]);
    printf("Red    : %02X\n", response[1]);
    printf("Green  : %02X\n", response[2]);
    printf("Blue   : %02X\n", response[3]);
    printf("On     : %u\n", (unsigned int)response[4] * 10);
    printf("Off    : %u\n", (unsigned int)response[5] * 10);

    mipTransportUninit(pTransport);
}
