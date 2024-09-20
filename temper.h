#ifndef _INC_TEMPER_READER
#define _INC_TEMPER_READER

#include <libusb-1.0/libusb.h>

// Existing TEMPer device Vendor and Product IDs
#define TEMPER_VENDOR           0x0c45
#define TEMPER_PRODUCT          0x7401

// New TEMPerGold_V3.5 device Vendor and Product IDs
#define TEMPERGOLD_V3_5_VENDOR  0x3553
#define TEMPERGOLD_V3_5_PRODUCT 0xa001

#define BUFSIZE 128
#define TIMEOUT 5000

typedef struct {
    libusb_context *ctx;
} temper_t;

typedef struct {
    libusb_device_handle *h;
} temper_stick_t;

// Initialize the TEMPer context
int temper_init(temper_t *t);

// Count the number of connected TEMPer devices
size_t temper_count_sticks(temper_t *t);

// Retrieve a specific TEMPer stick by index
temper_stick_t *temper_get_stick(temper_t *t, size_t i);

// Get temperature from a TEMPer stick
uint8_t temper_stick_get_temp(temper_stick_t *stick, int cal, float *temp);

// Free resources allocated for a TEMPer stick
void temper_stick_free(temper_stick_t *stick);

// Free the TEMPer context
void temper_free(temper_t *);

#endif
