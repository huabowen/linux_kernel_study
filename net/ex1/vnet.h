#ifndef __VNET_H__
#define __VNET_H__

#define DEBUG 1

struct vnet_priv {
	struct sk_buff *txskb;
	int rxlen;
	unsigned char rxdata[ETH_DATA_LEN];
};

#endif
