#include "gps_module.h"
#include <REGX52.H>
#include <string.h>
#define GPS_RX_BUF_SIZE 256
// 放到 XDATA 区，避免占用 DATA 段
static xdata char parse_buf[100];
static xdata volatile char rx_buf[GPS_RX_BUF_SIZE];
static volatile unsigned int rx_head = 0;
static volatile unsigned int rx_tail = 0;

static int uart_getchar(void) {
    char c;
    if (rx_tail == rx_head) return -1;
    c = rx_buf[rx_tail];
    rx_tail = (rx_tail + 1) % GPS_RX_BUF_SIZE;
    return c;
}

void GPS_UART_Init(void)
{
    TMOD &= 0x0F;
    TMOD |= 0x20;    // Timer1模式2
    TH1 = 0xFD;      // 9600bps@11.0592MHz
    TL1 = 0xFD;
    TR1 = 1;         // 启动定时器
    SCON = 0x50;     // 8位数据，可变波特率
    TI = 1;
    RI = 0;
    ES = 1;          // 使能串口中断
    EA = 1;          // 全局中断使能
}

void UART_ISR(void) interrupt 4
{
    char c;
    unsigned int next;

    if (RI) {
        RI = 0;
        c = SBUF;
        next = (rx_head + 1) % GPS_RX_BUF_SIZE;
        if (next != rx_tail) {
            rx_buf[rx_head] = c;
            rx_head = next;
        }
    }
    if (TI) {
        TI = 0;
    }
}

int GPS_GetSentence(char *sentence, int max_len) 
{
    static int idx = 0;
    int c;
    while ((c = uart_getchar()) != -1) {
        if (c == '$') {
            idx = 0;
        }
        if (idx < max_len - 1) {
            sentence[idx++] = (char)c;
        }
        if (c == '\n' && idx > 6) {
            sentence[idx] = 0;
            return 1;
        }
    }
    return 0;
}

int GPS_ParseGPRMC(const char *sentence, GPS_Data_t *da) 
{
    char *token;
    int field = 0;

    if (!sentence || !da)
        return 0;

    if (strstr(sentence, "GPRMC") == NULL)
        return 0;

    strcpy(parse_buf, sentence);

    token = strtok(parse_buf, ",");

    while (token) {
        field++;
        switch (field) {
            case 3:
                strcpy(da->latitude, token);
                break;
            case 4:
                da->lat_dir = token[0];
                break;
            case 5:
                strcpy(da->longitude, token);
                break;
            case 6:
                da->lon_dir = token[0];
                break;
            default:
                break;
        }
        token = strtok(NULL, ",");
    }

    if (da->latitude[0] == '\0' || da->longitude[0] == '\0')
        return 0;

    return 1;
}
