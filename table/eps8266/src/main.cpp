#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#include <Wire.h>
#include <WebSocketsServer.h>

#define NAMES C(UP)C(DOWN)C(STOP)
#define C(x) x,
enum direction { NAMES LENGTH };
#undef C
#define C(x) #x,    
const char * const DIRECTION[] = { NAMES };

const unsigned long SERIAL_BAUD_RATE = 115200;
const unsigned long WAIT_FOR_I2C_BYTES_TIMEOUT_MS = 100;

const char* WLAN_SSID = "";
const char* WLAN_PASSWORD = "";

const uint8_t moveTableUpPin = D11;
const uint8_t moveTableDownPin = D12;
const uint8_t sendWithoutHeightCheck = 0;
uint16_t position = 0;
uint8_t direction = 0;

MDNSResponder mdns;
ESP8266WebServer server(80);
WebSocketsServer webSocket(81); 

IPAddress ip(192,168,88,22);
IPAddress gateway(192,168,88,1);
IPAddress subnet(255,255,255,0);

uint8_t LAST_TOGGLE_STATE = 0;
uint8_t LAST_TABLE_COMMAND = STOP;
uint8_t FORCE_TOGGLE_ENABLE = 1;

const uint16_t POSITION_MIN = 180;
const uint16_t POSITION_MAX = 6400;

String payload_string;

const uint8_t I2C_ADDRESS = 0x10;

enum I2CCommand {
  I2C_CMD_NOOP,
  I2C_CMD_MOVE_STOP,
  I2C_CMD_MOVE_UP,
  I2C_CMD_MOVE_DOWN,
  I2C_CMD_MOVE_MEMORY_UP,
  I2C_CMD_MOVE_MEMORY_DOWN,
  I2C_CMD_READ_POSITIONS,
  I2C_CMD_READ_DIRECTION,
  I2C_CMD_MEMORY_DOWN,
  I2C_CMD_MEMORY_UP,
};

bool waitForI2CBytesAvailable(int waitForNumBytess) {
  unsigned long waitForI2CBytesTimeout = millis() + WAIT_FOR_I2C_BYTES_TIMEOUT_MS;
  while (Wire.available() < waitForNumBytess) {
    if (millis() > waitForI2CBytesTimeout) {
      return false;
    }
  }
  return true;
}

void sendDirectionToTable(I2CCommand command);
void initPins();
void initWiFi();
void initAPI();
void initWebSocket();
void sendState(uint8_t);
void processDirection();
void processPosition();

uint16_t currentTarget = 0;
uint8_t i2cReadCommand = I2C_CMD_NOOP;
uint8_t forceDirectionSended = false;

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  Wire.begin();

  initPins();
  initWiFi();
  initAPI();
  initWebSocket();
}

void loop() {
  server.handleClient();
  webSocket.loop();
  MDNS.update();

  if (LAST_TABLE_COMMAND != STOP) {
    processPosition();
    uint8_t send = 1;
    sendState(send);
    processDirection();
    forceDirectionSended = false;
  }

  if (!forceDirectionSended && direction == 252) {
    LAST_TABLE_COMMAND = STOP;
    sendState(sendWithoutHeightCheck);
    forceDirectionSended = true;
  }
}

void initPins() {
  Wire.begin(SDA, SCL);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(moveTableUpPin, OUTPUT);
  pinMode(moveTableDownPin, OUTPUT);
}

void initWiFi() {
  WiFi.begin(WLAN_SSID, WLAN_PASSWORD);
  WiFi.config(ip, gateway, subnet);
  WiFi.mode(WIFI_STA);

  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, digitalRead(LED_BUILTIN) == LOW ? HIGH : LOW);
    delay(100);
  }

  Serial.println("HTTP server started.");
  Serial.println(WiFi.localIP());
  digitalWrite(LED_BUILTIN, HIGH);
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        sendState(sendWithoutHeightCheck);
      }
      break;
    case WStype_TEXT:
      payload_string = (char*) payload;

      if (payload_string == "disableToggle") {
        FORCE_TOGGLE_ENABLE = 0;
      }

      if (payload_string == "enableToggle") {
        FORCE_TOGGLE_ENABLE = 1;
      }

      sendState(sendWithoutHeightCheck);
      break;
    case WStype_BIN:
      Serial.printf("[%u] get BIN: %s\n", num, payload);
      break;
    case WStype_ERROR:
      Serial.printf("[%u] error: %s\n", num, payload);
      break;
  } 
}

void initWebSocket() {
  webSocket.begin();                          // start the websocket server
  webSocket.onEvent(webSocketEvent);          // if there's an incomming websocket message, go to function 'webSocketEvent'
  Serial.println("WebSocket server started.");
}

