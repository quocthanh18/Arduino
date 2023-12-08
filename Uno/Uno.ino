#include <DHT.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>
#include <Servo.h>
#include <microTuple.h>

//Pins
const int dht_pin = 7;
const int light_pin = A0;
const int moisture_power_pin = 8;
const int moisture_pin = A1;
const int servo_pin = 9;
const int relay_pin = 6;

//
const int wet = 400;
const int dry = 850;

Servo servo;
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
  servo.attach(servo_pin);
}
void openServo(){
  servo.write(90);
}

void closeServo(){
  servo.write(0);
}
int readMoisture() {
	digitalWrite(moisture_power_pin, HIGH);	
	delay(10);							
	int val = analogRead(moisture_pin);	
	digitalWrite(moisture_power_pin, LOW);		
	return val;							
}

bool mainLogic(int temp, int light, int moisture){
  bool watered = false;
  if( moisture < wet || moisture > dry){
	  if( moisture < wet)
	    openServo();
    } else if(moisture > dry ) {
  	  closeServo();
	    //RELAY CALL
	    digitalWrite(relay_pin, HIGH);
      delay(5000);
      digitalWrite(relay_pin, LOW);
      watered = true;
	    //RELAY STOP
      }
    if ( readMoisture() > dry )
      closeServo();
    else if ( readMoisture() < wet )
      openServo();
    return watered;
}

void sendInfo(int temp, int light, int moisture, bool watered, bool servo_state){
  StaticJsonDocument<200> sensor;
  sensor["temp"] = temp;
  sensor["moisture"] = moisture;
  sensor["light"] = light;
  sensor["watered"] = watered;
  sensor["servo_state"] = servo_state;
  serializeJson(sensor, send);
  send.println();
}
void loop(){
  int temp = dht.readTemperature();
  int moisture = readMoisture();
  int light = analogRead(light_pin);
  bool watered = mainLogic(temp, light, moisture);
  sendInfo(temp, light, moisture, watered, servo.read());
  delay(15000);
  int pos;

}

