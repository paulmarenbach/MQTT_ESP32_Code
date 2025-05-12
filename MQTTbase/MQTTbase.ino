/*
  Please download and install these libs to upload & run the programm successfully:
  Installation in Arduino IDE: Sketch -> Include Lib. -> Add .zip
  https://github.com/mathieucarbou/AsyncTCP
  https://github.com/knolleary/pubsubclient

  Uploading after writing 100% can fail, but the upload still worked... Check the Serial Monitor for logs.
  Compiling also takes a while
*/

#include <WiFi.h>
#include <AsyncTCP.h>
#include <PubSubClient.h>

// Adjust to personal settings to connect to your Wifi & MQTT server IP
const char* ssid = "MqttTest"; // Wifi Name
const char* password = "MqttTest"; // Wifi Pass
const char* mqtt_server = "192.168.178.96"; // MQTT-Server IP
char* hello_message = "Hello from ESP32-W!"; // Hello msg
const char* client_name = "ESP_32-W"; // Unique Name of micro controller, as it will be seen on the mqtt network
char* topic = "topic"; // Default MQTT topic to subscribe, will change according to later assignment

WiFiClient espClient;
PubSubClient mqttclient(espClient);

// Construct the topic id from the current topic and the client_name
char* getTopicId() {
  static char topicBuffer[100];  // Static buffer persists between function calls
  // Concatinate char* to build the topic-id being in the form: client_name/topic
  snprintf(topicBuffer, sizeof(topicBuffer), "%s/%s", client_name, topic);
  return topicBuffer;
}

// Reconnect logic
void reconnect() {
  // Got disconnected, try to reconnect again!
  while (!mqttclient.connected()) {
    Serial.print("Attempting MQTT connection...");

    // If it worked, connect to previous topic again
    if (mqttclient.connect(client_name)) {
      Serial.println("connected");
      // Subscribe to topic
      subscribe(topic);

    // Else wait and try again
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttclient.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// Subscribe to a new topic
void subscribe(char* newTopic) {
  // String compare, if the newTopic is not empty (only used for setup)
  if (strcmp(newTopic, "") == 0) {
    mqttclient.unsubscribe(getTopicId());
    topic = newTopic;
  }
  mqttclient.subscribe(getTopicId());
}

// Send a message to the current topic
void send(char* message) {
  mqttclient.publish(getTopicId(), message);
}

// Method that runs after a message was received on mqtt
void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");

  // Get received message as string
  String messageTemp;
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // Do stuff with the topic
  //if (String(topic) == "topic") {
    // Do stuff with messageTemp
  //}
}

void setup(void) {
  Serial.begin(115200);

  // Wifi connection setup
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Retry Wifi connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Wifi connection success
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Mqtt server connection setup
  mqttclient.setServer(mqtt_server, 1883);
  mqttclient.setCallback(callback);

  // Publish to Topic on broker
  send(hello_message);
}

void loop(void) {
  // Reconnect logic
  if (!mqttclient.connected()) {
    reconnect();
  }

  // Do stuff
  send(hello_message);

  // Keep alive-ping etc. for MQTT communication
  mqttclient.loop();
}