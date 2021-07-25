#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>

#include <Wire.h>
#include <WebSocketsServer.h>

#define NAMES C(UP)C(DOWN)C(STOP)C(MEMORY_UP)C(MEMORY_DOWN)C(RECORD_DOWN)C(RECORD_UP)C(AUTO_MODE_ON)C(AUTO_MODE_OFF)C(TOGGLE)
#define C(x) x,
enum tableCMD { NAMES LENGTH };
#undef C
#define C(x) #x,    
const char * const TABLE_CMD[] = { NAMES };

const unsigned long SERIAL_BAUD_RATE = 115200;
const unsigned long WAIT_FOR_I2C_BYTES_TIMEOUT_MS = 100;

const char* WLAN_SSID = "";
const char* WLAN_PASSWORD = "";

const uint8_t moveTableUpPin = D11;
const uint8_t moveTableDownPin = D12;

const uint8_t DIRECTION_STOP_ID = 252;

const uint16_t POSITION_MIN = 180;
const uint16_t POSITION_MAX = 6400;

uint16_t position = 0;
uint16_t currentTarget = 0;

uint8_t forceDirectionSend = false;
uint8_t direction = 0;

WebSocketsServer webSocket(81); 

IPAddress ip(192,168,88,22);
IPAddress gateway(192,168,88,1);
IPAddress subnet(255,255,255,0);

uint8_t LAST_TOGGLE_STATE = false;
uint8_t LAST_DIRECTION = STOP;
uint8_t AUTO_MODE_ENABLED = false;

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

uint8_t i2cReadCommand = I2C_CMD_NOOP;

void sendDirectionToTable(I2CCommand command);
void initPins();
void initWiFi();
void initWebSocket();
void sendState(uint8_t);
void processDirection();
void processPosition();
void applyPayload(String);
void sendI2CCommand(I2CCommand cmd);

bool waitForI2CBytesAvailable(int waitForNumBytes) {
  unsigned long waitForI2CBytesTimeout = millis() + WAIT_FOR_I2C_BYTES_TIMEOUT_MS;
  while (Wire.available() < waitForNumBytes) {
    if (millis() > waitForI2CBytesTimeout) {
      return false;
    }
  }
  return true;
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

  digitalWrite(LED_BUILTIN, HIGH);
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      processDirection();
      sendState(false);
      break;
    case WStype_TEXT:
      payload_string = (char*) payload;
      applyPayload(payload_string);
      break;
    case WStype_ERROR:
      break;
  }
}

void initWebSocket() {
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void applyPayload(String payload) {
  if (payload == TABLE_CMD[TOGGLE] && AUTO_MODE_ENABLED) {
    sendDirectionToTable(LAST_TOGGLE_STATE ? I2C_CMD_MOVE_MEMORY_DOWN: I2C_CMD_MOVE_MEMORY_UP);
    LAST_TOGGLE_STATE = !LAST_DIRECTION;
  } else if (payload == TABLE_CMD[UP]) {
    sendDirectionToTable(I2C_CMD_MOVE_UP);
  } else if (payload == TABLE_CMD[DOWN]) {
    sendDirectionToTable(I2C_CMD_MOVE_DOWN);
  } else if (payload == TABLE_CMD[STOP]) {
    sendDirectionToTable(I2C_CMD_MOVE_STOP);
  } else if(payload == TABLE_CMD[RECORD_DOWN]) {
    sendI2CCommand(I2C_CMD_MEMORY_DOWN);
  } else if (payload == TABLE_CMD[RECORD_UP]) {
    sendI2CCommand(I2C_CMD_MEMORY_UP);
  } else if (payload == TABLE_CMD[MEMORY_DOWN]) {
    LAST_DIRECTION = DOWN;
    sendI2CCommand(I2C_CMD_MOVE_MEMORY_DOWN);
  } else if(payload == TABLE_CMD[MEMORY_UP]) {
    LAST_DIRECTION = UP;
    sendI2CCommand(I2C_CMD_MOVE_MEMORY_UP);
  } else if (payload == TABLE_CMD[AUTO_MODE_OFF]) {
    AUTO_MODE_ENABLED = false;
  } else if (payload == TABLE_CMD[AUTO_MODE_ON]) {
    AUTO_MODE_ENABLED = true;
  }

  sendState(false);
}

void sendI2CCommand(I2CCommand cmd) {
  Wire.beginTransmission(I2C_ADDRESS);
  Wire.write(cmd);
  Wire.endTransmission();
}

void sendDirectionToTable(I2CCommand command) {
  switch (command) {
  case I2C_CMD_MOVE_STOP:
    LAST_DIRECTION = STOP;
    break;
  case I2C_CMD_MOVE_UP:
  case I2C_CMD_MEMORY_UP:
    LAST_DIRECTION = UP;
    break;
  case I2C_CMD_MOVE_DOWN:
  case I2C_CMD_MOVE_MEMORY_DOWN:
    LAST_DIRECTION = DOWN;
    break;
  default:
    break;
  }

  sendI2CCommand(command);
}

uint16_t prevHeight = 0;

void sendState(uint8_t checkHeight) {
  if (checkHeight == 1 && prevHeight == position) {
    return;
  }

  prevHeight = position;
  DynamicJsonDocument doc(103);
  doc["direction"] = TABLE_CMD[LAST_DIRECTION];
  doc["autoMode"] = AUTO_MODE_ENABLED;
  doc["height"] = position;
  doc["rangeMax"] = POSITION_MAX;
  doc["rangeMin"] = POSITION_MIN;
  String output;
  serializeJson(doc, output);
  webSocket.broadcastTXT(output);
}

void processPosition() {
  sendI2CCommand(I2C_CMD_READ_POSITIONS);
  Wire.requestFrom(I2C_ADDRESS, 2);
  if (waitForI2CBytesAvailable(2)) {
    position = Wire.read() + (Wire.read() << 8);
  } 
}

void processDirection() {
  sendI2CCommand(I2C_CMD_READ_DIRECTION);
  Wire.beginTransmission(I2C_ADDRESS);
  Wire.requestFrom(I2C_ADDRESS, 1);
  if (waitForI2CBytesAvailable(1)) {
    direction = Wire.read();
    processPosition();
  }
}

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);

  initPins();
  initWiFi();
  initWebSocket();
}

void loop() {
  webSocket.loop();

  if (LAST_DIRECTION != STOP) {
    sendState(true);
    processDirection();
    forceDirectionSend = false;
  }

  if (!forceDirectionSend && direction == DIRECTION_STOP_ID) {
    LAST_DIRECTION = STOP;
    sendState(false);
    forceDirectionSend = true;
  }
}
