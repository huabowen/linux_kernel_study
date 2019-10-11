#ifndef _PDIUSBD12_H
#define _PDIUSBD12_H

struct d12key {
	unsigned char key[8];
};

struct d12led {
	unsigned char led[8];
};

#define PDIUSBD12_MAGIC   'p'

#define PDIUSBD12_GET_KEY _IOR(PDIUSBD12_MAGIC, 0, struct d12key)
#define PDIUSBD12_SET_LED _IOW(PDIUSBD12_MAGIC, 1, struct d12led)

#endif
