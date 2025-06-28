#ifndef _LCD_H
#define _LCD_H
#include <reg52.h>
#include <intrins.h>
#define uint8 unsigned char
#define uint16 unsigned int
#define LCD_databus P0 //���߽ӿڣ�ע��51��P0����Ҫ�����������
sbit DI = P2^3;	//DIΪ 0��дָ����״̬��1������
sbit RW = P2^4;	//1:д 0����
sbit EN = P2^5;	//ʹ��

sbit CS1 = P2^0;	//Ƭѡ1.�͵�ƽ��Ч�����������
sbit CS2 = P2^1;  //Ƭѡ2���͵�ƽ��Ч����������
sbit CS3 = P2^2;  //Ƭѡ3���͵�ƽ��Ч�������Ұ���

//��������
void Read_busy();
void write_LCD_command(uint8);
void write_LCD_data(uint8);
void Set_page(uint8);
void Set_line(uint8);
void Set_column(uint8);
void SetOnOff(uint8);
void SelectScreen(uint8);
void ClearScreen(uint8);
void InitLCD();
void show_num(uint8 ,uint8 ,uint8 ,uint8 *);
void show_ch(uint8,uint8,uint8,uint8 *);
void show_im(uint8,uint8,uint8,uint8 *);
#endif