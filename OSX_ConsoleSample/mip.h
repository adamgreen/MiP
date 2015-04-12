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
#define MIP_ERROR_BAD_RESPONSE  8 // Unexpected response from MiP.

// Maximum length of MiP request and response buffer lengths.
#define MIP_REQUEST_MAX_LEN     (17 + 1)    // Longest request is MPI_CMD_PLAY_SOUND.
#define MIP_RESPONSE_MAX_LEN    (5 + 1)     // Longest response is MPI_CMD_REQUEST_CHEST_LED.

typedef enum MiPGestureRadarMode
{
    MIP_GESTURE_RADAR_DISABLED = 0x00,
    MIP_GESTURE                = 0x02,
    MIP_RADAR                  = 0x04
} MiPGestureRadarMode;

typedef enum MiPRadar
{
    MIP_RADAR_NONE      = 0x01,
    MIP_RADAR_10CM_30CM = 0x02,
    MIP_RADAR_0CM_10CM  = 0x03
} MiPRadar;

typedef enum MiPGesture
{
    MIP_GESTURE_LEFT               = 0x0A,
    MIP_GESTURE_RIGHT              = 0x0B,
    MIP_GESTURE_CENTER_SWEEP_LEFT  = 0x0C,
    MIP_GESTURE_CENTER_SWEEP_RIGHT = 0x0D,
    MIP_GESTURE_CENTER_HOLD        = 0x0E,
    MIP_GESTURE_FORWARD            = 0x0F,
    MIP_GESTURE_BACKWARD           = 0x10
} MiPGesture;

typedef enum MiPHeadLED
{
    MIP_HEAD_LED_OFF        = 0,
    MIP_HEAD_LED_ON         = 1,
    MIP_HEAD_LED_BLINK_SLOW = 2,
    MIP_HEAD_LED_BLINK_FAST = 3
} MiPHeadLED;

typedef enum MiPDriveDirection
{
    MIP_DRIVE_FORWARD  = 0x00,
    MIP_DRIVE_BACKWARD = 0x01
} MiPDriveDirection;

typedef enum MiPTurnDirection
{
    MIP_TURN_LEFT  = 0x00,
    MIP_TURN_RIGHT = 0x01
} MiPTurnDirection;

typedef enum MiPFallDirection
{
    MIP_FALL_ON_BACK   = 0x00,
    MIP_FALL_FACE_DOWN = 0x01
} MiPFallDirection;

typedef enum MiPPosition
{
    MIP_POSITION_ON_BACK                = 0x00,
    MIP_POSITION_FACE_DOWN              = 0x01,
    MIP_POSITION_UPRIGHT                = 0x02,
    MIP_POSITION_PICKED_UP              = 0x03,
    MIP_POSITION_HAND_STAND             = 0x04,
    MIP_POSITION_FACE_DOWN_ON_TRAY      = 0x05,
    MIP_POSITION_ON_BACK_WITH_KICKSTAND = 0x06
} MiPPosition;

typedef enum MiPGetUp
{
    MIP_GETUP_FROM_FRONT  = 0x00,
    MIP_GETUP_FROM_BACK   = 0x01,
    MIP_GETUP_FROM_EITHER = 0x02
} MiPGetUp;

