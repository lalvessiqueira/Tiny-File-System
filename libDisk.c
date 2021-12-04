#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include "libDisk.h"

 /* The first part of the assignment is to build a 
 disk emulator, or more generically a “block device 
 emulator”. You will implement an emulator that will 
 accomplish basic block operations, like the kind supported 
 by block devices (e.g. hard disk drives), on a regular 
 Unix file.*/

 #define EXIT_FAILURE -1

int main() {
    printf("Hello World!");
    return 0;
}

int openDisk(char *filename, int nBytes) {
    int fd = -1;
    int i;

    if (!filename || nBytes < 0) {
        return EXIT_FAILURE;
    }

    if (nBytes == 0) {
        fd = open(filename, O_RDWR);
        if (fd == -1){
            return EXIT_FAILURE;
        }
    } else {
        fd = open(filename, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);
        if (fd == -1) {
            return EXIT_FAILURE;
        }

        for (i = 0; i < nBytes; i++) {
            int ret = write(fd, "\0", 1);
            if(ret == -1) {
                return EXIT_FAILURE;
            }
        }
    }
    return fd;
}

int closeDisk(int disk) {
    // do I need to check for disk?
    int ret = close(disk);
    if (ret == -1) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int readBlock(int disk, int bNum, void *block) {
// check for parameters

    // lseek write it on another local buffer
    int b_size = bNum * BLOCKSIZE;
    int ret = lseek(disk, b_size, SEEK_SET);
    // check if it worked correctly
    if (ret != b_size) {
        return EXIT_FAILURE;
    }
    ret = read(disk, block, BLOCKSIZE);
    // check
    if (ret == -1) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int writeBlock(int disk, int bNum, void *block) {
    // lseek write it on another local buffer
    int b_size = bNum * BLOCKSIZE;
    int ret = lseek(disk, b_size, SEEK_SET);
    // check if it worked correctly
    if (ret != b_size) {
        return EXIT_FAILURE;
    }
    ret = write(disk, block, BLOCKSIZE);
    // check
    if (ret == -1) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
