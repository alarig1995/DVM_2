
#include "DVM.h"

int main(void)
{
	PortsInit();
	UartInit();
	SpiInitMaster();
	Ad7792Reset();
	Ad7792SetInput(0x00, 0x91); // Кэф 1, биполярный режим, внутренний ИОН, 2-й канал
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
	SetRangeDc(CurrentRange);
	_delay_ms(DELAY_AFTER_SET_RANGE);               // Избавляемся от инерционности 
	AdcVoltage_nV_Dc = 0;								
	AdcVoltage_nV_Dc = GetAdcVoltage_nV();             // Получили напр. в нВ
	while ((labs(AdcVoltage_nV_Dc)) > (AdcVoltage_nV_Max)) // Напр. на АЦП по модулю больше макс? 
	{
		if (CurrentRange > RANGE_MAX)
		{
			AdcVoltage_nV_Dc = AdcVoltage_nV_Max;
		}
		else
		{
			CurrentRange++;                     // Следующий диапазон
			SetRangeDc(CurrentRange);
			_delay_ms(DELAY_AFTER_SET_RANGE);   // Избавляемся от инерционности 
			AdcVoltage_nV_Dc = GetAdcVoltage_nV();
		};
	};
	Voltage_uV_Dc = 0;
	Voltage_uV_Dc = AdcVoltage_nV_Dc / CommonDev;				// Получаем напр. в мВ		(Пример 123.1 мВ)
	Voltage_uV_Dc = (Voltage_uV_Dc * RangeKefDc[CurrentRange]); // Умножаем на кэф делителя	(Пример 1231000)
	Voltage_uV_Dc = (Voltage_uV_Dc / RangeDivDc[CurrentRange]); // Получаем напр. в мкВ		(Пример 123100 мкВ)
	
};

u16 GetAdcCode(u8 nmbr) 
{ 
	u32 res3; 
	res3 = Ad7792GetData();
	for (u8 t = 1; t < nmbr; t++)
	{
		res3 = (res3 + Ad7792GetData())/2;
	}
	return res3;
};

s32 GetAdcVoltage_nV (void) 
{
	s32 res;  
	u16 u16_tmp;                                 
	u16_tmp = GetAdcCode(NUMBER_OF_MEASURE);// Получили код АЦП
	res = (s32*)(u16_tmp - AdcCodeZero);	// Получили БП код
	res = res * AdcStep + AdcBias;			// Умножаем на шаг АЦП и добавляем смещение
	return res;
};

void SetRangeDc(u8 ran)
{
	switch(ran)
	{
		case RANGE_LOW:									// EN=0, A1A0=00
			ClearBit(MUX_DC_ADDRESS_PORT, MUX_DC_ADDRESS_BIT_A0);
			ClearBit(MUX_DC_ADDRESS_PORT, MUX_DC_ADDRESS_BIT_A1);
			ClearBit(MUX_DC_EN_PORT, MUX_DC_EN_BIT);
		break;
		case RANGE_NEXT1:								// EN=1, A1A0=11
			SetBit(MUX_DC_ADDRESS_PORT, MUX_DC_ADDRESS_BIT_A0);
			SetBit(MUX_DC_ADDRESS_PORT, MUX_DC_ADDRESS_BIT_A1);
			SetBit(MUX_DC_EN_PORT, MUX_DC_EN_BIT);
		break;
		case RANGE_NEXT2:								// EN=1, A1A0=10
			ClearBit(MUX_DC_ADDRESS_PORT, MUX_DC_ADDRESS_BIT_A0);
			SetBit(MUX_DC_ADDRESS_PORT, MUX_DC_ADDRESS_BIT_A1);
			SetBit(MUX_DC_EN_PORT, MUX_DC_EN_BIT);
		break;
		case RANGE_NEXT3:								// EN=1, A1A0=01
			SetBit(MUX_DC_ADDRESS_PORT, MUX_DC_ADDRESS_BIT_A0);
			ClearBit(MUX_DC_ADDRESS_PORT, MUX_DC_ADDRESS_BIT_A1);
			SetBit(MUX_DC_EN_PORT, MUX_DC_EN_BIT);
		break;
		case RANGE_MAX:									// EN=1, A1A0=00
			ClearBit(MUX_DC_ADDRESS_PORT, MUX_DC_ADDRESS_BIT_A0);
			ClearBit(MUX_DC_ADDRESS_PORT, MUX_DC_ADDRESS_BIT_A1);
			SetBit(MUX_DC_EN_PORT, MUX_DC_EN_BIT);
		break;
		default:										// EN=0, A1A0=00
			ClearBit(MUX_DC_ADDRESS_PORT, MUX_DC_ADDRESS_BIT_A0);
			ClearBit(MUX_DC_ADDRESS_PORT, MUX_DC_ADDRESS_BIT_A1);
			ClearBit(MUX_DC_EN_PORT, MUX_DC_EN_BIT);
		break;
	};
};

