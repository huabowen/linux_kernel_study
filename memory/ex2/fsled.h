#ifndef _FSLED_H
#define _FSLED_H

#define FSLED_MAGIC	'f'

#define FSLED_ON	_IOW(FSLED_MAGIC, 0, unsigned int)
#define FSLED_OFF	_IOW(FSLED_MAGIC, 1, unsigned int)

#define	LED2	0
#define	LED3	1
#define	LED4	2
#define	LED5	3

#endif
