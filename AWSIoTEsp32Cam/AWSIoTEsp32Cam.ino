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

#define AWS_IOT_PUBLISH_DATA_TOPIC   "esp32/pub/recognize"
#define AWS_IOT_SUBSCRIBE_DATA_TOPIC "esp32/sub/recognize"


#define LED 33   //built in LED of AI Thinker ESP32 CAM


// Change the WIFI name and password
// Change the awswebaddress below

//Recognition Element
String imagecompare="Floor";


int capture_interval = 10000; // Microseconds between captures

bool internet_connected = false;
long current_millis;
long last_capture_millis = 0;

// CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22



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

  // Subscribe to a topic
//  client1.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
  client1.subscribe(AWS_IOT_SUBSCRIBE_DATA_TOPIC);
  
  Serial.println("\n AWS IoT Connected!");
}


void messageHandler(String &topic, String &payload) {

  Serial.println("incoming Recognition: " + payload);
  int recognize = payload.indexOf(imagecompare);
  
  if (recognize!=-1){
    digitalWrite(LED,LOW);
  }
  
  else {
    digitalWrite(LED,HIGH);
  }
  
}



void setup()
{
  
  pinMode (LED, OUTPUT);
  digitalWrite(LED,HIGH);
  Serial.begin(9600);

  if (init_wifi()) { // Connected to WiFi
    internet_connected = true;
    Serial.println("Internet connected");
  }

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  //init with high specs to pre-allocate larger buffers
  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

    connectAWS();

}


void publishMessageDATA(String paylod)
{
  StaticJsonDocument<200> doc;
  doc["payload"] = paylod;

  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client
  Serial.println(jsonBuffer);

  client1.publish(AWS_IOT_PUBLISH_DATA_TOPIC, jsonBuffer);
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


esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
  switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
      Serial.println("HTTP_EVENT_ERROR");
      break;
    case HTTP_EVENT_ON_CONNECTED:
      Serial.println("HTTP_EVENT_ON_CONNECTED");
      break;
    case HTTP_EVENT_HEADER_SENT:
      Serial.println("HTTP_EVENT_HEADER_SENT");
      break;
    case HTTP_EVENT_ON_HEADER:
      Serial.println();
      Serial.printf("HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
      break;
    case HTTP_EVENT_ON_DATA:
      Serial.println();
      Serial.printf("HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
      if (!esp_http_client_is_chunked_response(evt->client)) {
        // Write out data
        // printf("%.*s", evt->data_len, (char*)evt->data);
      }
      break;
    case HTTP_EVENT_ON_FINISH:
      Serial.println("");
      Serial.println("HTTP_EVENT_ON_FINISH");
      break;
    case HTTP_EVENT_DISCONNECTED:
      Serial.println("HTTP_EVENT_DISCONNECTED");
         if (filename!=""){
        publishMessageDATA("images/Recognition/"+filename + ".jpg");
        int x=10000;
        while (x>0){
        client1.loop();
        x--;
      }
    
  
  }
      break;
  }
  return ESP_OK;
}

static esp_err_t take_send_photo()
{
  Serial.println("Taking picture...");
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;


 
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return ESP_FAIL;
  }
  
   
  int image_buf_size = 4000 * 1000;                                                  
 uint8_t *image = (uint8_t *)ps_calloc(image_buf_size, sizeof(char));

 size_t length=fb->len;

 size_t olen;

 Serial.print("Picture length is ");

 Serial.println(length);
 int err1 = mbedtls_base64_encode(image, image_buf_size, &olen, fb->buf, length);



  esp_http_client_handle_t http_client;
  
  esp_http_client_config_t config_client = {0};

   WiFiUDP ntpUDP;
   NTPClient timeClient(ntpUDP);
   timeClient.begin();
   timeClient.update();
   filename =  String(timeClient.getEpochTime());
   Serial.print("Filename:" );  Serial.print(filename+".jpg \n");
   String folder = "Recognition";
   String post_url2 = AWS_API_ENDPOINT + "/" + folder + "/" + filename; // Location where images are POSTED
   char post_url3[post_url2.length() + 1];
   post_url2.toCharArray(post_url3, sizeof(post_url3));
  
  config_client.url = post_url3;
  config_client.event_handler = _http_event_handler;
  config_client.method = HTTP_METHOD_POST;

  http_client = esp_http_client_init(&config_client);

  esp_http_client_set_post_field(http_client, (const char *)fb->buf, fb->len);
  

  esp_http_client_set_header(http_client, "Content-Type", "image/jpg");

  esp_err_t err = esp_http_client_perform(http_client);
  if (err == ESP_OK) {
    Serial.print("esp_http_client_get_status_code: ");
    Serial.println(esp_http_client_get_status_code(http_client));
  }

  esp_http_client_cleanup(http_client);

  esp_camera_fb_return(fb);

  
}



void loop()
{
  // TODO check Wifi and reconnect if needed
  
  current_millis = millis();
  if (current_millis - last_capture_millis > capture_interval) { // Take another picture
    last_capture_millis = millis();
    take_send_photo();
    
  } 
  
  
}
