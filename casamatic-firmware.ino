#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include <RF24Network.h>
#include <RF24.h>
#include <RF24Mesh.h>
#include <printf.h>
#include <EEPROM.h>

#include <Wire.h>

const char* ssid = "Platillo";
const char* password =  "supermosca";
const char* mqttServer = "broker.shiftr.io";
const int mqttPort = 1883;
const char* mqttUser = "try";
const char* mqttPassword = "try";
 
WiFiClient espClient;
PubSubClient client(espClient);

RF24 radio(2, 15); // CE, CSN

RF24Network network(radio);
RF24Mesh mesh(radio, network);

////////////////

int temperatura;                                                                                                                                                                                                                                                                                                                                                                                                                                                     
int tempSet;
int tempInputMultiplo = 1;
int tempInputAux;
int tempPrograma;
int contador;

#define STATE_PROGRAM_IDLE 0
#define STATE_CALENTANDO 1
#define STATE_FINISHED 2

int state = STATE_PROGRAM_IDLE;
int stateIdleBoot = 0;
int stateCalentandoBoot = 0;
int stateFinishedBoot = 0;

////////////////

struct payload_t {
  byte command;
  byte temperatura;
};

#define COMMAND_CALENTAR 1
#define COMMAND_ENCENDER 2
#define COMMAND_APAGAR 3
#define COMMAND_SWITCH 4

struct event_t {
  char from[5];
  byte device_type;
  byte event_type;
  char to[5];
  byte data;
};

#define DEVICE_TYPE_CASAMATIC 1
#define DEVICE_TYPE_SENSORMATIC 2
#define DEVICE_TYPE_TECLAMATIC 3

#define SENSORMATIC_EVENT_PRESENT 0
#define SENSORMATIC_EVENT_CURRENT_TEMP 1 // Current Temp
#define SENSORMATIC_EVENT_SET_TEMP 2 // New Set Temp
#define SENSORMATIC_EVENT_SET_MODE 3

#define CASAMATIC_EVENT_DEVICE_ADDED 1

#define I2C_ADDRESS 0x4

char mac[5] = {'c', 'c', 'c', '0', 0};

////////////////////// OLED
#include "SH1106Wire.h"   // legacy: #include "SH1106.h"

SH1106Wire display(0x3c, SDA, SCL);     // ADDRESS, SDA, SCL

//SH1106Wire display(0x3c, D2, D1);     // ADDRESS, SDA, SCL

/*void drawFontFaceDemo() {
    // Font Demo1
    // create more fonts at http://oleddisplay.squix.ch/
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 0, "Hello world");
    display.setFont(ArialMT_Plain_16);
    display.drawString(0, 10, "Hello world");
    display.setFont(ArialMT_Plain_24);
    display.drawString(0, 26, "Hello world");
}

void drawTextFlowDemo() {
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.drawStringMaxWidth(0, 0, 128,
      "Lorem ipsum\n dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore." );
}

void drawTextAlignmentDemo() {
    // Text alignment demo
  display.setFont(ArialMT_Plain_10);

  // The coordinates define the left starting point of the text
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 10, "Left aligned (0,10)");

  // The coordinates define the center of the text
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 22, "Center aligned (64,22)");

  // The coordinates define the right end of the text
  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  display.drawString(128, 33, "Right aligned (128,33)");
}

void drawProgressBarDemo() {
  int progress = (counter / 5) % 100;
  // draw the progress bar
  display.drawProgressBar(0, 32, 120, 10, progress);

  // draw the percentage as String
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 15, String(progress) + "%");
}*/

#define PULSADOR_A 1
#define PULSADOR_B 3
#define PULSADOR_C 16

#define RF_CHANNEL 44

///////////////////////////

void musiquita(byte i) {
  //Serial.print("beginTransmission");
  Wire.beginTransmission(I2C_ADDRESS);
  Wire.write(i);
  Wire.endTransmission();
  Serial.print("endTransmission");
}

void radioSetup() {
  Serial.println("Setup Radio");
  printf_begin();

  mesh.setNodeID(0);
  mesh.begin(RF_CHANNEL, RF24_250KBPS, 1000);
  mesh.loadDHCP();

  radio.printDetails();
  Serial.println("RF Mesh ready");
}

void setup() { 
  Serial.begin(115200);
  Serial.println();
  Serial.println();

  display.init();
  display.setFont(ArialMT_Plain_10);

  display.clear();  
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 10, "Booteando...");
  display.display();

  pinMode(PULSADOR_A, FUNCTION_3);
  pinMode(PULSADOR_B, FUNCTION_3);
  pinMode(PULSADOR_C, FUNCTION_3);

  Wire.begin();
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");

    display.clear();  
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 10, "Conectandose a\nla red WiFi...");
    display.display();
  }
 
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
 
  while (!client.connected()) {
    display.clear();  
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 10, "Conectandose a\nMQTT...");
    display.display();
 
    if (!client.connect("ESP8266Client", mqttUser, mqttPassword )) {
      display.clear();
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.drawString(64, 10, "Error conectando\na MQTT...");
      display.display();
      delay(2000);
    }
  }

  display.clear();  
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 10, "Configurando\nNRF24L01+...");
  display.display();
  radioSetup();

  display.clear();  
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 10, "Conectandose\nal server...");
  display.display();
    
  client.subscribe("esp/test");

  display.clear();  
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 10, "CasaMatic v0.2");
  display.display();

  stateIdleBoot = 1;
}

