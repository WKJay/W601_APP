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

void led_module_init(void);
void led_write(led_id_e led,uint8_t status);
char *json_create_device_status(void);

#endif /*__BOARD_MODULE_H*/
