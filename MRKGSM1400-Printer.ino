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
  "vin":"1ZVHT80N175369058"
  "color":"Forest Green",
  "make":"Ford",
  "model":"Mustang",
  "stock":"75369058",
  "vin":"1ZVHT80N175369058",
  "year":2007
}
*/

// libraries
#include <MKRGSM.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

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
const char clientId[] = "CPHPrinter1";
String TEST_Template = "~CT~~CD,~CC^~CT~^XA~TA000~JSN^LT0^MNW^MTT^POI^PMN^LH0,0^JMA^PR4,4~SD30^JUS^LRN^CI0^XZ^XA^MMT^PW609^LL1218^LS0**BOTTOM LARGE MAC QR**^FO160,850^BQ,2,10^FDHM,A{mac}^FS**BOTTOM SMALL MAC**^FT135,930^A0R,16,16^FB220,1,0,C^FD{mac}^FS^PQ1,0,1,Y^XZ";
StaticJsonBuffer<500> jsonBuffer;
char inData[500];

GSMSSLClient gsmClient;
GSMSecurity profile;
GPRS gprs;
GSM gsmAccess;

PubSubClient mqttClient(server, 8883, gsmClient);

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
    inData[i] = (char)payload[i];
  }
  Serial.println();

  JsonObject& root = jsonBuffer.parseObject(inData);
  printTemplate.replace("{vin}", root["vin"]); 

  Serial1.print(printTemplate);
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
      Serial.println("Not connected");
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

  mqttClient.setCallback(callback);
}

unsigned long prevNow = millis();

void loop() {
//  unsigned long now = millis();
//  if (now - prevNow >= 30000) {
//    prevNow = now;
//    if (mqttClient.publish(statusTopic, "{d: { status: \"connected!\"}")) {
//      Serial.println("Publish ok");
//    } else {
//      Serial.println("Publish failed");
//    }
//  }

  if (!mqttClient.connected()) {
    reconnect();
  }

  mqttClient.loop();
}
