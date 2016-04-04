CC = gcc
CFLAGS = -O3 -Wall -Werror
LIBS = -lrt

# a very simple makefile to build the inter-VM shared memory server

all: ivshmem_server ne_ivshmem_shm_host_usr ne_ivshmem_send_qeventfd

.c.o:
	$(CC) $(CFLAGS) -c $^ -o $@

ivshmem_server: ivshmem_server.o send_scm.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

ne_ivshmem_shm_host_usr: ne_ivshmem_shm_host_usr.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

ne_ivshmem_send_qeventfd: ne_ivshmem_send_qeventfd.o send_scm.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f *.o ivshmem_server


