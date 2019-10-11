#ifndef _CH368_H
#define _CH368_H

union addr_data {
	unsigned char addr;
	unsigned char data;
};

#define CH368_MAGIC   'c'

#define CH368_RD_CFG	_IOWR(CH368_MAGIC, 0, union addr_data)
#define CH368_WR_CFG	_IOWR(CH368_MAGIC, 1, union addr_data)
#define CH368_RD_IO	_IOWR(CH368_MAGIC, 2, union addr_data)
#define CH368_WR_IO	_IOWR(CH368_MAGIC, 3, union addr_data)

#endif
