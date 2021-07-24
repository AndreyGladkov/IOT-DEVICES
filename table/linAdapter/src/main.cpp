#include <Arduino.h>
#include "avr_util.h"
#include "custom_defs.h"
#include "hardware_clock.h"
#include "io_pins.h"
#include "lin_processor.h"
#include "system_clock.h"
#include <Wire.h>
#include <EEPROM.h>

uint16_t lastPosition = 0;
uint8_t lastDirection = 0;

const uint8_t TABLE_UP_PIN = 3;
const uint8_t TABLE_DOWN_PIN = 4;

const uint16_t POSITION_MIN = 300;
const uint16_t POSITION_MAX = 6300;
const uint8_t POSITION_THRESHOLD = 100;

uint8_t LIN_ID_TABLE_POSITION = 0x92;

const uint8_t I2C_ADDRESS = 0x10;

uint16_t memoryDown;
uint16_t memoryUp;
uint8_t MEMORY_ADDRESS_DOWN = 0;
uint8_t MEMORY_ADDRESS_UP = 1;

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
uint8_t i2cMemoryCommand = I2C_CMD_NOOP;

enum DIRECTION {
  STOP = 252,
  UP = 134,
  DOWN = 133,
};

void tableDirection(I2CCommand);
void savePosition(DIRECTION direction);
DIRECTION getNextDirection(uint16_t targetPosition);
boolean checkEdgeCases();

void processLINFrame(LinFrame frame) {
  uint8_t linId = frame.get_byte(0);

  if (linId == LIN_ID_TABLE_POSITION) {
    uint8_t data_2 = frame.get_byte(2);
    uint8_t data_1 = frame.get_byte(1);
    uint8_t direction = frame.get_byte(3);
    uint16_t currentPosition = data_2;

    currentPosition <<= 8;
    currentPosition = currentPosition + data_1;

    if (currentPosition != lastPosition && currentPosition != 0) {
      lastPosition = currentPosition;
    }

    if (direction == STOP || direction == DOWN || direction == UP) {
      lastDirection = direction;
    }
  }
}

void handleI2CRequest() {
  switch (i2cReadCommand) {
    case I2C_CMD_READ_POSITIONS:
      Wire.write((const uint8_t *) & lastPosition, 2);
      break;
    case I2C_CMD_READ_DIRECTION:
      Wire.write(lastDirection);
      break;
    default:
      break;
  }
}

void handleI2CReceive(int numBytes) {
  uint8_t command = Wire.read();

  switch (command) {
    case I2C_CMD_READ_POSITIONS:
    case I2C_CMD_READ_DIRECTION:
      i2cReadCommand = command;
      break;
    case I2C_CMD_MOVE_UP:
      lastDirection = UP;
      break;
    case I2C_CMD_MOVE_DOWN:
      lastDirection = DOWN;
      break;
    case I2C_CMD_MOVE_STOP:
      lastDirection = STOP;
      break;
    case I2C_CMD_MEMORY_DOWN:
      savePosition(DOWN);
      break;
    case I2C_CMD_MEMORY_UP:
      savePosition(UP);
      break;
    case I2C_CMD_MOVE_MEMORY_DOWN:
    case I2C_CMD_MOVE_MEMORY_UP:
      i2cMemoryCommand = command;
      DIRECTION nextDirection = getNextDirection(command == I2C_CMD_MOVE_MEMORY_DOWN ? memoryDown: memoryUp);
      lastDirection = nextDirection;
      break;
    default:
      break;
  }

  switch (lastDirection) {
    case DOWN:
      tableDirection(I2C_CMD_MOVE_DOWN);
      break;
    case UP:
      tableDirection(I2C_CMD_MOVE_UP);
      break;
    case STOP:
      tableDirection(I2C_CMD_MOVE_STOP);
      break;
    default:
      break;
  }
}

DIRECTION getNextDirection(uint16_t targetPosition) {
  int distance = lastPosition - targetPosition;

  if (abs(distance) < POSITION_THRESHOLD) {
    return STOP;
  }

  return distance > 0 ? DOWN : UP;
}

void tableDirection(I2CCommand direction) {
  if (direction != I2C_CMD_MOVE_STOP && checkEdgeCases()) {
    return;
  }

  switch (direction) {
    case I2C_CMD_MOVE_STOP:
      digitalWrite(TABLE_UP_PIN, HIGH);
      digitalWrite(TABLE_DOWN_PIN, HIGH);
      break;
    case I2C_CMD_MOVE_UP:
      digitalWrite(TABLE_UP_PIN, LOW);
      digitalWrite(TABLE_DOWN_PIN, HIGH);
      break;
    case I2C_CMD_MOVE_DOWN:
      digitalWrite(TABLE_UP_PIN, HIGH);
      digitalWrite(TABLE_DOWN_PIN, LOW);
      break;
    default:
      break;
  }
}

void savePosition(DIRECTION direction) {
  uint8_t memoryAddress = 0;

  switch (direction) {
    case UP:
      memoryAddress = MEMORY_ADDRESS_UP;
      break;
    case DOWN:
      memoryAddress = MEMORY_ADDRESS_DOWN;
      break;
  default:
      break;
  }

  if (lastPosition > POSITION_MIN && lastPosition < POSITION_MAX) {
    EEPROM.put(memoryAddress, lastPosition);
  }
}

boolean checkEdgeCases() {
  if (lastPosition == 0) {
    return false;
  }

  if (lastPosition <= POSITION_MIN && lastDirection == DOWN) {
    return true;
  }

  if (lastPosition >= POSITION_MAX && lastDirection == UP) {
    return true;
  }

  if (i2cMemoryCommand == I2C_CMD_MOVE_MEMORY_DOWN && abs(lastPosition - memoryDown) <= POSITION_THRESHOLD) {
    i2cMemoryCommand = I2C_CMD_NOOP;
    return true;
  }

  if (i2cMemoryCommand == I2C_CMD_MOVE_MEMORY_UP && abs(lastPosition - memoryUp) <= POSITION_THRESHOLD ) {
    i2cMemoryCommand = I2C_CMD_NOOP;
    return true;
  }

  return false;
}

void setup() {
  Serial.begin(9600);
  while (!Serial) {;};

  hardware_clock::setup();
  lin_processor::setup();

  Wire.begin(I2C_ADDRESS);
  Wire.onRequest(handleI2CRequest);
  Wire.onReceive(handleI2CReceive);

  pinMode(TABLE_UP_PIN, OUTPUT);
  pinMode(TABLE_DOWN_PIN, OUTPUT);

  EEPROM.get(MEMORY_ADDRESS_DOWN, memoryDown);
  EEPROM.get(MEMORY_ADDRESS_UP, memoryUp);
}

void loop() {
  system_clock::loop();
  LinFrame frame;

  if (lin_processor::readNextFrame(&frame)) {
    processLINFrame(frame);
  }

  if (checkEdgeCases()) {
    tableDirection(I2C_CMD_MOVE_STOP);
    lastDirection = STOP;
  }
}
