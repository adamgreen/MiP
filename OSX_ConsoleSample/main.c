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
/* Wall following sample that utilizes the mip* API. */
#include <stdio.h>
#include <unistd.h>
#include "mip.h"
#include "osxble.h"


// States for state machine.
typedef enum States
{
    WAITING_FOR_WALL,   // Waiting for user to place facing wall.
    WALL_DETECTED,      // Wall has been detected...start 5 seconds later.
    BACKING_UP,         // Back away from wall.
    WAIT_TO_TURN,       // Waiting to turn.
    TURNING_LEFT,       // Turning left.
    TOWARD_WALL,        // Drive ahead toward wall.
    TURN_AWAY,          // Turn away from wall.
    DONE,               // UNDONE: Done for now.
} States;


// Forward function declarations.
static void updateChestColourBasedOnRadarRange(MiP* pMiP, const MiPRadarNotification* pRadar);


int main(int argc, char *argv[])
{
    // Initialize the Core Bluetooth stack on this the main thread and start the worker robot thread to run the
    // code found in robotMain() below.
    osxMiPInitAndRun();
    return 0;
}


void robotMain(void)
{
    States               state = WAITING_FOR_WALL;
    MiPGestureRadarMode  mode = MIP_GESTURE_RADAR_DISABLED;
    MiPRadarNotification radar = {0, MIP_RADAR_NONE};
    uint32_t             cyclesToWait = 0;

    // Connect to the first MiP robot discovered.
    MiP* pMiP = mipInit(NULL);
    mipConnectToRobot(pMiP, NULL);

    printf("Enable radar mode\n");
    do
    {
        // Keep trying until it goes through.
        mipSetGestureRadarMode(pMiP, MIP_RADAR);
        mipGetGestureRadarMode(pMiP, &mode);
    } while (mode != MIP_RADAR);
    printf("Radar mode enabled\n");

    // Run state machine.
    do
    {
        mipGetLatestRadarNotification(pMiP, &radar);
        updateChestColourBasedOnRadarRange(pMiP, &radar);

        switch (state)
        {
        case WAITING_FOR_WALL:
            if (radar.radar == MIP_RADAR_0CM_10CM)
            {
                // Detected wall <10cm away so start timer and switch state.
                printf("Wall detected\n");
                state = WALL_DETECTED;
                cyclesToWait = 100;
            }
            break;
        case WALL_DETECTED:
            if (radar.radar != MIP_RADAR_0CM_10CM)
            {
                // Needs to stay in front of wall for at least 5 seconds.
                printf("Wall no longer detected\n");
                state = WAITING_FOR_WALL;
            }
            else if (--cyclesToWait == 0)
            {
                // Has been in front of wall for 5 seconds to switch state to backup to ~30 cm.
                printf("Backing up\n");
                state = BACKING_UP;
            }
            break;
        case BACKING_UP:
            if (radar.radar != MIP_RADAR_NONE)
            {
                // Back up until radar can't see wall anymore.
                mipContinuousDrive(pMiP, -10, 0);
            }
            else
            {
                // Have backed up far enough.  Start left turn by 90 degrees.
                printf("Backing up complete\n");
                cyclesToWait = 10;
                state = WAIT_TO_TURN;
            }
            break;
        case WAIT_TO_TURN:
            if (--cyclesToWait == 0)
            {
                // Waiting a bit for continuous drive commands to time out.
                // Now issue turn left command.
                printf("Turning left\n");
                mipTurnLeft(pMiP, 120, 16);
                cyclesToWait = 10;
                state = TURNING_LEFT;
            }
            break;
        case TURNING_LEFT:
            if (--cyclesToWait == 0)
            {
                // Left turn command should be complete now.
                printf("Left turn complete\n");
                printf("Drive toward wall\n");
                state = TOWARD_WALL;
            }
            break;
        case TOWARD_WALL:
            if (radar.radar == MIP_RADAR_NONE)
            {
                // Drive forward with a bit of a turn to the right until wall/obstacle is detected.
                mipContinuousDrive(pMiP, 8, 2);
            }
            else
            {
                // Once wall/obstacle is detected, we should start turning away from wall.
                printf("Driving away from wall\n");
                mipStop(pMiP);
                cyclesToWait = 20;
                state = TURN_AWAY;
            }
            break;
        case TURN_AWAY:
            // Decrement loop cycle count until it hits zero.
            if (cyclesToWait > 0)
                cyclesToWait--;
            if (cyclesToWait > 0 || radar.radar != MIP_RADAR_NONE)
            {
                // Keep turning left, away from wall for a minimum amount of time and until wall/obstacle
                // is no longer in view.
                mipContinuousDrive(pMiP, 0, -2);
            }
            else
            {
                printf("Drive toward wall\n");
                state = TOWARD_WALL;
            }
            break;
        case DONE:
            break;
        }

        // Pace out the continuous drive commands by 50 msec.
        usleep(50000);
    } while (state != DONE);

    mipUninit(pMiP);
}

static void updateChestColourBasedOnRadarRange(MiP* pMiP, const MiPRadarNotification* pRadar)
{
    static uint8_t lastRadarResult = 0x00;
    uint8_t        red = 0x01;
    uint8_t        green = 0x01;
    uint8_t        blue = 0x01;

    // Skip LED setting if the colour would be the same as last time.
    if (pRadar->radar == lastRadarResult)
        return;
    lastRadarResult = pRadar->radar;

    switch (pRadar->radar)
    {
    case 0x01:
        // No object is detected.
        // Set colour to green.
        green  = 0xFF;
        break;
    case 0x02:
        // Object is detected 10cm - 30cm out.
        // Set colour to amber.
        red = 0xFF;
        green = 0x40;
        break;
    case 0x03:
        // Object is detected < 10cm.
        // Set colour to red.
        red = 0xFF;
        break;
    default:
        break;
    }
    mipSetChestLED(pMiP, red, green, blue);
}
