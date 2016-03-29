#include "lib/pciDriver.h"
#include "driver/pciDriver.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>
#include <wchar.h>
#include </usr/include/sys/types.h>

typedef struct {
    uint64_t fpga_addr;
    uint64_t host_addr;
    uint64_t descriptor_addr;
    uint64_t control_length;
} Descriptor;

typedef struct {
	unsigned long *bar0;
	unsigned long *bar1;
	int opened;
} pci_dev;

pd_device_t dev;
pci_dev pci;

#define WR_DMA 			0
#define RD_DMA 			1
#define BASE_DMA_UP		(0x0028 >> 3)
#define BASE_DMA_DOWN		(0x0050 >> 3)

int bar_Map(int dev_id);
int ext_RST(int b);
int bar_size(int b);
int format_descriptor(Descriptor *desc, int i, int op, uint64_t fpga_addr, uint64_t host_addr, uint64_t descriptor_addr, uint64_t length, uint64_t control);
void write_buff_through_multiple_descriptor(unsigned long * buff, int size, int sizeD, unsigned long destination);

int main(void)
{
	int size = 512*2;
	int sizeD = 512;

	int i;		
	unsigned long * local_buf;
	local_buf = (unsigned long *)malloc(size*sizeof(unsigned long));

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

	printf("writing local buffer\n");
	for(i=0; i<size; i++){
		local_buf[i] =  0x1111222233330000 + i;
	}

	printf("writing DMA\n");
	write_buff_through_multiple_descriptor(local_buf,size,sizeD,0);

	pd_unmapBAR(&dev,0, pci.bar0 );
	pd_unmapBAR(&dev,1, pci.bar1);
	pd_close( &dev );
	printf("exiting.\n");
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

int format_descriptor(Descriptor *desc, int i, int op, uint64_t fpga_addr, uint64_t host_addr, uint64_t descriptor_addr, uint64_t length, uint64_t control) 
{
	desc[i].fpga_addr 		= (fpga_addr & 0xFFFFFFFF) << 32 | (fpga_addr & 0xFFFFFFFF00000000) >> 32;
	desc[i].host_addr 		= (host_addr & 0xFFFFFFFF) << 32 | (host_addr & 0xFFFFFFFF00000000) >> 32;
	desc[i].descriptor_addr 	= (descriptor_addr & 0xFFFFFFFF) << 32 | (descriptor_addr & 0xFFFFFFFF00000000) >> 32;
	desc[i].control_length		= (control & 0xFFFFFFFF) << 32 | (length & 0x00000000FFFFFFFF);
	return 0;
}

void write_buff_through_multiple_descriptor(unsigned long * buff, int size, int sizeD, unsigned long destination)
{	
	struct timespec ts;
	unsigned long st, et,sum;
	unsigned long dt[10];
	double bitrate, byterate;

	void *source, *sink;
	int ret;
	int i;
	unsigned long stat_ptr;
	uint32_t ctrl;
	uint64_t start_bda, bda;

	pd_umem_t um;
	pd_umem_t umdesc;
	Descriptor *descriptor;
	int num_of_descriptors = size/sizeD;
	
	// MEMORY ALLOC TO DESCRIPTORS 
	descriptor = (uint64_t *)malloc(sizeof(Descriptor)*num_of_descriptors);
	ret = pd_mapUserMemory(&dev, (void *)descriptor, sizeof(Descriptor)*num_of_descriptors/sizeof(uint64_t), &umdesc);
	if(ret != 0) {
		printf("failed\n");
		exit(-1);
	}
	start_bda =  (umdesc.sg[0].addr & 0xFFFFFFFF) << 32 | (umdesc.sg[0].addr & 0xFFFFFFFF00000000) >> 32;
    	printf("desc physical address: %lx\n", umdesc.sg[0].addr);
    	printf("desc physical address: %lx\n", start_bda);
    	printf("desc allocated size: %lx\n", umdesc.sg[0].size);
	
	for (i = 0; i < num_of_descriptors; i++)
	{
		// MEMORY ALLOC TO DATA 
		ret = pd_mapUserMemory(&dev, (void *)buff+sizeD*i, sizeD, &um);
		if(ret != 0) {
			printf("failed\n");
			exit(-1);
		}
	    	printf("buffer physical address: %lx\n", um.sg[0].addr);
	    	printf("buffer allocated size  : %lx\n", um.sg[0].size);
		ret = pd_syncUserMemory( &um, 0 );
		if(ret != 0) {
			printf("failed\n");
			exit(-1);
		}

		if(i == (num_of_descriptors-1)){
			ctrl = 0x03118000;
			bda = 0x0000000000000000;
		}
		else {
			ctrl = 0x02118000;
			bda = umdesc.sg->addr + 8*(i+1)*sizeof(Descriptor)/sizeof(uint64_t);
		}

	      //format_descriptor(*desc,     i, op,     fpga_addr,             host_addr,   descriptor_addr, length,        control);
		format_descriptor(descriptor,i, WR_DMA, destination+i*sizeD*8, um.sg->addr, bda,             um.sg->size*8,   ctrl);
	}
	
	// READ DESCRIPTORS
	for(i=0; i< num_of_descriptors; i++){
		printf("desc%d\t: %lx\n",i, descriptor[i].fpga_addr);
		printf("desc%d\t: %lx\n",i, descriptor[i].host_addr);
		printf("desc%d\t: %lx\n",i, descriptor[i].descriptor_addr);
		printf("desc%d\t: %lx\n",i, descriptor[i].control_length);
	}
	// PROBE 1
	clock_gettime(CLOCK_REALTIME, &ts);
	st = ts.tv_nsec;

	// RESET DMA
	*(pci.bar0 + BASE_DMA_DOWN+3) = 0x0201000a00000000;
	// START DMA DESCRIPTOR CHAIN
	*(pci.bar0 + BASE_DMA_DOWN)   = 0x0000000000000000;
	*(pci.bar0 + BASE_DMA_DOWN+1) = 0x0000000000000000; //descriptor[0].host_addr;
	*(pci.bar0 + BASE_DMA_DOWN+2) = start_bda;
	*(pci.bar0 + BASE_DMA_DOWN+3) = 0x0211800000000000;
	
	while(*(pci.bar0 + BASE_DMA_DOWN + 4) != 0x00000001)
	{	
		printf("waiting...\n");
	}
	
	// PROBE 2
	clock_gettime(CLOCK_REALTIME, &ts);
	et = ts.tv_nsec;
	dt[0] = et - st;
	bitrate = (61035.15625*size)/dt[0];
	byterate = (7629.39*size)/dt[0];
	printf("\nbitrate: %fMbps\n", bitrate);
	printf("byterate: %fMBps\n", byterate);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
void write_DMA(unsigned long * buff, int size)
{
	int i;	
	int sz_desc = 5;
	int sz_data = 512;
	int n_desc = size/sz_data;

	void *source, *sink;
    	unsigned long desc[];
    	unsigned long *dlist;
    	unsigned long *data;
	unsigned long stat_ptr;

	// MEMORY ALLOC TO DATA 
	data = (unsigned long *)pd_allocKernelMemory(&dev, size, &km1);
	if (data == NULL) {
		printf("failed\n");
		exit(-1);
	}
    	printf("Data buffer physical address: %lx\n", km1.pa);

	// MEMORY ALLOC TO DESCRIPTORS 
	dlist = (unsigned long *)pd_allocKernelMemory(&dev, n_desc*sz_desc, &km2);
	if (dlist == NULL) {
		printf("failed\n");
		exit(-1);
	}
    	printf("Descriptor buffer physical address: %lx\n", km2.pa);

	// WRITE DESCRIPTORS
	for(i=0; i< n_desc; i++){	
		desc[PAH] = 0x00000000;
		desc[PAL] = sz_data*i;
		desc[HAH] = ((km1.pa + sz_data*i) >> 32) & 0xffffffff;
		desc[HAL] = (km1.pa + sz_data*i) & 0xffffffff;
		desc[NBH] = ((km2.pa + sz_desc*i) >> 32) & 0xffffffff;
		desc[NBL] = (km2.pa + sz_desc*i) & 0xffffffff;
		desc[LEN] = sz_data*8;
		if(i == (n_desc-1))
			desc[CTL] = 0x03018000;
		else
			desc[CTL] = 0x02018000;

		dlist[i*5+0] = ((desc[PAL] << 32) | (desc[PAH] & 0x00000000ffffffff));
		dlist[i*5+1] = ((desc[HAL] << 32) | (desc[HAH] & 0x00000000ffffffff));
		dlist[i*5+2] = ((desc[NBL] << 32) | (desc[NBH] & 0x00000000ffffffff));
		dlist[i*5+3] = ((desc[CTL] << 32) | (desc[LEN] & 0x00000000ffffffff));
	}

	// READ DESCRIPTORS
	for(i=0; i< sz_desc*n_desc; i++)
		printf("desc%d\t: %lx\n",i,dlist[i]);

	source = (void*) buff;
	sink   = (void*) data;
	memcpy( sink, source, size*sizeof(unsigned long) );
                                                      
	// RESET DMA
	reset_DMA(pci.bar0 + BASE_DMA_DOWN, WR_DMA);

	// SEND A DMA TRANSFER
	for(i=0; i< n_desc; i++)
		transact_DMA(pci.bar0 + BASE_DMA_DOWN, , WR_DMA);
	
	while(*(pci.bar0 + BASE_DMA_DOWN + 4) != 0x00000001)
	{	printf("waiting...\n");
	}

	pd_freeKernelMemory( &km1 );
	pd_freeKernelMemory( &km2 );
//	pd_unmapBAR(&dev,0, pci.bar0 ); 
//	pd_unmapBAR(&dev,1, pci.bar1);
//	pd_close( &dev );

//    return 0;
}

void transact_DMA(unsigned long *base, unsigned long *desc, int op) 
{
    if(op == WR_DMA){
        base[0] = ((desc[PAL] << 32) | (desc[PAH] & 0x00000000ffffffff));
        base[1] = ((desc[HAL] << 32) | (desc[HAH] & 0x00000000ffffffff));
        base[2] = ((desc[NBL] << 32) | (desc[NBH] & 0x00000000ffffffff));
        base[3] = ((desc[CTL] << 32) | (desc[LEN] & 0x00000000ffffffff));
    }
    else if(op == RD_DMA){
        base[0] = ((desc[PAH] << 32) | (base[0]   & 0x00000000ffffffff));
        base[1] = ((desc[HAH] << 32) | (desc[PAL] & 0x00000000ffffffff));
        base[2] = ((desc[NBH] << 32) | (desc[HAL] & 0x00000000ffffffff));
        base[3] = ((desc[LEN] << 32) | (desc[NBL] & 0x00000000ffffffff));
        base[4] = ((base[4] & 0xffffffff00000000) | (desc[CTL] & 0x00000000ffffffff));
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// External DMA write
void write_DMA(unsigned long * buff, int size)
{
	void *source, *sink;
    	void *ptr;
	unsigned long stat_ptr;

	ptr = pd_allocKernelMemory(&dev, size, &km);
	if (ptr == NULL) {
		printf("failed\n");
		exit(-1);
	}
    	printf("Kernel buffer physical address: %lx\n", km.pa);

	source = (void*) buff;
	sink   = ptr;
	memcpy( sink, source, size*sizeof(unsigned long) );
                                                      
	//reset DMA
	reset_DMA(pci.bar0 + BASE_DMA_DOWN, WR_DMA);

	// Send a DMA transfer
	dma.PA_h = 0x00000000;
	dma.PA_l = 0x00000000;
	dma.HA_h = (km.pa >> 32) & 0xffffffff;
	dma.HA_l = (km.pa & 0xffffffff);
	dma.next_bda_h = 0x00000000;
	dma.next_bda_l = 0x00000000;
	dma.length     = size;
	dma.control    = 0x03018000;

 //   sleep(0.01);

	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	printf("strt000: %llu\n", ts.tv_nsec);

	transact_DMA(pci.bar0 + BASE_DMA_DOWN, WR_DMA);
//	sleep(5);
	
	while(*(pci.bar0 + BASE_DMA_DOWN + 4) != 0x00000001)
	{	printf("waiting...\n");
	}

	clock_gettime(CLOCK_REALTIME, &ts);
	printf("end: %llu\n", ts.tv_nsec);

//	printf("stat_ptr: %lx\n", *(pci.bar0 + BASE_DMA_DOWN + 4));

//	status_DMA(pci.bar0 + BASE_DMA_DOWN, WR_DMA);

	//reset DMA
//	reset_DMA(pci.bar0 + BASE_DMA_DOWN, WR_DMA);

	pd_freeKernelMemory( &km );
//	pd_unmapBAR(&dev,0, pci.bar0 );
//	pd_unmapBAR(&dev,1, pci.bar1);
//	pd_close( &dev );

//    return 0;
}
*/
