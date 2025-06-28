#include <reg52.h>
#include <string.h>

// LCD1602引脚定义
//sbit RS = P1^0;
//sbit RW = P1^1;
//sbit EN = P1^2;
sbit RS=P2^6;
sbit RW=P2^5;
sbit EN=P2^7;
#define LCD_DATA P0

// 调试指示灯引脚
sbit STATUS_LED = P2^0;  // 状态指示灯
sbit DATA_LED = P2^1;    // 数据接收指示灯

// GPS数据结构
typedef struct {
    char status;            // 定位状态：'A'有效，'V'无效
    char ns, ew;            // 南北纬(N/S)、东西经(E/W)
    unsigned long latitude; // 纬度（整数部分）
    unsigned long longitude;// 经度（整数部分）
    unsigned int lat_frac;  // 纬度小数部分（0-9999）
    unsigned int lon_frac;  // 经度小数部分（0-9999）
} GPS_Data;

// 优化内存使用
idata char gps_buffer[80];  // 缓冲区大小
bit gps_data_ready = 0;     // 数据就绪标志
data GPS_Data current_gps;  // 解析后的GPS数据

// 调试计数器
unsigned int rx_count = 0;  // 串口接收计数
unsigned int parse_count = 0; // 解析计数
unsigned int gga_count = 0; // GPGGA语句计数
unsigned int other_count = 0; // 其他语句计数

/*-------- LCD1602驱动函数 --------*/
void LCD_Delay(unsigned int ms) {
    unsigned int i, j;
    for (i = 0; i < ms; i++)
        for (j = 0; j < 114; j++);
}

void LCD_Write_Cmd(unsigned char cmd) {
    RS = 0; RW = 0; EN = 0;
    LCD_DATA = cmd;
    EN = 1; LCD_Delay(1); EN = 0;
}

void LCD_Write_Data(unsigned char dat) {
    RS = 1; RW = 0; EN = 0;
    LCD_DATA = dat;
    EN = 1; LCD_Delay(1); EN = 0;
}

void LCD_Init() {
    LCD_Write_Cmd(0x38);    // 8位模式，2行显示
    LCD_Write_Cmd(0x0C);    // 开显示，关光标
    LCD_Write_Cmd(0x06);    // 光标右移
    LCD_Write_Cmd(0x01);    // 清屏
    LCD_Delay(20);
}

void LCD_Set_Pos(unsigned char row, unsigned char col) {
    LCD_Write_Cmd(row ? 0xC0 | col : 0x80 | col);
}

void LCD_Show_Str(unsigned char row, unsigned char col, char *str) {
    LCD_Set_Pos(row, col);
    while (*str) {
        LCD_Write_Data(*str++);
    }
}

// 显示固定长度的数字
void LCD_Show_Num(unsigned char row, unsigned char col, unsigned long num, unsigned char len) {
    char buf[10];
    unsigned char i = 0;
    
    // 将数字转换为字符串（反向）
    do {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    } while (num && i < len);
    
    // 补足前导零
    while (i < len) buf[i++] = '0';
    
    // 反向输出（从最高位开始）
    LCD_Set_Pos(row, col);
    while (i--) {
        LCD_Write_Data(buf[i]);
    }
}

/*-------- 串口初始化 --------*/
void UART_Init() {
    TMOD = 0x20;            // 定时器1模式2（自动重载）
    TH1 = TL1 = 0xFD;       // 波特率9600（11.0592MHz）
    SCON = 0x50;            // 串口模式1，允许接收
    TR1 = 1;                // 启动定时器1
    EA = 1;                 // 开总中断
    ES = 1;                 // 开串口中断
    
    // 初始化指示灯
    STATUS_LED = 1;         // 初始关闭
    DATA_LED = 1;
}

