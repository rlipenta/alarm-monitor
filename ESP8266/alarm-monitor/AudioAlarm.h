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
 #ifndef AudioAlarm_H_
#define AudioAlarm_H_

class AudioAlarm;

class AudioAlarm {
public:
  typedef void (*THandlerFunction) (void);
  void onAlarmTriggered(THandlerFunction handler);
  void onAlarmCleared(THandlerFunction handler);
  void handleAlarm();
protected:
  THandlerFunction _triggeredHandler;
  THandlerFunction _clearedHandler;
  int _currentState;
  int _lastState;
  bool _initialized = false;
};

#endif; //AudioAlarm_H_

