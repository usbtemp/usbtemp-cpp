#pragma once

#include "windows.h"

#define DS18X20_GENERATOR 0x8c
#define DS18X20_ROM_SIZE 8
#define DS18X20_SP_SIZE 9

class usbtemp {
private:
	HANDLE fd;
public:
	int open(LPCWSTR);
	void close(void);
	int rom(unsigned char *);
	int temperature(float *);
};