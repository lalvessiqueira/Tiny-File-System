#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
 /* The first part of the assignment is to build a 
 disk emulator, or more generically a “block device 
 emulator”. You will implement an emulator that will 
 accomplish basic block operations, like the kind supported 
 by block devices (e.g. hard disk drives), on a regular 
 Unix file.*/

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

    }



}

int closeDisk(int disk) {

}

int readBlock(int disk, int bNum, void *block) {

}

int writeBlock(int disk, int bNum, void *block) {

}
