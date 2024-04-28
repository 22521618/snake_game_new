#include <driver/gpio.h>

#define BTN_UP GPIO_NUM_23
#define BTN_DN GPIO_NUM_5
#define BTN_L GPIO_NUM_22
#define BTN_R GPIO_NUM_18
volatile extern uint8_t button_pressed;

void button_config();
