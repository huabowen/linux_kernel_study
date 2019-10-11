#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>

struct dma_chan *chan;
unsigned int *txbuf;
unsigned int *rxbuf;
dma_addr_t txaddr;
dma_addr_t rxaddr;

static void dma_callback(void *data)
{
	int i;
	unsigned int *p = rxbuf;
	printk("dma complete\n");

	for (i = 0; i < PAGE_SIZE / sizeof(unsigned int); i++)
		printk("%d ", *p++);
	printk("\n");
}

static bool filter(struct dma_chan *chan, void *filter_param)
{
	printk("%s\n", dma_chan_name(chan));
	return strcmp(dma_chan_name(chan), filter_param) == 0;
}

static int __init memcpy_init(void)
{
	int i;
	dma_cap_mask_t mask;
	struct dma_async_tx_descriptor *desc;
	char name[] = "dma2chan0";
	unsigned int *p;

	dma_cap_zero(mask);
	dma_cap_set(DMA_MEMCPY, mask);
	chan = dma_request_channel(mask, filter, name);
	if (!chan) {
		printk("dma_request_channel failure\n");
		return -ENODEV;
	}
	
	txbuf = dma_alloc_coherent(chan->device->dev, PAGE_SIZE, &txaddr, GFP_KERNEL);
	if (!txbuf) {
		printk("dma_alloc_coherent failure\n");
		dma_release_channel(chan);
		return -ENOMEM;
	}

	rxbuf = dma_alloc_coherent(chan->device->dev, PAGE_SIZE, &rxaddr, GFP_KERNEL);
	if (!rxbuf) {
		printk("dma_alloc_coherent failure\n");
		dma_free_coherent(chan->device->dev, PAGE_SIZE, txbuf, txaddr);
		dma_release_channel(chan);
		return -ENOMEM;
	}

	for (i = 0, p = txbuf; i < PAGE_SIZE / sizeof(unsigned int); i++)
		*p++ = i;
	for (i = 0, p = txbuf; i < PAGE_SIZE / sizeof(unsigned int); i++)
		printk("%d ", *p++);
	printk("\n");

	memset(rxbuf, 0, PAGE_SIZE);
	for (i = 0, p = rxbuf; i < PAGE_SIZE / sizeof(unsigned int); i++)
		printk("%d ", *p++);
	printk("\n");

	desc = chan->device->device_prep_dma_memcpy(chan, rxaddr, txaddr, PAGE_SIZE, DMA_CTRL_ACK | DMA_PREP_INTERRUPT);
	desc->callback = dma_callback;
	desc->callback_param = NULL;

	dmaengine_submit(desc);
	dma_async_issue_pending(chan);

	return 0;
}

static void __exit memcpy_exit(void)
{
	dma_free_coherent(chan->device->dev, PAGE_SIZE, txbuf, txaddr);
	dma_free_coherent(chan->device->dev, PAGE_SIZE, rxbuf, rxaddr);
	dma_release_channel(chan);
}

module_init(memcpy_init);
module_exit(memcpy_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kevin Jiang <jiangxg@farsight.com.cn>");
MODULE_DESCRIPTION("simple driver using dmaengine");
