#ifndef _INC_TEMPER_READER
#define _INC_TEMPER_READER

#include <libusb-1.0/libusb.h>

#define TEMPER_VENDOR 0x0c45
#define TEMPER_PRODUCT 0x7401
#define BUFSIZE 128
#define TIMEOUT 5000

typedef struct {
	libusb_context *ctx;
} temper_t;

typedef struct {
	libusb_device_handle *h;	
} temper_stick_t;

int temper_init(temper_t *t);
size_t temper_count_sticks(temper_t *t);
temper_stick_t *temper_get_stick(temper_t *t, size_t i);
uint8_t temper_stick_get_temp(temper_stick_t *stick, int cal, float *temp);
void temper_stick_free(temper_stick_t *stick);
void temper_free(temper_t *);


#endif
