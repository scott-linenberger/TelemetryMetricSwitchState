#include "TELEM_CONFIG.h"
#include <TelemetryNode.h>
#include <TelemetryMetricSwitchState.h>

/* Connections */
WiFiClient wiFiClient;
MqttClient mqttClient(wiFiClient);

/* Telemetry Node */
TelemetryNode telemNode = TelemetryNode(
  wiFiClient,
  mqttClient,
  TELEM_CONFIG);

TelemetryMetricSwitchState door = TelemetryMetricSwitchState(
  "garage-exterior-door",  // switch name
  "smart-house/garage",
  true,  // mqtt retain
  0      // QOS
);

void setup() {
  telemNode.begin();
  telemNode.connect();

  // do MQTT pub/sub
  mqttClient.onMessage(onMqttMessage);
  mqttClient.subscribe(TELEM_CONFIG.topic.incoming_actions, 1);  //subscribe to actions

  door.begin(
    telemNode.getMqttClient(),
    0, // sensor pin,
    LOW, // open state
    1500 // public debounce
  );
}

void loop() {
    yield();
    telemNode.run();
    yield();
    door.run();
}

void onMqttMessage(int messageSize) {
    JsonDocument json = telemNode.processIncomingMessage(messageSize);
    door.onMessage(json);
}