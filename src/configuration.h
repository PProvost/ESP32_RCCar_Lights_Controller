#ifndef __CONFIGURATION_H
#define __CONFIGURATION_H

#include "constants.h"

// To know which GPIO pins you can use, the following reference is helpful
//   https://randomnerdtutorials.com/esp32-pinout-reference-gpios/
// Additionally, refer to your specific board pinouts to determine what is
//   available to use.

// INPUT PINS - RC signals 
const int InputPin_Steering = 34;
const int InputPin_Throttle = 35;
const int InputPin_Headlights = 37;
// const int InputPin_Aux2 = 39;

// OUTPUT PINS - LED output pins (on is high)
// Note: Set the second value to -1 if you only need one pin for that channel
const int LedPins[NUM_CHANNELS][2] = {
    {21, 25}, // Headlights
    {23, 33}, // Brake lights
    {18, 27}, // Backup lights
    {26, 32}, // Left signals
    {19, 22}  // Right signals
};

// Adjust the constants below to change the intensities for the different states
#define CHANNEL_STATE_OFF 0x00            // 0
#define CHANNEL_STATE_ON 0xC0             // 192 (not 100% to save power and life of LED)
#define BLINK_PERIOD_MS 500               // how fast a blinker goes in ms... 1/2 sec is about right

// Input (RC) PWM values (shouldn't need to change)
#define PWM_MIN 900
#define PWM_CENTER 1500
#define PWM_MAX 2100
#define PWM_BAND_SEP_LOW 1350   // below this it is considered "low"
#define PWM_BAND_SEP_HIGH 1650  // above this it is considered "high"

// Output PWM properties for LED channels (shouldn't need to change)
#define OUTPUT_PWM_FREQ 5000
#define OUTPUT_PWM_RES 8

#endif // __CONFIGURATION_H