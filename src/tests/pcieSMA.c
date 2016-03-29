
//#include "pcieSMA.h"
#include "lib/pciDriver.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX 		256
#define RESET 		'R'
#define SND_SMA 	'S'
#define HOLD		'H'


typedef struct {
	volatile unsigned long *bar0;
	volatile unsigned long *bar1;
	int opened;
} pci_dev;


int status_read(volatile unsigned long * bar)
{
	int status;
	status = (bar[0] >> 32) & 0xFF;
	printf("Status Read\t: %x\n",status);
	return status;
}

int command_read(volatile unsigned long * bar)
{
	int command;
	command = (bar[1] >> 32) & 0xFF;
	printf("Command Read\t: %x\n",command);
	return command;
}

int command_write(volatile unsigned long * bar, int command)
{
	volatile unsigned long tmp;
  	tmp	= command ;
	tmp	= tmp << 32;
	tmp	= tmp | bar[1] & 0xFFFFFF00FFFFFFFF;
	bar[1]	= tmp;
	printf("Command Written\t: %x\n",command);
}

void read_buff(volatile unsigned long * bar, volatile unsigned long * buff, int ptr, int size)
{
	volatile unsigned long buff1[MAX];
	int i;
	memcpy( (void*)buff1, (void*)&bar[ptr], size*sizeof(unsigned long) );
	sync();
	printf("Reading BAR.....\n");
	for(i=0;i<size;i++) {
		buff[i] = (buff1[i]>>32);// & 0xFF;
		printf("%x :: %lx \n",i,buff[i]);
	}
	printf("BAR Read succesfully\n\n");
}

void write_buff(volatile unsigned long * bar,volatile unsigned long * buff, int ptr, int size)
{
	int i;
	volatile unsigned long buff1[MAX];
	printf("Writing BAR1 with following data:\n");
	for(i=0;i<size;i++) {
		buff1[i] = (buff[i]<<32);
		printf("%x :: %lx\n",i,buff1[i]);
	}
	memcpy( (void*)&bar[ptr], (void*)buff1, size*sizeof(unsigned long) );
	sync();
	printf("BAR written\n\n");	
}


pci_dev bar_Map(int dev_id)
{
    pd_device_t dev;
    pci_dev pci;
    int ret;

//Opening PCI device identified by X(=0) in device file name /dev/fpgaX,
	printf("\nTrying device %d ....", dev_id);
	ret = pd_open(dev_id, &dev);
	if(ret != 0) {
		pci.opened = 0;
		return pci;
	}
    pci.opened = 1;
	printf("Device OK\n");

//Mapping BAR area of device into user space
	pci.bar0 = pd_mapBAR(&dev, 0 );
	pci.bar1 = pd_mapBAR(&dev, 1 );
    return pci;
}

int main(int argc, char *argv[])
{
    pci_dev pci;

	volatile unsigned long buf[MAX], rd_buf[MAX];
	char rd_cmd,cmd;
	int i;
    
    printf("marker 1");
    pci = bar_Map(0);
    if(pci.opened == 0){
        printf("Failed. Try again\n");
        return;
    }
    
//Application Interface

	while((cmd = getopt(argc, argv, "hwRSr")) != -1)	
		switch(cmd)
		{
			case 'w':
				command_write(pci.bar1, HOLD);
				for(i=0;i<MAX;i++) {
 					buf[i] = 0x12345678;//((i*2)<<8) | i;
				//	printf("%lx\n",buf[i]);
				}
				write_buff(pci.bar1, buf, 0x000, (MAX) );
			break;

			case 'R':
				command_write(pci.bar1, RESET);
				rd_cmd = command_read(pci.bar1);
				while(rd_cmd != RESET);
			break;

			case 'S':
				command_write(pci.bar1, SND_SMA);
				rd_cmd = command_read(pci.bar1);
       		                while(rd_cmd != SND_SMA);
			break;

			case 'r':
				read_buff(pci.bar1, rd_buf, 0, MAX);
				break;

			case 'h':
			printf("\nRun code using  <./smaLoop -<x>>  command where 'x' is:\n");
			printf("-w: Write BAR1\n-R: Reset SMA\n");
			printf("-S: Send SMA  \n-r: Read BAR1\n\n");
			break;

			default: break;
		}
	return 0;
}
	
