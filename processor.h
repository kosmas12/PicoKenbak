//
// Created by kosmas on 5/12/21.
//

#include <stdint.h>

#ifndef PICOKENBAK_PROCESSOR_H
#define PICOKENBAK_PROCESSOR_H

#define A_REGISTER_ADDRESS 0x0
#define B_REGISTER_ADDRESS 0x1
#define X_REGISTER_ADDRESS 0x2
#define P_REGISTER_ADDRESS 0x3
#define OUTPUT_REGISTER_ADDRESS 0x80
#define CARRY_BIT 1
#define OVERFLOW_BIT 0
#define OVERFLOWANDCARRY_A_ADDRESS 0x81
#define OVERFLOWANDCARRY_B_ADDRESS 0x82
#define OVERFLOWANDCARRY_X_ADDRESS 0x83
#define INPUT_REGISTER_ADDRESS 0xFF
// Define to avoid confusion when looking at the code
#define PROGRAM_COUNTER_VALUE memory[P_REGISTER_ADDRESS]

typedef enum {
    ADDRESSING_MODE_IMMEDIATE,
    ADDRESSING_MODE_MEMORY,
    ADDRESSING_MODE_INDIRECT,
    ADDRESSING_MODE_INDEXED,
    ADDRESSING_MODE_INDIRECT_INDEXED
}AddressingMode;

typedef enum {
    JUMP_CONDITION_NON_ZERO,
    JUMP_CONDITION_ZERO,
    JUMP_CONDITION_NEGATIVE,
    JUMP_CONDITION_POSITIVE,
    JUMP_CONDITION_POSITIVE_NON_ZERO,
    JUMP_CONDITION_UNCONDITIONAL
}JumpCondition;

uint8_t memory[256];

uint8_t determineRegisterToUse(uint8_t instruction);
AddressingMode determineAddressingMode(uint8_t instruction);
uint8_t fetchRealOperand(AddressingMode addressingMode, uint8_t operand);
uint8_t checkForJumpCondition(uint8_t registerToCheck, JumpCondition condition);
JumpCondition getJumpCondition(uint8_t instruction);
uint8_t getRegisterToCheckForJump(uint8_t instruction);

uint8_t getBit(uint8_t byte, uint8_t bitToGet);
void setBit(uint8_t *byte, uint8_t bitToSet, uint8_t value);

void add(uint8_t instruction);
void sub(uint8_t instruction);

void load(uint8_t instruction);
void store(uint8_t instruction);

void logicalAnd(uint8_t instruction);
void logicalOr(uint8_t instruction);

void loadComplement(uint8_t instruction);

void jump(uint8_t instruction);

void skipOnZero(uint8_t instruction);
void skipOnOne(uint8_t instruction);
void skip(uint8_t instruction);

void setZero(uint8_t instruction);
void setOne(uint8_t instruction);
void set(uint8_t instruction);

void shiftRotate(uint8_t instruction);

void nop();
#endif //PICOKENBAK_PROCESSOR_H