void NumberToString(s32 vlt, u8 buff[])
{
	s32 nmbr;
	nmbr = labs(vlt) + 1000000000;  
	ltoa(nmbr, buff, 10);
	if (vlt >= 0)
	{
		buff[0] = '+';
	}
	else
	{
		buff[0] = '-';
	}
};

void Comm (void)   
{
	memset(UartString, 0, UART_STRING_SIZE);
	NumberToString(Voltage_uV_Dc, UartString);
	UartlSendString("Udvm[uV] = ");
	UartlSendString(UartString);
	UartSendNewLine();
	
};

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
	SetBit(MUX_DC_EN_DDR, MUX_DC_EN_BIT);			// EN 
	SetBit(MUX_DC_ADDRESS_DDR, MUX_DC_ADDRESS_BIT_A0);	// A0 
	SetBit(MUX_DC_ADDRESS_DDR, MUX_DC_ADDRESS_BIT_A1);	// A1 
}

void SpiInitMaster(void) 	
{			
	SetBit(SPI_PORT_DDR, SPI_PORT_BIT_SS);		// Настроить выводы DDRB на выход (SS, MOSI, SCK)
	SetBit(SPI_PORT_DDR, SPI_PORT_BIT_MOSI);
	SetBit(SPI_PORT_DDR, SPI_PORT_BIT_SCK);
	ClearBit(SPI_PORT_DDR, SPI_PORT_BIT_MISO);	// MISO На вход
	SetBit(SPCR, MSTR);							// Режим мастер, F=Fosc/4
	SetBit(SPCR, CPOL);							// Импульсы отр. полярности
	SetBit(SPCR, CPHA);							// Обработка по заднему фронту
	SetBit(SPCR, SPE);							// Включить SPI
	SetBit(SPI_PORT,SPI_PORT_BIT_SS);			// SS на 1
}

u8 SpiReadByte(u8 byt)		
{
	u8 res;								//	Переменная для результата
	ClearBit(SPI_PORT,SPI_PORT_BIT_SS);	//	SS в 0
	SPDR = byt;							//	Отправить байт		
	while(BitIsClear(SPSR, SPIF))		//	Ждем завршения отправки
	{};
	res = SPDR;							//	Фиксируем принятый байт
	SetBit(SPI_PORT,SPI_PORT_BIT_SS);	//	SS в 1
	return res;							//	Возвращаем байт данных
};

void Ad7792Reset(void)				
{
	SpiCode = SpiReadByte(0xFF);	//	Посылаем 32 клока
	SpiCode = SpiReadByte(0xFF);
	SpiCode = SpiReadByte(0xFF);
	SpiCode = SpiReadByte(0xFF);
	_delay_ms(10);					//	Пауза для перезагрузки АЦП
};

void Ad7792SetInput(u8 gn, u8 ch)					
{
	SpiCode = SpiReadByte(AD7792_CONF_REG); // Заходим в CONFIGURATION регистр
	SpiCode = SpiReadByte(gn); 				// Старший байт - Кэф, биполярный/униполярный режим
	SpiCode = SpiReadByte(ch); 				// Младший байт - внутренний/ внешний ИОН, канал
};

void Ad7792Calibration(void)					
{
	SpiCode = SpiReadByte(AD7792_MODE_REG);			//	Заходим в Mode регистр
	SpiCode = SpiReadByte(0x80);					//	Выбераем Offset калибровку
	SpiCode = SpiReadByte(0x01);					//	Частота 470 Гц							
	while(BitIsSet(SPI_PORT, SPI_PORT_BIT_MISO))	//	Ждем конца калибровки
	{};
	SpiCode = SpiReadByte(AD7792_MODE_REG);	 
	SpiCode = SpiReadByte(0xA0);					//	Выбераем FullScale калибровку
	SpiCode = SpiReadByte(0x01);					//	Частота 470 Гц			                   
	while(BitIsSet(SPI_PORT, SPI_PORT_BIT_MISO))	//	Ждем конца калибровки		
	{};
};

void Ad7792SetModeOfConversion(void)				
{
	SpiCode = SpiReadByte(AD7792_MODE_REG);	//	Заходим в Mode регистр
	SpiCode = SpiReadByte(0x00);	        // Старший байт - Режим непр. преобразования
	SpiCode = SpiReadByte(0x01);			// Младший байт - Частота преоб-я 470 Гц
};

u16 Ad7792GetData(void)				
{
	u16 b3;				            // Переменная для хранения результата
	u16 b1;				            // Переменная для хранения старшего байта
	u8 b2;				            // Переменная для хранения младшего байта
	
	while(BitIsSet(SPI_PORT, SPI_PORT_BIT_MISO));	// Ждем когда данные подготовятся
	SpiCode = SpiReadByte(0x58);					// Заходим в DATA REG
	b1 = SpiReadByte(0xFF);							// Получаем старший байт данных
	b2 = SpiReadByte(0xFF);							// Младший байт данных
	b3 = (b2 | (b1 << 8));							// Формируем байт данных
	return b3;										// Отправь данные
};



