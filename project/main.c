#include "gps_module.h"
#include <REGX52.H>
#include <string.h>
#include "LCD1602.h"
void main() {
static xdata char sentence[100];
    GPS_Data_t gps_data;

    GPS_UART_Init();  // 这里你写硬件初始化

    while (1) {
        // 轮询获取完整NMEA句
        if (GPS_GetSentence(sentence, sizeof(sentence))) {
            if (GPS_ParseGPRMC(sentence, &gps_data)) {
                // 成功解析，拿到经纬度了
            LCD_ShowString(0, 0,gps_data.latitude);
//			LCD_ShowString(0, 20,gps_data.lat_dir);
			LCD_ShowString(1, 0,gps_data.longitude);
//			LCD_ShowString(1, 20,gps_data.lon_dir);				
            }
        }
        // 其他任务
    }
}
