#include "TelemetryMetricSwitchState.h"


void TelemetryMetricSwitchState::begin(
    MqttClient* mqttClient,
    uint8_t pin,
    bool stateOpen,
    unsigned long msPublishTimeout) {
    /* assign instance vars */
    _mqttClient = mqttClient;
    _pin = pin;
    _stateOpen = stateOpen;
    _msPublishDebounce = msPublishTimeout;

    /* timeout the publish delay */
    _msLastPublish = msPublishTimeout + 1;

    /* set state */
    _runType = METRIC_RUN_TYPE_SWITCH_START;
}

void TelemetryMetricSwitchState::pauseReporting() {
    _runType = METRIC_RUN_TYPE_SWITCH_UPDATED_TO_PAUSED;
}

void TelemetryMetricSwitchState::_publishEvent(String eventName) {
    String stringEventTopic = _mqttTopic + "/" + _switchName + "/event";
    int topicLength = stringEventTopic.length() + 1;
    char charEventTopic[topicLength];

    stringEventTopic.toCharArray(charEventTopic, topicLength);

    _mqttClient->beginMessage(
    charEventTopic,
    _retainMqttMessage,
    _mqttQos);

    _mqttClient->print(eventName);
    _mqttClient->endMessage();
}

void TelemetryMetricSwitchState::_publishState() {
    /* check if the publish timeout has expired */
    if (millis() - _msLastPublish < _msPublishDebounce) {
        return;
    }

    /* publish timeout expired, publish state */

    yield(); // yield before we begin state publish
    
    bool currentState = digitalRead(_pin);

    String stringEventTopic = _mqttTopic + "/" + _switchName + "/isOpen";
    int topicLength = stringEventTopic.length() + 1;
    char charEventTopic[topicLength];

    stringEventTopic.toCharArray(charEventTopic, topicLength);

    _mqttClient->beginMessage(
    charEventTopic,
    _retainMqttMessage,
    _mqttQos);

    
    _mqttClient->print(currentState == _stateOpen ? "true" : "false");
    _mqttClient->endMessage();

    /* update the last publish timestamp */
    _msLastPublish = millis();
    /* record previous state */
    _statePrevious = currentState;
}

void TelemetryMetricSwitchState::onMessage(JsonDocument json) {
  String target = json["target"];

  if (target != _switchName) {
    return; // message not meant for this sensor
  }

  int action = json["action"];
  unsigned long msDebounce = json["msDebounce"];

  if (action == 210) { // pause
    pauseReporting();
  }

  if (action == 211) { // resume
    resumeReporting();
  }

  if (action == 212) { // change the publish debounce timeout
    setPublishDebounce(msDebounce);
  }
}

void TelemetryMetricSwitchState::resumeReporting() {
    _runType = METRIC_RUN_TYPE_SWITCH_UPDATED_TO_RUNNING;
}

void TelemetryMetricSwitchState::setPublishDebounce(unsigned long msDebounce) {
    _msPublishDebounce = msDebounce;
    _runType = METRIC_RUN_TYPE_SWITCH_PUBLISH_DEBOUNCE_CHANGED;
}

void TelemetryMetricSwitchState::run() {

    /* manage state */
    switch(_runType) {
        case METRIC_RUN_TYPE_SWITCH_UPDATED_TO_PAUSED:
            _publishEvent("PAUSED"); // publish an event
            _runType = METRIC_RUN_TYPE_SWITCH_PAUSED;
            return;

        case METRIC_RUN_TYPE_SWITCH_PAUSED:
            return; // nothing to do, paused
        
        case METRIC_RUN_TYPE_SWITCH_UPDATED_TO_RUNNING:
            _publishEvent("RUNNING");
            _runType = METRIC_RUN_TYPE_SWITCH_RUNNING;
            return;

        case METRIC_RUN_TYPE_SWITCH_START:
            _publishEvent("STARTED"); // publish a start event
            _publishEvent("RUNNING");
            _runType = METRIC_RUN_TYPE_SWITCH_PUBLISH_STATE; // flag state to be published
            return;

        case METRIC_RUN_TYPE_SWITCH_PUBLISH_STATE:
            _publishState(); // publish state
            _runType = METRIC_RUN_TYPE_SWITCH_RUNNING; // resume running
            return;

        case METRIC_RUN_TYPE_SWITCH_PUBLISH_DEBOUNCE_CHANGED:
            _publishEvent("SWITCH_PUBLISH_DEBOUNCE_CHANGED"); // publish an event
            _publishEvent("RUNNING"); // resume running
            _runType = METRIC_RUN_TYPE_SWITCH_RUNNING;
            return;
    }

    /* if running, read the pin state */
    bool currentState = digitalRead(_pin);

    /* if the pin state matches that of the last publish */
    if (currentState == _statePrevious) {
        return; // do nothing
    }

    /* if pinState does NOT match the previous state, flag a publish */
    _runType = METRIC_RUN_TYPE_SWITCH_PUBLISH_STATE;
}