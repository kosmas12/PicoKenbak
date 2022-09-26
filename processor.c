//
// Created by kosmas on 5/12/21.
//

#include <stdint.h>
#include <pico/printf.h>
#include "processor.h"

uint8_t getBit(uint8_t byte, uint8_t bitToGet) {
    return (byte >> bitToGet) & 1;
}

void setBit(uint8_t *byte, uint8_t bitToSet, uint8_t value) {
    if (value == 1) {
        *byte |= value << bitToSet;
    }
    else if (value == 0) {
        *byte &= ~(1 << bitToSet);
    }
}

uint8_t fetchRealOperand(AddressingMode addressingMode, uint8_t operand) {
    switch (addressingMode) {
        case ADDRESSING_MODE_IMMEDIATE:
            return operand;
        case ADDRESSING_MODE_MEMORY:
            return memory[operand];
        case ADDRESSING_MODE_INDIRECT:
            return memory[memory[operand]];
        case ADDRESSING_MODE_INDEXED:
            return memory[operand + memory[X_REGISTER_ADDRESS]];
        case ADDRESSING_MODE_INDIRECT_INDEXED:
            return memory[memory[operand] + memory[X_REGISTER_ADDRESS]];
        default:
            break;
    }
}

uint8_t determineRegisterToUse(uint8_t instruction) {
    uint8_t mostSignificant2bits = instruction & 0xC0;

    switch (mostSignificant2bits) {
        case 0:
            return A_REGISTER_ADDRESS;
        case 0x40:
            return B_REGISTER_ADDRESS;
        case 0xC0:
            return X_REGISTER_ADDRESS;
        default:
            // Defying the rules of binary everybody
            return 0;
    }
}

AddressingMode determineAddressingMode(uint8_t instruction) {
    uint8_t leastSignificant3bits = instruction & 0x7;

    switch (leastSignificant3bits) {
        case 0x3:
            return ADDRESSING_MODE_IMMEDIATE;
        case 0x4:
            return ADDRESSING_MODE_MEMORY;
        case 0x5:
            return ADDRESSING_MODE_INDIRECT;
        case 0x6:
            return ADDRESSING_MODE_INDEXED;
        case 0x7:
            return ADDRESSING_MODE_INDIRECT_INDEXED;
        default:
            // How did we even get here?
            return 0;
    }
}

void add(uint8_t instruction) {
    uint8_t registerToAddTo = determineRegisterToUse(instruction);
    AddressingMode addressingMode = determineAddressingMode(instruction);

    uint8_t operand = memory[PROGRAM_COUNTER_VALUE++];

    uint8_t numberToAdd = fetchRealOperand(addressingMode, operand);
    uint16_t result = memory[registerToAddTo] + numberToAdd;

    memory[registerToAddTo] += numberToAdd;

    // The Overflow and Carry address for a register can be found by adding 0x81
    setBit(&memory[registerToAddTo + 0x81], CARRY_BIT, result > 0xFF);
    setBit(&memory[registerToAddTo + 0x81], OVERFLOW_BIT, result > 0x7F);

    printf("0x%X is now %d from addition\n", registerToAddTo, memory[registerToAddTo]);
}

void sub(uint8_t instruction) {
    uint8_t registerToSubtractFrom = determineRegisterToUse(instruction);
    AddressingMode addressingMode = determineAddressingMode(instruction);

    uint8_t operand = memory[PROGRAM_COUNTER_VALUE++];
    uint8_t numberToSubtract = fetchRealOperand(addressingMode, operand);
    uint16_t result = memory[registerToSubtractFrom] + numberToSubtract;

    memory[registerToSubtractFrom] -= numberToSubtract;

    // The Overflow and Carry address for a register can be found by adding 0x81
    setBit(&memory[registerToSubtractFrom + 0x81], CARRY_BIT, result > 0xFF);
    setBit(&memory[registerToSubtractFrom + 0x81], OVERFLOW_BIT, result > 0x7F);
}


