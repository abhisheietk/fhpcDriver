
#include "lib/pciDriver.h"
#include <iostream>
#include <iomanip>
#include <unistd.h>

#include <pthread.h>
#include <string.h>

using namespace pciDriver;
using namespace std;

#define MAX 1024*64
#define KBUF_SIZE (4096)
#define UBUF_SIZE (4096)

void testDevice( int i );
void testBARs(pciDriver::PciDevice *dev);
void testDirectIO(pciDriver::PciDevice *dev);
void writeDMA( unsigned long *bar1, unsigned long pa, unsigned long addr, unsigned long next, unsigned long size, bool inc, bool block);
void readDMA( unsigned long *bar1, unsigned long pa, unsigned long addr, unsigned long next, unsigned long size, bool inc, bool block);
void testDMA(pciDriver::PciDevice *dev);
void testInterrupts(pciDriver::PciDevice *dev);

class BDA {
public:
    unsigned long src_addr_h;
    unsigned long src_addr_l;
    unsigned long dst_addr_h;
    unsigned long dst_addr_l;
    unsigned long length;
    unsigned long control;
    unsigned long next_bda_h;
    unsigned long next_bda_l;
    unsigned long status;

    void write(unsigned long *base) {
        base[0] = src_addr_h;
        base[1] = src_addr_l;
        base[2] = dst_addr_h;
        base[3] = dst_addr_l;
        base[4] = next_bda_h;
        base[5] = next_bda_l;
        base[6] = length;
        base[7] = control;		// control is written at the end, starts DMA
        sleep(5);
        status = base[8];
        cout << "status:" << status << endl;
    }

    void reset(unsigned long *base) {
        base[7] = 0x0200000A;
    }
};


class DDList {
public:
    DDList(int size) {
        lod = new BDA[size];
        this->count = size;
    }
    ~DDList() {
        delete lod;
    }

    int length() { return count; }

    BDA& operator[](int index) {
        return lod[index];
    }
private:
    int count;
    BDA *lod;
};

int main() 
{
    int i;
    testDevice(0);
    return 0;
}

void testDevice(int i) {
	pciDriver::PciDevice *device;
	
	try {
		cout << "Trying device " << i << " ... ";
		device = new pciDriver::PciDevice(i);
		cout << "Device found" << endl;

	//	testBARs(device);
	//	testDirectIO(device);
		testDMA(device);
	//	testInterrupts(device);

		delete device;

	} catch (Exception& e) {
		cout << "failed: " << e.toString() << endl;
		return;
	}

}
/*
void testBARs(pciDriver::PciDevice *dev)
{
	int i;
	void *bar;
	unsigned int size;
	
	dev->open();
	
	for(i=0;i<6;i++) {
		cout << "Mapping BAR " << i << " ... ";
		try {
			bar = dev->mapBAR(i);
			cout << "mapped ";
			dev->unmapBAR(i,bar);
			cout << "unmapped ";
			size = dev->getBARsize(i);
			cout << size << endl;
		} catch (Exception& e) {
			cout << "failed" << endl;
		}
	}

	dev->close();
}
*/
void writeDMA( unsigned long *bar1, unsigned long pa, unsigned long addr, unsigned long next,
               unsigned long size, bool inc, bool block)
{
	BDA dma;
	
	unsigned int buf[UBUF_SIZE], i, val;
	const unsigned long BASE_DMA_DOWN = 0x0050 >> 2; (0x0C040 >> 2);

	// TODO: add inc to control word
	// TODO: add lastDescriptor to control word, based on 'next'

	cout << "Reset DMA" << endl;
	dma.reset(bar1+BASE_DMA_DOWN);

	// Send a DMA transfer
	dma.src_addr_h = 0x00000000;
	dma.src_addr_l = pa;
	dma.dst_addr_h = 0x00000000;
	dma.dst_addr_l = addr;
	dma.length     = size;
	dma.control    = 0x03028000;
	dma.next_bda_h = 0x00000000;
	dma.next_bda_l = next;

	cout << "DMA write" << endl;
	dma.write(bar1+BASE_DMA_DOWN);

	if (block) {
		// TODO: wait for the finished status
		
	}	
}


void readDMA( unsigned long *bar1, unsigned long pa, unsigned long addr, unsigned long next,
              unsigned long size, bool inc, bool block )
{
	BDA dma;
	const unsigned long BASE_DMA_UP   = 0x002c >> 2; (0x0C000 >> 2);

	// TODO: add inc to control word
	// TODO: add lastDescriptor to control word, based on 'next'

	dma.reset(bar1+BASE_DMA_UP);

	// Send a DMA transfer
	dma.src_addr_h = 0x00000000;
	dma.src_addr_l = addr;
	dma.dst_addr_h = 0x00000000;
	dma.dst_addr_l = pa;
	dma.length     = size;
	dma.control    = 0x03028000;
	dma.next_bda_h = 0x00000000;
	dma.next_bda_l = next;

	cout << "DMA read" << endl;
	dma.write(bar1+BASE_DMA_UP);

	if (block) {
		// TODO: wait for the finished status
			
	}	
}


