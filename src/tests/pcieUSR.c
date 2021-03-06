#include "lib/pciDriver.h"
#include "driver/pciDriver.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>
#include <wchar.h>

#define MAX 			256
#define RESET 			'R'
#define SND_SMA			'S'
#define HOLD			'H'

#define STATUS 0
#define COMMAND 1

#define RD_DMA 			20
#define WR_DMA 			23

#define BASE_DMA_UP		(0x0028 >> 3)
#define BASE_DMA_DOWN		(0x0050 >> 3)
#define INT_STATUS		(0x0008 >> 3)
#define INT_EN			(0x0010 >> 3)
#define INT_CTRL		(0x0080 >> 3)
#define INT_LATENCY		(0x0084 >> 3)
#define INT_ON			(0x0088 >> 3)
#define INT_OFF			(0x008c >> 3)

#define BRAM_SPACE		(0x8000 >> 3)

typedef struct {
    unsigned long src_addr_h;
    unsigned long src_addr_l;
    unsigned long dst_addr_h;
    unsigned long dst_addr_l;
    unsigned long next_bda_h;
    unsigned long next_bda_l;
    unsigned long length;
    unsigned long control;
    unsigned long status;
} bda_t;


typedef struct {
	unsigned long *bar0;
	unsigned long *bar1;
	int opened;
} pci_dev;

pd_device_t dev;
pd_kmem_t km;
pd_umem_t um;
pci_dev pci;
bda_t dma;

int bar_Map(int dev_id);
unsigned long status_read(void);
unsigned long command_read(void);
int command_write(unsigned long command);
int read_buff (unsigned long * buff, int ptr, int size);
int write_buff(unsigned long * buff, int ptr, int size);
void read_DMA  (unsigned long * buff, int size);
void write_DMA (unsigned long * buff, int size);
int ext_RST(int b);
int bar_size(int b);
void reset_DMA(unsigned long *base, int op); 
void transact_DMA(unsigned long *base, int op); 
void status_DMA(unsigned long *base, int op);

int main(void)
{
	int size = 512;
	//int sizeB = size*8;

	int i;	
	struct timespec ts, ts2;	
	unsigned long * local_buf;
	local_buf = (unsigned long *)malloc(sizeof(unsigned long)*size);

	if (bar_Map(0) == 0)
		printf("PCI Initialized\n");
	else 
		return -1;
	printf("BAR0 Size:%08x\n", bar_size(0));
	printf("BAR1 Size:%08x\n", bar_size(1));

	if (ext_RST(0) == 0)
		printf("BAR0 Resetted\n");
	else 
		return -1;

	printf("PAGE SIZE: %lx\n",getpagesize());
	printf("writing local buffer\n");
	for(i=0; i<size; i++){
		local_buf[i] = 0x1111888844440000+i;
		//printf("%lX\n", local_buf[i]);
	}


	write_DMA(local_buf,size);

	pd_unmapBAR(&dev,0, pci.bar0 );
	pd_unmapBAR(&dev,1, pci.bar1);
	pd_close( &dev );
	printf("exiting...\n");
	return 0;

}

/*************************** Driver Interface Functions ***********************/

unsigned long status_read(void)
{
    sync();
    return pci.bar1[STATUS];
    sync();
}

unsigned long command_read(void)
{
    sync();
    return pci.bar1[COMMAND];
    sync();
}

int command_write(unsigned long command)
{
    sync();
    pci.bar1[COMMAND] = command;
    sync();
    return 0;
}

int read_buff (unsigned long * buff, int ptr, int size)
{
    void *source, *sink;
    source = (void*) &pci.bar1[ptr];
    //buff = (unsigned long *) malloc(size*sizeof(unsigned long));
    sink = (void*) buff;
    sync();
    sync();
    memcpy( sink, source, size*sizeof(unsigned long) );
    sync();
    sync();
    return 0;
}

int write_buff(unsigned long * buff, int ptr, int size)
{
    void *source, *sink;
    sink = (void*) &pci.bar1[ptr];
    source = (void*) buff;
    sync();
    sync();
    memcpy( sink, source, size*sizeof(unsigned long) );
    sync();
    sync();
    return 0;
}


int bar_Map(int dev_id)
{
	int ret;
	//Opening PCI device identified by X(=0) in device file name /dev/fpgaX,
	printf("\nTrying device %d ....", dev_id);
	ret = pd_open(dev_id, &dev);
	if(ret != 0) {
		printf("failed\n");
		return -1;
	}
	printf("Device OK\n");

	//Mapping BAR area of device into user space
	pci.bar0 = pd_mapBAR(&dev, 0 );
	pci.bar1 = pd_mapBAR(&dev, 1 );
	printf("BAR0 physical address: %lx\n", pci.bar0);
	printf("BAR1 physical address: %lx\n", pci.bar1);
	return 0;
}

int bar_size(int b){
    return pd_getBARsize(&dev, b);
}

int ext_RST(int b){
    sync();
    if(b == 0)
        memset( pci.bar0, 0x0a, 0xfff );
    else if(b == 1)
        memset( pci.bar1, 0x00, 0xfff );
    sync();
	sleep(0.01);
    return 0;
}

/************************************ DMA Functions *********************************/

// Local reset DMA
void reset_DMA(unsigned long *base, int op) 
{
	dma.control = 0x0201000a;
	if(op == WR_DMA){
		base[3] = 0x0201000a00000000;
	}
	else if(op == RD_DMA){
		base[4] = ((base[4] & 0xffffffff00000000) |  0x000000000201000a);
	}
}