/*-------- 通用NMEA解析器 --------*/
bit GPS_Parse(char *buffer) {
    char *ptr = buffer;
    unsigned char field = 0;
    unsigned long temp;
    unsigned char i; // 循环变量必须在开头声明
    char sentence_type[6] = {0};
    
    // 检查NMEA语句起始
    if (*ptr != '$') return 0;
    
    // 提取语句类型
    for (i = 0; i < 5 && ptr[i] != ',' && ptr[i] != '\0'; i++) {
        sentence_type[i] = ptr[i];
    }
    sentence_type[i] = '\0';
    
    // 只解析包含位置信息的语句
    if (strcmp(sentence_type, "$GPGGA") == 0) {
        gga_count++;
    } else if (strcmp(sentence_type, "$GNGGA") == 0) {
        gga_count++;
    } else if (strcmp(sentence_type, "$GPRMC") == 0) {
        other_count++;
    } else {
        return 0; // 忽略其他语句
    }
    
    // 通用解析逻辑
    while (*ptr && ptr < buffer + sizeof(gps_buffer)) {
        if (*ptr == ',') field++;
        
        // 纬度字段（GGA在第3字段，RMC在第4字段）
        if ((field == 2 && (strcmp(sentence_type, "$GPGGA") == 0 || strcmp(sentence_type, "$GNGGA") == 0)) ||
            (field == 3 && strcmp(sentence_type, "$GPRMC") == 0)) {
            if (*(ptr+1) != ',' && *(ptr+1) != '\0') {
                temp = 0;
                ptr++; // 跳过逗号
                
                // 解析纬度整数部分
                while (*ptr != '.' && *ptr != ',' && *ptr != '\0') {
                    if (*ptr >= '0' && *ptr <= '9') {
                        temp = temp * 10 + (*ptr - '0');
                    }
                    ptr++;
                }
                
                if (temp > 0) {
                    current_gps.latitude = temp / 100;   // 度
                    current_gps.lat_frac = (temp % 100) * 100; // 分整数部分
                    
                    // 处理小数部分
                    if (*ptr == '.') {
                        ptr++;
                        temp = 0;
                        for (i = 0; i < 4 && *ptr >= '0' && *ptr <= '9'; i++) {
                            temp = temp * 10 + (*ptr - '0');
                            ptr++;
                        }
                        current_gps.lat_frac += temp; // 合并小数部分
                    }
                }
            }
        }
        
        // 南北纬字段
        if ((field == 3 && (strcmp(sentence_type, "$GPGGA") == 0 || strcmp(sentence_type, "$GNGGA") == 0)) ||
            (field == 4 && strcmp(sentence_type, "$GPRMC") == 0)) {
            if (*(ptr+1) == 'N' || *(ptr+1) == 'S') {
                current_gps.ns = *(ptr+1);
            }
        }
        
        // 经度字段
        if ((field == 4 && (strcmp(sentence_type, "$GPGGA") == 0 || strcmp(sentence_type, "$GNGGA") == 0)) ||
            (field == 5 && strcmp(sentence_type, "$GPRMC") == 0)) {
            if (*(ptr+1) != ',' && *(ptr+1) != '\0') {
                temp = 0;
                ptr++; // 跳过逗号
                
                // 解析经度整数部分
                while (*ptr != '.' && *ptr != ',' && *ptr != '\0') {
                    if (*ptr >= '0' && *ptr <= '9') {
                        temp = temp * 10 + (*ptr - '0');
                    }
                    ptr++;
                }
                
                if (temp > 0) {
                    current_gps.longitude = temp / 100;   // 度
                    current_gps.lon_frac = (temp % 100) * 100; // 分整数部分
                    
                    // 处理小数部分
                    if (*ptr == '.') {
                        ptr++;
                        temp = 0;
                        for (i = 0; i < 4 && *ptr >= '0' && *ptr <= '9'; i++) {
                            temp = temp * 10 + (*ptr - '0');
                            ptr++;
                        }
                        current_gps.lon_frac += temp; // 合并小数部分
                    }
                }
            }
        }
        
        // 东西经字段
        if ((field == 5 && (strcmp(sentence_type, "$GPGGA") == 0 || strcmp(sentence_type, "$GNGGA") == 0)) ||
            (field == 6 && strcmp(sentence_type, "$GPRMC") == 0)) {
            if (*(ptr+1) == 'E' || *(ptr+1) == 'W') {
                current_gps.ew = *(ptr+1);
            }
        }
        
        // 定位状态字段
        if ((field == 6 && (strcmp(sentence_type, "$GPGGA") == 0 || strcmp(sentence_type, "$GNGGA") == 0)) ||
            (field == 2 && strcmp(sentence_type, "$GPRMC") == 0)) {
            if (*(ptr+1) == '0' || *(ptr+1) == '1' || *(ptr+1) == 'A' || *(ptr+1) == 'V') {
                char status_char = *(ptr+1);
                if (status_char == '0' || status_char == 'V') {
                    current_gps.status = 'V';
                } else if (status_char == '1' || status_char == 'A') {
                    current_gps.status = 'A';
                    STATUS_LED = 0; // 点亮状态灯
                }
                return 1; // 解析成功
            }
        }
        
        ptr++;
    }
    return 0; // 解析失败
}

/*-------- 串口中断服务程序 --------*/
void UART_ISR() interrupt 4 {
    static unsigned char i = 0;
    char ch;
    
    if (RI) {
        ch = SBUF;
        RI = 0;
        rx_count++; // 增加接收计数
        
        // 数据接收指示灯
        DATA_LED = 0; // 接收数据时点亮
        
        // 帧头检测
        if (ch == '$') {
            i = 0;
            gps_buffer[i++] = ch;
        } 
        // 数据接收
        else if (i > 0 && i < sizeof(gps_buffer) - 1) {
            gps_buffer[i++] = ch;
        }
        
        // 帧尾检测（换行符）
        if (ch == '\n' && i > 10) { // 确保有足够的数据
            gps_buffer[i] = '\0'; // 字符串结束符
            if (GPS_Parse(gps_buffer)) { // 解析数据
                gps_data_ready = 1;
                parse_count++;
            }
            i = 0; // 重置缓冲区索引
        }
    }
}

