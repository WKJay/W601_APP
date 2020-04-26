#ifndef __AP3216C_MODULE_H
#define __AP3216C_MODULE_H

typedef struct
{
    float cur_light;         
    float cur_dis;            
} w601_ap3216c_t;


int ap3216c_module_init(void);

#endif
