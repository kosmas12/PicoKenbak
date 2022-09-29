#include "pico/stdlib.h"
#include "processor.h"

#define ADDRESS_DISPLAY_BUTTON 10
#define ADDRESS_SET_BUTTON 11
#define READ_MEMORY_BUTTON 9
#define STORE_MEMORY_BUTTON 13
#define START_BUTTON 14
#define STOP_BUTTON 15
#define INPUT_LAMP 0
#define ADDRESS_LAMP 1
#define MEMORY_LAMP 2
#define RUN_LAMP 3
#define ALL_LAMPS_OFF 4

void execute() {
    PROGRAM_COUNTER_VALUE = 0x4;
    while (gpio_get(STOP_BUTTON)) {
        uint8_t instruction = memory[PROGRAM_COUNTER_VALUE++];
        if ((instruction & 0b111) == 0) {
            if (instruction & 0b10000000) {
                nop();
            }
            // Halt
            else {
                return;
            }
        }

        else if ((instruction & 0b00000111) == 0b00000001) {
                shiftRotate(instruction);
        }

        else if ((instruction & 0b00000111) == 0b0000010) {
            if ((instruction & 0b10000000) == 0b10000000) {
                skip(instruction);
            }
            else {
                set(instruction);
            }
        }

        else if ((instruction & 0b11111000) == 0b11011000) {
            loadComplement(instruction);
        }

        else if ((instruction & 0b11111000) == 0b11000000) {
            logicalOr(instruction);
        }

        else if ((instruction & 0b11111000) == 0b11010000) {
            logicalAnd(instruction);
        }

        else if ((instruction & 0b00111000) == 0b00011000) {
            store(instruction);
        }

        else if ((instruction & 0b00111000) == 0b00010000) {
            load(instruction);
        }
        else if ((instruction & 0b00111000) == 0b00001000) {
            sub(instruction);
        }
        else if ((instruction & 0b00111000) == 0) {
            add(instruction);
        }
        else if ((instruction & 0b00100000) == 0b00100000) {
            jump(instruction);
        }
    }
    // Quick hack for more accurate timing (The KENBAK-1 averaged <1000 instructions per second)
    // TODO: More accurate timing than this
    sleep_us(1050);
}

void setControlLamps(uint8_t lampToLightUp) {
    static uint8_t initialized = 0;

    uint8_t controlLampPins[4] = {
            19, 17,
            18, 16
    };

    if (!initialized) {
        for (uint8_t i = 0; i < 3; ++i) {
            gpio_init(controlLampPins[i]);
            gpio_set_dir(controlLampPins[i], GPIO_OUT);
            gpio_put(controlLampPins[i], 0);
        }
        initialized = 1;
    }


    for (uint8_t i = 0; i < 3; ++i) {
        if (i == lampToLightUp) {
            gpio_put(controlLampPins[i], 1);
        }
        else {
            // Only one lamp can be lit up at a time
            gpio_put(controlLampPins[i], 0);
        }
    }
}

