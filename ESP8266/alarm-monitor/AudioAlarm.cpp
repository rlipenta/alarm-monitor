/**
 *    Alarm Monitor
 *        Uses a simple ESP8266 circuit to detect the sound of a (not-so-smart) alarm
 *
 *    Copyright 2017 Ross Lipenta
 *
 *    Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 *    in compliance with the License. You may obtain a copy of the License at:
 *
 *            http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software distributed under the License is distributed
 *    on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License
 *    for the specific language governing permissions and limitations under the License.
 *
 */
#include <Arduino.h>
#include "AudioAlarm.h"

/**
 * Event subscription for when the alarm is triggered
 * 
 *  A single handler can handle this event. Last in wins.
 * 
 * @handler the function that will handle the event
 */
void AudioAlarm::onAlarmTriggered(AudioAlarm::THandlerFunction handler) {
  _triggeredHandler = handler;
}

/**
 * Event subscription for when the alarm is cleared
 * 
 *  A single handler can handle this event. Last in wins.
 * 
 * @handler the function that will handle the event
 */
void AudioAlarm::onAlarmCleared(AudioAlarm::THandlerFunction handler) {
  _clearedHandler = handler;
}

/**
 * Handler intended to be called by the main loop
 * 
 *  Checks the state of the alarm and fires the appropriate
 *  event when the state changes.
 *  
 *  This is a temporary implementation that simply reads from
 *  GPIO Pin 2. When the pin is LOW the alarm is considered
 *  triggered. This will be replaced with an implementation
 *  that will read from an audio input device (a microphone).
 */
void AudioAlarm::handleAlarm() {
  if (!_initialized) {
    pinMode(2, INPUT);
    _lastState = HIGH;
    _currentState = HIGH;
    _initialized = true;
  }

  _currentState = digitalRead(2);
  if (_currentState == LOW && _lastState == HIGH && _triggeredHandler) {
    _lastState = _currentState;
    _triggeredHandler();
  }
  if (_currentState == HIGH && _lastState == LOW && _clearedHandler) {
    _lastState = _currentState;
    _clearedHandler();
  }
}


