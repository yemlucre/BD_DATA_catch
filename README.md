# GNSS定位模块在AT89C52RC上数据传输  
## 各文件说明  

* 666 文件为直接抓取串口通过RX、TX发送至单片机上的数据并显示。  
* analyse是通过数据筛选只输出和解析$GPRMC的信号  
* DX-GP10_resource 是大夏龙雀的GNSS定位module资料，与我用的不完全相同可做参考。  
* 该模块可以直接使用GNSSToolKit3通过串口于电脑连接显示时间经纬度。
* project是之前写的经过了Chat优化但是没有运行。
* C51 GPS是Bilibili博主缘不负卿的仿真程序  [bilibili](https://www.bilibili.com/video/BV1hB4y1S7JS/?share_source=copy_web&vd_source=318d317c1404fb1dfeb57deb06f23b93) 
  
---  

## 项目材料  
* 本项目使用AT89C52作为MCU  
* 显示模块主要是用LCD1602  
* GNSS模块使用大概是 HT2828Z3G5L
---  
  
## 实验过程图片  
![GNSSToolKit3](https://i.postimg.cc/PxsG7Mvx/Screenshot-2025-06-26-142009.png)  
![uart](https://i.postimg.cc/J4hFPFYx/Screenshot-2025-06-26-100846.png)  
![pheno](https://i.postimg.cc/rpGg8GYz/20250628180124.jpg)