/*
  AWS IoT client

 This sketch connects to AWS IoT using TLS 1.2 through a MKR GSM 1400 board.

 Circuit:
 * MKR GSM 1400 board
 * Antenna
 * SIM card with a data plan

 created 11 April 2018
 by Darren Jeacocke

{
   "vin":"1ZVHT80N175369058",
   "stock":"75369058",
   "color":"Forest Green",
   "make":"Ford",
   "model":"Mustang",
   "year":2007
}
*/

// libraries
#include <MKRGSM.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Thread.h>

#include "arduino_secrets.h"
#include "template.h"
// Please enter your sensitive data in the Secret tab or arduino_secrets.h
// PIN Number
const char PINNUMBER[]     = SECRET_PINNUMBER;
// APN data
const char GPRS_APN[]      = SECRET_GPRS_APN;
const char GPRS_LOGIN[]    = SECRET_GPRS_LOGIN;
const char GPRS_PASSWORD[] = SECRET_GPRS_PASSWORD;

// remove -ats from endpoint to use legacy root CA
const char server[] = "ah2fpu2q2t4m4.iot.us-east-1.amazonaws.com";
const char statusTopic[] = "printer/status";
const char printTopic[] = "printer/print";
const char updateTemplateTopic[] = "template/update";
const char clientId[] = "CPHPrinter1";

StaticJsonBuffer<1024> jsonBuffer;

GSMSSLClient gsmClient;
GSMSecurity profile;
GPRS gprs;
GSM gsmAccess;

Thread printThread = Thread();

PubSubClient mqttClient(server, 8883, gsmClient);

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println();
  
  if(String(topic) == printTopic) {
    sendPrint(payload, length);
  }
  else if(String(topic) == updateTemplateTopic) {
    updateTemplate(payload, length);
  }
}

void updateTemplate(byte* payload, unsigned int length) {

  char inData[length];
  for (int i=0;i<length;i++) {
    inData[i] = (char)payload[i];
  }

  // DEBUG
  for (int i=0;i<length;i++) {
    Serial.print(inData[i]);
  }
  Serial.println();

//  JsonObject& root = jsonBuffer.parseObject(inData);
//  String newTemplate = root["template"];

  printTemplate = String(inData);

  //  DEBUG
  Serial.print(printTemplate);
}

void sendPrint(byte* payload, unsigned int length) {

  char inData[500];
  String tempPrintTemplate = printTemplate;
  
  for (int i=0;i<length;i++) {
    inData[i] = (char)payload[i];
  }

  // DEBUG
  for (int i=0;i<length;i++) {
    Serial.print(inData[i]);
  }
  Serial.println();

  JsonObject& root = jsonBuffer.parseObject(inData);
  tempPrintTemplate.replace("{vin}", root["vin"]); 
  tempPrintTemplate.replace("{stock}", root["stock"]); 
  tempPrintTemplate.replace("{year}", root["year"]); 
  tempPrintTemplate.replace("{make}", root["make"]); 
  tempPrintTemplate.replace("{model}", root["model"]); 
  tempPrintTemplate.replace("{color}", root["color"]); 

  Serial1.print(tempPrintTemplate);
}

void sendPrintCB(){

}

void reconnect() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqttClient.connect(clientId)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      mqttClient.publish(statusTopic, "{d: { status: \"connected!\"}");
      // ... and resubscribe
      mqttClient.subscribe(printTopic);
      mqttClient.subscribe(updateTemplateTopic);
      digitalWrite(LED_BUILTIN, HIGH);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      digitalWrite(LED_BUILTIN, LOW);
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  // set up LED
  pinMode(LED_BUILTIN, OUTPUT);
  
  // initialize serial communications and wait for port to open:
  Serial.begin(9600);
  Serial1.begin(9600);
  
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  Serial.println("Starting Arduino MQTT client...");
  // connection state
  boolean connected = false;

  // After starting the modem with GSM.begin()
  // attach the shield to the GPRS network with the APN, login and password
  while (!connected) {
    if ((gsmAccess.begin(PINNUMBER) == GSM_READY) &&
        (gprs.attachGPRS(GPRS_APN, GPRS_LOGIN, GPRS_PASSWORD) == GPRS_READY)) {
      connected = true;
    } else {
      Serial.println("NETWORK Not connected");
      delay(1000);
    }
  }

  Serial.println("Importing certificates...");

  profile.setRootCertificate(SECRET_ROOT_CERT);
  profile.setClientCertificate(SECRET_CLIENT_CERT);
  profile.setPrivateKey(SECRET_PRIVATE_KEY);
  profile.setValidation(SSL_VALIDATION_ROOT_CERT);
  profile.setVersion(SSL_VERSION_TLS_1_2);
  profile.setCipher(SSL_CIPHER_AUTO);
  gsmClient.setSecurityProfile(profile);

  mqttClient.setCallback(mqttCallback);

  // Setup print Thread
  printThread.setInterval(100);
  printThread.enabled = true;
  printThread.onRun(sendPrintCB);

}

unsigned long prevNow = millis();

void loop() {
  
  if (!mqttClient.connected()) {
    reconnect();
  }

  mqttClient.loop();
}
