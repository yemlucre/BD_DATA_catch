#include <reg52.h>
#include <intrins.h>
#include <LCD.h>
#include <FONT.h>


unsigned char GPS_i=0;//������λ��
unsigned char RX_count=0;//GPS����״ָ̬ʾ����

data struct{
		
	unsigned char EW;//����/����
	unsigned char latiude_Degree[5];//����

	
	unsigned char NS;//��/��γ
	unsigned char longitude_Degree[4];//ά��_��

	//ʱ����
	unsigned char time_hour[2];
	unsigned char time_min[2];
	unsigned char time_sec[2];
//}GPS_INFO;	
}GPS_INFO={'E',{1,1,3,2,4},'N',{2,4,1,7},{1,1},{2,4},{0,8}};//Ԥ��ֵ�������

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
	
	InitLCD();//LCD��ʼ��
	ClearScreen(0);
	Set_line(0);
	UsartInit();
	//��ʾ���
	show_im(3,3,16,image1);//ͼ��
	show_ch(1,0,40,ch1+32*0);//ʱ
	show_ch(2,0,16,ch1+32*1);//��
	show_ch(2,0,48,ch1+32*2);//��
	//����
	show_ch(1,2,40,ch1+32*3);//��
	show_ch(2,2,16,ch1+32*5);//��
	show_ch(2,2,48,ch1+32*6);//��
	//ά��
	show_ch(1,4,40,ch1+32*4);//ά
	show_ch(2,4,16,ch1+32*5);//��
	show_ch(2,4,48,ch1+32*6);//��
	
	
	while(1)
	{	
	show_num(1,0,24,num+16*(GPS_INFO.time_hour[0]));//ʱ ʮλ
	show_num(1,0,32,num+16*(GPS_INFO.time_hour[1]));//ʱ ��λ
	
	show_num(2,0,0,num+16*(GPS_INFO.time_min[0]));//�� ʮλ
	show_num(2,0,8,num+16*(GPS_INFO.time_min[1]));//�� ��λ
	
	show_num(2,0,32,num+16*(GPS_INFO.time_sec[0]));//�� ʮλ
	show_num(2,0,40,num+16*(GPS_INFO.time_sec[1]));//�� ��λ
		
	if(GPS_INFO.EW=='E')
		show_ch(1,2,24,ch1+32*7);//��
	else if(GPS_INFO.EW=='W')
		show_ch(1,2,24,ch1+32*8);//��
		
	show_num(1,2,56,num+16*(GPS_INFO.latiude_Degree[0]));//���ȡ��� ��λ
	show_num(2,2,0,num+16*(GPS_INFO.latiude_Degree[1]));//���ȡ��� ʮλ
	show_num(2,2,8,num+16*(GPS_INFO.latiude_Degree[2]));//���ȡ��� ��λ
	
	show_num(2,2,32,num+16*(GPS_INFO.latiude_Degree[3]));//���ȡ��� ʮλ
	show_num(2,2,40,num+16*(GPS_INFO.latiude_Degree[4]));//���ȡ��� ��λ
		
	if(GPS_INFO.NS=='S')
		show_ch(1,4,24,ch1+32*9);//��
	else if(GPS_INFO.NS=='N')
		show_ch(1,4,24,ch1+32*10);//��
	
	show_num(2,4,0,num+16*(GPS_INFO.longitude_Degree[0]));//ά�ȡ��� ʮλ
	show_num(2,4,8,num+16*(GPS_INFO.longitude_Degree[1]));//ά�ȡ��� ��λ
	
	show_num(2,4,32,num+16*(GPS_INFO.longitude_Degree[2]));//ά�ȡ��� ʮλ
	show_num(2,4,40,num+16*(GPS_INFO.longitude_Degree[3]));//ά�ȡ��� ��λ
		;
	}
}
	//$GPGGA,194635.656,0000.033333,N,00000.033333,E,1,9,0,0,M,0,M,,*42
	//���ڽ����ж�
void Usart() interrupt 4
{
	unsigned char receiveData;
	if(RI)
	{
		receiveData=SBUF;//���յ�������
		RI = 0;//��������жϱ�־λ
		
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
			case 4 : if(receiveData=='A')RX_count=5; //��ȡ��GPGGA��ͷ
								else RX_count=0;
									break;

			case 5 : if(receiveData==',')RX_count=6;
								else RX_count=0;
									break;

			case 6 :												
								GPS_INFO.time_hour[GPS_i]=receiveData-'0';//GPS - ʱ
								GPS_i++;
								if(GPS_i>1){RX_count=7;GPS_i=0;}
									break;
			case 7 : GPS_INFO.time_min[GPS_i]=receiveData-'0';//GPS - ��
								GPS_i++;
								if(GPS_i>1){RX_count=8;GPS_i=0;}
									break;
			case 8 : GPS_INFO.time_sec[GPS_i]=receiveData-'0';//GPS - ��
								GPS_i++;
								if(GPS_i>1){RX_count=9;GPS_i=0;}
									break;

			case 9 : if(receiveData==',')RX_count=10;//�������С��λ
									break;
								
			case 10 : GPS_INFO.longitude_Degree[GPS_i]=receiveData-'0';//GPS - ά��
								GPS_i++;
								if(GPS_i>3){RX_count=11;GPS_i=0;}
									break;
								
			case 11 : if(receiveData==',')RX_count=12;//����ά�ȵ�С��λ
									break;
								
			case 12 : GPS_INFO.NS=receiveData;RX_count=13;//�ϱ�γ
									break;
								
			case 13 : RX_count=14;break;
								
			case 14 : GPS_INFO.latiude_Degree[GPS_i]=receiveData-'0';//GPS - ����
								GPS_i++;
								if(GPS_i>4){RX_count=15;GPS_i=0;}
									break;

			case 15 : if(receiveData==',')RX_count=16;//�������ȵ�С��λ
									break;
								
			case 16 : GPS_INFO.EW=receiveData;RX_count=0;//������
								break;

			default : break;
		}

	}

	else
  TI = 0;	
}
	