void sendT(byte tem) {
  payload_t rf_payload = { COMMAND_CALENTAR, tem};
  Serial.println("Enviando...");
  bool ok = mesh.write(&rf_payload, 'M', sizeof(rf_payload));// , 1);
  Serial.println(ok);
  if (ok) {
    Serial.println("ok");
  } else {
    //
  }  
}

void sendTeclamatic(byte command) {
  payload_t rf_payload = { command, 0};
  Serial.println("Enviando...");
  bool ok = mesh.write(&rf_payload, 'M', sizeof(rf_payload));// , 1);
  Serial.println(ok);
  if (ok) {
    Serial.println("ok");
  } else {
    //
  }  
}
 
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
 
  Serial.println("Temperatura:");
  Serial.println(payload[0]);

  payload_t rf_payload = { COMMAND_CALENTAR, payload[0] };
  bool ok = mesh.write(&rf_payload, 'M', sizeof(rf_payload));
  if (ok) {
    Serial.println("ok"); // TODO blink built-in LED
  } else {
    Serial.println("failed");  // TODO send back MQTT error, track entire flow with request ID (h)
  }
}
 
void loop() {
  mesh.update();
  mesh.DHCP();

  client.loop();

  if (state == STATE_PROGRAM_IDLE) {
    if (stateIdleBoot == 1) {
      tempSet = 0;
      tempInputMultiplo = 1;
      stateIdleBoot = 0;
    }
  } else if (state == STATE_CALENTANDO) {
    if (stateCalentandoBoot == 1) {
      Serial.println("Calentar a");
      Serial.println(tempSet);
      sendT((byte)tempSet);
      stateCalentandoBoot = 0;
      state = STATE_PROGRAM_IDLE;
      stateIdleBoot = 1;
    }
            
  }

  if (network.available()) {
    mesh.saveDHCP();
      
    RF24NetworkHeader header;
    network.peek(header);

    event_t event;
    switch(header.type){
      // Display the incoming millis() values from the sensor nodes
      case 'M':
        {
        network.read(header, &event, sizeof(event));
        char from[5] = { 0 };
        char to[5] = { 0 };
        memcpy(from, event.from, sizeof(char) * 5);
        memcpy(to, event.to, sizeof(char) * 5);

        display.clear();  
        display.setTextAlignment(TEXT_ALIGN_CENTER);
        display.drawString(64, 10, "f: " + String(from) + "\ndev_type: " + String(event.device_type, DEC) + "\nev_type: " + String(event.event_type, DEC) + "\ndata: " + String(event.data, DEC));
        display.display();
        
        /*if (event.device_type == DEVICE_TYPE_TERMOMATIC) {
          if (event.event_type == TERMOMATIC_EVENT_DONE) {
            client.publish("esp/user", "Ya esta lista el agua calentita!");
            musiquita(1);
          }
          if (event.event_type == TERMOMATIC_EVENT_UPDATE) {
            // client.publish("esp/user", "Ya esta lista el agua calentita!");
          }
        } else */
        
        if (event.device_type == DEVICE_TYPE_SENSORMATIC) {
          if (event.event_type == SENSORMATIC_EVENT_PRESENT) {
            event_t payload;
            payload.device_type = DEVICE_TYPE_CASAMATIC;
            payload.event_type = CASAMATIC_EVENT_DEVICE_ADDED;
            payload.data = 0;
            memcpy(payload.from, mac, sizeof(char) * 5);
            memcpy(payload.to, event.from, sizeof(event.from) * 5);

            bool ok = mesh.write(&payload, 'M', sizeof(payload));

            char from[5] = { 0 };
            char to[5] = { 0 };
            memcpy(from, payload.from, sizeof(char) * 5);
            memcpy(to, payload.to, sizeof(char) * 5);
        
            display.clear();
            display.setTextAlignment(TEXT_ALIGN_CENTER);
            display.drawString(64, 10, "from: " + String(from) + "\nto: " + String(to));
            display.display();
            if (ok) {
              Serial.println("ok");
            } else {
              // TODO
              delay(1000);
              display.clear();
              display.setTextAlignment(TEXT_ALIGN_CENTER);
              display.drawString(64, 10, "Error!");
              display.display();
            }
          } else if (event.event_type == SENSORMATIC_EVENT_CURRENT_TEMP) {
            if (event.data >= tempSet) {
              //sendTeclamatic(COMMAND_APAGAR); // estufa
              sendTeclamatic(COMMAND_ENCENDER); // ventilador
            } else {
              // sendTeclamatic(COMMAND_ENCENDER); // estufa
              sendTeclamatic(COMMAND_APAGAR); // ventilador
            }
          } else if (event.event_type == SENSORMATIC_EVENT_SET_TEMP) {
            tempSet = event.data;
            sendTeclamatic(COMMAND_ENCENDER);
          }
        }
        break;
        }
      default:
        network.read(header,0,0);
        Serial.println(header.type);
        break;
    }
  }
}
