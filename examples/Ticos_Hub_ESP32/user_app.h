#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void board_init();
void led_ctl(int status);

typedef void (*key_cb_t)(int key, void *user_data);
void set_key_cb(key_cb_t cb, void *user_data);
void key_scan();

int get_led_light();
void set_led_light(int light);
int get_switch_state();

void user_init();

#ifdef __cplusplus
}
#endif