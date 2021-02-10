#ifndef __BOARD_MODULE_H
#define __BOARD_MODULE_H
#include <stdint.h>

#define LED_ON  0
#define LED_OFF 1

typedef enum
{
    RED,
    GREEN,
    BLUE
} led_id_e;

typedef struct
{
    uint8_t red_led_status;
    uint8_t green_led_status;
    uint8_t blue_led_status;
} board_device_t;

extern board_device_t w601_board;

void led_module_init(void);
void led_write(led_id_e led,uint8_t status);
char *json_create_device_status(void);

#endif /*__BOARD_MODULE_H*/
