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
  byte device_type;
  byte event_type;
  uint16_t data;
};

#define DEVICE_TYPE_SENSORMATIC 5
#define DEVICE_TYPE_TECLAMATIC 6
#define TERMOMATIC_EVENT_DONE 1
#define TERMOMATIC_EVENT_UPDATE 2

#define SENSORMATIC_EVENT_CURRENT_TEMP 1 // Current Temp
#define SENSORMATIC_EVENT_SET_TEMP 2 // New Set Temp

#define I2C_ADDRESS 0x4

////////////////////// OLED
#include "SH1106Wire.h"   // legacy: #include "SH1106.h"
SH1106Wire display(0x3c, D2, D1);     // ADDRESS, SDA, SCL

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
  // SPI.begin();
  printf_begin();

  mesh.setNodeID(0);
  mesh.begin(40, RF24_250KBPS, 1000);
  mesh.loadDHCP();

  radio.printDetails();
  Serial.println("RF Mesh ready");
}

void setup() { 
  Serial.begin(115200);
  Serial.println();
  Serial.println();

  // Initialising the UI will init the display too.
  display.init();
  //display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);

  display.clear();  
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 10, "Booteando...");
  display.display();

  //pinMode(PULSADOR_A, INPUT);
  //pinMode(PULSADOR_B, INPUT);
  //pinMode(PULSADOR_C, INPUT);

  pinMode(PULSADOR_A, FUNCTION_3);
  pinMode(PULSADOR_B, FUNCTION_3);
  pinMode(PULSADOR_C, FUNCTION_3);


  Wire.begin();

  //musiquita(1); // testing
  
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");

    display.clear();  
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 10, "Conectandose a\nla red WiFi...");
    display.display();

    /*lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Conectando");
    lcd.setCursor(0, 1);
    lcd.print("al Wifi...");*/
  }
  Serial.println("Connected to the WiFi network");
 
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
 
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");

    display.clear();  
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 10, "Conectandose a\nMQTT...");
    display.display();
 
    if (client.connect("ESP8266Client", mqttUser, mqttPassword )) {
      Serial.println("connected");
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
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
    /*lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Joya!");
    lcd.setCursor(0, 1);
    lcd.print("Calentando...");*/
  } else {
    /*
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Error!");
    lcd.setCursor(0, 1);
    lcd.print("Resetear termomatic");*/
  }  
}

void sendTeclamatic(byte command) {
  payload_t rf_payload = { command, 0};
  Serial.println("Enviando...");
  bool ok = mesh.write(&rf_payload, 'M', sizeof(rf_payload));// , 1);
  Serial.println(ok);
  if (ok) {
    Serial.println("ok");
    /*lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Switch!");*/
  } else {
    /*
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Error!");
    lcd.setCursor(0, 1);
    lcd.print("Revisar Teclamatic");*/
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
    /*
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Joya!");
    lcd.setCursor(0, 1);
    lcd.print("Calentando...");*/
  } else {
    /*
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Error!");
    lcd.setCursor(0, 1);
    lcd.print("Resetear termomatic");*/
    Serial.println("failed");  // TODO send back MQTT error, track entire flow with request ID (h)
  }
  /*
  n = payload[0];
  Serial.print(n);
  musiquita(n);
 
  Serial.println();
  Serial.println("-----------------------");
  */
}
 
