#ifndef AHT10_MODULE_H
#define AHT10_MODULE_H

#include <stdint.h>

//模块初始化
int aht10_device_init(void);
//aht10 线程创建
int aht10_module_init(void);

typedef struct
{
    float cur_temp;         //当前温度
    float cur_humi;         //当前湿度
    float temp_warn;        //温度报警值
    float temp_data[24];    //24小时内的温度数据
    float humi_data[24];    //24小时内的湿度数据
    uint8_t cur_temp_index; //当前写入的温度数组索引
    uint8_t cur_humi_index; //当前写入的湿度数组索引
} w601_aht10_t;

extern w601_aht10_t w601_aht10;

float *aht10_temp_data_get(void);
float *aht10_humi_data_get(void);
char *json_create_aht10_current_data(void);
char *json_create_aht10_saved_data(void);

#endif
