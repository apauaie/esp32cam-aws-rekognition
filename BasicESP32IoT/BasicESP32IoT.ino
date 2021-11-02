#include "esp_http_client.h"
#include "esp_camera.h"
#include <WiFi.h>
#include "Arduino.h"
#include "Base64.h"
#include "mbedtls/base64.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "secrets.h"
#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>


// The MQTT topics that this device should publish/subscribe

#define AWS_IOT_SUBSCRIBE_DATA_TOPIC "touching/topic"

bool internet_connected = false;


#define LED 33   //built in LED of AI Thinker ESP32 CAM
#define TOUCH 14

WiFiClientSecure net = WiFiClientSecure();
MQTTClient client1 = MQTTClient(512);

String filename; //Global variable Filename

void connectAWS()
{

  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client1.begin(AWS_IOT_ENDPOINT, 8883, net);

  // Create a message handler
  client1.onMessage(messageHandler);

  Serial.print("Connecting to AWS IOT");

  while (!client1.connect(THINGNAME)) {
    Serial.print(".");
    delay(100);
  }

  if (!client1.connected()) {
    Serial.println("AWS IoT Timeout!");
    return;
  }

  client1.subscribe(AWS_IOT_SUBSCRIBE_DATA_TOPIC);
  
  Serial.println("\n AWS IoT Connected!");
}


void messageHandler(String &topic, String &payload) {

  Serial.println("incoming : " + payload);
 
}



void setup()
{
  
  pinMode (LED, OUTPUT);
  pinMode (TOUCH, INPUT);
  digitalWrite(LED,HIGH);
  Serial.begin(9600);

  if (init_wifi()) { // Connected to WiFi
    internet_connected = true;
    Serial.println("Internet connected");
  }


   connectAWS();

}


void publishMessage(String paylod)
{
  StaticJsonDocument<200> doc;
  doc["message"] = paylod;

  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client
  Serial.println(jsonBuffer);

  client1.publish(AWS_IOT_SUBSCRIBE_DATA_TOPIC, jsonBuffer);

}


bool init_wifi()
{
  int connAttempts = 0;
  Serial.println("\r\nConnecting to: " + String(ssid));
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED ) {
    delay(500);
    Serial.print(".");
    if (connAttempts > 10) return false;
    connAttempts++;
  }
  return true;
}




void loop()
{
  // TODO check Wifi and reconnect if needed
    int buttonState = touchRead(TOUCH);
    Serial.print(buttonState);
    delay(200);
    client1.loop();
   if (buttonState <50) {
      digitalWrite(LED,LOW);
      Serial.print("Sending Data to MQTT");
      publishMessage("Touching ESP32");
      delay(2000); //wait to ensure single click button
  } 
  

  
}
