void Delay10us(unsigned char t)//@11.0592MHz
{
	while(t--)
	{
	unsigned char i;
 
	i = 2;
	while (--i);

	}
}

void Delay(unsigned int xms)
{
	unsigned char i, j;
	while(xms--)
	{
		i = 2;
		j = 239;
		do
		{
			while (--j);
		} while (--i);
	}
}

