#ifndef MODE_SWITCH_H
#define MODE_SWITCH_H

#include <stdint.h>

#define DEVICE_MODE   1U
#define HOST_MODE     2U

extern volatile uint8_t current_mode;

void switch_to_device(void);
void switch_to_host(void);

#endif
