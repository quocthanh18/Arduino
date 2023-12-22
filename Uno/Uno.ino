#include <DHT.h>  
#include <ArduinoJson.h>
#include <SoftwareSerial.h>
#include <Servo.h>

//Pins
const int dht_pin = 7;
const int light_pin = A0;
const int moisture_power_pin = 8;
const int moisture_pin = A1;
const int relay_pin = 6;

//
const int wet = 400;
const int dry = 900;

SoftwareSerial send(13, 12); // RX, TX
DHT dht(dht_pin, DHT11); //temp

void setup(){

  Serial.begin(9600);
  send.begin(9600);
  pinMode(moisture_power_pin, OUTPUT); //moisture power
  pinMode(relay_pin, OUTPUT);
  pinMode(light_pin, INPUT);
  digitalWrite(moisture_power_pin, LOW);
  dht.begin();
}

int readMoisture() {
	digitalWrite(moisture_power_pin, HIGH);	
	delay(10);							
	int val = analogRead(moisture_pin);	
	digitalWrite(moisture_power_pin, LOW);		
	return val;							
}

unsigned long startTime = millis();
bool temp = false;
//Bên ngoài hàm

bool mainLogic() {
  if (!temp) {
    digitalWrite(relay_pin, HIGH);
    temp = true;
  } else {
    if (millis() - startTime > 3000) {
      temp = false;
      digitalWrite(relay_pin, LOW);
      if (readMoisture() > dry) {
        digitalWrite(relay_pin, LOW);
        return false;
      } else
        temp = false;
      return true;
    }
    if (readMoisture() < dry) {
      digitalWrite(relay_pin, LOW);
      return true;
    }
  }
  return false
}


void sendInfo(int temp, int light, int moisture, bool watered){
  StaticJsonDocument<200> sensor;
  sensor["temp"] = temp;
  sensor["moisture"] = moisture;
  sensor["light"] = light;
  sensor["watered"] = watered;
  serializeJson(sensor, send);
  send.println();
}

void receiveInfo()
{
  if ( send.available())
  {
    bool turn_relay = send.readString() == "true" ? 1 : 0;
    digitalWrite(relay_pin, turn_relay);
    delay(5000);
    digitalWrite(relay_pin, !turn_relay);
  }
}

void loop(){
  int temp = dht.readTemperature();
  int moisture = readMoisture();
  int light = analogRead(light_pin);
  bool watered = mainLogic();
  sendInfo(temp, light, moisture, watered);
  Serial.println(temp);
  Serial.println(moisture);
  Serial.println(light);
  Serial.println(watered);
  receiveInfo();
  delay(15000);
}