// Local send DMA data
void transact_DMA(unsigned long *base, int op) 
{
    if(op == WR_DMA){
        //  32 BIT LEFT SHIFTED
        base[0] = ((dma.src_addr_l << 32) | (dma.src_addr_h & 0x00000000ffffffff));
        base[1] = ((dma.dst_addr_l << 32) | (dma.dst_addr_h & 0x00000000ffffffff));
        base[2] = ((dma.next_bda_l << 32) | (dma.next_bda_h & 0x00000000ffffffff));
        base[3] = ((dma.control    << 32) | (dma.length     & 0x00000000ffffffff));
    }
    else if(op == RD_DMA){
        base[0] = ((dma.src_addr_h << 32) | (base[0]        & 0x00000000ffffffff));
        base[1] = ((dma.dst_addr_h << 32) | (dma.src_addr_l & 0x00000000ffffffff));
        base[2] = ((dma.next_bda_h << 32) | (dma.dst_addr_l & 0x00000000ffffffff));
        base[3] = ((dma.length     << 32) | (dma.next_bda_l & 0x00000000ffffffff));
        base[4] = ((base[4] & 0xffffffff00000000) | (dma.control & 0x00000000ffffffff));
    }
}


// Local DMA status check
void status_DMA(unsigned long *base, int op)
{
    if(op == WR_DMA){
        //dma.status =  (base[4] & 0x00000000ffffffff);
        while((base[4] & 0x00000000ffffffff) != 0x00000001);
    }
    else if(op == RD_DMA){
        //dma.status =  (base[4] >> 32);
        while((base[4] >> 32) != 0x00000001);
    }
}

// External DMA read
void read_DMA (unsigned long * buff, int size)
{
	void *source, *sink;
	void *ptr;

	ptr = pd_allocKernelMemory(&dev, size, &km);
	if (ptr == NULL) {
		printf("failed\n");
		exit(-1);
	}
    	printf("Kernel buffer physical address: %lx\n", km.pa);

	//reset DMA
	reset_DMA(pci.bar0 + BASE_DMA_UP, RD_DMA);

	// Send a DMA transfer
	dma.src_addr_h = 0x00000000;
	dma.src_addr_l = 0x00008000; 
	dma.dst_addr_h = (km.pa >> 32) & 0xffffffff;
	dma.dst_addr_l = (km.pa & 0xffffffff);
	dma.next_bda_h = 0x00000000;
	dma.next_bda_l = 0x00000000;
	dma.length     = size;
	dma.control    = 0x03018000;
    
	transact_DMA(pci.bar0 + BASE_DMA_UP, RD_DMA);
//	sleep(4);
	while(*(pci.bar0 + BASE_DMA_DOWN + 4) != 0x00000001)
	{	
		printf("waiting...\n");
	}

	source = ptr;
	sink   = (void*) buff;
	memcpy( sink, source, size*sizeof(unsigned long) );

	pd_freeKernelMemory( &km );
//	pd_unmapBAR(&dev,0, pci.bar0 );
//	pd_unmapBAR(&dev,1, pci.bar1);
//	pd_close( &dev );
//      return 0;
}

// External DMA write
void write_DMA(unsigned long * buff, int size)
{
	unsigned long st, et,sum;
	unsigned long dt[10];
	double bitrate, byterate;
	void *source, *sink;
	int ret;
	int i;
	unsigned long stat_ptr;

	ret = pd_mapUserMemory(&dev, (void *)buff, size, &um);
	if(ret != 0) {
		printf("failed\n");
		exit(-1);
	}
	ret = pd_syncUserMemory( &um, 1 );
	if(ret != 0) {
		printf("failed\n");
		exit(-1);
	}
    	printf("Kernel buffer physical address: %lx\n", um.sg[0].addr);
    	printf("Kernel buffer allocated size: %lx\n", um.sg[0].size);
    	printf("Kernel buffer number of entries: %lx\n", um.nents);
    	printf("Kernel buffer physical address: %lx\n", um.sg[1].addr);
    	printf("Kernel buffer allocated size: %lx\n", um.sg[1].size);

	/*for (i=0; i<um.sg->size; i++)
	{
		printf("#%lx\n", buff[i]);
	}*/
                                                      
	//reset DMA
	reset_DMA(pci.bar0 + BASE_DMA_DOWN, WR_DMA);

	// Send a DMA transfer
	dma.src_addr_h = 0x00000000;
	dma.src_addr_l = 0x00000000;
	dma.dst_addr_h = (um.sg->addr >> 32) & 0xffffffff;
	dma.dst_addr_l = (um.sg->addr & 0xffffffff);
	dma.next_bda_h = 0x00000000;
	dma.next_bda_l = 0x00000000;
	dma.length     = um.sg->size*8;
	dma.control    = 0x03018000;

//probe 1
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	st = ts.tv_nsec;
//	printf("strt: %llu\n", ts.tv_nsec);
	transact_DMA(pci.bar0 + BASE_DMA_DOWN, WR_DMA);
	
	while(*(pci.bar0 + BASE_DMA_DOWN + 4) != 0x00000001)
	{	
		printf("waiting...\n");
	}
	//probe 2
	clock_gettime(CLOCK_REALTIME, &ts);
	et = ts.tv_nsec;
//	printf("end: %llu\n", ts.tv_nsec);
	dt[0] = et - st;
	bitrate = (61035.15625*size)/dt[0];
	byterate = (7629.39*size)/dt[0];
	printf("\nbitrate: %fMbps\n", bitrate);
	printf("byterate: %fMBps\n", byterate);

	pd_unmapUserMemory( &um );
	printf("returning...\n");
//	pd_unmapBAR(&dev,0, pci.bar0 );
//	pd_unmapBAR(&dev,1, pci.bar1);
//	pd_close( &dev );
//      return 0;
}

