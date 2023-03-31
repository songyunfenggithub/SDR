#include "stdafx.h"
#include "public.h"
#include "stdio.h"
#include "math.h"
#include "stdlib.h"
#include <string.h>
#include <sstream>

#include "CWinSpectrum.h"
#include "CAudioWin.h"

HINSTANCE	hInst;

//HANDLE  cuda_FFT_hMutexBuff;

BOOLEAN Program_In_Process = true;
BOOLEAN isGetDataExited = false;

CHAR IniFilePath[] = "./default.ini";

void MainInit(void)
{

}

void StringToHex(char* p, int len)
{
	static char hex[] = "0123456789ABCDEF";

	for (int i = 0; i < len; i++)
	{
		printf("%c", hex[(unsigned char)p[i] >> 4]);
		printf("%c ", hex[(unsigned char)p[i] & 0xf]);
	}
	printf("\r\n");
}

char* DoubleToFormat(double val, int dotlen, char* p)
{
	double v = val > 0 ? val : -val;
	long lval = floor(v);
	double lvaldot = v - lval;
	int i,n = 0;
	char t[100];
	for (i = 0; lval > 0; lval /= 10, i++)
	{
		if (i && i % 3 == 0)t[i++] = ',';
		t[i] = (lval % 10) + '0';
	}
	n = i;
	i = 0;
	if (val < 0)
	{
		p[i++] = '-';
		n++;
	}
	for (; i < n; i++) p[i] = t[n - i - 1];
	p[n] = '.';
	for (i = n + 1, n = 0, lvaldot *= 10; n < dotlen; n++)
	{
		if (n && n % 3 == 0) p[i++] = ',';
		p[i++] = (char)floor(lvaldot) + '0';
		lvaldot -= floor(lvaldot);
		lvaldot *= 10;
	}
	p[i] = '\0';

	//printf("%s\r\n", p);
	return p;
}


char* fomatKINT64(INT64 v, char* str)
{
	int n = 50;
	INT64 m = v, y = 0;
	str[n] = '\0';
	char tstr[100];
	while (m != 0)
	{
		y = m % 1000;
		m = m / 1000;
		if (m != 0)
		{
			if (y < 0) y *= -1;
			n -= 4;
			sprintf(tstr, ",%03d", y);
			memcpy(str + n, tstr, 4);
		}
	}
	int e = sprintf(tstr, "%03d", y);
	n -= e;
	memcpy(str + n, tstr, e);
	return str + n;
}

char* fomatKINT64Width(INT64 v, int w, char* str)
{
	int n = 50;
	INT64 m = v, y = 0;
	str[n] = '\0';
	char tstr[100];
	for (int i = 0; i < 4; i++)
	{
		y = m % 1000;
		m = m / 1000;
		if (y < 0) y *= -1;
		n -= 4;
		sprintf(tstr, ",%03d", y);
		memcpy(str + n, tstr, 4);
	}
	n++;
	if (v < 0) {
		n--;
		*(str + n) = '-';
	}
	return str + n;
}

char* fomatLong(UINT64 n, char* str)
{
	int i = 1, q;
	UINT64 p, m, sn = 0;
	UINT64 t = n;
	while ((n / 10) != 0) {
		n = n / 10;
		i++;
	}//到这 n不存在了
	i = i - 1;
	p = 1;
	for (q = 1; q <= i; q++) {
		p = p * 10;
	}
	while (i >= 0) {
		m = t / p;
		t = t - m * p;
		sn += sprintf(str + sn, "%d", m);
		p = p / 10;
		if (i % 3 == 0 && i != 0) {
			sn += sprintf(str + sn, ",");
		}
		i = i - 1;
	}
	return str;
}

char* formatKDouble(double val, double ref, char* unitTag, char* tempstr)
{
	int u = 0;
	while (ref < 1.0) {
		ref *= 1000.0;
		u++;
		if (u == 5)break;
	}
	double v = val > 0 ? val : -val;
	UINT64 a = (UINT64)floor(v);
	UINT64 b = (UINT64)(floor((1.0 + (v - floor(v))) * pow(1000.0, u + 1)));
	char ds[100];
	char as[100] = { 0 };
	char bs[100] = { 0 };

	fomatLong(a, as);
	fomatLong(b, bs);

	int n = sprintf(ds, "%s.%s", as, &bs[2]);
	n--;
	while (ds[n] == '0' || ds[n] == ',' || ds[n] == '.') {
		if (ds[n] == '.') {
			ds[n--] = '\0';
			break;
		}
		ds[n--] = '\0';
	}
	sprintf(tempstr, "%s%s%s", val < 0 ? "-" : "", ds, unitTag);
	return tempstr;
}

char* formatKKDouble(double val, char* unitTag, char* tempstr)
{
	int n, i = 0;
	char ds[100];
	double v = val;
	val = val > 0 ? val : -val;
	if (val >= 1.0) {
		while (val >= 1.0) {
			val /= (double)1000.0;
			i++;
			if (i == 4)break;
		}
		val *= 1000.0;
		static char* U[] = { "", "k", "M", "G", "T" };
		n = sprintf(ds, "%.3f", val);
		n--;
		while (ds[n] == '0' || ds[n] == ',' || ds[n] == '.') {
			if (ds[n] == '.') {
				ds[n--] = '\0';
				break;
			}
			ds[n--] = '\0';
		}
		sprintf(tempstr, "%s%s%s%s", v > 0 ? "" : "-", ds, U[i - 1], unitTag);
	}
	else {
		while (val < 1.0) {
			val *= (double)1000.0;
			i++;
			if (i == 5)break;
		}
		n = sprintf(ds, "%.3f", val);
		n--;
		while (ds[n] == '0' || ds[n] == ',' || ds[n] == '.') {
			if (ds[n] == '.') {
				ds[n--] = '\0';
				break;
			}
			ds[n--] = '\0';
		}
		static char u[] = "munpf";
		sprintf(tempstr, "%s%s%c%s", v > 0 ? "" : "-", ds, u[i - 1], unitTag);
	}
	return tempstr;
}

char* k_formating(double dData, char* tempstr)
{
	double t;
	char c;
	if (dData > 1.0)
	{
		if ((t = dData / 1000) > 1.0)
		{
			dData = t;
			if ((t = dData / 1000) > 1.0)
			{
				dData = t;
				if ((t = dData / 1000) > 1.0) { dData = t; c = 'G'; }
				else c = 'M';
			}
			else c = 'k';
		}
		else c = 0;
	}
	else
	{
		if ((dData = dData * 1000) > 1.0) c = 'm';
		else
		{
			if ((dData = dData * 1000) > 1.0) c = 'u';
			else
			{
				if ((dData = dData * 1000) > 1.0) c = 'n';
				else
				{
					dData = dData * 1000;
					c = 'p';
				}
			}
		}
	}
	sprintf(tempstr, "%.03f% c", dData, c);
	return tempstr;
}

UINT64 MoveBits(UINT64 u, int bit)
{
	return bit > 0 ? (u << bit) : (u >> (-bit));
}

void charsToHex(char* str, int len)
{
	static char hex[] = "0123456789ABCDEF";

	for (int i = 0; i < len; i++)
	{
		printf("%c", hex[(unsigned char)str[i] >> 4]);
		printf("%c ", hex[(unsigned char)str[i] & 0xf]);
	}
	printf("\r\n");
}