/*-------- 显示诊断信息 --------*/
void Show_Diagnostic_Info() {
    // 第一行：接收计数和语句类型
    LCD_Set_Pos(0, 0);
    LCD_Write_Data('R');
    LCD_Write_Data('x');
    LCD_Write_Data(':');
    LCD_Show_Num(0, 3, rx_count, 5);
    
    LCD_Set_Pos(0, 8);
    LCD_Write_Data('G');
    LCD_Write_Data('G');
    LCD_Write_Data('A');
    LCD_Write_Data(':');
    LCD_Show_Num(0, 12, gga_count, 3);
    
    // 第二行：其他语句计数和状态
    LCD_Set_Pos(1, 0);
    LCD_Write_Data('O');
    LCD_Write_Data('t');
    LCD_Write_Data('h');
    LCD_Write_Data(':');
    LCD_Show_Num(1, 4, other_count, 3);
    
    LCD_Set_Pos(1, 8);
    LCD_Write_Data('S');
    LCD_Write_Data('t');
    LCD_Write_Data(':');
    LCD_Write_Data(current_gps.status);
}

/*-------- 显示原始数据（调试用） --------*/
void Show_Raw_Data() {
    unsigned char i; // 循环变量必须在开头声明
    
    // 显示接收到的原始数据（前16字节）
    LCD_Set_Pos(0, 0);
    i = 0;
    while (i < 16 && gps_buffer[i] != '\0') {
        // 只显示可打印字符
        if (gps_buffer[i] >= 32 && gps_buffer[i] <= 126) {
            LCD_Write_Data(gps_buffer[i]);
        } else {
            LCD_Write_Data('?'); // 非打印字符显示为?
        }
        i++;
    }
    
    // 第二行显示后16字节
    LCD_Set_Pos(1, 0);
    i = 16;
    while (i < 32 && gps_buffer[i] != '\0') {
        if (gps_buffer[i] >= 32 && gps_buffer[i] <= 126) {
            LCD_Write_Data(gps_buffer[i]);
        } else {
            LCD_Write_Data('?');
        }
        i++;
    }
}

/*-------- 主函数 --------*/
void main() {
    unsigned int debug_timer = 0;
    bit show_raw_data = 1; // 初始显示原始数据
    
    // 初始化变量
    current_gps.status = 'V';
    current_gps.ns = 'N';
    current_gps.ew = 'E';
    current_gps.latitude = 0;
    current_gps.longitude = 0;
    current_gps.lat_frac = 0;
    current_gps.lon_frac = 0;
    
    UART_Init();
    LCD_Init();
    
    // 初始显示
    LCD_Show_Str(0, 0, "GPS Debug Mode");
    LCD_Show_Str(1, 1, "Show Raw Data");
    LCD_Delay(2000);
    LCD_Write_Cmd(0x01); // 清屏
    
    while (1) {
        debug_timer++;
        
        // 每0.5秒更新显示
        if (debug_timer >= 500) {
            if (show_raw_data) {
                Show_Raw_Data();
            } else {
                Show_Diagnostic_Info();
            }
            
            // 熄灭数据指示灯
            DATA_LED = 1;
            debug_timer = 0;
        }
        
        // 如果收到有效数据，切换为解析显示
        if (gps_data_ready) {
            show_raw_data = 0;
            
            // 第一行显示纬度
            LCD_Set_Pos(0, 0);
            LCD_Write_Data('L');
            LCD_Write_Data('a');
            LCD_Write_Data('t');
            LCD_Write_Data(':');
            LCD_Show_Num(0, 4, current_gps.latitude, 2);
            LCD_Write_Data('.');
            LCD_Show_Num(0, 7, current_gps.lat_frac, 4);
            LCD_Set_Pos(0, 14);
            LCD_Write_Data(current_gps.ns);
            LCD_Set_Pos(0, 15);
            LCD_Write_Data(current_gps.status);
            
            // 第二行显示经度
            LCD_Set_Pos(1, 0);
            LCD_Write_Data('L');
            LCD_Write_Data('o');
            LCD_Write_Data('n');
            LCD_Write_Data(':');
            LCD_Show_Num(1, 4, current_gps.longitude, 3);
            LCD_Write_Data('.');
            LCD_Show_Num(1, 8, current_gps.lon_frac, 4);
            LCD_Set_Pos(1, 14);
            LCD_Write_Data(current_gps.ew);
            
            gps_data_ready = 0;
        }
        
        // 如果连续10秒无有效数据，返回原始数据显示
        if (debug_timer > 10000 && parse_count == 0) {
            show_raw_data = 1;
            LCD_Write_Cmd(0x01); // 清屏
        }
        
        LCD_Delay(1);
    }
}