#include "lib/pciDriver.h"
#include "driver/pciDriver.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX 16

#define Status_Pt 	0x000
#define Command_Pt 	0x001
#define Iv_Pt 		0x010
#define Key_Pt 		0x020
#define Iv_Size 	0x00C
#define Key_Size 	0x010
#define Buff1_Pt 	0x030
#define Buff2_Pt 	0x090
#define Buff_Size 	0x060

#define null 		0x00
#define init 		0x01
#define gen_ks 		0x02
#define buf_one_read_done 0x03
#define buf_two_read_done 0x04
#define reset		0x05
#define HARDRESET 	0xE5

	// output values of the states
#define not_init 	0x01
#define reset_done 	0x02
#define init_busy 	0x03
#define init_done	0x04
#define ks_gen_busy 	0x05
#define buf_one_com 	0x06
#define ks_gen_hlt_buf_two 0x07
#define buf_two_com 	0x08
#define ks_gen_hlt_buf_one 0x09
#define error  		0x0A

void testDevice(int i);
void testDirectIO(pd_device_t *dev);
void testDMA(pd_device_t *dev);

typedef struct {
	unsigned long src_addr_h;
	unsigned long src_addr_l;
	unsigned long dst_addr_h;
	unsigned long dst_addr_l;
	unsigned long length;
	unsigned long control;
	unsigned long next_bda_h;
	unsigned long next_bda_l;
	unsigned long status;
} bda_t;

int status_read(volatile unsigned long * bar)
{
	int status;
	status = (bar[0] >> 32) & 0xFF;
	printf("status \t: %x\n",status);
	return status;
}

int command_read(volatile unsigned long * bar)
{
	int command;
	command = (bar[1] >> 32) & 0xFF;
	printf("command \t: %x\n",command);
	return command;
}

int command_write(volatile unsigned long * bar, int command)
{
	volatile unsigned long tmp;
  	tmp	= command ;
	tmp	= tmp << 32;
	tmp	= tmp | bar[1] & 0xFFFFFF00FFFFFFFF;
	bar[1]	= tmp;
	printf("command : %x\n",command);
}

int read_buff(volatile unsigned long * bar, unsigned int * buff, int ptr, int size)
{
	volatile unsigned long buff1[MAX];
	int i;
	memcpy( (void*)buff1, (void*)&bar[ptr], size*sizeof(unsigned long) );
		sync();
	printf("Reading BAR.....\n");
	for(i=0;i<size;i++) {
		buff[i] = (buff1[i]>>32) & 0xFF;
		printf("%x :: %2x \n",i,buff[i]);
	}
	//printf("\n\n");
//printf("#################################################\n");
}

int write_buff(volatile unsigned long * bar,unsigned int * buff, int ptr, int size)
{
	int i;
	volatile unsigned long buff1[MAX];
	for(i=0;i<size;i++) {
		buff1[i] = buff[i];
		buff1[i] = (buff1[i] & 0xFF)<<32;
		//printf("%x :: %08lx\n",i,buff1[i]);
	}
	printf("Writing BAR....\n");
	memcpy( (void*)&bar[ptr], (void*)buff, size*sizeof(unsigned long) );
	sync();
//printf("#################################################\n");
}


void write_dma(unsigned long *base, bda_t *dma);
void write_dma(unsigned long *base, bda_t *dma);