void loop() {
  mesh.update();
  mesh.DHCP();

  client.loop();

  if (state == STATE_PROGRAM_IDLE) {
    if (stateIdleBoot == 1) {
      tempSet = 0;
      tempInputMultiplo = 1;
      /*lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("CasaMatic v0.1");
      lcd.setCursor(0, 1);
      lcd.print("----------------");*/           
      stateIdleBoot = 0;
    }

    bool processNumero = false;

  /*
    display.clear();  
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 10, "Apretate algo");
    display.display(); */

  /*
    if (digitalRead(PULSADOR_A) == HIGH) {
      display.clear();  
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.drawString(64, 10, "Pulsador A");
      display.display();
      delay(1000);
    }
    if (digitalRead(PULSADOR_B) == HIGH) {
      display.clear();  
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.drawString(64, 10, "Pulsador B");
      display.display();
      delay(1000);
    }
    if (digitalRead(PULSADOR_C) == HIGH) {
      display.clear();  
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.drawString(64, 10, "Pulsador C");
      display.display();
      delay(1000);
    }*/

    /*if (irrecv.decode(&results)) {
      Serial.println((long)results.value, HEX);

      switch (results.value) {                                                                                                                                                                                                              
        case IR_DIGIT_1:                                                                                                                                                                                                             
          tempInputAux = 1;                                                                                                                                                                                                          
          processNumero = true;                                                                                                                                                                                                      
          break;                                                                                                                                                                                                                     
        case IR_DIGIT_2:                                                                                                                                                                                                             
          tempInputAux = 2;                                                                                                                                                                                                          
          processNumero = true;                                                                                                                                                                                                      
          break;                                                                                                                                                                                                                     
        case IR_DIGIT_3:                                                                                                                                                                                                             
          tempInputAux = 3;                                                                                                                                                                                                          
          processNumero = true;                                                                                                                                                                                                      
          break;                                                                                                                                                                                                                     
        case IR_DIGIT_4:                                                                                                                                                                                                             
          tempInputAux = 4;                                                                                                                                                                                                          
          processNumero = true;                                                                                                                                                                                                      
          break;                                                                                                                                                                                                                     
        case IR_DIGIT_5:                                                                                                                                                                                                             
          tempInputAux = 5;                                                                                                                                                                                                          
          processNumero = true;                                                                                                                                                                                                      
          break;                                                                                                                                                                                                                     
        case IR_DIGIT_6:                                                                                                                                                                                                             
          tempInputAux = 6;                                                                                                                                                                                                          
          processNumero = true;                                                                                                                                                                                                      
          break;                                                                                                                                                                                                                     
        case IR_DIGIT_7:                                                                                                                                                                                                             
          tempInputAux = 7;                                                                                                                                                                                                          
          processNumero = true;                                                                                                                                                                                                      
          break;                                                                                                                                                                                                                     
        case IR_DIGIT_8:                                                                                                                                                                                                             
          tempInputAux = 8;                                                                                                                                                                                                          
          processNumero = true;                                                                                                                                                                                                      
          break;                                                                                                                                                                                                                     
        case IR_DIGIT_9:                                                                                                                                                                                                             
          tempInputAux = 9;                                                                                                                                                                                                          
          processNumero = true;                                                                                                                                                                                                      
          break;                                                                                                                                                                                                                     
        case IR_DIGIT_0:                                                                                                                                                                                                             
          tempInputAux = 0;                                                                                                                                                                                                          
          processNumero = true;                                                                                                                                                                                                      
          break;
        case IR_JUMP:
          tempPrograma = tempSet;                                                                                                                                                                                                    
          state = STATE_CALENTANDO;
          stateCalentandoBoot = 1;
          break;
        case IR_POWER:
          sendTeclamatic(COMMAND_SWITCH);
          break;
        default:
          processNumero = false;
          Serial.println((long)results.value, HEX);                                                                                                                                                                                                  
      }

      if (processNumero == true) {
        if (tempInputMultiplo == 1) {                                                                                                                                                                                                
          tempSet = 0;                                                                                                                                                                                                               
        }                                                                                                                                                                                                                            
        tempSet = tempSet * tempInputMultiplo + tempInputAux;                                                                                                                                                                        
        tempInputMultiplo = tempInputMultiplo * 10;                                                                                                                                                                                  
        if (tempInputMultiplo == 100) {                                                                                                                                                                                              
          tempInputMultiplo = 1;                                                                                                                                                                                                     
        }
        lcd.clear();                                                                                                                                                                                                                 
        lcd.setCursor(0, 0);                                                                                                                                                                                                         
        lcd.print("Temperatura:");                                                                                                                                                                                                   
        lcd.setCursor(0, 1);                                                                                                                                                                                                         
        lcd.print(tempSet);
      }

      irrecv.resume();
    }*/
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
    
    Serial.println("Algo hay...");
      
    RF24NetworkHeader header;
    network.peek(header);

    event_t event;
    switch(header.type){
      // Display the incoming millis() values from the sensor nodes
      case 'M':
        network.read(header, &event, sizeof(event));

        display.clear();  
        display.setTextAlignment(TEXT_ALIGN_CENTER);
        display.drawString(64, 10, "device_type: " + String(event.device_type, DEC) + "\nevent_type: " + String(event.event_type, DEC) + "\ndata: " + String(event.data, DEC));
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
          if (event.event_type == SENSORMATIC_EVENT_CURRENT_TEMP) {
            if (event.data >= tempSet) {            if (event.data >= tempSet) {

              sendTeclamatic(COMMAND_APAGAR);
            } else {
              sendTeclamatic(COMMAND_ENCENDER);  
            }
          } else if (event.event_type == SENSORMATIC_EVENT_SET_TEMP) {
            tempSet = event.data;
            sendTeclamatic(COMMAND_ENCENDER);
          }
        }
        break;
      default:
        network.read(header,0,0);
        Serial.println(header.type);
        break;
    }
  }
}
