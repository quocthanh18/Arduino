#include <Wire.h>
#include <Adafruit_SH110X.h>
#include <EMailSender.h>
#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <ThingSpeak.h>
#include "bytearray.h"
#define i2c_Address 0x3c
#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels
#define OLED_RESET -1
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
//SoftwareSerial setup
SoftwareSerial sensor(14, 12);  //rx, tx
//WiFi setup
WiFiClient WiFiClient;
const char* account = "Thanhdeptrai";
const char* password = "0906264681";

//MQTT setup
const char* mqttServer = "broker.hivemq.com";  //test.mosquitto.org //mqtt.eclipse.org //mqtt.flespi.io
const int port = 1883;
PubSubClient mqttClient(WiFiClient);

//Email setup
EMailSender emailSend("thanhwtf95@gmail.com", "dzcv rifo wtxv oeos");
const char * emailUser = "thanhwtf95@gmail.com";

//Cloud setup
const unsigned long channelID = 2373265;
const char * API = "SSJD58O3H2KT2UVI";

const int wet = 400;
const int dry = 850;
const int hot = 40;
const int cold = 10;
const int dark = 1000;
const int bright = 200;
const int sensorThresholds[3][2] = {
  {cold, hot}, 
  {wet, dry},    
  {bright, dark} 
};
void WiFiConnect() {
  WiFi.hostname("ESP8266 CH340G");
  WiFi.begin(account, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void mqttConnect() {
  while (!mqttClient.connected()) {
    Serial.println("Attempting MQTT connection...");
    String ClientID = "ESP8266";
    ClientID += String(random(0xffff), HEX);
    if (mqttClient.connect(ClientID.c_str())) {
      Serial.println("Connected");
      //Topics
      mqttClient.subscribe("21127428_21127273/Water_btn");
      
    } else {
      Serial.println("Trying again in 5 seconds");
      delay(5000);
    }
  }
}

void sendEmailNoti(const char* subject, const char* message_, const char* email) {
  EMailSender::EMailMessage message;
  message.subject = subject;
  message.message = message_;
  EMailSender::Response resp = emailSend.send(email, message);
  Serial.println("Sending status: ");
  Serial.println(resp.status);
  Serial.println(resp.code);
  Serial.println(resp.desc);
}
void handleNormalNoti(const int value[], bool watered, const char* email){
  String fullTopic = "Arduino Smart Pot";
  const char* cstrTopic = fullTopic.c_str();
  String msg;
  String attr[3] = {"Temperature", "Soil Moisture", "Light"};
  for ( int i = 0; i < 3; i++)
    msg += attr[i] + ": " + String(value[i]) + '\n';
  msg += watered ? "Watered: Yes" : "Watered: No";
  const char* msg_ = msg.c_str();
  sendEmailNoti(cstrTopic, msg_, email);
}

void handleEmergencyNoti(const int value[], const char* email){
  String fullTopic = "Arduino Smart Pot Emergency";
  const char* cstrTopic = fullTopic.c_str();
  String msg;
  String attr[3] = {"TEMPERATURE", "SOIL MOISTURE", "LIGHT"};
  for ( int i = 0; i < 3; i++)
  {
    if ( (value[i] < sensorThresholds[i][0]) || (value[i] > sensorThresholds[i][1]))
    {
      msg += attr[i] + " IS NOT IDEAL!!!";
      break;
    }
  }
  if (msg.length() > 0) {
    const char* msg_ = msg.c_str();
    sendEmailNoti(cstrTopic, msg_, email);
  }
}

void callback(char* topic, byte* message, unsigned int length) {
  StaticJsonDocument<100> info;  
  char strMsg[length + 1]; 
  for (int i = 0; i < length; i++) {
    strMsg[i] = (char)message[i];  
  }
  strMsg[length] = '\0';  
  Serial.println(topic);
  //***Code here to process the received package***
  if ( String(topic) == "21127428_21127273/Water_btn")
    sensor.write(strMsg);

}
void displayOLED(const int sensors[3]){
  for (int i = 0; i < 3; i++) {
    display.drawBitmap(0, 0, myBitmapArray[i], 128, 64, 1);
    display.setTextColor(1);
    display.setTextSize(2);
    display.setCursor(64, 27);
    display.println(sensors[i]);
    display.display();
    delay(5000);  
    display.clearDisplay();
  }
}
void setup() {
  Serial.begin(9600);
  sensor.begin(9600);
  display.begin();
  display.display();
  display.clearDisplay();
  Serial.print("Connecting to WiFi: ");
  WiFiConnect();
  ThingSpeak.begin(WiFiClient);
  mqttClient.setServer(mqttServer, port);
  mqttClient.setCallback(callback);
  mqttClient.setKeepAlive( 90 );
}
void websiteHandler(const int sensors[], bool watered){
  mqttClient.publish("21127428_21127273/Temp", String(sensors[0]).c_str());
  mqttClient.publish("21127428_21127273/Moist", String(sensors[1]).c_str());
  mqttClient.publish("21127428_21127273/Light", String(sensors[2]).c_str());
  const char* watered_ = watered ? "Yes" : "No";
  mqttClient.publish("21127428_21127273/Water_state", watered_);
} 

void cloudHandler(const int sensors[]){
  ThingSpeak.setField(1, sensors[0]);
  ThingSpeak.setField(2, sensors[1]);
  ThingSpeak.setField(3, sensors[2]);
  ThingSpeak.writeFields(channelID, API);
}

void loop() {
  // If MQTT is not connected, attempt to connect.
  if (!mqttClient.connected()) {
    mqttConnect();
  }
  mqttClient.loop();

  int sensors[3] = {0, 0, 0};
  bool watered = false;
  if (sensor.available()) {
    char json[200];
    int index = 0;
    while (sensor.available() && index < sizeof(json) - 1) {
      json[index++] = (char)sensor.read();  
    }
    json[index] = '\0';  

    StaticJsonDocument<200> info;  
    DeserializationError error = deserializeJson(info, json);
    if (!error) {
      sensors[0] = info["temp"];
      sensors[1] = info["moisture"];
      sensors[2] = info["light"];
      watered = info["watered"];
    } else {
      Serial.print(F("deserializeJson() failed: "));  
      Serial.println(error.c_str());
    }
  }
  // Display bitmap images.
  displayOLED(sensors);

  // Notify user
  // handleNormalNoti(sensors, watered, emailUser);
  // handleEmergencyNoti(sensors, emailUser);

  //Send info to website
  websiteHandler(sensors, watered);

  //Send info to cloud
  // cloudHandler(sensors);

  delay(5000);
}
