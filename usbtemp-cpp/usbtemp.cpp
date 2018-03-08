#include "stdafx.h"
#include "usbtemp.h"

#define OK 0
#define FAIL -1

static unsigned char lsb_crc8(unsigned char *data_in, unsigned int len, const unsigned char generator)
{
	unsigned char i, bit_counter;
	unsigned char crc = 0;

	for (i = 0; i < len; i++) {
		crc ^= *(data_in + i);
		bit_counter = 8;
		do {
			if (crc & 0x01)
				crc = (((crc >> 1) & 0x7f) ^ generator);
			else
				crc = (crc >> 1) & 0x7f;
			bit_counter--;
		} while (bit_counter > 0);
	}
	return crc;
}

static int owReset(HANDLE fd)
{
	int rv;
	DWORD wbytes, rbytes;
	unsigned char wbuff, rbuff;
	DCB dcb;

	PurgeComm(fd, PURGE_RXCLEAR | PURGE_TXCLEAR);

	if (!GetCommState(fd, &dcb)) {
		return FAIL;
	}
	dcb.BaudRate = CBR_9600;
	dcb.ByteSize = 8;
	if (!SetCommState(fd, &dcb)) {
		return FAIL;
	}

	wbuff = 0xf0;
	WriteFile(fd, &wbuff, 1, &wbytes, NULL);
	if (wbytes != 1) {
		return FAIL;
	}

	ReadFile(fd, &rbuff, 1, &rbytes, NULL);
	if (rbytes != 1) {
		rv = FAIL;
	}
	else {
		switch (rbuff) {
		case 0x00:
		case 0xf0:
			rv = FAIL;
			break;
		default:
			rv = OK;
		}
	}

	dcb.BaudRate = CBR_115200;
	dcb.ByteSize = 6;
	if (!SetCommState(fd, &dcb)) {
		return FAIL;
	}

	return rv;
}

static unsigned char owWriteByte(HANDLE fd, unsigned char wbuff)
{
	char buf[8];
	DWORD wbytes, rbytes, remaining;
	unsigned char rbuff, i;

	PurgeComm(fd, PURGE_RXCLEAR | PURGE_TXCLEAR);

	for (i = 0; i < 8; i++) {
		buf[i] = (wbuff & (1 << (i & 0x7))) ? 0xff : 0x00;
	}
	WriteFile(fd, &buf, 8, &wbytes, NULL);
	if (wbytes != 8) {
		return FAIL;
	}

	rbuff = 0;
	remaining = 8;
	while (remaining > 0) {
		ReadFile(fd, &buf, remaining, &rbytes, NULL);
		for (i = 0; i < rbytes; i++) {
			rbuff >>= 1;
			rbuff |= (buf[i] & 0x01) ? 0x80 : 0x00;
			remaining--;
		}
	}
	return rbuff;
}

static unsigned char owRead(HANDLE fd)
{
	return owWriteByte(fd, 0xff);
}

static int owWrite(HANDLE fd, unsigned char wbuff)
{
	if (owWriteByte(fd, wbuff) != wbuff) {
		return FAIL;
	}
	return OK;
}

int usbtemp::open(LPCWSTR serial_port)
{
	DCB dcb = { 0 };
	COMMTIMEOUTS timeouts = { 0 };

	fd = CreateFile(serial_port, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (fd == INVALID_HANDLE_VALUE) {
		return FAIL;
	}

	SecureZeroMemory(&dcb, sizeof(DCB));
	dcb.DCBlength = sizeof(DCB);

	if (!GetCommState(fd, &dcb)) {
		CloseHandle(fd);
		return FAIL;
	}

	dcb.BaudRate = CBR_115200;
	dcb.ByteSize = 6;
	dcb.StopBits = ONESTOPBIT;
	dcb.Parity = NOPARITY;

	if (!SetCommState(fd, &dcb)) {
		CloseHandle(fd);
		return FAIL;
	}

	timeouts.ReadIntervalTimeout = 50;
	timeouts.ReadTotalTimeoutConstant = 50;
	timeouts.ReadTotalTimeoutMultiplier = 10;
	timeouts.WriteTotalTimeoutConstant = 50;
	timeouts.WriteTotalTimeoutMultiplier = 10;

	if (!SetCommTimeouts(fd, &timeouts)) {
		CloseHandle(fd);
		return FAIL;
	}

	return OK;
}

void usbtemp::close()
{
	CloseHandle(fd);
}

int usbtemp::rom(unsigned char *rom)
{
	unsigned char i, crc;

	if (owReset(fd) == FAIL) {
		return FAIL;
	}
	if (owWrite(fd, 0x33) == FAIL) {
		return FAIL;
	}

	for (i = 0; i < DS18X20_ROM_SIZE; i++) {
		rom[i] = owRead(fd);
	}

	crc = lsb_crc8(rom, DS18X20_ROM_SIZE - 1, DS18X20_GENERATOR);
	if (rom[DS18X20_ROM_SIZE - 1] != crc) {
		return FAIL;
	}

	return 0;
}

int usbtemp::temperature(float *temperature)
{
	unsigned short T;
	unsigned char i, crc, sp_sensor[DS18X20_SP_SIZE];

	if (owReset(fd) == FAIL) {
		return FAIL;
	}
	if (owWrite(fd, 0xcc) == FAIL) {
		return FAIL;
	}
	if (owWrite(fd, 0x44) == FAIL) {
		return FAIL;
	}

	Sleep(800);

	if (owReset(fd) == FAIL) {
		return FAIL;
	}
	if (owWrite(fd, 0xcc) == FAIL) {
		return FAIL;
	}
	if (owWrite(fd, 0xbe) == FAIL) {
		return FAIL;
	}

	for (i = 0; i < DS18X20_SP_SIZE; i++) {
		sp_sensor[i] = owRead(fd);
	}

	if ((sp_sensor[4] & 0x9f) != 0x1f) {
		return FAIL;
	}

	crc = lsb_crc8(&sp_sensor[0], DS18X20_SP_SIZE - 1, DS18X20_GENERATOR);
	if (sp_sensor[DS18X20_SP_SIZE - 1] != crc) {
		return FAIL;
	}

	T = (sp_sensor[1] << 8) + (sp_sensor[0] & 0xff);
	if ((T >> 15) & 0x01) {
		T--;
		T ^= 0xffff;
		T *= -1;
	}
	*temperature = (float)T / 16;

	return OK;
}