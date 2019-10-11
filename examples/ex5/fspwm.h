#ifndef _FSPWM_H
#define _FSPWM_H

#define FSPWM_MAGIC	'f'

#define FSPWM_START	_IO(FSPWM_MAGIC, 0)
#define FSPWM_STOP	_IO(FSPWM_MAGIC, 1)
#define FSPWM_SET_FREQ	_IOW(FSPWM_MAGIC, 2, unsigned int)

#endif
