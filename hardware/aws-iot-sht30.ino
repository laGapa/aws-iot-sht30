/* Commited by sp0f <sp0f(at)gawra.net.pl> 

   Based on ESP8266 AWS IoT example by Evandro Luis Copercini:
   https://github.com/copercini/esp32-iot-examples

   Needed libraries:
   - PubSubClient https://pubsubclient.knolleary.net/
   - WEMOS_SHT3x - https://github.com/wemos/WEMOS_SHT3x_Arduino_Library

   You will also need "Arduino ESP8266 filesystem uploader" to upload certificate: 
   https://github.com/esp8266/arduino-esp8266fs-plugin
*/

#include "FS.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WEMOS_SHT3X.h>

SHT3X sht30(0x45);

// Update these with values suitable for your network.

const char* ssid = "<wifi_ssid>";
const char* password = "<wifi_password>";

const char* AWS_endpoint = "<endpoint_id>.iot.<region>.amazonaws.com"; //MQTT broker ip


int chipId = ESP.getChipId();

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("[*] Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

}
WiFiClientSecure espClient;
PubSubClient client(AWS_endpoint, 8883, callback, espClient); //set  MQTT port number to 8883 as per //standard


void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("[*] Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("[*] WiFi connected");
  Serial.print("[?] IP address: ");
  Serial.println(WiFi.localIP());
}


void reconnect() {
  // Loop until we're reconnected
  if (!espClient.connected()) {
    Serial.println("[*] SSL socked disconnected. Ceconnecting ...");
    ssl_config();   
  }
  
  while (!client.connected()) {
    Serial.print("[*] Attempting AWS IoT MQTT connection...");
    // Attempt to connect
    if (client.connect("ESPthing")) {
      Serial.println("[*] MQTT connected");
      // Once connected, publish an announcement...
      // client.publish("outTopic", "hello world");
      // ... and resubscribe
      //client.subscribe("inTopic");
    } else {
      Serial.print("[!] MQTT connection failed, client state=");
      Serial.println(client.state());
      Serial.println("[?] Will try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {

  Serial.begin(115200);
  Serial.setDebugOutput(true);
  setup_wifi();
  delay(1000);
  if (!SPIFFS.begin()) {
    Serial.println("[!] Failed to mount file system");
    return;
  }

  ssl_config();
  
  Serial.print("[?] Heap: "); Serial.println(ESP.getFreeHeap());
}


void sleep(int seconds) {
  delay(seconds*1000);
}

void ssl_config() {
  if (!SPIFFS.begin()) {
      Serial.println("[!] Failed to mount file system");
      return;
    }
  
    Serial.print("[?] Initial heap: "); Serial.println(ESP.getFreeHeap());
  
    // Load certificate file
    File cert = SPIFFS.open("/cert.der", "r"); //replace cert.crt eith your uploaded file name
    if (!cert) {
      Serial.println("[!] Failed to open cert file");
    }
    else
      Serial.println("[*] Successfully opened cert file");
  
    delay(1000);
  
    if (espClient.loadCertificate(cert))
      Serial.println("[*] Cert loaded");
    else
      Serial.println("[!] Cert not loaded");
  
    // Load private key file
    File private_key = SPIFFS.open("/private.der", "r"); //replace private eith your uploaded file name
    if (!private_key) {
      Serial.println("[!] Failed to open private cert file");
    }
    else
      Serial.println("[*] Successfully opened private cert file");
  
    delay(1000);
  
    if (espClient.loadPrivateKey(private_key))
      Serial.println("[*] Private key loaded");
    else
      Serial.println("[!] Private key not loaded");
  
}

void loop() {
  char topic_buf[32];
  char message_buf[64];
  
  sprintf(topic_buf, "gawra/ESP8266/%d", chipId);
  const char* topic = topic_buf;
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("[!] Wifi disconnected. Reconnecting ...");
    setup_wifi();
  }

  if (!client.connected()) {
    reconnect();
  }

 
  
  client.loop();
  Serial.println("\n------------");
  
  // build message from sensor data
  if (sht30.get() == 0) {
    char message_buf[64];
    sprintf(message_buf,"{ temperature: %2.2f, humidity: %2.2f }", sht30.cTemp, sht30.humidity);
    
  } else {
    Serial.println("[!] Error while receiving sensor data");
    sprintf(message_buf,"{ temperature: 0, humidity: 0 }");
  }

  const char* message = message_buf;
  
  Serial.print("[*] Publish message: ");
  Serial.println(message);
    
  if (client.publish(topic, message)) {
    Serial.println("[*] Message sent successfully");
    sleep(120);
  } else {
      Serial.println("[!] Error when sending message");
      //client.disconnect();
      //WiFi.disconnect();
  }
  Serial.print("Heap: "); Serial.println(ESP.getFreeHeap()); //Low heap can cause problems
}

