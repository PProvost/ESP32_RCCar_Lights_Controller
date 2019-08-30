[![Build Status](https://travis-ci.org/PProvost/ESP32_RCCar_Lights_Controller.svg?branch=master)](https://travis-ci.org/PProvost/ESP32_RCCar_Lights_Controller)

# Wizard's LED Light Controller for RC Cars/Trucks

ESP32-based. No additional hardware required. Pretty much any
ESP32 module/board will work, as long as it has enough GPIO pins.
For mine, I used the ESP32-Pico-Kit board.

IMPORTANT: This project was created with Platformio. If you are using another
development tool, you may need to move some files around, author a makefile,
etc.

## Architecture

Two independent FreeRTOS Tasks

- Main code (setup & loop) - Monitors the incoming PWM feed, writes to shared global state
- Second task - Update light groups based on shared global state

## Controls & Inputs

### PWM Inputs

Minimum required:

- Steering (typ. CH1)
- Throttle (typ. CH2)

Optional:

- Headlights on/off (jumper or pwm channel)
- Hazards on/off (jumper or pwm channel)

## Jumpers / switch pins (not implemented yet)

You can use the jumpers below to control some of the behaviors
using a physical switch.

- Headlights on/off jumper between pin X and (vcc/gnd)
- Hazards on/off jumper between pin Y and (vcc/gnd)
- High-beams on/off jumper between pin Z and (vcc/gnd)

## Input States

These are the states that are set in the Input Processing task.

- Movement direction
  - Forward - throttle pwm high
  - Reverse - throttle pwm low
  - Braking - throttle pwm center
- Turning
  - Straight - steering pwm center
  - Left - steering pwm low
  - Right - steering pwm high
- Special
  - Hazards
    - On - pwm high or jumper closed
    - Off - pwm low or jumper open
  - High beams
    - On - pwm high or jumper closed
    - Off - pwm high or jumper open

## Control Logic

The following rules are processed in order every time through the loop,
allowing later rules to override earlier ones.

```
No Input PWM signal:
    Blink everything

Headlights: On
    HEADLIGHTS_LED_GROUP to HEADLIGHTS_LEVEL_ON
    TAILLIGHTS_LED_GROUP to TAILLIGHTS_LEVEL_ON
Headlights: Off
    HEADLIGHTS_LED_GROUP to HEADLIGHTS_LEVEL_OFF
    TAILLIGHTS_LED_GROUP to TAILLIGHTS_LEVEL_OFF

Turning: Straight
    LEFTSIGNALS_LED_GROUP to SIGNALS_LEVEL_OFF
    RIGHTSIGNALS_LED_GROUP to SIGNALS_LEVEL_OFF
Turning: Left
    LEFTSIGNALS_LED_GROUP to SIGNALS_LEVEL_ON
    RIGHTSIGNALS_LED_GROUP to SIGNALS_LEVEL_OFF
Turning right
    LEFTSIGNALS_LED_GROUP to SIGNALS_LEVEL_OFF
    RIGHTSIGNALS_LED_GROUP to SIGNALS_LEVEL_ON

Movement Direction: Reverse
    REVERSELIGHTS_LED_GROUP to REVERSELIGHTS_LEVEL_ON
Movement Direction: Forward
    REVERSELIGHTS_LED_GROUP REVERSELIGHTS_LEVEL_ON
Movement Direction: Braking
    TAILLIGHTS_LED_GROUP to TAILLIGHTS_LEVEL_BRAKING
    REVERSELIGHTS_LED_GROUP REVERSELIGHTS_LEVEL_OFF

Special: high beams (not implemented yet)
    HEADLIGHTS_LED_GROUP to HEADLIGHT_LEVEL_HIGH
Special: hazards (not implemented yet)
    LEFTSIGNALS_LED_GROUP to SIGNALS_LEVEL_ON
    RIGHTSIGNALS_LED_GROUP to SIGNALS_LEVEL_ON
```

After the logic is complete, the values are written to the LED controller and the loop repeats.

## Thoughts and ideas

- Separate (optional) channel for a dedicated high beams led group (instead of changing the brightness)
- BLE control from mobile

## License

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