int main(int argc, char *argv[])
{
	unsigned long val,i,j;
	char c;
	pd_device_t dev;
	volatile unsigned long *bar0, *bar1, buf[MAX];
	unsigned int buff[MAX];
	volatile unsigned long status1,tem;
	unsigned int key[]={0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};
	unsigned int iv[]={0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b};
	int status;
	int command;

	int ret;
	
	printf("Trying device %d ...", 0);
	ret = pd_open( 0, &dev );
	if (ret != 0) {
		printf("failed\n");
		return;
	}
	printf("ok\n");	

	bar0 = pd_mapBAR( &dev, 0 );
	bar1 = pd_mapBAR( &dev, 1 );

/*	printf("argc = %d\n", argc);
	for (i=0; i < argc; i++)
	{
		printf("argv[%d] = %s\n",i,argv[i]);
		
	}
*/
	while ((c = getopt (argc, argv, "rwxhz")) != -1)
		switch (c)
		{
			case 'r':
				for(i=0;i<MAX;i++) {
					val = bar1[i];
					printf("%08lx\n",val);
				}
				printf("\n\n");
				break;
			case 'w':
				for(i=0;i<MAX;i++) {
 					buf[i] = 5;
					//printf("%08lx\n",buf[i]);
				}
				memcpy( (void*)bar1, (void*)buf, MAX*sizeof(unsigned long) );
				sync();
				for(i=0;i<MAX;i++) {
					val = bar1[i];
					printf("%08lx\n",val);
				}
				printf("\n\n");
				break;
			case 'x': 
				status = status_read(bar1);
				
				command_write(bar1, HARDRESET);
				while(status != 0){
					status = status_read(bar1);
					
				}
				command_write(bar1, reset);
				while(status !=reset_done)
					status = status_read(bar1);
				write_buff(bar1, key, Key_Pt , Key_Size );
				write_buff(bar1, iv, Iv_Pt , Iv_Size );
				command_write(bar1, init);
				while(status !=init_done)
					status = status_read(bar1);
				command_write(bar1, gen_ks);
				while(status !=ks_gen_hlt_buf_one)
					status = status_read(bar1);
				//read_buff(bar1,1);
				read_buff(bar1, buff, Buff2_Pt, Buff_Size );
				for(i=0; i < Buff_Size/16; i++)
				{
					for(j=0; j < 16; j++)
						printf("%2x",buff[i*16+j]);
					printf("\n");
				}
				for(i=i*16+j; i< Buff_Size; i++)
					printf("%2x",buff[i]);
				printf("\n");
/*					buf[i] = 0x1111222233334444;
					//printf("%08lx\n",buf[i]);
				
				memcpy( (void*)bar1, (void*)buf, MAX*sizeof(unsigned long) );
				sync();
				for(i=0;i<MAX;i++) {
					val = bar1[i];
					printf("%08lx\n",val);
				}
				printf("\n\n");*/
				break;
			case 'z':	
		//		memcpy( (void*)buf, (void*) bar1, MAX*sizeof(unsigned long) );
				
				if((status == 0x0a) || (status == 0x01))
				{
					buf[1] = 0xe5e5e5e5e5e5e5e5;
					memcpy( (void*)bar1, (void*)buf, MAX*sizeof(unsigned long) );
					sync();
					tem = bar1[0];
					status1 = (tem && 0x000000ff00000000);
					status = status1 >> 32;
					printf("bar[0] : %.16lx\n",bar1[0]);
					printf("command: %.16lx\n",bar1[1]);
					printf("status1: %.16lx\n",status1);
					printf("status : %02x\n",status);
				}
				if(status == 0x00)
				{	buf[1] = 0x0505050105050505;
				        memcpy( (void*)bar1, (void*)buf, MAX*sizeof(unsigned long) );
                                	sync();
					tem = bar1[0];
					status1 = (tem && 0x000000ff00000000);
                               		 
                                	status = status1 >> 32;
                                	printf("bar[0] : %.16lx\n",bar1[0]);
                                	printf("command: %.16lx\n",bar1[1]);
					printf("status1: %.16lx\n",status1);
                                	printf("status : %02x\n",status);
				}
				if(status1 == 0x02)
                                {       buf[1] = 0x0101010101010101;
                                        memcpy( (void*)bar1, (void*)buf, MAX*sizeof(unsigned long) );
                                        sync();
                                        status1 = bar1[0] && 0x000000ff00000000;
                                        status = status1 >> 32;
                                        printf("bar[0] : %.16lx\n",bar1[0]);
                                        printf("command: %.16lx\n",bar1[1]);
                                        printf("status : %02x\n",status);
                                }
				if(status1 == 0x04)
                                {       buf[1] = 0x0202020202020202;
                                        memcpy( (void*)bar1, (void*)buf, MAX*sizeof(unsigned long) );
                                        sync();
                                        status1 = bar1[0] && 0x000000ff00000000;
                                        status = status1 >> 32;
                                        printf("bar[0] : %.16lx\n",bar1[0]);
                                        printf("command: %.16lx\n",bar1[1]);
                                        printf("status : %02x\n",status);
                                }
				if(status1 == 0x09)
                                {       buf[1] = 0x0303030303030303;
                                        memcpy( (void*)bar1, (void*)buf, MAX*sizeof(unsigned long) );
                                        sync();
                                        status1 = bar1[0] && 0x000000ff00000000;
                                        status = status1 >> 32;
                                        printf("bar[0] : %.16lx\n",bar1[0]);
                                        printf("command: %.16lx\n",bar1[1]);
                                        printf("status : %02x\n",status);
                                }
				if(status1 == 0x07)
                                {       buf[1] = 0x0404040404040404;
                                        memcpy( (void*)bar1, (void*)buf, MAX*sizeof(unsigned long) );
                                        sync();
                                        status1 = bar1[0] && 0x000000ff00000000;
                                        status = status1 >> 32;
                                        printf("bar[0] : %.16lx\n",bar1[0]);
                                        printf("command: %.16lx\n",bar1[1]);
                                        printf("status : %02x\n",status);
                                }

				break;
			case '?':
			case 'h':
				printf("Read bar1 : -r\n");
				break;
			default: break;
       		}


	//testDevice(0);
	return 0;
}

