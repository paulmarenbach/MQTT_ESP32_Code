/*
  THIS IS THE PING TEST SETUP. ONLY USE FOR TESTING!

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
const char* ssid = ""; // Wifi Name
const char* password = ""; // Wifi Pass
const char* mqtt_server = ""; // MQTT-Server IP 
const char* client_name = ""; // Unique Name of micro controller, as it will be seen on the mqtt network

char* hello_message = ""; // Hello msg or test msg
const char* topic = ""; // Default MQTT topic to publish to

WiFiClient espClient;
PubSubClient mqttclient(espClient);

// Construct the default topic to subscribe to from the client_name and * topic
char* getDefaultTopic() {
  static char defaultTopicBuffer[100];  // Static buffer persists between function calls
  // Concatinate char* to build the topic-id being in the form: client_name/#
  snprintf(defaultTopicBuffer, sizeof(defaultTopicBuffer), "%s/#", client_name);
  return defaultTopicBuffer;
}

// Construct the topic id from the current topic and the client_name
char* getTopicId(char* subTopic) {
  static char topicBuffer[100];  // Static buffer persists between function calls
  // Concatinate char* to build the topic-id being in the form: client_name/topic
  snprintf(topicBuffer, sizeof(topicBuffer), "%s/%s/%s", client_name, topic, subTopic);
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
      // Subscribe to topic & all sub topics of the client_name topic
      subscribe(getDefaultTopic());

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
  mqttclient.subscribe(newTopic);
}

// Send a message to the current topic
void send(char* message, char* subTopic) {
  char payload[256];
  // Tag with client name to filter own messages
  snprintf(payload, sizeof(payload), "[%s] %s", client_name, message);
  mqttclient.publish(getTopicId(subTopic), payload);
}

// Method that runs after a message was received on mqtt
void callback(char* topic, byte* message, unsigned int length) {
  String messageTemp;
  for (int i = 0; i < length; i++) {
    messageTemp += (char)message[i];
  }

  // Ignore own messages
  if (messageTemp.startsWith("[" + String(client_name) + "]")) return;

  // Remove signature tag: "[ESP_32-A] "
  int tagEnd = messageTemp.indexOf("] ");
  if (messageTemp.startsWith("[") && tagEnd != -1) {
    messageTemp = messageTemp.substring(tagEnd + 2); // Skip "] "
  }

  Serial.print("Received: ");
  Serial.println(messageTemp);

  // Echo cleaned message back on 'echo' subtopic
  send((char*)messageTemp.c_str(), (char*)"echo");
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
  send(hello_message, "");
}

void loop(void) {
  // Reconnect logic
  if (!mqttclient.connected()) {
    reconnect();
  }

  // Do stuff
  send(hello_message, "");

  // Keep alive-ping etc. for MQTT communication
  mqttclient.loop();
}