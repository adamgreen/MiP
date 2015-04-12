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
    mipEnableClap()
    mipSetClapDelay()
    mipGetClapSettings()
    mipGetLatestClapNotification()
*/
#include <stdio.h>
#include <unistd.h>
#include "mip.h"
#include "osxble.h"


static void printClapSettings(const MiPClapSettings* pSettings);


int main(int argc, char *argv[])
{
    // Initialize the Core Bluetooth stack on this the main thread and start the worker robot thread to run the
    // code found in robotMain() below.
    osxMiPInitAndRun();
    return 0;
}

void robotMain(void)
{
    int                    result = -1;
    MiP*                   pMiP = mipInit(NULL);

    printf("\tExample - Use clap related functions.\n");

    // Connect to first MiP robot discovered.
    result = mipConnectToRobot(pMiP, NULL);

    MiPClapSettings settings = {0, 0};
    result = mipGetClapSettings(pMiP, &settings);
    printf("Initial clap settings.\n");
    printClapSettings(&settings);

    // Modify clap settings.
    // NOTE: Need some delay between settings or second one will be dropped.
    result = mipEnableClap(pMiP, MIP_CLAP_ENABLED);
    sleep(1);
    result = mipSetClapDelay(pMiP, 501);

    result = mipGetClapSettings(pMiP, &settings);
    printf("Updated clap settings.\n");
    printClapSettings(&settings);

    printf("Waiting for user to clap.\n");
    MiPClap clap = {0, 0};
    while (MIP_ERROR_NONE != mipGetLatestClapNotification(pMiP, &clap))
    {
    }
    printf("Detected %u claps\n", clap.count);

    mipUninit(pMiP);
}

static void printClapSettings(const MiPClapSettings* pSettings)
{
    printf("  Enabled = %s\n", pSettings->enabled ? "ON" : "OFF");
    printf("    Delay = %u\n", pSettings->delay);
}
