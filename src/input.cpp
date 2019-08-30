#include <Arduino.h>
#include "configuration.h"

typedef enum
{
  PWMSTATE_NO_SIGNAL,
  PWMSTATE_LOW,
  PWMSTATE_CENTER,
  PWMSTATE_HIGH
} PwmState;

PwmState GetPwmState(unsigned long pwmValue)
{
  if ((pwmValue < PWM_MIN) || (pwmValue > PWM_MAX))
    return PWMSTATE_NO_SIGNAL;
  else if (pwmValue > PWM_BAND_SEP_HIGH)
    return PWMSTATE_HIGH;
  else if (pwmValue < PWM_BAND_SEP_LOW)
    return PWMSTATE_LOW;
  else
    return PWMSTATE_CENTER;
}

void handleHeadlightsPwm(int *states)
{
  unsigned long headlightsPwm;
  headlightsPwm = pulseIn(InputPin_Headlights, HIGH, PULSEIN_TIMOUT);
  PwmState headlightsPwmState = GetPwmState(headlightsPwm);

  switch (headlightsPwmState)
  {
  case PWMSTATE_NO_SIGNAL:
    states[CHANNEL_HEAD_LIGHTS] = CHANNEL_STATE_ON | CHANNEL_STATE_MASK_BLINK;
    break;
  case PWMSTATE_HIGH:
    states[CHANNEL_HEAD_LIGHTS] = CHANNEL_STATE_ON;
    states[CHANNEL_BRAKE_LIGHTS] = CHANNEL_STATE_DIM;
    break;
  default:
    break;
  }

  // Serial.printf("Headlights PWM: %lu (%d) -> (0x%x)\n", headlightsPwm, headlightsPwmState, states[CHANNEL_BRAKE_LIGHTS]);
}

void handleThrottlePwm(int *states)
{
  unsigned long throttlePwm;
  throttlePwm = pulseIn(InputPin_Throttle, HIGH, PULSEIN_TIMOUT);

  PwmState throttlePwmState = GetPwmState(throttlePwm);
  switch (throttlePwmState)
  {
  case PWMSTATE_NO_SIGNAL:
    states[CHANNEL_BRAKE_LIGHTS] = CHANNEL_STATE_ON | CHANNEL_STATE_MASK_BLINK;
    states[CHANNEL_BACKUP_LIGHTS] = CHANNEL_STATE_ON | CHANNEL_STATE_MASK_BLINK;
    break;
  case PWMSTATE_CENTER:
    states[CHANNEL_BRAKE_LIGHTS] = CHANNEL_STATE_ON;
    break;
  case PWMSTATE_HIGH:
    states[CHANNEL_BACKUP_LIGHTS] = CHANNEL_STATE_ON;
    break;
  case PWMSTATE_LOW:
    break;
  default:
    break;
  }

  // Serial.printf("Throttle PWM: %lu (%d) -> (0x%x)\n", throttlePwm, throttlePwmState, states[CHANNEL_BRAKE_LIGHTS]);
}

void handleSteeringPwm(int *states)
{
  unsigned long steeringPwm;
  steeringPwm = pulseIn(InputPin_Steering, HIGH, PULSEIN_TIMOUT);

  PwmState steeringPwmState = GetPwmState(steeringPwm);
  switch (steeringPwmState)
  {
  case PWMSTATE_NO_SIGNAL:
    states[CHANNEL_LEFT_SIGNAL] = CHANNEL_STATE_ON | CHANNEL_STATE_MASK_BLINK;
    states[CHANNEL_RIGHT_SIGNAL] = CHANNEL_STATE_ON | CHANNEL_STATE_MASK_BLINK;
    break;
  case PWMSTATE_LOW: // Left
    states[CHANNEL_LEFT_SIGNAL] = CHANNEL_STATE_ON | CHANNEL_STATE_MASK_BLINK;
    break;
  case PWMSTATE_HIGH: // Right
    states[CHANNEL_RIGHT_SIGNAL] = CHANNEL_STATE_ON | CHANNEL_STATE_MASK_BLINK;
    break;
  case PWMSTATE_CENTER:
  default:
    break;
  }

  // Serial.printf("Steering PWM: %lu -> (0x%x:0x%x)\n", steeringPwm, states[CHANNEL_LEFT_SIGNAL], states[CHANNEL_RIGHT_SIGNAL]);
}

