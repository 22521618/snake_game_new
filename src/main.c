#include <stdio.h>
#include <time.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_idf_version.h>
#include <max7219.h>
#include "snek_buttons.h"
#ifndef APP_CPU_NUM
#define APP_CPU_NUM PRO_CPU_NUM
#endif

#define BUTTS true

#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(4, 0, 0)
#define HOST HSPI_HOST
#else
#define HOST SPI2_HOST
#endif

#define CASCADE_SIZE 1
#define MOSI_PIN 26
#define CS_PIN 27
#define CLK_PIN 14

#define TURN_INTERVAL_MS 750

static max7219_t dev = {
    .cascade_size = CASCADE_SIZE,
    .digits = 0,
    .mirrored = true};
void init()
{

    spi_bus_config_t cfg = {
        .mosi_io_num = MOSI_PIN,
        .miso_io_num = -1,
        .sclk_io_num = CLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 0,
        .flags = 0};
    ESP_ERROR_CHECK(spi_bus_initialize(HOST, &cfg, 1));

    ESP_ERROR_CHECK(max7219_init_desc(&dev, HOST, MAX7219_MAX_CLOCK_SPEED_HZ, CS_PIN));
    ESP_ERROR_CHECK(max7219_init(&dev));
}
void task(int length)
{
    // void *pvParameter
    uint64_t symbols[] = {

        0x007e0018244a7e08, // digits 0-9
        0x10207e005a5a5a7e,
        0x0042423c007e2010,
        0x50503e007c02027c,
        0x00344a4a7e00003e,
        0x207e001e3050301e,
        0x002800007e040810, // :
        0x002800007e040810};
    if (length == 2)
    {
        symbols[7] = 0x0062f2928ace4600;
    }
    else if (length == 3)
    {
        symbols[7] = 0x006cfe9292c64400;
    }
    else if (length == 4)
    {
        symbols[7] = 0x0008fefe48281800;
    }
    else if (length == 5)
    {
        symbols[7] = 0x009cbea2a2e6e400;
    }
    else if (length == 6)
    {
        symbols[7] = 0x004cde9292fe7c00;
    }
    else if (length == 7)
    {
        symbols[7] = 0x00c0f0be8ec0c000;
    }
    else if (length == 8)
    {
        symbols[7] = 0x006c9292926c0000;
    }
    else if (length == 9)
    {
        symbols[7] = 0x007effc9c9fb7200;
    }
    else if (length == 10)
    {
        symbols[7] = 0x003c42423c007e20;
    }
    else if (length == 11)
    {
        symbols[7] = 0x0000007e20007e20;
    }

    size_t symbols_size = sizeof(symbols) - sizeof(uint64_t) * CASCADE_SIZE;
    size_t offs = 0;
    int k;
    for (k = 0; k <= 55; k++)
    {
        for (uint8_t i = 0; i < CASCADE_SIZE; i++)
            max7219_draw_image_8x8(&dev, i * 8, (uint8_t *)symbols + i * 8 + offs);
        vTaskDelay(pdMS_TO_TICKS(100));

        if (++offs == symbols_size)
            offs = 0;
    }
}
typedef enum
{
    dir_N = 0,
    dir_E = 1,
    dir_S = 2,
    dir_W = 3
} dir_t;

typedef struct
{
    uint8_t x;
    uint8_t y;
} pos_t;

#define BORDER_SZ 8

typedef struct
{
    pos_t pos[BORDER_SZ * BORDER_SZ];
    uint8_t length;
    dir_t dir;
} snake_t;

void move_snake(snake_t *snake)
{
    // Shift each 'pixel' on the snake by -1 position, except pos[0]
    for (uint8_t i = snake->length - 1; i >= 1; i--)
    {
        snake->pos[i] = snake->pos[i - 1];
    }

    // move snake.pos[0] based on direction
    switch (snake->dir)
    {
    case dir_N:
        snake->pos[0].y--;
        break;
    case dir_E:
        snake->pos[0].x++;
        break;
    case dir_S:
        snake->pos[0].y++;
        break;
    case dir_W:
        snake->pos[0].x--;
        break;
    }
}

pos_t move_fruit(snake_t snake)
{

    pos_t fruit;
    bool collides;

    do
    {
        // Generate random position
        fruit.x = (uint8_t)1 + (((double)rand() / RAND_MAX) * (BORDER_SZ - 1));
        fruit.y = (uint8_t)1 + (((double)rand() / RAND_MAX) * (BORDER_SZ - 1));

        collides = false;

        // Check each snake pixel for collision
        for (uint8_t i = 0; i < snake.length; i++)
        {
            if (snake.pos[i].x == fruit.x && snake.pos[i].y == fruit.y)
            {
                collides = true;
            }
        }
    } while (collides == true);
    return fruit;
}