void testDMAKernelMemory( 
		unsigned long *bar0, 
		unsigned long *bar1, 
		KernelMemory *km, 
		const unsigned long BRAM_SIZE )
{
	BDA dma;
	int err,i,j=0;
	unsigned long *adrs[100];
	unsigned long *base;
	unsigned long *ptr;

	ptr = static_cast<unsigned long *>( km->getBuffer() );

	//**** DMA Write (down)

	// fill buffer
	// send buffer
	// compare buffer
	
	//**** DMA Read (up)
	
	// fill buffer with simple IO
	// read buffer
	// compare buffer
	

	cout << "Fill buffer with zeros" << endl;
	memset( ptr, '$', BRAM_SIZE );

	writeDMA( bar0, km->getPhysicalAddress(), 0x55555555, 0x00000000, BRAM_SIZE, true, true );

/*	// Check
	cout << "Checking BRAM..." << flush;
	for(err=0,i=0;i<(BRAM_SIZE >> 2);i++) {
	//	cout << bar1[i] << endl;
        if ( bar1[i] != '$' ) {
            err++;
            
        }
    }
    cout << err << endl;
	if (err==0)
		cout << "no error" << endl;*/
/*
	// Print contents of the BlockRAM
	cout << "Contents of the BRAM" << endl;
	for(i=0;i< 0x9000;i++) {
		if(bar1[i] == 0){
            adrs[j] = &bar1[i];
            j++;
        }
        cout << setw(8) << hex << &bar1[i] << ": " << setw(8) << bar1[i] << endl;
    } 
	cout << endl;

    cout << "adrs" << endl;
	for(j=0;j< 100;j+6) {
        cout << setw(8) << hex << adrs[j] << endl;
    }
	cout << endl;
*/
/* /////////////////////////////////////////////////////////////////////////////////////////////////////
	// second write
	cout << "Fill buffer with a pattern" << endl;
	// fill with pattern
	for(i=0;i<(BRAM_SIZE >> 2);i++) {
		if ((i & 0x00000001) == 0)
			ptr[i] = 0x5555aaaa;
		else
			ptr[i] = 0xbbbb6666;
	}

	writeDMA( bar0, km->getPhysicalAddress(), 0x00000000, 0x00000000, BRAM_SIZE, true, true );

    cout << "Reading bar1 via memcpy" << endl;
	memcpy( (void*)buf, (void*)bar1, MAX*sizeof(unsigned int) );
	for(i=0;i< (BRAM_SIZE >> 2);i++) {
		val = buf[i];
		cout << i << hex << setw(8) << val << endl;
	}
	cout << endl << endl;

	// Print contents of the BlockRAM
	cout << "Contents of the BRAM after DMA write" << endl;
	for(i=0;i<(BRAM_SIZE >> 2);i++) 
		cout << setw(4) << hex << i*4 << ": " << setw(8) << bar1[i] << endl;
	cout << endl;

	//**** DMA Read (up)
	// From Device to Host. Uses DMA UP

	// Clear buffer
	cout << "Fill buffer with dollars:" << endl;
	memset( ptr, '$', BRAM_SIZE );

	readDMA( bar0, km->getPhysicalAddress(), 0x00000000, 0x00000000, BRAM_SIZE, true, true );

	// Get Buffer contents
	cout << "Get Buffer content after DMA read" << endl;
	for(i=0;i<(BRAM_SIZE >> 2);i++) 
		cout << setw(4) << hex << i*4 << ": " << setw(8) << ptr[i] << endl;
	cout << endl << endl;

	// Clear buffer
	cout << "Clear FPGA area with zeros:" << endl;
	memset( bar0, 0, BRAM_SIZE );

	readDMA( bar0, km->getPhysicalAddress(), 0x00000000, 0x00000000, BRAM_SIZE, true, true );

	// Get Buffer contents
	cout << "Get Buffer content after DMA" << endl;
	for(i=0;i<(BRAM_SIZE >> 2);i++) 
		cout << setw(4) << hex << i*4 << ": " << setw(8) << ptr[i] << endl;
	cout << endl << endl;

*/
}

void testDMA(pciDriver::PciDevice *dev)
{
	KernelMemory *km;
	UserMemory *um;
	unsigned int i, val;
	unsigned long *ptr;
	unsigned long *bar0, *bar1;

	const unsigned long BRAM_SIZE = 0x1000;
	unsigned int umBuf[BRAM_SIZE];


	unsigned long *base;

	try {
		// Open device
		dev->open();

		// Map BARs
		bar0 = static_cast<unsigned long *>( dev->mapBAR(0) );
		bar1 = static_cast<unsigned long *>( dev->mapBAR(1) );

		// Create buffers
		km = &dev->allocKernelMemory( BRAM_SIZE );
		um = &dev->mapUserMemory( umBuf ,BRAM_SIZE, true );

		// Test Kernel Buffer

		cout << "Kernel Buffer User address: " << hex << setw(8) << ptr << endl;
		cout << "Kernel Buffer Physical address: " << hex << setw(8) << km->getPhysicalAddress() << endl;
		cout << "Kernel Buffer Size: " << hex << setw(8) << km->getSize() << endl;

		testDMAKernelMemory( bar0, bar1, km, BRAM_SIZE );
		//testDMAUserMemory( bar0, bar1, um, BRAM_SIZE );

		// Delete buffer descriptors
		delete km;
		delete um;

		// Unmap BARs
		dev->unmapBAR(0,bar0);
		dev->unmapBAR(1,bar1);

		// Close device
		dev->close();

	} catch(Exception&e) {
		cout << "Exception: " << e.toString() << endl;
	}	
}