#ifndef _LCD_H
#define _LCD_H
#include <reg52.h>
#include <intrins.h>
#define uint8 unsigned char
#define uint16 unsigned int
#define LCD_databus P0 //总线接口，注意51的P0口需要外接上拉电阻
sbit DI = P2^3;	//DI为 0：写指令或读状态，1：数据
sbit RW = P2^4;	//1:写 0：读
sbit EN = P2^5;	//使能

sbit CS1 = P2^0;	//片选1.低电平有效，控制左半屏
sbit CS2 = P2^1;  //片选2，低电平有效，控制中屏
sbit CS3 = P2^2;  //片选3，低电平有效，控制右半屏

//声明函数
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