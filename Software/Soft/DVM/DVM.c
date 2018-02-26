
#include "DVM.h"

int main(void)
{
	PortsInit();
	UartInit();
	SpiInitMaster();
	Ad7792Reset();
	Ad7792SetInput();
	Ad7792Calibration();
	Ad7792SetModeOfConversion();
    while(1)
    {
        Measure();
		Comm(); 
    };
};

void Measure (void)
{
	CurrentRange = RANGE_LOW;                       // Установили наим. диапазон
	SetRange(CurrentRange);
	AdcVoltage_nV = 0;								
	AdcVoltage_nV = GetAdcVoltage_nV();             // Получили напр. в нВ
	while ((labs(AdcVoltage_nV)) > (AdcVoltage_nV_Max)) // Напр. на АЦП по модулю больше макс? 
	{
		if (CurrentRange > RANGE_MAX)
		{
			AdcVoltage_nV = AdcVoltage_nV_Max;
		}
		else
		{
			CurrentRange++;                     // Следующий диапазон
			SetRange(CurrentRange);
			AdcVoltage_nV = GetAdcVoltage_nV();
		};
	};
	Voltage_uV = 0;
	Voltage_uV = AdcVoltage_nV / CommonDev;				// Получаем напр. в uV
	Voltage_uV = (Voltage_uV * RangeKef[CurrentRange]); // Умножаем на кэф делителя
	Voltage_uV = (Voltage_uV / RangeDiv[CurrentRange]);
};

u16 GetAdcCode(u16 nmbr) 
{ 
	u16 res3; 
	res3 = Ad7792GetData();
	for (u16 t = 1; t < nmbr; t++)
	{
		res3 = (res3 + Ad7792GetData())/2;
	}
	return res3;
};

s32 GetAdcVoltage_nV (void) 
{
	_delay_ms(DelayAfterSetRange);              // Избавляемся от инерционности 
	s32 res;                                    // Temp для результата
	AdcCode_u16 = GetAdcCode(16);				// Получили код АЦП
	res = (s32*)(AdcCode_u16 - AdcCodeZero);	// Получили БП код
	res = res * AdcStep + AdcBias;				// Умножаем на шаг АЦП и добавляем смещение
	return res;
};

void SetRange(u8 ran)
{
	switch(ran)
	{
		case RANGE_LOW:									// EN=0, A1A0=00
			ClearBit(MUX_PORT_DIV_ADDRESS, MUX_PIN_A0);
			ClearBit(MUX_PORT_DIV_ADDRESS, MUX_PIN_A0);
			ClearBit(MUX_PORT_DIV_EN, MUX_PIN_EN);
		break;
		case RANGE_NEXT1:								// EN=1, A1A0=11
			SetBit(MUX_PORT_DIV_ADDRESS, MUX_PIN_A0);
			SetBit(MUX_PORT_DIV_ADDRESS, MUX_PIN_A0);
			SetBit(MUX_PORT_DIV_EN, MUX_PIN_EN);
		break;
		case RANGE_NEXT2:								// EN=1, A1A0=10
			ClearBit(MUX_PORT_DIV_ADDRESS, MUX_PIN_A0);
			SetBit(MUX_PORT_DIV_ADDRESS, MUX_PIN_A0);
			SetBit(MUX_PORT_DIV_EN, MUX_PIN_EN);
		break;
		case RANGE_NEXT3:								// EN=1, A1A0=01
			SetBit(MUX_PORT_DIV_ADDRESS, MUX_PIN_A0);
			ClearBit(MUX_PORT_DIV_ADDRESS, MUX_PIN_A0);
			SetBit(MUX_PORT_DIV_EN, MUX_PIN_EN);
		break;
		case RANGE_MAX:									// EN=1, A1A0=00
			ClearBit(MUX_PORT_DIV_ADDRESS, MUX_PIN_A0);
			ClearBit(MUX_PORT_DIV_ADDRESS, MUX_PIN_A0);
			SetBit(MUX_PORT_DIV_EN, MUX_PIN_EN);
		break;
		default:										// EN=0, A1A0=00
			ClearBit(MUX_PORT_DIV_ADDRESS, MUX_PIN_A0);
			ClearBit(MUX_PORT_DIV_ADDRESS, MUX_PIN_A0);
			ClearBit(MUX_PORT_DIV_EN, MUX_PIN_EN);
		break;
	};
};

