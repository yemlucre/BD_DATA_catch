#include <reg52.h>
#include <string.h>

// LCD1602���Ŷ���
//sbit RS = P1^0;
//sbit RW = P1^1;
//sbit EN = P1^2;
sbit RS=P2^6;
sbit RW=P2^5;
sbit EN=P2^7;
#define LCD_DATA P0

// ����ָʾ������
sbit STATUS_LED = P2^0;  // ״ָ̬ʾ��
sbit DATA_LED = P2^1;    // ���ݽ���ָʾ��

// GPS���ݽṹ
typedef struct {
    char status;            // ��λ״̬��'A'��Ч��'V'��Ч
    char ns, ew;            // �ϱ�γ(N/S)��������(E/W)
    unsigned long latitude; // γ�ȣ��������֣�
    unsigned long longitude;// ���ȣ��������֣�
    unsigned int lat_frac;  // γ��С�����֣�0-9999��
    unsigned int lon_frac;  // ����С�����֣�0-9999��
} GPS_Data;

// �Ż��ڴ�ʹ��
idata char gps_buffer[80];  // ��������С
bit gps_data_ready = 0;     // ���ݾ�����־
data GPS_Data current_gps;  // �������GPS����

// ���Լ�����
unsigned int rx_count = 0;  // ���ڽ��ռ���
unsigned int parse_count = 0; // ��������
unsigned int gga_count = 0; // GPGGA������
unsigned int other_count = 0; // ����������

/*-------- LCD1602�������� --------*/
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
    LCD_Write_Cmd(0x38);    // 8λģʽ��2����ʾ
    LCD_Write_Cmd(0x0C);    // ����ʾ���ع��
    LCD_Write_Cmd(0x06);    // �������
    LCD_Write_Cmd(0x01);    // ����
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

// ��ʾ�̶����ȵ�����
void LCD_Show_Num(unsigned char row, unsigned char col, unsigned long num, unsigned char len) {
    char buf[10];
    unsigned char i = 0;
    
    // ������ת��Ϊ�ַ���������
    do {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    } while (num && i < len);
    
    // ����ǰ����
    while (i < len) buf[i++] = '0';
    
    // ��������������λ��ʼ��
    LCD_Set_Pos(row, col);
    while (i--) {
        LCD_Write_Data(buf[i]);
    }
}

/*-------- ���ڳ�ʼ�� --------*/
void UART_Init() {
    TMOD = 0x20;            // ��ʱ��1ģʽ2���Զ����أ�
    TH1 = TL1 = 0xFD;       // ������9600��11.0592MHz��
    SCON = 0x50;            // ����ģʽ1���������
    TR1 = 1;                // ������ʱ��1
    EA = 1;                 // �����ж�
    ES = 1;                 // �������ж�
    
    // ��ʼ��ָʾ��
    STATUS_LED = 1;         // ��ʼ�ر�
    DATA_LED = 1;
}

