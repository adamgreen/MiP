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
/* Example used in following API documentation:
    mipStartRobotDiscovery()
    mipGetDiscoveredRobotCount()
    mipGetDiscoveredRobotName()
    mipStopRobotDiscovery()
*/
#include <stdio.h>
#include "mip.h"
#include "osxble.h"


int main(int argc, char *argv[])
{
    // Initialize the Core Bluetooth stack on this the main thread and start the worker robot thread to run the
    // code found in robotMain() below.
    osxMiPInitAndRun();
    return 0;
}

void robotMain(void)
{
    int         result = -1;
    size_t      robotCount = 0;
    const char* pRobotName = NULL;
    MiP*        pMiP = mipInit(NULL);

    printf("\tExample - Discover MiP robots to which a connection can be made.\n"
           "\tShould see first MiP robot be listed when it is powered up near\n"
           "\tthe test machine.\n");
    result = mipStartRobotDiscovery(pMiP);
    do
    {
        result = mipGetDiscoveredRobotCount(pMiP, &robotCount);
    } while (robotCount == 0);
    result = mipGetDiscoveredRobotName(pMiP, 0, &pRobotName);
    printf("\tDiscovered %s\n", pRobotName);
    result = mipStopRobotDiscovery(pMiP);

    mipUninit(pMiP);
}