typedef enum MiPSoundIndex
{
    MIP_SOUND_ONEKHZ_500MS_8K16BIT = 1,
    MIP_SOUND_ACTION_BURPING,
    MIP_SOUND_ACTION_DRINKING,
    MIP_SOUND_ACTION_EATING,
    MIP_SOUND_ACTION_FARTING_SHORT,
    MIP_SOUND_ACTION_OUT_OF_BREATH,
    MIP_SOUND_BOXING_PUNCHCONNECT_1,
    MIP_SOUND_BOXING_PUNCHCONNECT_2,
    MIP_SOUND_BOXING_PUNCHCONNECT_3,
    MIP_SOUND_FREESTYLE_TRACKING_1,
    MIP_SOUND_MIP_1,
    MIP_SOUND_MIP_2,
    MIP_SOUND_MIP_3,
    MIP_SOUND_MIP_APP,
    MIP_SOUND_MIP_AWWW,
    MIP_SOUND_MIP_BIG_SHOT,
    MIP_SOUND_MIP_BLEH,
    MIP_SOUND_MIP_BOOM,
    MIP_SOUND_MIP_BYE,
    MIP_SOUND_MIP_CONVERSE_1,
    MIP_SOUND_MIP_CONVERSE_2,
    MIP_SOUND_MIP_DROP,
    MIP_SOUND_MIP_DUNNO,
    MIP_SOUND_MIP_FALL_OVER_1,
    MIP_SOUND_MIP_FALL_OVER_2,
    MIP_SOUND_MIP_FIGHT,
    MIP_SOUND_MIP_GAME,
    MIP_SOUND_MIP_GLOAT,
    MIP_SOUND_MIP_GO,
    MIP_SOUND_MIP_GOGOGO,
    MIP_SOUND_MIP_GRUNT_1,
    MIP_SOUND_MIP_GRUNT_2,
    MIP_SOUND_MIP_GRUNT_3,
    MIP_SOUND_MIP_HAHA_GOT_IT,
    MIP_SOUND_MIP_HI_CONFIDENT,
    MIP_SOUND_MIP_HI_NOT_SURE,
    MIP_SOUND_MIP_HI_SCARED,
    MIP_SOUND_MIP_HUH,
    MIP_SOUND_MIP_HUMMING_1,
    MIP_SOUND_MIP_HUMMING_2,
    MIP_SOUND_MIP_HURT,
    MIP_SOUND_MIP_HUUURGH,
    MIP_SOUND_MIP_IN_LOVE,
    MIP_SOUND_MIP_IT,
    MIP_SOUND_MIP_JOKE,
    MIP_SOUND_MIP_K,
    MIP_SOUND_MIP_LOOP_1,
    MIP_SOUND_MIP_LOOP_2,
    MIP_SOUND_MIP_LOW_BATTERY,
    MIP_SOUND_MIP_MIPPEE,
    MIP_SOUND_MIP_MORE,
    MIP_SOUND_MIP_MUAH_HA,
    MIP_SOUND_MIP_MUSIC,
    MIP_SOUND_MIP_OBSTACLE,
    MIP_SOUND_MIP_OHOH,
    MIP_SOUND_MIP_OH_YEAH,
    MIP_SOUND_MIP_OOPSIE,
    MIP_SOUND_MIP_OUCH_1,
    MIP_SOUND_MIP_OUCH_2,
    MIP_SOUND_MIP_PLAY,
    MIP_SOUND_MIP_PUSH,
    MIP_SOUND_MIP_RUN,
    MIP_SOUND_MIP_SHAKE,
    MIP_SOUND_MIP_SIGH,
    MIP_SOUND_MIP_SINGING,
    MIP_SOUND_MIP_SNEEZE,
    MIP_SOUND_MIP_SNORE,
    MIP_SOUND_MIP_STACK,
    MIP_SOUND_MIP_SWIPE_1,
    MIP_SOUND_MIP_SWIPE_2,
    MIP_SOUND_MIP_TRICKS,
    MIP_SOUND_MIP_TRIIICK,
    MIP_SOUND_MIP_TRUMPET,
    MIP_SOUND_MIP_WAAAAA,
    MIP_SOUND_MIP_WAKEY,
    MIP_SOUND_MIP_WHEEE,
    MIP_SOUND_MIP_WHISTLING,
    MIP_SOUND_MIP_WHOAH,
    MIP_SOUND_MIP_WOO,
    MIP_SOUND_MIP_YEAH,
    MIP_SOUND_MIP_YEEESSS,
    MIP_SOUND_MIP_YO,
    MIP_SOUND_MIP_YUMMY,
    MIP_SOUND_MOOD_ACTIVATED,
    MIP_SOUND_MOOD_ANGRY,
    MIP_SOUND_MOOD_ANXIOUS,
    MIP_SOUND_MOOD_BORING,
    MIP_SOUND_MOOD_CRANKY,
    MIP_SOUND_MOOD_ENERGETIC,
    MIP_SOUND_MOOD_EXCITED,
    MIP_SOUND_MOOD_GIDDY,
    MIP_SOUND_MOOD_GRUMPY,
    MIP_SOUND_MOOD_HAPPY,
    MIP_SOUND_MOOD_IDEA,
    MIP_SOUND_MOOD_IMPATIENT,
    MIP_SOUND_MOOD_NICE,
    MIP_SOUND_MOOD_SAD,
    MIP_SOUND_MOOD_SHORT,
    MIP_SOUND_MOOD_SLEEPY,
    MIP_SOUND_MOOD_TIRED,
    MIP_SOUND_SOUND_BOOST,
    MIP_SOUND_SOUND_CAGE,
    MIP_SOUND_SOUND_GUNS,
    MIP_SOUND_SOUND_ZINGS,
    MIP_SOUND_SHORT_MUTE_FOR_STOP,
    MIP_SOUND_FREESTYLE_TRACKING_2,
    MIP_SOUND_VOLUME_OFF = 0xF7,
    MIP_SOUND_VOLUME_1   = 0xF8,
    MIP_SOUND_VOLUME_2   = 0xF9,
    MIP_SOUND_VOLUME_3   = 0xFA,
    MIP_SOUND_VOLUME_4   = 0xFB,
    MIP_SOUND_VOLUME_5   = 0xFC,
    MIP_SOUND_VOLUME_6   = 0xFD,
    MIP_SOUND_VOLUME_7   = 0xFE
} MiPSoundIndex;

typedef enum MiPClapEnabled
{
    MIP_CLAP_DISABLED = 0x00,
    MIP_CLAP_ENABLED  = 0x01
} MiPClapEnabled;

typedef struct MiPRadarNotification
{
    uint32_t millisec;
    MiPRadar radar;
} MiPRadarNotification;

typedef struct MiPGestureNotification
{
    uint32_t   millisec;
    MiPGesture gesture;
} MiPGestureNotification;

