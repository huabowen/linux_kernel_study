#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>

#include <linux/ip.h>

#include "vnet.h"

struct net_device *vnet_dev;

static int vnet_open(struct net_device *dev)
{
	netif_start_queue(dev);
	return 0;
}

static int vnet_stop(struct net_device *dev)
{
	netif_stop_queue(dev);
	return 0;
}

void vnet_rx(struct net_device *dev)
{
	struct sk_buff *skb;
	struct vnet_priv *priv = netdev_priv(dev);

	skb = dev_alloc_skb(priv->rxlen + 2);
	if (IS_ERR(skb)) {
		printk(KERN_NOTICE "vnet: low on mem - packet dropped\n");
		dev->stats.rx_dropped++;
		return;
	}
	skb_reserve(skb, 2);
	memcpy(skb_put(skb, priv->rxlen), priv->rxdata, priv->rxlen);

	skb->dev = dev;
	skb->protocol = eth_type_trans(skb, dev);
	skb->ip_summed = CHECKSUM_UNNECESSARY;
	dev->stats.rx_packets++;
	dev->stats.rx_bytes += priv->rxlen;
	netif_rx(skb);

	return;
}

static void vnet_rx_int(char *buf, int len, struct net_device *dev)
{
	struct vnet_priv *priv;

	priv = netdev_priv(dev);
	priv->rxlen = len;
	memcpy(priv->rxdata, buf, len);
	vnet_rx(dev);

	return;
}

static void vnet_hw_tx(char *buf, int len, struct net_device *dev)
{
#if DEBUG
	int i;
#endif

	struct iphdr *ih;
	struct net_device *dest;
	struct vnet_priv *priv;
	u32 *saddr, *daddr;

	if (len < sizeof(struct ethhdr) + sizeof(struct iphdr)) {
		printk("vnet: Packet too short (%i octets)\n", len);
		return;
	}

#if DEBUG
	printk("len is %i\n", len);
	printk("data: ");
	for (i = 0; i < len; i++)
		printk(" %02x",buf[i] & 0xff);
	printk("\n");
#endif

	ih = (struct iphdr *)(buf + sizeof(struct ethhdr));
	saddr = &ih->saddr;
	daddr = &ih->daddr;
	((u8 *)saddr)[3] = ((u8 *)saddr)[3] ^ ((u8 *)daddr)[3];
	((u8 *)daddr)[3] = ((u8 *)saddr)[3] ^ ((u8 *)daddr)[3];
	((u8 *)saddr)[3] = ((u8 *)saddr)[3] ^ ((u8 *)daddr)[3];

	ih->check = 0;
	ih->check = ip_fast_csum((unsigned char *)ih,ih->ihl);

#if DEBUG
	printk("len is %i\n", len);
	printk("data: ");
	for (i = 0; i < len; i++)
		printk(" %02x",buf[i] & 0xff);
	printk("\n\n");
#endif

	dest = vnet_dev;
	vnet_rx_int(buf, len, dest);

        dev->stats.tx_packets++;
        dev->stats.tx_bytes += len;
	priv = netdev_priv(dev);
        dev_kfree_skb(priv->txskb);
}

static netdev_tx_t vnet_tx(struct sk_buff *skb, struct net_device *dev)
{
	int len;
	char *data, shortpkt[ETH_ZLEN];
	struct vnet_priv *priv = netdev_priv(dev);
	
	data = skb->data;
	len = skb->len;
	if (len < ETH_ZLEN) {
		memset(shortpkt, 0, ETH_ZLEN);
		memcpy(shortpkt, skb->data, skb->len);
		len = ETH_ZLEN;
		data = shortpkt;
	}
	dev->trans_start = jiffies;
	priv->txskb = skb;
	vnet_hw_tx(data, len, dev);

	return 0;
}

static const struct net_device_ops vnet_ops = {
	.ndo_open		= vnet_open,
	.ndo_stop		= vnet_stop,
	.ndo_start_xmit 	= vnet_tx,
};


static int __init vnet_init(void)
{
	int status;
	struct vnet_priv *priv;

	vnet_dev = alloc_etherdev(sizeof(struct vnet_priv));
	if (IS_ERR(vnet_dev))
		return -ENOMEM;

	ether_setup(vnet_dev);
	vnet_dev->netdev_ops = &vnet_ops;
	vnet_dev->flags |= IFF_NOARP;
	priv = netdev_priv(vnet_dev);
	memset(priv, 0, sizeof(struct vnet_priv));

	status = register_netdev(vnet_dev);
	if (status) {
		free_netdev(vnet_dev);
		return status;
	}

	return 0;
}

static void __exit vnet_exit(void)
{

	unregister_netdev(vnet_dev);
	free_netdev(vnet_dev);
}

module_init(vnet_init);
module_exit(vnet_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kevin Jiang <jiangxg@farsight.com.cn>");
MODULE_DESCRIPTION("Virtual ethernet driver");
