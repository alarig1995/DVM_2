#include "Artem.h"

u16 MeasureMiddle(u16 arr1[], u16 length1)	
{
	u8 Msr;										 // Temp для счетчика
	u16 res2;									 // Temp для результата
	u32 Accum = 0;								 // Иниц. аккумулятор
	for (Msr = 0; Msr < length1; Msr++)
	{
		Accum += arr1[Msr];
	};
	Accum = Accum / length1;
	res2 = ((u16*) (Accum));					 // Приведение типов
	return res2;
};