void load(uint8_t instruction) {
    uint8_t registerToLoadTo = determineRegisterToUse(instruction);
    AddressingMode addressingMode = determineAddressingMode(instruction);

    uint8_t operand = memory[PROGRAM_COUNTER_VALUE++];
    uint8_t valueToLoad = fetchRealOperand(addressingMode, operand);

    memory[registerToLoadTo] = valueToLoad;
}

void store(uint8_t instruction) {
    uint8_t registerToStore = determineRegisterToUse(instruction);
    AddressingMode addressingMode = determineAddressingMode(instruction);

    uint8_t operand = memory[PROGRAM_COUNTER_VALUE++];
    uint8_t addressOfValueToStore = fetchRealOperand(addressingMode, operand);

    memory[addressOfValueToStore] = memory[registerToStore];

    printf("0x%X is now %d\n", addressOfValueToStore, memory[addressOfValueToStore]);
}

void logicalAnd(uint8_t instruction) {
    AddressingMode addressingMode = determineAddressingMode(instruction);

    uint8_t operand = memory[PROGRAM_COUNTER_VALUE++];
    uint8_t numberToAnd = fetchRealOperand(addressingMode, operand);

    memory[A_REGISTER_ADDRESS] &= numberToAnd;
}

void logicalOr(uint8_t instruction) {
    AddressingMode addressingMode = determineAddressingMode(instruction);

    uint8_t operand = memory[PROGRAM_COUNTER_VALUE++];
    uint8_t numberToOr = fetchRealOperand(addressingMode, operand);

    memory[A_REGISTER_ADDRESS] |= numberToOr;
}

void loadComplement(uint8_t instruction) {
    AddressingMode addressingMode = determineAddressingMode(instruction);

    uint8_t operand = memory[PROGRAM_COUNTER_VALUE++];
    uint8_t numberToGetComplementOf = fetchRealOperand(addressingMode, operand);

    memory[A_REGISTER_ADDRESS] = 0 - numberToGetComplementOf;
}

uint8_t checkForJumpCondition(uint8_t registerToCheck, JumpCondition condition) {
    switch (condition) {
        case JUMP_CONDITION_NON_ZERO:
            return (registerToCheck != 0);
        case JUMP_CONDITION_ZERO:
            return (registerToCheck == 0);
        case JUMP_CONDITION_NEGATIVE:
            return (registerToCheck < 0);
        case JUMP_CONDITION_POSITIVE:
            return (registerToCheck >= 0);
        case JUMP_CONDITION_POSITIVE_NON_ZERO:
            return (registerToCheck > 0);
        case JUMP_CONDITION_UNCONDITIONAL:
            return 1;
        default:
            // Invalid?????
            return 0;
    }
}

uint8_t getRegisterToCheckForJump(uint8_t instruction) {
    if (getBit(instruction, 6) && getBit(instruction, 7)) {
        return A_REGISTER_ADDRESS;
    }
    if (getBit(instruction, 6)) {
        return B_REGISTER_ADDRESS;
    }
    if (getBit(instruction, 7)) {
        return X_REGISTER_ADDRESS;
    }
    return 0;
}

JumpCondition getJumpCondition(uint8_t instruction) {
    uint8_t leastSignificant3bits = instruction & 0x7;

    switch (leastSignificant3bits) {
        case 0x3:
            return JUMP_CONDITION_NON_ZERO;
        case 0x4:
            return JUMP_CONDITION_ZERO;
        case 0x5:
            return JUMP_CONDITION_NEGATIVE;
        case 0x6:
            return JUMP_CONDITION_POSITIVE;
        case 0x7:
            return JUMP_CONDITION_POSITIVE_NON_ZERO;
        default:
            // Invalid jump
            return 0xFF;
    }
}

