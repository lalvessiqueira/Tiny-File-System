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
    int fd = -1;
    int i;

    if (!filename || nBytes < 0) {
        return EXITFAILURE;
    }

    if (nBytes == 0) {
        fd = open(filename, O_RDWR);
        if (fd == -1){
            return EXITFAILURE;
        }
    } else {
        fd = open(filename, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);
        if (fd == -1) {
            return EXITFAILURE;
        }

        for (i = 0; i < nBytes; i++) {
            int ret = write(fd, "\0", 1);
            if(ret == -1) {
                return EXITFAILURE;
            }
        }
    }
    return fd;
}

int closeDisk(int disk) {
    // do I need to check for disk?
    int ret = close(disk);
    if (ret == -1) {
        return EXITFAILURE;
    }
    return EXIT_SUCCESS;
}

int readBlock(int disk, int bNum, void *block) {
// check for parameters
    int b_size = bNum * BLOCKSIZE;
    // lseek write it on another local buffer
    int ret;
    // check if it worked correctly
    if ((ret = lseek(disk, 0, SEEK_SET)) != b_size) {
        return EXITFAILURE;
    }
    ret = pread(disk, block, BLOCKSIZE, b_size);
    // check
    if (ret == -1) {
        return EXITFAILURE;
    }
    return EXIT_SUCCESS;
}

int writeBlock(int disk, int bNum, void *block) {
    // lseek write it on another local buffer
    int b_size = bNum * BLOCKSIZE;
    int ret = lseek(disk, 0, SEEK_SET);
    // check if it worked correctly
    if (ret != b_size) {
        return EXITFAILURE;
    }
    // check
    if ((ret = write(disk, block, BLOCKSIZE)) == -1) {
        printf("HERE!");
        return EXITFAILURE;
    }
    return EXIT_SUCCESS;
}
