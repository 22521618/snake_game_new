#include "snek_buttons.h"

gpio_num_t buttons[4] = {BTN_UP, BTN_DN, BTN_L, BTN_R};
volatile uint8_t button_pressed = 0;

static void gpio_isr_handler(void *arg)
{
    // Write the pressed button into memory,
    // the main loop will process it in time.
    button_pressed = *((uint8_t *)arg);
    // I played around with fancy debouncing,
    // but it's unnecessary in this project.
}

void button_config()
{
    gpio_install_isr_service(0);

    for (uint8_t i = 0; i < 4; i++)
    {
        gpio_config_t button_conf = {
            .pin_bit_mask = 1 << buttons[i],
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_NEGEDGE // Interrupt on button press
        };
        ESP_ERROR_CHECK(gpio_config(&button_conf));
        // *arg will contain the gpio_num_t of the button.
        ESP_ERROR_CHECK(gpio_isr_handler_add(buttons[i], gpio_isr_handler, (void *)(buttons + i)));
    }
}
