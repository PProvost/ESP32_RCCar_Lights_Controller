/*
   Copyright 2019 Peter Provost <peter@provost.org>

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

#ifndef __CONSTANTS_H
#define __CONSTANTS_H

// Constants for the output channels (don't change)
#define NUM_CHANNELS 5
#define CHANNEL_HEAD_LIGHTS 0
#define CHANNEL_BRAKE_LIGHTS 1
#define CHANNEL_BACKUP_LIGHTS 2
#define CHANNEL_LEFT_SIGNAL 3
#define CHANNEL_RIGHT_SIGNAL 4

// Internal constants (don't change)
#define CHANNEL_STATE_MASK_BLINK (1 << 8) // You can Bitwise-OR this with the value to make it blink
#define TASK_DELAY 100         // controls the speed of the LED update loop
#define PULSEIN_TIMOUT 25000UL // micro-seconds - smaller is more responsive, too small and it won't read

#endif