#ifndef LINENTOOLS_TELEM_METRIC_SWITCH_STATE_H
#define LINENTOOLS_TELEM_METRIC_SWITCH_STATE_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ArduinoMqttClient.h>

/* run types */
enum MetricRunTypeSwitchState {
    METRIC_RUN_TYPE_SWITCH_UPDATED_TO_PAUSED,
    METRIC_RUN_TYPE_SWITCH_PAUSED,
    METRIC_RUN_TYPE_SWITCH_UPDATED_TO_RUNNING,
    METRIC_RUN_TYPE_SWITCH_RUNNING,
    METRIC_RUN_TYPE_SWITCH_START,
    METRIC_RUN_TYPE_SWITCH_PUBLISH_STATE,
    METRIC_RUN_TYPE_SWITCH_PUBLISH_DEBOUNCE_CHANGED,
};

class TelemetryMetricSwitchState {
    private:
        String _switchName;
        uint8_t _pin;
        unsigned long _msPublishDebounce;
        bool _stateOpen;
        bool _statePrevious;
        String  _mqttTopic;
        bool    _retainMqttMessage;
        uint8_t _mqttQos;

        MqttClient* _mqttClient;
        unsigned long _msLastPublish;

        MetricRunTypeSwitchState _runType;
        void _publishEvent(String eventName);
        void _publishState();

    public:
        TelemetryMetricSwitchState(
            String switchName,
            String mqttTopic,
            bool retain,
            uint8_t qos
        ): 
        _switchName(switchName),
        _mqttTopic(mqttTopic), 
        _retainMqttMessage(retain), 
        _mqttQos(qos){};
          void begin(
            MqttClient* mqttClient,
            uint8_t pin,
            bool stateOpen,
            unsigned long msPublishTimeout);
          void run();
          void pauseReporting();
          void resumeReporting();
          void setPublishDebounce(unsigned long msDebounce);
          void onMessage(JsonDocument json);
};

#endif