bool check_gameover_conditions(const snake_t snake)
{

    // Stage 1: Check if snake.pos[0] is within bounds
    if (snake.pos[0].x <= BORDER_SZ && snake.pos[0].x > 0 && snake.pos[0].y <= BORDER_SZ && snake.pos[0].y > 0)
    {
        // If within bounds,
        for (uint8_t i = 1; i < snake.length; i++)
        {
            // check whether snake.pos[0] collision with other dots
            if (snake.pos[0].x == snake.pos[i].x && snake.pos[0].y == snake.pos[i].y)
            {
                return true;
            }
        }
        // if not within bounds, game over
    }
    else
        return true;

    // if all checks passed without returning, good to go
    return false;
}

void compose_map(uint8_t *map, const snake_t snake, const pos_t fruit)
{

    // The fruit blinks, for ease of identification.
    static bool show_fruit = true;

    // first, add the fruit to where it should be,
    // or hide it if blinking:
    if (show_fruit)
    {
        map[fruit.y - 1] |= 1 << (fruit.x - 1);
        // "rows[dot.ypos] |= 1 << (dot.xpos - 1)" syntax
        // is because each row is a 0x00-0xFF bitfield,
        // each bit (0 to 7) representing a segment (dot).
    }
    show_fruit = !show_fruit;

    // then, add each dot of the snake
    for (uint8_t i = 0; i < snake.length; i++)
    {
        map[snake.pos[i].y - 1] |= 1 << (snake.pos[i].x - 1);
    }
}

void game_over(max7219_t *dev)
{
    printf("it's over\n");
    const uint8_t sadface[] = {
        // thanks to xantorohara's LED Matrix editor
        0x00, 0xe7, 0x00, 0x22, 0x00, 0x3c, 0x42, 0x81
        // as before, sadface[0] is unused, using 1-8
    };
    max7219_render(dev, sadface);
}
void app_main()
{

    init();

    srand(time(NULL));
    button_config();
    while (true)
    {
        // Game setup
        uint16_t turn = 0;
        snake_t snake = {{{3, 7}, {3, 8}}, 2, dir_N};
        pos_t fruit = {3, 4}; // first fruit is easy

        /* HOW IT WORKS
            At the beginning of each turn, game state is rendered.
            The player is given an interval of time to react.
              This interval is also used to blink the fruit.
            At the end of the interval,
            user input is processed and the game state is updated,
            and the next turn starts.
        */

        // Game loop
        while (check_gameover_conditions(snake) == false)
        {

            // Dot matrix display blinks the fruit to distinguish it.
            const uint8_t blinks = 2;
            for (uint8_t i = 0; i < blinks; i++)
            {

                uint8_t map[8] = {0};
                compose_map(map, snake, fruit);
                max7219_render(&dev, map);

                vTaskDelay((TURN_INTERVAL_MS / blinks) / portTICK_PERIOD_MS);
            }

            if (BUTTS)
                // 2. Digest latest input
                if (button_pressed)
                {
                    switch (button_pressed)
                    {
                    // Do not allow turning 180 degrees
                    case (BTN_UP):
                        if (snake.dir != dir_S)
                            snake.dir = dir_N;
                        break;
                    case (BTN_R):
                        if (snake.dir != dir_W)
                            snake.dir = dir_E;
                        break;
                    case (BTN_DN):
                        if (snake.dir != dir_N)
                            snake.dir = dir_S;
                        break;
                    case (BTN_L):
                        if (snake.dir != dir_E)
                            snake.dir = dir_W;
                        break;
                    }
                    printf("butt %d pressed, dir now %d\n", button_pressed, snake.dir);
                    button_pressed = 0;
                }

            // 3. Update game state
            move_snake(&snake);
            // If snake eats fruit, increment length
            if (snake.pos[0].x == fruit.x && snake.pos[0].y == fruit.y)
            {
                snake.length++;

                // remove the fruit for 1 turn after eating it
                fruit.x = 0;
                fruit.y = 0;
                // after that, put it somewhere random
            }
            else if (fruit.x == 0 && fruit.y == 0)
            {
                fruit = move_fruit(snake);
            }

            turn++;
        }

        // show game over sadface for 3 s, then restart
        //
        // xTaskCreatePinnedToCore(task, "task", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL, APP_CPU_NUM);
        task(snake.length);

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}