void VoltageToString(s32 voltag, u8 buff[])
{
	s32 nmbr;
	nmbr = abs(voltag) + 1000000000;  
	ltoa(nmbr, buff, 10);
	if (voltag > 0)
	{
		buff[0] = '+';
	}
	else
	{
		buff[0] = '-';
	}
};

void I2cPrepare(void)
{
	
};

void Comm (void)   
{
	memset(UartString, 0, 24);
	ltoa(AdcVoltage_nV, UartString, 10);  // Отправляем на терминал нВ
	UartlSendString("Uadc[nV] = ");
	UartlSendString(UartString);
	UartSendNewLine();
	
	TerminalPrepare(UartString, 24, Voltage_uV);
	I2cPrepare();
	UartlSendString("Udvm[uV] = ");
	UartlSendString(UartString);
	UartSendNewLine();
	
};

void TerminalPrepare(u8 buff1[], u16 len1, s32 volt)
{
	memset(buff1, 0, len1);
	VoltageToString(volt, buff1);
}

void UartlSendString(u8* str)
{
	while ((*str) != '\0')		//	Пока не встретили конец строки
	{
		UartSendByte((*str));	//	Отправляем байт по Uart
		str++;					//	Увеличеваем значение указателя
	};
};

void UartSendByte (u8 byt)			
{
	while (BitIsClear(UCSR0A, UDRE0))	//	Выйдем из цикла когда UDR будет пуст
	{};
	UDR0 = byt;
};

void UartSendNewLine (void) 
{
	UartSendByte(0x0A);
	UartSendByte(0x0D);
}

void UartInit (void)			
{
	UBRR0H = 0;	   	//	Задаем скорость перадчи по UART (9600 baud)
	UBRR0L = 51;
	UCSR0A = 0x00; 	// 8 битовая посылка, 1 стоп бит,
	UCSR0B = 0x18; 	// контроля четности нет
	UCSR0C = 0x86;
};

void PortsInit(void) 
{
	SetBit(MUX_DDR_DIV_EN, MUX_PIN_EN);			// EN 
	SetBit(MUX_DDR_DIV_ADDRESS, MUX_PIN_A0);	// A0 
	SetBit(MUX_DDR_DIV_ADDRESS, MUX_PIN_A0);	// A1 
}

void SpiInitMaster(void) 	
{			
	SetBit(SPI_DDR, SPI_PIN_SS);		// Настроить выводы DDRB на выход (SS, MOSI, SCK)
	SetBit(SPI_DDR, SPI_PIN_MOSI);
	SetBit(SPI_DDR, SPI_PIN_SCK);
	ClearBit(SPI_DDR, SPI_PIN_MISO);	// MISO На вход
	SetBit(SPCR, MSTR);					// Режим мастер, F=Fosc/4
	SetBit(SPCR, CPOL);					// Импульсы отр. полярности
	SetBit(SPCR, CPHA);					// Обработка по заднему фронту
	SetBit(SPCR, SPE);					// Включить SPI
	SetBit(SPI_PORT,SPI_PIN_SS);		// SS на 1
}

u8 SpiReadByte(u8 byt)		
{
	u8 res;							//	Переменная для результата
	ClearBit(SPI_PORT,SPI_PIN_SS);	//	SS в 0
	SPDR = byt;						//	Отправить байт		
	while(BitIsClear(SPSR, SPIF));	//	Ждем завршения отправки
	res = SPDR;						//	Фиксируем принятый байт
	SetBit(SPI_PORT,SPI_PIN_SS);	//	SS в 1
	return res;						//	Возвращаем байт данных
};