typedef struct MiPStatus
{
    uint32_t    millisec;
    float       battery;
    MiPPosition position;
} MiPStatus;

typedef struct MiPWeight
{
    uint32_t millisec;
    int8_t   weight;
} MiPWeight;

typedef struct MiPClap
{
    uint32_t millisec;
    uint8_t  count;
} MiPClap;

typedef struct MiPChestLED
{
    uint16_t onTime;
    uint16_t offTime;
    uint8_t  red;
    uint8_t  green;
    uint8_t  blue;
} MiPChestLED;

typedef struct MiPHeadLEDs
{
    MiPHeadLED led1;
    MiPHeadLED led2;
    MiPHeadLED led3;
    MiPHeadLED led4;
} MiPHeadLEDs;

typedef struct MiPSoftwareVersion
{
    uint16_t year;
    uint8_t  month;
    uint8_t  day;
    uint8_t  uniqueVersion;
} MiPSoftwareVersion;

typedef struct MiPHardwareInfo
{
    uint8_t voiceChip;
    uint8_t hardware;
} MiPHardwareInfo;

typedef struct MiPSound
{
    MiPSoundIndex sound;
    uint16_t      delay;
} MiPSound;

typedef struct MiPClapSettings
{
    MiPClapEnabled enabled;
    uint16_t       delay;
} MiPClapSettings;

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

int mipSetGestureRadarMode(MiP* pMiP, MiPGestureRadarMode mode);
int mipGetGestureRadarMode(MiP* pMiP, MiPGestureRadarMode* pMode);

int mipSetChestLED(MiP* pMiP, uint8_t red, uint8_t green, uint8_t blue);
int mipFlashChestLED(MiP* pMiP, uint8_t red, uint8_t green, uint8_t blue, uint16_t onTime, uint16_t offTime);
int mipGetChestLED(MiP* pMiP, MiPChestLED* pChestLED);
int mipSetHeadLEDs(MiP* pMiP, MiPHeadLED led1, MiPHeadLED led2, MiPHeadLED led3, MiPHeadLED led4);
int mipGetHeadLEDs(MiP* pMiP, MiPHeadLEDs* pHeadLEDs);

int mipContinuousDrive(MiP* pMiP, int8_t velocity, int8_t turnRate);
int mipDistanceDrive(MiP* pMiP, MiPDriveDirection driveDirection, uint8_t cm,
                                MiPTurnDirection turnDirection, uint16_t degrees);
int mipTurnLeft(MiP* pMiP, uint16_t degrees, uint8_t speed);
int mipTurnRight(MiP* pMiP, uint16_t degrees, uint8_t speed);
int mipDriveForward(MiP* pMiP, uint8_t speed, uint16_t time);
int mipDriveBackward(MiP* pMiP, uint8_t speed, uint16_t time);
int mipStop(MiP* pMiP);
int mipFallDown(MiP* pMiP, MiPFallDirection direction);
int mipGetUp(MiP* pMiP, MiPGetUp getup);

int mipPlaySound(MiP* pMiP, const MiPSound* pSounds, size_t soundCount, uint8_t repeatCount);
int mipSetVolume(MiP* pMiP, uint8_t volume);
int mipGetVolume(MiP* pMiP, uint8_t* pVolume);

int mipReadOdometer(MiP* pMiP, float* pDistanceInCm);
int mipResetOdometer(MiP* pMiP);

int mipGetStatus(MiP* pMiP, MiPStatus* pStatus);

int mipGetWeight(MiP* pMiP, MiPWeight* pWeight);

int mipGetClapSettings(MiP* pMiP, MiPClapSettings* pSettings);
int mipEnableClap(MiP* pMiP, MiPClapEnabled enabled);
int mipSetClapDelay(MiP* pMiP, uint16_t delay);

int mipGetLatestRadarNotification(MiP* pMiP, MiPRadarNotification* pNotification);
int mipGetLatestGestureNotification(MiP* pMiP, MiPGestureNotification* pNotification);
int mipGetLatestStatusNotification(MiP* pMiP, MiPStatus* pStatus);
int mipGetLatestShakeNotification(MiP* pMiP);
int mipGetLatestWeightNotification(MiP* pMiP, MiPWeight* pWeight);
int mipGetLatestClapNotification(MiP* pMiP, MiPClap* pClap);

int mipGetSoftwareVersion(MiP* pMiP, MiPSoftwareVersion* pSoftware);
int mipGetHardwareInfo(MiP* pMiP, MiPHardwareInfo* pHardware);

int mipRawSend(MiP* pMiP, const uint8_t* pRequest, size_t requestLength);
int mipRawReceive(MiP* pMiP, const uint8_t* pRequest, size_t requestLength,
                             uint8_t* pResponseBuffer, size_t responseBufferSize, size_t* pResponseLength);
int mipRawReceiveNotification(MiP* pMiP, uint8_t* pNotifyBuffer, size_t notifyBufferSize, size_t* pNotifyLength);


#endif // MIP_H_
