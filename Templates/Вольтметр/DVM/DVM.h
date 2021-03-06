﻿
#define F_CPU 8000000UL
#include <avr/io.h>
#include <avr/delay.h>
#include <stdlib.h>

//-------- Макросы тивов данных --------
#define b1	bit					//	0 to 1
#define u8	unsigned char		//	0 to 255
#define	s8	signed char			//	-128 to 127
#define u16	unsigned int		//	0 to 65535
#define s16 signed int			//	-32768 to 32767
#define u32 unsigned long int	//	0 to 4294967295
#define s32 signed long int		//	-2147483648 to 2147483647

//-------- Макросы для работы с битами --------
#define ClearBit(reg, bit)		reg &= (~(1 << (bit)))		//	Сбросить бит в 0
#define SetBit(reg, bit)		reg |= (1 << (bit))			//	Установить бит в 1
#define InvertBit(reg, bit)		reg ^= (1 << (bit))			//	Инвертировать бит
#define BitIsClear(reg, bit)	((reg & (1 << (bit))) == 0)	//	Бит сброшен?
#define BitIsSet(reg, bit)		((reg & (1 << (bit))) != 0)	//	Бит установлен?


//-------- Define для работы с SPI --------
#define SPI_PIN_SS		PB2
#define SPI_PIN_MOSI	PB3
#define SPI_PIN_MISO	PB4
#define SPI_PIN_SCK	    PB5
#define SPI_PORT	    PORTB
#define SPI_DDR		    DDRB

//-------- Определния и макросы для работы с AD7792 --------

#define AD7792_MODE_REG 0x08  // Адрес регистра Mode
#define AD7792_CONF_REG 0x10  // Адрес регистра Configuration

#define AD7792_DELAY_FOR_DATA 10  // Задержка между измерениями данных

#define AD7792_CONF_REG_GAIN_1   0x00
#define AD7792_CONF_REG_GAIN_2   0x01
#define AD7792_CONF_REG_GAIN_4   0x02
#define AD7792_CONF_REG_GAIN_8   0x03
#define AD7792_CONF_REG_GAIN_16  0x04
#define AD7792_CONF_REG_GAIN_32  0x05
#define AD7792_CONF_REG_GAIN_64  0x06
#define AD7792_CONF_REG_GAIN_128 0x07

//-------- Define для мультиплексора --------

#define MUX_DC_ADDRESS_PORT		PORTD 
#define MUX_DC_EN_PORT		    PORTB 

#define MUX_DC_ADDRESS_DDR		DDRD 
#define MUX_DC_EN_DDR		    DDRB

#define MUX_DC_ADDRESS_BIT_A0	PD5
#define MUX_DC_ADDRESS_BIT_A1	PD6
#define MUX_DC_EN_BIT			PB7

/*
#define MUX_AC_ADDRESS_PORT	PORTB
#define MUX_AC_EN_PORT		PORTD

#define MUX_AC_ADDRESS_DDR	DDRB
#define MUX_AC_EN_DDR		DDRD

#define MUX_AC_ADDRESS_BIT_A0	PB0
#define MUX_AC_ADDRESS_BIT_A1	PB1
#define MUX_AC_EN_BIT			PD7
*/

#define RANGE_LOW	0	// Коэффициент делителя 1:1
#define RANGE_NEXT1	1	// Коэффициент 1:5
#define RANGE_NEXT2	2   // Коэффициент 1:40
#define RANGE_NEXT3	3   // Коэффициент 1:389
#define RANGE_MAX	4   // Коэффициент 1:3884

//-------- Определения и макросы для вычисления напряжения --------

#define DELAY_AFTER_SET_RANGE  600	//	Задержка для устранения инерционности

//-------- Прочие определения и макросы --------
#define UART_STRING_SIZE	24
#define NUMBER_OF_MEASURE	16

//-------- Декларация функций --------
void Measure(void);					// Выдает напр в нВ и мкВ

void Comm(void);	

void PortsInit (void);				// Задаем линии для мультиплескора

void UartInit(void);
void UartSendByte(u8 byt);			//	Процедура для отправки байта по UART
void UartSendNewLine(void);			//	Процедура для отправки символа переноса строки по UART
void UartlSendString(u8* str);		//	Процедура для отправки строки по UART

void SetRangeDc(u8 ran);			// Процедура для управления мультиплексором DC
void SetRangeAc(u8 ran);			// Процедура для управления мультиплексором AC
s32 GetAdcVoltage_nV(void);			// Функция для получения напряжения в нВ с АЦП
u16 GetAdcCode(u16 nmbr);			// Функция для усредненяем nmbr штук выборок АЦП

void SpiInitMaster(void);           //  Процедура для иниц-ии SPI
u8 SpiReadByte(u8 byt);             //  Функция для отправки байта (byt) по SPI

u16 Ad7792GetData(void);            //  Функция возвращает данные с АЦП
void Ad7792Reset();                 //	Процедура для сброса АЦП
void Ad7792SetInput(u8 gn, u8 ch);  //	Процедура для выбора канала АЦП
void Ad7792Calibration();           //	Процедура для калибровки АЦП
void Ad7792SetModeOfConversion();   //	Процедура для настройки режима преоб-я и частоты АЦП
//void Ad7792SetGain(u8 gn);          //  Процедура для настройки КЭФа (gn) АЦП

void NumberToString(s32 voltag, u8 buff[]);				//	Число  в строку
void TerminalPrepare(u8 buff1[], u16 len1, s32 volt);
void I2cPrepare(void);

//-------- Константы --------
const s32 AdcStep = 35706;		
const s32 AdcCodeZero = 32768;
const s32 AdcBias = 0;
const s32 AdcVoltage_nV_Max = 500000000;	          //	Макс. напр-е АЦП в нВ
const s32 RangeDivDc[5] = {10, 10, 10, 10, 1};
const s32 RangeKefDc[5] = {1004, 4930, 39991, 392508, 383984};
//const s32 RangeDivAc[5] = {10, 10, 10, 10, 1};
//const s32 RangeKefAc[5] = {710, 3486, 28278, 277545, 271518};
const s32 CommonDev = 100000;

//-------- Объявление переменных --------

u8 SpiCode;						//	Код АЦП
u8 UartString[UART_STRING_SIZE];//	Строка для Uart
u16 AdcCode_u16;				//	Код АЦП	(0 to 65535)
s32 AdcVoltage_nV_Dc;			//	Напряжение на входе АЦП в нВ (DC)
//s32 AdcVoltage_nV_Ac;			//	Напряжение на входе АЦП в нВ (AC)
s32 Voltage_uV_Dc;				//	Напряжение на входе вольметра в мкВ (DC)
//s32 Voltage_uV_Ac;				//	Напряжение на входе вольметра в мкВ (AC)
u8 CurrentRange;				//	Режим делителя
