#include <reg52.h>
#include <intrins.h>
#include <LCD.h>
#include <FONT.h>


unsigned char GPS_i=0;//接收移位量
unsigned char RX_count=0;//GPS接收状态指示变量

data struct{
		
	unsigned char EW;//东经/西经
	unsigned char latiude_Degree[5];//经度

	
	unsigned char NS;//南/北纬
	unsigned char longitude_Degree[4];//维度_度

	//时分秒
	unsigned char time_hour[2];
	unsigned char time_min[2];
	unsigned char time_sec[2];
//}GPS_INFO;	
}GPS_INFO={'E',{1,1,3,2,4},'N',{2,4,1,7},{1,1},{2,4},{0,8}};//预设值方便测试

void UsartInit()
{
    TMOD = 0x20;
    SCON = 0x50;
    TH1 = 0xFA;
    TL1 = TH1;
    PCON = 0x80;
    EA = 1;
    ES = 1;
    TR1 = 1;
}




void main()
{
	
	InitLCD();//LCD初始化
	ClearScreen(0);
	Set_line(0);
	UsartInit();
	//显示框架
	show_im(3,3,16,image1);//图标
	show_ch(1,0,40,ch1+32*0);//时
	show_ch(2,0,16,ch1+32*1);//分
	show_ch(2,0,48,ch1+32*2);//秒
	//经度
	show_ch(1,2,40,ch1+32*3);//经
	show_ch(2,2,16,ch1+32*5);//度
	show_ch(2,2,48,ch1+32*6);//分
	//维度
	show_ch(1,4,40,ch1+32*4);//维
	show_ch(2,4,16,ch1+32*5);//度
	show_ch(2,4,48,ch1+32*6);//分
	
	
	while(1)
	{	
	show_num(1,0,24,num+16*(GPS_INFO.time_hour[0]));//时 十位
	show_num(1,0,32,num+16*(GPS_INFO.time_hour[1]));//时 个位
	
	show_num(2,0,0,num+16*(GPS_INFO.time_min[0]));//分 十位
	show_num(2,0,8,num+16*(GPS_INFO.time_min[1]));//分 个位
	
	show_num(2,0,32,num+16*(GPS_INFO.time_sec[0]));//秒 十位
	show_num(2,0,40,num+16*(GPS_INFO.time_sec[1]));//秒 个位
		
	if(GPS_INFO.EW=='E')
		show_ch(1,2,24,ch1+32*7);//东
	else if(GPS_INFO.EW=='W')
		show_ch(1,2,24,ch1+32*8);//西
		
	show_num(1,2,56,num+16*(GPS_INFO.latiude_Degree[0]));//经度—度 百位
	show_num(2,2,0,num+16*(GPS_INFO.latiude_Degree[1]));//经度—度 十位
	show_num(2,2,8,num+16*(GPS_INFO.latiude_Degree[2]));//经度—度 个位
	
	show_num(2,2,32,num+16*(GPS_INFO.latiude_Degree[3]));//经度—分 十位
	show_num(2,2,40,num+16*(GPS_INFO.latiude_Degree[4]));//经度—分 个位
		
	if(GPS_INFO.NS=='S')
		show_ch(1,4,24,ch1+32*9);//南
	else if(GPS_INFO.NS=='N')
		show_ch(1,4,24,ch1+32*10);//北
	
	show_num(2,4,0,num+16*(GPS_INFO.longitude_Degree[0]));//维度—度 十位
	show_num(2,4,8,num+16*(GPS_INFO.longitude_Degree[1]));//维度—度 个位
	
	show_num(2,4,32,num+16*(GPS_INFO.longitude_Degree[2]));//维度—分 十位
	show_num(2,4,40,num+16*(GPS_INFO.longitude_Degree[3]));//维度—分 个位
		;
	}
}
	//$GPGGA,194635.656,0000.033333,N,00000.033333,E,1,9,0,0,M,0,M,,*42
	//串口接收中断
void Usart() interrupt 4
{
	unsigned char receiveData;
	if(RI)
	{
		receiveData=SBUF;//接收到的数据
		RI = 0;//清除接收中断标志位
		
		switch(RX_count)
		{
			case 0 : if(receiveData=='G')RX_count=1; break;
			case 1 : if(receiveData=='P')RX_count=2;
								else RX_count=0;
									break;
			case 2 : if(receiveData=='G')RX_count=3;
								else RX_count=0;
									break;
			case 3 : if(receiveData=='G')RX_count=4;
								else RX_count=0;
									break;
			case 4 : if(receiveData=='A')RX_count=5; //获取到GPGGA字头
								else RX_count=0;
									break;

			case 5 : if(receiveData==',')RX_count=6;
								else RX_count=0;
									break;

			case 6 :												
								GPS_INFO.time_hour[GPS_i]=receiveData-'0';//GPS - 时
								GPS_i++;
								if(GPS_i>1){RX_count=7;GPS_i=0;}
									break;
			case 7 : GPS_INFO.time_min[GPS_i]=receiveData-'0';//GPS - 分
								GPS_i++;
								if(GPS_i>1){RX_count=8;GPS_i=0;}
									break;
			case 8 : GPS_INFO.time_sec[GPS_i]=receiveData-'0';//GPS - 秒
								GPS_i++;
								if(GPS_i>1){RX_count=9;GPS_i=0;}
									break;

			case 9 : if(receiveData==',')RX_count=10;//跳过秒的小数位
									break;
								
			case 10 : GPS_INFO.longitude_Degree[GPS_i]=receiveData-'0';//GPS - 维度
								GPS_i++;
								if(GPS_i>3){RX_count=11;GPS_i=0;}
									break;
								
			case 11 : if(receiveData==',')RX_count=12;//跳过维度的小数位
									break;
								
			case 12 : GPS_INFO.NS=receiveData;RX_count=13;//南北纬
									break;
								
			case 13 : RX_count=14;break;
								
			case 14 : GPS_INFO.latiude_Degree[GPS_i]=receiveData-'0';//GPS - 经度
								GPS_i++;
								if(GPS_i>4){RX_count=15;GPS_i=0;}
									break;

			case 15 : if(receiveData==',')RX_count=16;//跳过经度的小数位
									break;
								
			case 16 : GPS_INFO.EW=receiveData;RX_count=0;//东西经
								break;

			default : break;
		}

	}

	else
  TI = 0;	
}
	