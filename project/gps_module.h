#ifndef GPS_MODULE_H
#define GPS_MODULE_H

#include <string.h> // 添加 string.h 以支持 strtok 和 strstr

typedef struct {
    char latitude[15];   // 纬度字符串（格式一般是ddmm.mmmm）
    char lat_dir;        // 纬度方向，N或S
    char longitude[15];  // 经度字符串（格式一般是dddmm.mmmm）
    char lon_dir;        // 经度方向，E或W
} GPS_Data_t;

void GPS_UART_Init(void);
int GPS_GetSentence(char *buf, int max_len);
int GPS_ParseGPRMC(const char *sentence,GPS_Data_t *da);
#endif