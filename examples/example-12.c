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
    mipSetHeadLEDs()
    mipGetHeadLEDs()
*/
#include <stdio.h>
#include <unistd.h>
#include "mip.h"
#include "osxble.h"


static const char* headLEDString(MiPHeadLED led);


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
    MiPHeadLEDs headLEDs;
    MiP*        pMiP = mipInit(NULL);

    printf("\tExample - Use head LED functions.\n"
           "\tShould set each head LED to different state.\n");

    // Connect to first MiP robot discovered.
    result = mipConnectToRobot(pMiP, NULL);

    result = mipSetHeadLEDs(pMiP, MIP_HEAD_LED_OFF, MIP_HEAD_LED_ON, MIP_HEAD_LED_BLINK_SLOW, MIP_HEAD_LED_BLINK_FAST);

    sleep(4);

    result = mipGetHeadLEDs(pMiP, &headLEDs);
    printf("Head LEDs\n");
    printf("led1: %s\n", headLEDString(headLEDs.led1));
    printf("led2: %s\n", headLEDString(headLEDs.led2));
    printf("led3: %s\n", headLEDString(headLEDs.led3));
    printf("led4: %s\n", headLEDString(headLEDs.led4));

    mipUninit(pMiP);
}

static const char* headLEDString(MiPHeadLED led)
{
    switch (led)
    {
    case MIP_HEAD_LED_OFF:
        return "Off";
    case MIP_HEAD_LED_ON:
        return "On";
    case MIP_HEAD_LED_BLINK_SLOW:
        return "Blink Slow";
    case MIP_HEAD_LED_BLINK_FAST:
        return "Blink Fast";
    default:
        return "";
    }
}
