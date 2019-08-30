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