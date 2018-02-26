//	Описание макроопределений

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

//--------Умножение и деление на 2--------

#define PowerOf2(power)	(1 << (power))  // Получаем число 2 в степени power
#define RegDivPowerOf2(reg,power)	reg = ((reg) >> power) // Делим reg на 2 в power степени

u16 MeasureMiddle(u16 arr1[], u16);