void jump(uint8_t instruction) {
    uint8_t operand = memory[PROGRAM_COUNTER_VALUE++];
    uint8_t addressToJumpTo = operand;

    uint8_t registerToCheck = getRegisterToCheckForJump(instruction);
    JumpCondition condition = getJumpCondition(instruction);

    // Invalid jump
    if (condition == 0xFF) {
        return;
    }

    // The function to get the register returns 0 for an unconditional jump
    if (registerToCheck == 0) {
        condition = JUMP_CONDITION_UNCONDITIONAL;
    }

    uint8_t mark = getBit(instruction, 4);
    uint8_t indirect = getBit(instruction, 3);

    uint8_t shouldJump = checkForJumpCondition(registerToCheck, condition);

    if(shouldJump) {
        if (indirect) {
            addressToJumpTo = memory[operand];
        }

        if (mark) {
            memory[addressToJumpTo++] = operand;
        }

        PROGRAM_COUNTER_VALUE = addressToJumpTo;
    }
}

void skipOnZero(uint8_t instruction) {
    uint8_t bitToCheck = (instruction & 0x38) >> 3;

    uint8_t operand = memory[PROGRAM_COUNTER_VALUE++];

    if (!getBit(operand, bitToCheck)) {
        PROGRAM_COUNTER_VALUE += 2;
    }
}

void skipOnOne(uint8_t instruction) {
    uint8_t bitToCheck = (instruction & 0x38) >> 3;

    uint8_t operand = memory[PROGRAM_COUNTER_VALUE++];

    if (getBit(operand, bitToCheck)) {
        PROGRAM_COUNTER_VALUE += 2;
    }
}

// This is what the emulator calls for both instructions
void skip(uint8_t instruction) {
    // The instruction changes depending on bit 6
    if (getBit(instruction, 6)) {
        skipOnOne(instruction);
    }
    else {
        skipOnZero(instruction);
    }
}

void setZero(uint8_t instruction) {
    uint8_t bitToSet = (instruction & 0x38) >> 3;

    uint8_t operand = memory[PROGRAM_COUNTER_VALUE++];

    setBit(&memory[operand], bitToSet, 0);
}

void setOne(uint8_t instruction) {
    uint8_t bitToSet = (instruction & 0x38) >> 3;

    uint8_t operand = memory[PROGRAM_COUNTER_VALUE++];

    setBit(&(memory[operand]), bitToSet, 1);
}

// This is what the emulator calls for both instructions
void set(uint8_t instruction) {
    // The instruction changes depending on bit 6
    if (getBit(instruction, 6)) {
        setOne(instruction);
    }
    else {
        setZero(instruction);
    }
}

uint8_t rotateLeft(uint8_t value, uint8_t places) {
    // First, shift the bits. Then, the new bits that are 0 can be assigned the old bits that
    // were supposed to end up there by shifting the value to the opposite direction of the rotation,
    // accordingly.
    return (value << places) | (value >> ((sizeof(uint8_t) * 8) - places));
}

uint8_t rotateRight(uint8_t value, uint8_t places) {
    return (value >> places) | (value << ((sizeof(uint8_t) * 8) - places));
}

// These have similar opcodes, so I decided to group them together
void shiftRotate(uint8_t instruction) {
    // 0 for Right, 1 for Left
    uint8_t direction = (instruction & 0x80) >> 7;
    // 0 for Shift, 1 for Rotate
    uint8_t operation = (instruction & 0x40) >> 6;
    uint8_t registerToWorkOn = (instruction & 0x20) >> 5;
    uint8_t places = (instruction & 0x18) >> 3;
    // 0 actually means 4 places
    if (places == 0) {
        places = 4;
    }

    if (!operation) {
        if (direction) {
            memory[registerToWorkOn] <<= places;
        }
        else {
            memory[registerToWorkOn] >>= places;
        }
    }
    else {
        if (direction) {
            memory[registerToWorkOn] = rotateLeft(memory[registerToWorkOn], places);
        }
        else {
            memory[registerToWorkOn] = rotateRight(memory[registerToWorkOn], places);
        }
    }
}

void nop() {
    ++PROGRAM_COUNTER_VALUE;
}