void testDevice( int i )
{
	pd_device_t dev;
	int ret;
	printf("Trying device %d ...", i);
	ret = pd_open( i, &dev );

	if (ret != 0) {
		printf("failed\n");
		return;
	}

	printf("ok\n");	

	testDirectIO(&dev);
	/*scanf("%d",&c);
	printf("%d",c);
	testDMA(&dev);
	scanf("%d",&c);	
	printf("%d",c);
	pd_close( &dev );
	scanf("%d",&c);
	printf("%d",c);*/
}

void testDirectIO(pd_device_t *dev)
{
	int i,j,ret;
	unsigned long val,buf[MAX];
	unsigned long *bar0, *bar1;

	bar0 = pd_mapBAR( dev, 0 );
	bar1 = pd_mapBAR( dev, 1 );

	/* test register memory */
	//bar0[0] = 0x1234565;
	//bar0[1] = 0x5aa5c66c;
	for(i=0;i<MAX;i++) {
		val = bar1[i];
		printf("%08lx\n",val);
	}
	printf("\n\n");

printf("#################################################\n");
	/* write block */
	for(i=0;i<MAX;i++) {
		buf[i] = ~i;
	}

	memcpy( (void*)bar1, (void*)buf, MAX*sizeof(unsigned int) );

	/* read block */
	memcpy( (void*)buf, (void*)bar1, MAX*sizeof(unsigned int) );
	for(i=0;i<MAX;i++) {
		val = buf[i];
		printf("%08lx\n",val);
	}
	printf("\n\n");
printf("#################################################\n");

	/* unmap BARs */
	pd_unmapBAR( dev,0, bar0 );
	pd_unmapBAR( dev,1, bar1 );
	
}


void testDMA(pd_device_t *dev)
{
	int i,j,ret;
	unsigned int val;
	unsigned long *bar0, *bar1;
	pd_kmem_t km;
	unsigned int *ptr;
	bda_t dma;
	const unsigned long BASE_DMA_UP   = (0x0C000 >>2);
	const unsigned long BASE_DMA_DOWN = (0x0C040 >>2);	
	const unsigned long BRAM_SIZE = 0x1000;

	bar0 = pd_mapBAR( dev, 0 );
	bar1 = pd_mapBAR( dev, 1 );

	/* test register memory */
	bar0[0] = 0x1234565;
	bar0[1] = 0x5aa5c66c;

	ptr = (unsigned int*)pd_allocKernelMemory( dev, 32768, &km );
	if (ptr == NULL) {
		printf("failed\n");
		pd_unmapBAR( dev,0, bar0 );
		pd_unmapBAR( dev,1, bar1 );
		exit(-1);
	}


	/* Print kernel buffer info */
	printf("Kernel buffer physical address: %lx\n", km.pa);
	printf("Kernel buffer size: %lx\n", km.size);

	/* Reset the DMA channel */
	*(bar1+BASE_DMA_DOWN+0x5) = 0x0000000A;

	/* Fill buffer with zeros */
	memset( ptr, 0, BRAM_SIZE );

	/* Send a DMA transfer */
	dma.src_addr_h = 0x00000000;
	dma.src_addr_l = km.pa;
	dma.dst_addr_h = 0x00000000;
	dma.dst_addr_l = 0x00000000;
	dma.length = BRAM_SIZE;
	dma.control = 0x01000000;		
	dma.next_bda_h = 0x00000000;
	dma.next_bda_l = 0x00000000;

	write_dma(bar1+BASE_DMA_DOWN,&dma);	

	/* wait */
	sleep(5);

	for(i=0;i<BRAM_SIZE/4;i++) {
		val = bar0[i];
		printf("%08x\n",val);
	}		

	ret = pd_freeKernelMemory( &km );

	/* unmap BARs */
	pd_unmapBAR( dev,0,bar0 );
	pd_unmapBAR( dev,1,bar1 );
	
}

void write_dma(unsigned long *base, bda_t *dma) {
		base[0] = dma->src_addr_h;
		base[1] = dma->src_addr_l;
		base[2] = dma->dst_addr_h;
		base[3] = dma->dst_addr_l;
		base[4] = dma->length;
		base[6] = dma->next_bda_h;
		base[7] = dma->next_bda_l;
		base[5] = dma->control;	}	/* control is written at the end, starts DMA */