/*-------- ͨ��NMEA������ --------*/
bit GPS_Parse(char *buffer) {
    char *ptr = buffer;
    unsigned char field = 0;
    unsigned long temp;
    unsigned char i; // ѭ�����������ڿ�ͷ����
    char sentence_type[6] = {0};
    
    // ���NMEA�����ʼ
    if (*ptr != '$') return 0;
    
    // ��ȡ�������
    for (i = 0; i < 5 && ptr[i] != ',' && ptr[i] != '\0'; i++) {
        sentence_type[i] = ptr[i];
    }
    sentence_type[i] = '\0';
    
    // ֻ��������λ����Ϣ�����
    if (strcmp(sentence_type, "$GPGGA") == 0) {
        gga_count++;
    } else if (strcmp(sentence_type, "$GNGGA") == 0) {
        gga_count++;
    } else if (strcmp(sentence_type, "$GPRMC") == 0) {
        other_count++;
    } else {
        return 0; // �����������
    }
    
    // ͨ�ý����߼�
    while (*ptr && ptr < buffer + sizeof(gps_buffer)) {
        if (*ptr == ',') field++;
        
        // γ���ֶΣ�GGA�ڵ�3�ֶΣ�RMC�ڵ�4�ֶΣ�
        if ((field == 2 && (strcmp(sentence_type, "$GPGGA") == 0 || strcmp(sentence_type, "$GNGGA") == 0)) ||
            (field == 3 && strcmp(sentence_type, "$GPRMC") == 0)) {
            if (*(ptr+1) != ',' && *(ptr+1) != '\0') {
                temp = 0;
                ptr++; // ��������
                
                // ����γ����������
                while (*ptr != '.' && *ptr != ',' && *ptr != '\0') {
                    if (*ptr >= '0' && *ptr <= '9') {
                        temp = temp * 10 + (*ptr - '0');
                    }
                    ptr++;
                }
                
                if (temp > 0) {
                    current_gps.latitude = temp / 100;   // ��
                    current_gps.lat_frac = (temp % 100) * 100; // ����������
                    
                    // ����С������
                    if (*ptr == '.') {
                        ptr++;
                        temp = 0;
                        for (i = 0; i < 4 && *ptr >= '0' && *ptr <= '9'; i++) {
                            temp = temp * 10 + (*ptr - '0');
                            ptr++;
                        }
                        current_gps.lat_frac += temp; // �ϲ�С������
                    }
                }
            }
        }
        
        // �ϱ�γ�ֶ�
        if ((field == 3 && (strcmp(sentence_type, "$GPGGA") == 0 || strcmp(sentence_type, "$GNGGA") == 0)) ||
            (field == 4 && strcmp(sentence_type, "$GPRMC") == 0)) {
            if (*(ptr+1) == 'N' || *(ptr+1) == 'S') {
                current_gps.ns = *(ptr+1);
            }
        }
        
        // �����ֶ�
        if ((field == 4 && (strcmp(sentence_type, "$GPGGA") == 0 || strcmp(sentence_type, "$GNGGA") == 0)) ||
            (field == 5 && strcmp(sentence_type, "$GPRMC") == 0)) {
            if (*(ptr+1) != ',' && *(ptr+1) != '\0') {
                temp = 0;
                ptr++; // ��������
                
                // ����������������
                while (*ptr != '.' && *ptr != ',' && *ptr != '\0') {
                    if (*ptr >= '0' && *ptr <= '9') {
                        temp = temp * 10 + (*ptr - '0');
                    }
                    ptr++;
                }
                
                if (temp > 0) {
                    current_gps.longitude = temp / 100;   // ��
                    current_gps.lon_frac = (temp % 100) * 100; // ����������
                    
                    // ����С������
                    if (*ptr == '.') {
                        ptr++;
                        temp = 0;
                        for (i = 0; i < 4 && *ptr >= '0' && *ptr <= '9'; i++) {
                            temp = temp * 10 + (*ptr - '0');
                            ptr++;
                        }
                        current_gps.lon_frac += temp; // �ϲ�С������
                    }
                }
            }
        }
        
        // �������ֶ�
        if ((field == 5 && (strcmp(sentence_type, "$GPGGA") == 0 || strcmp(sentence_type, "$GNGGA") == 0)) ||
            (field == 6 && strcmp(sentence_type, "$GPRMC") == 0)) {
            if (*(ptr+1) == 'E' || *(ptr+1) == 'W') {
                current_gps.ew = *(ptr+1);
            }
        }
        
        // ��λ״̬�ֶ�
        if ((field == 6 && (strcmp(sentence_type, "$GPGGA") == 0 || strcmp(sentence_type, "$GNGGA") == 0)) ||
            (field == 2 && strcmp(sentence_type, "$GPRMC") == 0)) {
            if (*(ptr+1) == '0' || *(ptr+1) == '1' || *(ptr+1) == 'A' || *(ptr+1) == 'V') {
                char status_char = *(ptr+1);
                if (status_char == '0' || status_char == 'V') {
                    current_gps.status = 'V';
                } else if (status_char == '1' || status_char == 'A') {
                    current_gps.status = 'A';
                    STATUS_LED = 0; // ����״̬��
                }
                return 1; // �����ɹ�
            }
        }
        
        ptr++;
    }
    return 0; // ����ʧ��
}