void setCrossOrigin(){
    server.sendHeader(F("Access-Control-Allow-Origin"), F("*"));
    server.sendHeader(F("Access-Control-Max-Age"), F("600"));
    server.sendHeader(F("Access-Control-Allow-Methods"), F("PUT,POST,GET,OPTIONS"));
    server.sendHeader(F("Access-Control-Allow-Headers"), F("*"));
    server.sendHeader(F("access-control-allow-credentials"), F("false"));
};

void initAPI() {
  server.on("/status", HTTP_GET, [](){
    setCrossOrigin();
    server.send(200, "text/plain", "The Ikeablan is alive!");
  });

  server.on("/toggle", HTTP_GET, [](){
    setCrossOrigin();

    if (FORCE_TOGGLE_ENABLE == 1) {
      if (LAST_TOGGLE_STATE == 0) {
        sendDirectionToTable(I2C_CMD_MOVE_MEMORY_UP);
        LAST_TOGGLE_STATE = 1;
      } else {
        sendDirectionToTable(I2C_CMD_MOVE_MEMORY_DOWN);
        LAST_TOGGLE_STATE = 0;
      }
    }

    server.send(204);
  });

  server.on("/up", HTTP_GET, [](){
    setCrossOrigin();
    sendDirectionToTable(I2C_CMD_MOVE_UP);
    server.send(204);
  });

  server.on("/down", HTTP_GET, [](){
    setCrossOrigin();
    sendDirectionToTable(I2C_CMD_MOVE_DOWN);
    server.send(204);
  });

  server.on("/stop", HTTP_GET, [](){
    setCrossOrigin();
    sendDirectionToTable(I2C_CMD_MOVE_STOP);
    server.send(204);
  });

  server.on("/record/down", HTTP_GET, [](){
    setCrossOrigin();
    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(I2C_CMD_MEMORY_DOWN);
    Wire.endTransmission();
    server.send(204);
  });

  server.on("/record/up", HTTP_GET, [](){
    setCrossOrigin();
    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(I2C_CMD_MEMORY_UP);
    Wire.endTransmission();
    server.send(204);
  });

  server.on("/memory/down", HTTP_GET, [](){
    setCrossOrigin();
    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(I2C_CMD_MOVE_MEMORY_DOWN);
    Wire.endTransmission();
    server.send(204);
  });

  server.on("/memory/up", HTTP_GET, [](){
    setCrossOrigin();
    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(I2C_CMD_MOVE_MEMORY_UP);
    Wire.endTransmission();
    server.send(204);
  });

  server.onNotFound([]() {
    setCrossOrigin();
    server.send(404);
  });

  server.begin();
}


void sendDirectionToTable(I2CCommand command) {
  switch (command)
  {
  case I2C_CMD_MOVE_STOP:
    LAST_TABLE_COMMAND = STOP;
    break;
  case I2C_CMD_MOVE_UP:
  case I2C_CMD_MEMORY_UP:
    LAST_TABLE_COMMAND = UP;
    break;
  case I2C_CMD_MOVE_DOWN:
  case I2C_CMD_MOVE_MEMORY_DOWN:
    LAST_TABLE_COMMAND = DOWN;
    break;
  default:
    break;
  }
  Wire.beginTransmission(I2C_ADDRESS);
  Wire.write(command);
  Wire.endTransmission();
  sendState(sendWithoutHeightCheck);
}

uint16_t prevHeight = 0;

void sendState(uint8_t checkHeight) {
  if (checkHeight == 1 && prevHeight == position) {
    return;
  }

  prevHeight = position;
  DynamicJsonDocument doc(103);
  doc["direction"] = DIRECTION[LAST_TABLE_COMMAND];
  doc["autoMode"] = FORCE_TOGGLE_ENABLE;
  doc["height"] = position;
  doc["rangeMax"] = POSITION_MAX;
  doc["rangeMin"] = POSITION_MIN;
  String output;
  serializeJson(doc, output);
  webSocket.broadcastTXT(output);
}

void processPosition() {
    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(I2C_CMD_READ_POSITIONS);
    Wire.endTransmission();
    Wire.requestFrom(I2C_ADDRESS, 2);
    if (waitForI2CBytesAvailable(2)) {
      position = Wire.read() + (Wire.read() << 8);
    } 
}

void processDirection() {
    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(I2C_CMD_READ_DIRECTION);
    Wire.endTransmission();
    Wire.requestFrom(I2C_ADDRESS, 2);
    if (waitForI2CBytesAvailable(2)) {
      direction = Wire.read();
    }
}
