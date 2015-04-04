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
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include "mip.h"


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

// Notifications of interest.
typedef struct Notifications
{
    uint8_t radar;
    uint8_t claps;
} Notifications;


// Forward function declarations.
static Notifications latestNotifications(MiPTransport* pTransport);
static void updateChestColourBasedOnRadarRange(MiPTransport* pTransport, uint8_t radarResult);
static void displayNotifications(MiPTransport* pTransport);


int main(int argc, char *argv[])
{
    // Initialize the Core Bluetooth stack on this the main thread and start the worker robot thread to run the
    // code found in robotMain() below.
    osxMiPInitAndRun();
    return 0;
}


void robotMain(void)
{
    States          state = WAITING_FOR_WALL;
    size_t          responseLength = 0;
    uint32_t        cyclesToWait = 0;
    struct timeval  startTime;
    struct timeval  currentTime;
    uint8_t         response[MIP_RESPONSE_MAX_LEN];

    // Connect to the first MiP robot discovered.
    MiPTransport* pTransport = mipTransportInit(NULL);
    mipTransportConnectToRobot(pTransport, NULL);

    printf("Enable radar mode\n");
    do
    {
        static const uint8_t enableRadar[] = "\x0C\x04";
        static const uint8_t getRadarModeState[] = "\x0D";

        // Keep trying until it goes through.
        mipTransportSendRequest(pTransport, enableRadar, sizeof(enableRadar)-1, MIP_EXPECT_NO_RESPONSE);
        mipTransportSendRequest(pTransport, getRadarModeState, sizeof(getRadarModeState)-1, MIP_EXPECT_RESPONSE);
        mipTransportGetResponse(pTransport, response, sizeof(response), &responseLength);
    } while (responseLength != 2 || response[1] != 0x04);
    printf("Radar mode enabled\n");

    Notifications notifications;
    do
    {
        gettimeofday(&currentTime, NULL);
        notifications = latestNotifications(pTransport);
        updateChestColourBasedOnRadarRange(pTransport, notifications.radar);

        switch (state)
        {
        case WAITING_FOR_WALL:
            if (notifications.radar == 0x03)
            {
                // Detected wall <10cm away so start timer and switch state.
                printf("Wall detected\n");
                state = WALL_DETECTED;
                startTime = currentTime;
            }
            break;
        case WALL_DETECTED:
            if (notifications.radar != 0x03)
            {
                // Needs to stay in front of wall for at least 5 seconds.
                printf("Wall no longer detected\n");
                state = WAITING_FOR_WALL;
            }
            else if (currentTime.tv_sec - startTime.tv_sec >= 5)
            {
                // Has been in front of wall for 5 seconds to switch state to backup to ~30 cm.
                printf("Backing up\n");
                state = BACKING_UP;
            }
            break;
        case BACKING_UP:
            if (notifications.radar != 0x01)
            {
                // Back up until radar can't see wall anymore.
                static const uint8_t driveBackwards[] = "\x78\x30\x00";
                mipTransportSendRequest(pTransport, driveBackwards, sizeof(driveBackwards)-1, MIP_EXPECT_NO_RESPONSE);
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
                static const uint8_t turnLeft[] = "\x73\x18\x10";
                mipTransportSendRequest(pTransport, turnLeft, sizeof(turnLeft)-1, MIP_EXPECT_NO_RESPONSE);
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
            if (notifications.radar == 0x01)
            {
                // Drive forward with a bit of a turn to the right until wall/obstacle is detected.
                static const uint8_t driveForwardRight[] = "\x78\x08\x41";
                mipTransportSendRequest(pTransport, driveForwardRight, sizeof(driveForwardRight)-1, MIP_EXPECT_NO_RESPONSE);
            }
            else
            {
                // Once wall/obstacle is detected, we should start turning away from wall.
                printf("Driving away from wall\n");
                static const uint8_t stop[] = "\x77";
                mipTransportSendRequest(pTransport, stop, sizeof(stop)-1, MIP_EXPECT_NO_RESPONSE);
                cyclesToWait = 20;
                state = TURN_AWAY;
            }
            break;
        case TURN_AWAY:
            // Decrement loop cycle count until it hits zero.
            if (cyclesToWait > 0)
                cyclesToWait--;
            if (cyclesToWait > 0 || notifications.radar != 0x01)
            {
                // Keep turning away from wall for a minimum amount of time and until wall/obstacle is no longer in view.
                static const uint8_t turnLeft[] = "\x78\x00\x62";
                mipTransportSendRequest(pTransport, turnLeft, sizeof(turnLeft)-1, MIP_EXPECT_NO_RESPONSE);
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

    // Get volume setting just to send a command that will wait for a response before quiting.
    static const uint8_t getVolumeLevel[] = "\x16";
    mipTransportSendRequest(pTransport, getVolumeLevel, sizeof(getVolumeLevel)-1, MIP_EXPECT_RESPONSE);
    mipTransportGetResponse(pTransport, response, sizeof(response), &responseLength);
    printf("Volume level = %u\n", response[1]);

    // Dump out of band notifications sent from MiP during this session.
    displayNotifications(pTransport);

    mipTransportUninit(pTransport);
}

static Notifications latestNotifications(MiPTransport* pTransport)
{
    static Notifications latestNotifications;
    size_t               responseLength = 0;
    uint8_t              response[MIP_RESPONSE_MAX_LEN];

    while (MIP_ERROR_NONE == mipTransportGetOutOfBandResponse(pTransport, response, sizeof(response), &responseLength))
    {
        // Must have at least one byte to indicate which response is being given.
        if (responseLength < 1)
            continue;
        switch (response[0])
        {
        case 0x0C:
            if (responseLength == 2)
                latestNotifications.radar = response[1];
            break;
        case 0x1D:
            if (responseLength == 2)
                latestNotifications.claps = response[1];
            break;
        default:
            break;
        }

        printf("notification -> ");
        for (int i = 0 ; i < responseLength ; i++)
        {
            printf("%02X", response[i]);
        }
        printf("\n");
    }

    return latestNotifications;
}

static void updateChestColourBasedOnRadarRange(MiPTransport* pTransport, uint8_t radarResult)
{
    static uint8_t lastRadarResult = 0x00;
    uint8_t        chestColourCommand[] = { 0x84, 0x00, 0x00, 0x00 };

    // Skip LED setting if the colour would be the same as last time.
    if (radarResult == lastRadarResult)
        return;
    lastRadarResult = radarResult;

    printf("radar = %u\n", radarResult);
    switch (radarResult)
    {
    case 0x01:
        // No object is detected.
        // Set colour to green.
        chestColourCommand[2] = 0xFF;
        break;
    case 0x02:
        // Object is detected 10cm - 30cm out.
        // Set colour to amber.
        chestColourCommand[1] = 0xFF;
        chestColourCommand[2] = 0x40;
        break;
    case 0x03:
        // Object is detected < 10cm.
        // Set colour to red.
        chestColourCommand[1] = 0xFF;
        break;
    default:
        break;
    }
    mipTransportSendRequest(pTransport, chestColourCommand, sizeof(chestColourCommand)-1, MIP_EXPECT_NO_RESPONSE);
}

static void displayNotifications(MiPTransport* pTransport)
{
    // The current latestRadarNotification() function prints all notifications that it encounters.
    latestNotifications(pTransport);
}