/*-------- �����жϷ������ --------*/
void UART_ISR() interrupt 4 {
    static unsigned char i = 0;
    char ch;
    
    if (RI) {
        ch = SBUF;
        RI = 0;
        rx_count++; // ���ӽ��ռ���
        
        // ���ݽ���ָʾ��
        DATA_LED = 0; // ��������ʱ����
        
        // ֡ͷ���
        if (ch == '$') {
            i = 0;
            gps_buffer[i++] = ch;
        } 
        // ���ݽ���
        else if (i > 0 && i < sizeof(gps_buffer) - 1) {
            gps_buffer[i++] = ch;
        }
        
        // ֡β��⣨���з���
        if (ch == '\n' && i > 10) { // ȷ�����㹻������
            gps_buffer[i] = '\0'; // �ַ���������
            if (GPS_Parse(gps_buffer)) { // ��������
                gps_data_ready = 1;
                parse_count++;
            }
            i = 0; // ���û���������
        }
    }
}

/*-------- ��ʾ�����Ϣ --------*/
void Show_Diagnostic_Info() {
    // ��һ�У����ռ������������
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
    
    // �ڶ��У�������������״̬
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

/*-------- ��ʾԭʼ���ݣ������ã� --------*/
void Show_Raw_Data() {
    unsigned char i; // ѭ�����������ڿ�ͷ����
    
    // ��ʾ���յ���ԭʼ���ݣ�ǰ16�ֽڣ�
    LCD_Set_Pos(0, 0);
    i = 0;
    while (i < 16 && gps_buffer[i] != '\0') {
        // ֻ��ʾ�ɴ�ӡ�ַ�
        if (gps_buffer[i] >= 32 && gps_buffer[i] <= 126) {
            LCD_Write_Data(gps_buffer[i]);
        } else {
            LCD_Write_Data('?'); // �Ǵ�ӡ�ַ���ʾΪ?
        }
        i++;
    }
    
    // �ڶ�����ʾ��16�ֽ�
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

/*-------- ������ --------*/
void main() {
    unsigned int debug_timer = 0;
    bit show_raw_data = 1; // ��ʼ��ʾԭʼ����
    
    // ��ʼ������
    current_gps.status = 'V';
    current_gps.ns = 'N';
    current_gps.ew = 'E';
    current_gps.latitude = 0;
    current_gps.longitude = 0;
    current_gps.lat_frac = 0;
    current_gps.lon_frac = 0;
    
    UART_Init();
    LCD_Init();
    
    // ��ʼ��ʾ
    LCD_Show_Str(0, 0, "GPS Debug Mode");
    LCD_Show_Str(1, 1, "Show Raw Data");
    LCD_Delay(2000);
    LCD_Write_Cmd(0x01); // ����
    
    while (1) {
        debug_timer++;
        
        // ÿ0.5�������ʾ
        if (debug_timer >= 500) {
            if (show_raw_data) {
                Show_Raw_Data();
            } else {
                Show_Diagnostic_Info();
            }
            
            // Ϩ������ָʾ��
            DATA_LED = 1;
            debug_timer = 0;
        }
        
        // ����յ���Ч���ݣ��л�Ϊ������ʾ
        if (gps_data_ready) {
            show_raw_data = 0;
            
            // ��һ����ʾγ��
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
            
            // �ڶ�����ʾ����
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
        
        // �������10������Ч���ݣ�����ԭʼ������ʾ
        if (debug_timer > 10000 && parse_count == 0) {
            show_raw_data = 1;
            LCD_Write_Cmd(0x01); // ����
        }
        
        LCD_Delay(1);
    }
}