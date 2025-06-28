#include <reg52.h>
#include <intrins.h>
#include "LCD1602.h"
#include "Delay.h"
#include "FONT.h"
/*
 * 串口接收状态指示变量
 * GPS_i 是接收字段的索引（比如接收小时、分钟、经纬度各位数时用的）
 * RX_count 是状态机的状态（接收NMEA语句逐步解析时用的）
 */
unsigned char GPS_i = 0;
unsigned char RX_count = 0;

/*
 * GPS 信息结构体
 * 用于保存解析出的时间和经纬度
 */
data struct {
    unsigned char EW;                    // E / W
    unsigned char latiude_Degree[5];     // 纬度数值各位（假设拆成5位数字）

    unsigned char NS;                    // N / S
    unsigned char longitude_Degree[4];   // 经度数值各位（拆成4位数字）

    unsigned char time_hour[2];          // 小时（2位）
    unsigned char time_min[2];           // 分钟（2位）
    unsigned char time_sec[2];           // 秒（2位）
} GPS_INFO = {
    'E', {1, 1, 3, 2, 4},
    'N', {2, 4, 1, 7},
    {1, 1}, {2, 4}, {0, 8}
}; // 预设初始值便于上电后调试显示

/*
 * 串口初始化
 * 9600bps@11.0592MHz
 */
void UsartInit()
{
    TMOD = 0x20;      // 定时器1，方式2（8位自动重装）
    SCON = 0x50;      // 串口工作方式1，允许接收
    TH1 = 0xFA;       // 波特率设置
    TL1 = TH1;
    PCON = 0x80;      // SMOD=1，波特率加倍
    EA = 1;           // 开总中断
    ES = 1;           // 开串口中断
    TR1 = 1;          // 启动定时器1
}


/*
 * 主函数
 * 初始化LCD并循环刷新显示
 */
void main()
{
    LCD_Init();        // 初始化LCD
    UsartInit();      // 初始化串口

	
    while (1)
    {

        // 经度方向
        if (GPS_INFO.EW == 'E')
            LCD_ShowChar(0, 1, 'E'); // “东”
        else if (GPS_INFO.EW == 'W')
            LCD_ShowChar(0, 1, 'W'); // “西”

        // 经度数值
        LCD_ShowChar(0, 4,GPS_INFO.latiude_Degree[0]);  // 经度百位
        LCD_ShowChar(0, 5,GPS_INFO.latiude_Degree[1]);   // 十位
        LCD_ShowChar(0, 6,GPS_INFO.latiude_Degree[2]);   // 个位
        LCD_ShowChar(0, 8,GPS_INFO.latiude_Degree[3]);  // 分十位
        LCD_ShowChar(0, 9,GPS_INFO.latiude_Degree[4]);  // 分个位

        // 纬度方向
        if (GPS_INFO.NS == 'S')
            LCD_ShowChar(1, 1, 'S');  // “南”
        else if (GPS_INFO.NS == 'N')
            LCD_ShowChar(1, 1, 'N'); // “北”

        // 纬度数值
        LCD_ShowChar(1, 5, GPS_INFO.longitude_Degree[0]); // 纬度百位
        LCD_ShowChar(1, 6, GPS_INFO.longitude_Degree[1]); // 十位
        LCD_ShowChar(1, 8, GPS_INFO.longitude_Degree[2]);// 分十位
        LCD_ShowChar(1, 9, GPS_INFO.longitude_Degree[3]);// 分个位
				
				Delay(100);
    }
}


/*
 * 串口中断服务函数
 * 负责解析$GPGGA句型里提取的时间、经度、纬度等信息
 */
/*
 * 串口中断服务函数
 * 负责解析$GPRMC句型里提取的时间、经度、纬度等信息
 * 只要大致能显示即可
 */
void Usart() interrupt 4
{
    unsigned char receiveData;

    if (RI)
    {
        receiveData = SBUF;
        RI = 0;

        switch (RX_count)
        {
            // ----------------- 头部 $GNRMC, -----------------
            case 0: RX_count = (receiveData == '$') ? 1 : 0; break;
            case 1: RX_count = (receiveData == 'G') ? 2 : 0; break;
            case 2: RX_count = (receiveData == 'N') ? 3 : 0; break;
            case 3: RX_count = (receiveData == 'R') ? 4 : 0; break;
            case 4: RX_count = (receiveData == 'M') ? 5 : 0; break;
            case 5: RX_count = (receiveData == 'C') ? 6 : 0; break;
            case 6: RX_count = (receiveData == ',') ? 7 : 0; break;

            // ----------------- 6~8 跳过字段1(时间)和字段2(状态A/V) -----------------
            case 7:
                if (receiveData == ',') RX_count = 8;
                break;
            case 8:
                if (receiveData == ',') RX_count = 9;
                break;

            // ----------------- 8: 纬度字段（数字部分） -----------------
            case 9:
                if (receiveData == ',') {
                    RX_count = 10;
                    GPS_i = 0;
                }
                else if (receiveData >= '0' && receiveData <= '9' && GPS_i < 5)
                    GPS_INFO.latiude_Degree[GPS_i++] = receiveData - '0';
                break;

            // ----------------- 9: N/S方向 -----------------
            case 10:
                GPS_INFO.NS = receiveData;
                RX_count = 11;
                break;

            // ----------------- 10: 逗号之后到经度字段 -----------------
            case 11:
                if (receiveData == ',') RX_count = 12;
                break;

            // ----------------- 11: 经度字段（数字部分） -----------------
            case 12:
                if (receiveData == ',') {
                    RX_count = 13;
                    GPS_i = 0;
                }
                else if (receiveData >= '0' && receiveData <= '9' && GPS_i < 4)
                    GPS_INFO.longitude_Degree[GPS_i++] = receiveData - '0';
                break;

            // ----------------- 12: E/W方向 -----------------
            case 13:
                GPS_INFO.EW = receiveData;
                RX_count = 0;
                break;

            default:
                RX_count = 0;
                break;
        }
    }
    else
        TI = 0;
}
