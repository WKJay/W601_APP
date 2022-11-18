#ifndef __WOL_H
#define __WOL_H

#include <stdint.h>

int wol(uint8_t *mac);
int wol_str(char *mac_str);

#endif /* __WOL_H */
