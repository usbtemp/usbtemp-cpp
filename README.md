usbtemp-cpp
===

This source is intended to be used under Windows.

Source code for `usbtemp` class is located in files `usbtemp.cpp` and `usbtemp.h`.

The class provides the following functions.
```
	int open(LPCWSTR);
	void close(void);
	int rom(unsigned char *);
	int temperature(float *);
```
When applicable, function returns 0 (zero) at success or -1 otherwise.