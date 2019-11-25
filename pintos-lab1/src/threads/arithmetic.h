#include <stdint.h>

#define f 16384

int intToFix (int n)
{
	return n*f;
}

//rounded to nearest


int fixToInt (int x) 
{
	return x/f;
}

int fixToIntRound (int x) 
{
	if (x>=0) 
		return (x+f/2)/f;
	else 
		return (x-f/2)/f;
}

int addTwoFix(int x, int y)
{
	return x+y;
}

int subTwoFix(int x, int y)
{
	return x-y;
}

int addIntFix(int x, int n)
{
	return x+n*f;
}

int subIntFix(int x, int n)
{
	return x-n*f;
}

int multTwoFix(int x, int y)
{
	return ((long long)x*y/f);
}

int multIntFix(int x, int n)
{
	return x*n;
}

int divTwoFix(int x, int y)
{
	return (long long) x * f / y;
}

int divIntFix(int x, int n)
{
	return  x/n;
}