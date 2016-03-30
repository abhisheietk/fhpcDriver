#!/bin/bash

qemu-system-x86_64 -enable-kvm -smp 2 -m 1024 -drive file=vm1.img,cache=writeback -device ivshmem,shm=ivshmem,size=1 -net tap -net nic