int main() {
    stdio_init_all();

    /*
     * Pin numbers. Change according to your pinout.
     * The first 12 pins in the buttons array correspond to LEDs.
     * The first 8 are for the data LEDs (from least to most significant bit).
     * The other 4 are for the control lamps.
     * The last 2 buttons don't trigger any LEDs.
     * The code works with these assumptions so handle with care.
     */
    uint8_t pushButtonPins[14] = {
        2, 3, 4, 5, 12,
        6, 7, 8, 11,
        15, 13, 14, 10, 9
    };

    uint8_t LEDPins[8] = {
            0, 1, 28, 27, 26,
            22, 21, 20,
    };

    // GPIO initialization
    for (int i = 0; i < sizeof(pushButtonPins)/sizeof(uint8_t); ++i) {
        gpio_init(pushButtonPins[i]);
        gpio_set_dir(pushButtonPins[i], GPIO_IN);
        gpio_pull_up(pushButtonPins[i]);
    }

    for (int i = 0; i < sizeof(LEDPins)/sizeof(uint8_t); ++i) {
        gpio_init(LEDPins[i]);
        gpio_set_dir(LEDPins[i], GPIO_OUT);
    }

    /*
     * Zero out all memory.
     * Normally, the KENBAK's RAM is random-initialized but that's unnecessary overhead here.
     */
    for (int i = 0; i < sizeof(memory); ++i) {
        memory[i] = 0;
    }

    // Keep track of the button state for latest and previous presses, so we can detect if a button press is new.
    uint8_t pushButtonPinStates[14] = {0};
    uint8_t currentPushButtonPinStates[14] = {0};

    uint8_t lampToLightUp = INPUT_LAMP;
    setControlLamps(lampToLightUp);

    for(;;) {
        for (int i = 0; i < sizeof(pushButtonPins)/sizeof(uint8_t); ++i) {
            uint8_t button = pushButtonPins[i];
            // The buttons are pulled up, so we need to reverse the read
            currentPushButtonPinStates[i] = !gpio_get(button);

            // Check if the button press is new
            uint8_t isStateNew = 0;
            for (int j = 0; j < sizeof(currentPushButtonPinStates); ++j) {
                if (currentPushButtonPinStates[i] != pushButtonPinStates[i]) {
                    isStateNew = 1;
                }
            }

            uint8_t isButtonNewlyPressed = (currentPushButtonPinStates[i] && isStateNew);

            if (i < 8 && isButtonNewlyPressed) {
                setBit(&(memory[INPUT_REGISTER_ADDRESS]), i, 1);
            }

            if (isButtonNewlyPressed) {
                switch (button) {
                    // Printing values is handled after the switch
                    case ADDRESS_SET_BUTTON:
                        memory[P_REGISTER_ADDRESS] = memory[INPUT_REGISTER_ADDRESS];
                        memory[INPUT_REGISTER_ADDRESS] = 0;
                        break;
                    case ADDRESS_DISPLAY_BUTTON:
                        lampToLightUp = ADDRESS_LAMP;
                        setControlLamps(lampToLightUp);
                        break;
                    case STORE_MEMORY_BUTTON:
                        memory[memory[P_REGISTER_ADDRESS]++] = memory[INPUT_REGISTER_ADDRESS];
                        memory[INPUT_REGISTER_ADDRESS] = 0;
                        break;
                    case READ_MEMORY_BUTTON:
                        lampToLightUp = MEMORY_LAMP;
                        setControlLamps(lampToLightUp);
                        break;
                    case START_BUTTON:
                        lampToLightUp = RUN_LAMP;
                        setControlLamps(lampToLightUp);
                        // STOP_BUTTON is handled inside here
                        execute();
                        lampToLightUp = ALL_LAMPS_OFF;
                        setControlLamps(lampToLightUp);
                        break;
                    default:
                        lampToLightUp = INPUT_LAMP;
                        setControlLamps(lampToLightUp);
                        break;
                }
                for (int j = 0; j < 8; ++j) {
                    if (lampToLightUp == INPUT_LAMP) {
                        gpio_put(LEDPins[j], getBit(memory[INPUT_REGISTER_ADDRESS], j));
                    }
                    if (lampToLightUp == ADDRESS_LAMP) {
                        gpio_put(LEDPins[j], getBit(memory[P_REGISTER_ADDRESS], j));
                    }
                    if (lampToLightUp == MEMORY_LAMP) {
                        gpio_put(LEDPins[j], getBit(memory[memory[P_REGISTER_ADDRESS]], j));
                        if (j == 7) {
                            ++(memory[P_REGISTER_ADDRESS]);
                        }
                    }
                    if (lampToLightUp == ALL_LAMPS_OFF) {
                        gpio_put(LEDPins[j], getBit(memory[OUTPUT_REGISTER_ADDRESS], j));
                    }
                    // RUN_LAMP is never on when here, so we don't need to check for it.
                }
            }
            pushButtonPinStates[i] = currentPushButtonPinStates[i];
        }

        // Make sure this does not run too fast.
        sleep_ms(100);

    }

    return 0;
}