void Ad7792Reset(void)				
{
	SpiCode = SpiReadByte(0xFF);	//	Посылаем 32 клока
	SpiCode = SpiReadByte(0xFF);
	SpiCode = SpiReadByte(0xFF);
	SpiCode = SpiReadByte(0xFF);
	_delay_ms(10);					//	Пауза для перезагрузки АЦП
};

void Ad7792SetInput(void)					
{
	SpiCode = SpiReadByte(AD7792_CONF_REG); // Заходим в CONFIGURATION регистр
	SpiCode = SpiReadByte(0x00); 			// Старший байт - Усиление 1, биполярный режим
	SpiCode = SpiReadByte(0x91); 			// Младший байт - Внутренний ИОН, канал 2
};

void Ad7792Calibration(void)					
{
	SpiCode = SpiReadByte(AD7792_MODE_REG);		//	Заходим в Mode регистр
	SpiCode = SpiReadByte(0x80);				//	Offset калибровка
	SpiCode = SpiReadByte(0x01);											
	while(BitIsSet(SPI_PORT, SPI_PIN_MISO));	//	Ждем конца калибровки
	
	SpiCode = SpiReadByte(AD7792_MODE_REG);	 
	SpiCode = SpiReadByte(0xA0);				//	Запускаем FullScale калибровку
	SpiCode = SpiReadByte(0x0F);							                   
	while(BitIsSet(SPI_PORT, SPI_PIN_MISO));	//	Ждем конца калибровки
};

void Ad7792SetModeOfConversion(void)				
{
	SpiCode = SpiReadByte(AD7792_MODE_REG);	//	Заходим в Mode регистр
	SpiCode = SpiReadByte(0x00);	        // Старший байт - Режим непр. преобразования
	SpiCode = SpiReadByte(0x01);			// Младший байт - Частота преоб-я 470 Гц
};

void Ad7792SetGain(u8 gn)
{
	SpiCode = SpiReadByte(AD7792_CONF_REG);   			// Заходим в CONFIGURATION регистр
	switch (gn)
	{
	case AD7792_CONF_REG_GAIN_1:
		SpiCode = SpiReadByte(AD7792_CONF_REG_GAIN_1);
		break;
	case AD7792_CONF_REG_GAIN_2:
		SpiCode = SpiReadByte(AD7792_CONF_REG_GAIN_2);
		break;
	case AD7792_CONF_REG_GAIN_4:
		SpiCode = SpiReadByte(AD7792_CONF_REG_GAIN_4);
		break;
	case AD7792_CONF_REG_GAIN_8:
		SpiCode = SpiReadByte(AD7792_CONF_REG_GAIN_8);
		break;
	case AD7792_CONF_REG_GAIN_16:
		SpiCode = SpiReadByte(AD7792_CONF_REG_GAIN_16);
		break;
	case AD7792_CONF_REG_GAIN_32:
		SpiCode = SpiReadByte(AD7792_CONF_REG_GAIN_32);
		break;
	case AD7792_CONF_REG_GAIN_64:
		SpiCode = SpiReadByte(AD7792_CONF_REG_GAIN_64);
		break;
	case AD7792_CONF_REG_GAIN_128:
		SpiCode = SpiReadByte(AD7792_CONF_REG_GAIN_128);
		break;
	default:
		SpiCode = SpiReadByte(AD7792_CONF_REG_GAIN_1);
		break;
	};
	SpiCode = SpiReadByte(0x91); 						// Младший байт - Внутренний ИОН, канал 2
}

u16 Ad7792GetData(void)				
{
	u16 b3;				            // Переменная для хранения результата
	u16 b1;				            // Переменная для хранения старшего байта
	u8 b2;				            // Переменная для хранения младшего байта
	
	while(BitIsSet(SPI_PORT, SPI_PIN_MISO));	// Ждем когда данные подготовятся
	SpiCode = SpiReadByte(0x58);				// Заходим в DATA REG
	b1 = SpiReadByte(0xFF);						// Получаем старший байт данных
	b2 = SpiReadByte(0xFF);						// Младший байт данных
	b3 = (b2 | (b1 << 8));						// Формируем байт данных
	return b3;									// Отправь данные
};



