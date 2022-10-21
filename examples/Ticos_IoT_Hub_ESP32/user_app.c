#include "driver/gpio.h"
#include "user_app.h"
#include "ti_iot_api.h"

#define KEY_GPIO        16
#define LED_GPIO        7

void board_init()
{
    gpio_set_direction(KEY_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(KEY_GPIO, GPIO_PULLUP_ONLY);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
}

void led_ctl(int status)
{
    gpio_set_level(LED_GPIO, status);
}

static int key_down()
{
    return !gpio_get_level(KEY_GPIO);
}

static key_cb_t key_cb;
static void *key_user_data = NULL;
static int key_process = 0;

void set_key_cb(key_cb_t cb, void *user_data)
{
    key_cb = cb;
    key_user_data = user_data;
}

void key_scan()
{
    if (key_down()) {
        if (!key_process) {
            key_process = 1;
            if (key_cb)
                key_cb(1, key_user_data);
        }
    } else {
        key_process = 0;
    }
}

static int switch_state = 0;
static int led_light = 0;
int get_led_light()
{
    return led_light;
}

void set_led_light(int light)
{
    led_light = light;
    led_ctl(led_light);
}

int get_switch_state()
{
    return switch_state;
}

static void user_key_cb(int key, void *user_data)
{
    if (switch_state) {
        switch_state = 0;
    } else {
        switch_state = 1;
    }
    if (led_light) {
        led_light = 0;
    } else {
        led_light = 1;
    }
    led_ctl(led_light);
    ti_iot_property_report();
}

void user_init()
{
    board_init();
    set_led_light(led_light);
    set_key_cb(user_key_cb, NULL);
}

