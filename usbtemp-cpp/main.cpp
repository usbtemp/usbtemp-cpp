#include "stdafx.h"
#include "usbtemp.h"
#include <windows.h>
#include <iostream>
#include <iomanip>

int main()
{
	LPCWSTR serial_port = L"COM6";
	unsigned char rom[DS18X20_ROM_SIZE];
	usbtemp Thermometer;
	float temperature;

	std::cout << "usbtemp.com C++ demo" << std::endl;

	if (Thermometer.open(serial_port)) {
		std::cerr << "Cannot open serial port!" << std::endl;
		return 1;
	}

	/* READ ROM */
	if (Thermometer.rom(rom)) {
		std::cerr << "Cannot read the ROM!" << std::endl;
		Thermometer.close();
		return 1;
	}

	/* READ TEMPERATURE */
	if (Thermometer.temperature(&temperature)) {
		std::cerr << "Cannot read the temperature!" << std::endl;
		Thermometer.close();
		return 1;
	}

	std::cout << "ROM: " << std::hex << std::setfill('0');
	for (int i = 0; i < DS18X20_ROM_SIZE; ++i) {
		std::cout << std::setw(2) << (unsigned int)rom[i] << " ";
	}
	std::cout << std::endl;

	std::cout << "T: " << std::fixed << std::setprecision(2) << temperature << std::endl;

	Thermometer.close();

	return 0;
}