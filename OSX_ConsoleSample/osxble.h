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
/* This header file describes the public API that an application uses on OS X to initialize to communicate with a
   WowWee MiP self-balancing robot via the Bluetooth Low Energy transport.
*/
#ifndef OSXBLE_H_
#define OSXBLE_H_

#include "mip.h"

// Initialize the MiP transport on OS X to use BLE (Bluetooth Low Energy).
// * It should be called from a console application's main().
// * It initializes the low level transport layer and starts a separate thread to run the developer's robot code.  The
//   developer provides this code in their implementation of the robotMain() function.
void osxMiPInitAndRun(void);

// This is the API that the developer must provide to run their robot code.  It will be run on a separate thread while
// the main thread is used for handling OS X events via a NSApplicationDelegate.
void robotMain(void);

#endif // OSXBLE_H_
