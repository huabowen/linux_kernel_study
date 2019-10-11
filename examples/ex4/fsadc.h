#ifndef _FSADC_H
#define _FSADC_H

#define FSADC_MAGIC	'f'

union chan_val {
	unsigned int chan;
	unsigned int val;
};

#define FSADC_GET_VAL	_IOWR(FSADC_MAGIC, 0, union chan_val)

#define AIN0	0
#define AIN1	1
#define AIN2	2
#define AIN3	3

#endif
