#ifndef _INSTANCE_H
#define _INSTANCE_H 1

#include <stdint.h>

extern void opl_instance_init(void);

extern void opl_instance_done(void);

extern int opl_instance_clock(int16_t *buf, int buflen);

extern void opl_instance_write (uint_least8_t addr, uint8_t data);

#endif
