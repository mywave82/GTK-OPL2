#include "adplug-instance.h"

#include <adplug/fmopl.h>

#if 0
static const int slot_array[32] =
{
         0,  2,  4,  1,  3,  5, -1, -1,
         6,  8, 10,  7,  9, 11, -1, -1,
        12, 14, 16, 13, 15, 17, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1
};
#endif


FM_OPL *instance;

void opl_instance_init(void)
{
	if (!instance)
	{
		instance = OPLCreate(OPL_TYPE_YM3812, 3579545, 44100);
	}
}

void opl_instance_done(void)
{
	if (instance)
	{
		OPLDestroy (instance);
		instance = 0;
	}
}


int opl_instance_clock(int16_t *buf, int buflen)
{
        YM3812UpdateOne(instance, buf, buflen);
}

void opl_instance_write (uint_least8_t reg, uint8_t val)
{
#if 0
        int slot = slot_array[reg&0x1f];

        switch(reg&0xe0)
        {
                case 0xe0:
                        if (slot==-1)
                                goto done;
                        wavesel[slot]=val&3;
                        break;
                case 0x40:
                        if (slot==-1)
                                goto done;
                        hardvols[slot][0] = val;
                        if (mute[slot])
                                return;
                        break;
                case 0xc0:
                        if (slot==-1)
                                goto done;
                        if (reg<=0xc8)
                        hardvols[reg-0xc0][1] = val;
                        if (mute[reg-0xc0]&&mute[reg-0xc0+9])
                                return;
                        break;
        }

done:
#endif
        OPLWrite(instance, 0, reg);
        OPLWrite(instance, 1, val);
}
