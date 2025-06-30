/*
  THIS IS THE PING TEST SETUP. ONLY USE FOR TESTING!

  Please download and install these libs to upload & run the programm successfully:
  Installation in Arduino IDE: Sketch -> Include Lib. -> Add .zip
  https://github.com/mathieucarbou/AsyncTCP
  https://github.com/knolleary/pubsubclient

  Uploading after writing 100% can fail, but the upload still worked... Check the Serial Monitor for logs.
  Compiling also takes a while
*/

// Adjust to personal settings to connect to your Wifi & MQTT server IP
const char* ssid = ""; // Wifi Name
const char* password = ""; // Wifi Pass
const char* mqtt_server = ""; // MQTT-Server IP 
const char* client_name = ""; // Unique Name of micro controller, as it will be seen on the mqtt network

char* hello_message = ""; // Hello msg or test msg
const char* topic = ""; // Default MQTT topic to publish to

WiFiClient espClient;
PubSubClient mqttclient(espClient);

unsigned long testStartTime;
unsigned long testDuration = 600000; // 10 minutes
unsigned long lastSendTime = 0;
unsigned long sendInterval = 2000;   // Send ping every 2 seconds
unsigned long sentTime = 0;

String results = ""; // CSV log

// Construct topic string like client_name/topic/subtopic
char* getTopicId(char* subTopic) {
  static char topicBuffer[100];
  snprintf(topicBuffer, sizeof(topicBuffer), "%s/%s/%s", client_name, topic, subTopic);
  return topicBuffer;
}

// Construct default subscription topic: client_name/#
char* getDefaultTopic() {
  static char defaultTopicBuffer[100];
  snprintf(defaultTopicBuffer, sizeof(defaultTopicBuffer), "%s/#", client_name);
  return defaultTopicBuffer;
}

// Send a message to the current topic
void send(char* message, char* subtopic) {
  char payload[256];
  snprintf(payload, sizeof(payload), "[%s] %s", client_name, message);
  mqttclient.publish(getTopicId(subtopic), payload);
}

// Reconnect if MQTT disconnected
void reconnect() {
  while (!mqttclient.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqttclient.connect(client_name)) {
      Serial.println("connected");
      mqttclient.subscribe(getDefaultTopic());
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttclient.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// Callback after MQTT message is received
void callback(char* topic, byte* message, unsigned int length) {
  String messageTemp = "";
  for (int i = 0; i < length; i++) {
    messageTemp += (char)message[i];
  }

  // Ignore own messages
  if (messageTemp.startsWith("[" + String(client_name) + "]")) return;

  // Do stuff with the topic
  if (String(topic).indexOf("echo") > 0) {
    // Measure RTT
    unsigned long now = millis();
    unsigned long rtt = now - sentTime;

    Serial.printf("Echo received: %s | RTT: %lums\n", messageTemp.c_str(), rtt);
    results += String(now) + "," + String(rtt) + "\n";
  }
}

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  mqttclient.setServer(mqtt_server, 1883);
  mqttclient.setCallback(callback);

  reconnect(); // Connect and subscribe

  testStartTime = millis();
}

void loop() {
  if (!mqttclient.connected()) {
    reconnect();
  }

  mqttclient.loop();

  unsigned long now = millis();

  // End test after 10 minutes and publish CSV
  if (now - testStartTime >= testDuration) {
    Serial.println("Test complete. Sending results...");

    // Split and publish line by line
    int lineStart = 0;
    int lineEnd;
    while ((lineEnd = results.indexOf('\n', lineStart)) != -1) {
      String line = results.substring(lineStart, lineEnd);
      send((char*)line.c_str(), (char*)"results");
      delay(50);  // Give broker time to receive
      lineStart = lineEnd + 1;
    }
    send("Test complete. Data sent to results.", (char*)"status");
    delay(1000); // Allow last message to send
    while (true); // Stop execution
  }

  // Send timestamp every 2 seconds
  if (now - lastSendTime >= sendInterval) {
    lastSendTime = now;
    sentTime = now;

    char msgBuffer[64];
    snprintf(msgBuffer, sizeof(msgBuffer), "%lu", sentTime);
    send(msgBuffer, (char*)"ping");
  }
}