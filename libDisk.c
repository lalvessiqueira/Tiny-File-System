#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include "libDisk.h"
#include "libTinyFS.h"
#include "tinyFS_errno.h"

 /* The first part of the assignment is to build a 
 disk emulator, or more generically a “block device 
 emulator”. You will implement an emulator that will 
 accomplish basic block operations, like the kind supported 
 by block devices (e.g. hard disk drives), on a regular 
 Unix file.*/

int openDisk(char *filename, int nBytes) {
    fileDescriptor o_disk, w_disk;
    int i;
    char data[nBytes];

    if (!filename || nBytes < 0) {
        return EXITFAILURE;
    }

    if (nBytes == 0) {
        o_disk = open(filename, O_RDWR, S_IRUSR | S_IWUSR);
        if (o_disk < 0){
            return EXITFAILURE;
        }
    } else {
        o_disk = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
        if (o_disk < 0) {
            return EXITFAILURE;
        }

        for (i = 0; i < nBytes; i++) {
            data[i] = '\0';
        }

        w_disk = write(o_disk, data, nBytes);
        if(w_disk == -1) {
           return EXITFAILURE;
        }
    }
    return o_disk;
}

int closeDisk(int disk) {
    fileDescriptor c_disk;
    c_disk = close(disk);
    if (c_disk < 0) {
        return EXITFAILURE;
    }
    return EXIT_SUCCESS;
}

int readBlock(int disk, int bNum, void *block) {
// check for parameters
    fileDescriptor r_disk, ls;
    int b_size = bNum * BLOCKSIZE;

    // lseek write it on another local buffer
    ls = lseek(disk, 0, SEEK_SET);
    if (ls == -1) {
        fprintf(stderr,"Oops, lseek error!\n");
        return EXITFAILURE;
    }
    // check if it worked correctly
    r_disk = pread(disk, block, BLOCKSIZE, b_size);
    if (r_disk == -1) {
        return EXITFAILURE;
    }
    return EXIT_SUCCESS;
}

int writeBlock(int disk, int bNum, void *block) {
    // lseek write it on another local buffer
    int b_size = bNum * BLOCKSIZE;
    fileDescriptor ls, w_disk;
    ls = lseek(disk, 0, SEEK_SET);

    // check if it worked correctly
    if (ls == -1) {
        fprintf(stderr,"Oops, lseek error!\n");
        return EXITFAILURE;
    }
    // check
    w_disk = pwrite(disk, block, BLOCKSIZE, b_size);
    if (w_disk  == -1) {
        return EXITFAILURE;
    }
    return EXIT_SUCCESS;
}
