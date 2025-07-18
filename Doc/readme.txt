/*********************************************************************************************/
本文档使用 TAB = 4 对齐，使用keil5默认配置打开阅读比较方便。

【*】程序简介

-工程名称：触摸画板实验
-实验平台: 野火STM32 指南者 开发板 
-MDK版本：5.16
-ST固件库版本：3.5


【 ！】功能简介：
显示一个触摸画板应用，展示LCD的触摸屏功能
学习目的：驱动电阻触摸屏

【 ！】实验操作：
连接好配套的3.2寸液晶屏，下载程序后复位开发板即可，

屏幕可能会提示要进行触摸校正，依次准确点击屏幕显示的十字，完成校正即可，
若触摸不准确，可能会提示多次校正，直至成功为止。

若汉字不正常显示，需要重新往外部FLASH烧录字模！！！

【*】注意事项：
本程序液晶显示的汉字字模是存储在外部FLASH的。
字模：GB2312汉字库，16*16，宋体，支持中文标点。字模位置见FLASH空间表。
 
!!!若汉字显示不正常，可使用配套例程中的“刷外部FLASH程序”给FLASH恢复出厂数据

如果填充颜色出现花屏打开bsp_ili9341_lcd.C文件，找到ILI9341_Write_Cmd (0XCB)一项，
最后一个Write_Data的数值可以改成0x02到0x06，最大0x06。


/***************************************************************************************************************/

【 ！】外部Flash使用情况说明（W25Q64）	
		

|-------------------------------------------------------------------------------------------------------------------|												
|序号	|文件名/工程					|功能										|起始地址		|长度				|
|-------|---------------------------|-------------------------------------------|---------------|-------------------|
|1		|外部flash读写例程			|预留给裸机Flash测试							|0				|1024 (BYTE)		|
|-------|---------------------------|-------------------------------------------|---------------|-------------------|
|2		|裸机触摸屏例程				|裸机触摸校准参数							|1024			|2*1024(BYTE)		|
|-------|---------------------------|-------------------------------------------|---------------|-------------------|
|3		|裸机中文显示例程（旧版）		|裸机中文字库（HZLIB.bin）					|4096			|53*4096 (212KB)	|
|-------|---------------------------|-------------------------------------------|---------------|-------------------|
|4		|app.c						|XBF字库文件（emWin使用,songti.xbf）			|60*4096		|317*4096 (1.23MB)	|
|-------|---------------------------|-------------------------------------------|---------------|-------------------|
|5		|裸机中文显示例程（新版）		|裸机中文字库（GB2312_H1616.FON）			|387*4096		|64*4096 (256KB)	|
|-------|---------------------------|-------------------------------------------|---------------|-------------------|
|6		|外部flash读写例程			|文件系统中文支持字库(emWin使用,UNIGBK.BIN)	|465*4096		|43*4096 (172KB)	|
|-------|---------------------------|-------------------------------------------|---------------|-------------------|
|7		|Touch_CalibrationApp.c		|电阻屏触摸校准参数（emWin使用）				|510*4096		|34 (BYTE)			|
|-------|---------------------------|-------------------------------------------|---------------|-------------------|
|8		|外部flash读写例程			|文件系统中文支持字库(emWin使用,UNIGBK.BIN)	|512*4096		|1536*4096 (6MB)	|
|-------|---------------------------|-------------------------------------------|---------------|-------------------|


*FLASH芯片的第一个扇区（0-4096字节）是专门预留给本实验及裸机触摸做测试的，
 若用户修改本实验，写入数据到其它扇区，会导致FLASH芯片其它数据毁坏，
 做其它用到这些数据的实验时需要重新给FLASH写入这些数据。
 
 可使用配套例程中的“刷外部FLASH程序”给FLASH恢复出厂数据。

/***************************************************************************************************************/

【*】 引脚分配

液晶屏：
液晶屏接口与STM32的FSMC接口相连，支565格式，
使用FSMC产生8080时序与液晶屏驱动芯片ILI9341通讯.


		/*液晶控制信号线*/
		ILI9341_CS 	<--->	PD7      	//片选，选择NOR/SRAM块 BANK1_NOR/SRAM1
		ILI9341_DC  <---> 	PD11		//PD11为FSMC_A16,本引脚决定了访问LCD时使用的地址
		ILI9341_WR 	<---> 	PD5			//写使能
		ILI9341_RD  <---> 	PD4			//读使能
		ILI9341_RST	<---> 	PE1			//复位引脚
		ILI9341_BK 	<---> 	PD12 		//背光引脚
		
	数据信号线省略,本实验没有驱动触摸屏，详看触摸画板实验。
	
触摸屏：
触摸屏控制芯片XPT2046与STM32的普通GPIO相连，使用模拟SPI进行通讯。
		XPT2046_SCK		<--->PE0
		XPT2046_MISO	<--->PE3
		XPT2046_MOSI	<--->PE2
		XPT2046_CS		<--->PD13
		
		XPT2046_PENIRQ	<--->PE4	//触摸信号，低电平表示有触摸


FLASH(W25Q64)：
FLASH芯片的SPI接口与STM32的SPI3相连。
		SCK	<--->PA5
		MISO<--->PA6
		MOSI<--->PA7
		CS	<--->PC0

串口(TTL-USB TO USART)：
CH340的收发引脚与STM32的发收引脚相连。
	RX<--->PA9
	TX<--->PA10
												
/*********************************************************************************************/

【*】 版本

-程序版本：1.0
-发布日期：2013-10

/*********************************************************************************************/

【*】 联系我们

-野火官网  :https://embedfire.com
-野火论坛  :http://www.firebbs.cn
-野火商城  :https://yehuosm.tmall.com/
-野火资料下载中心：http://doc.embedfire.com/products/link

/*********************************************************************************************/