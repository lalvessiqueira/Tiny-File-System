#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include "libDisk.c"
// #include "libTinyFS.c"
#include "libTinyFS.h"
#include "tinyFS_errno.h"

#define FILENAME1 "input"
#define FILENAME2 "names"
#define FILENAME3 "output"


int main(void) {
    int f1, f2, d1;
    char bufferBlock[BLOCKSIZE] = {"This is the last assignment of the quarter!"}, bufferBlock2[BLOCKSIZE] = {"Almost Done with this?"}, get_byte;

    printf("tfs_mkfs testing:\n");
    printf("\nMaking a blanks TinyFS\nReturn: %d\n\n",tfs_mkfs(DEFAULT_DISK_NAME, DEFAULT_DISK_SIZE));
	
    printf("\n----------------------------\n");
    printf("No file system to unmount!\nReturn: %d\n\n", tfs_unmount());
    printf("\n----------------------------\n");

    printf("Mouting a tiny file system...\n");
    printf("File System mounted!\nReturn: %d\n", tfs_mount(DEFAULT_DISK_NAME));

    printf("\n----------------------------\n");
    printf("Opening a file...\n");
    f1 = tfs_openFile(FILENAME1);
    if (f1) {
        printf("File %s opened successfuly!\nReturn: %d\n", FILENAME1, f1);
    } else {
        fprintf(stderr, "File %s couldn't open!\nReturn: %d\n", FILENAME1, f1);
    }
    printf("View Dynamic Resource Table:\n");
    viewDRT();
    printf("\n----------------------------\n");

    printf("Writing to a file...\n");
    f1 = tfs_writeFile(f1, bufferBlock, BLOCKSIZE);
    if (f1) {
        printf("Writing to file %s successfuly!\nReturn: %d\n", FILENAME1, f1);
    } else {
        fprintf(stderr, "Could not write to file %s!\nReturn: %d\n", FILENAME1, f1);
    }
    printf("View Dynamic Resource Table:\n");
    viewDRT();

    printf("\n----------------------------\n");

    printf("Opening another file...\n");
    f2 = tfs_openFile(FILENAME2);
    if (f2) {
        printf("File %s opened successfuly!\nReturn: %d\n", FILENAME2, f2);
    } else {
        fprintf(stderr, "File %s couldn't open!\nReturn: %d\n", FILENAME2, f2);
    }
    printf("Writing to a file...\n");
    f2 = tfs_writeFile(f2, bufferBlock2, BLOCKSIZE);
    if (f2) {
        printf("Writing to file %s successfuly!\nReturn: %d\n", FILENAME2, f2);
    } else {
        fprintf(stderr, "Could not write to file %s!\nReturn: %d\n", FILENAME2, f2);
    }
    printf("View Dynamic Resource Table:\n");
    viewDRT();

    printf("\n----------------------------\n");
    printf("Deleting %s...\n", FILENAME1);
    d1 = tfs_deleteFile(f1);
    if (d1) {
        printf("File %s deleted successfuly!\nReturn: %d\n", FILENAME1, d1);
    } else {
        fprintf(stderr, "File %s couldn't be deleted! Try again :(\nReturn: %d\n", FILENAME1, d1);
    }
	printf("View Dynamic Resource Table:\n");
    viewDRT();
    printf("\n----------------------------\n");

    f2 = tfs_openFile(FILENAME2);
    printf("ATTEMPT of reading the first byte from file %s...\n", FILENAME2);
	printf("Byte read successfully!\nReturn: %d\n", tfs_readByte(f2, &get_byte));
	printf("Byte read: %c\n", get_byte);
    viewDRT();
    printf("\n----------------------------\n");
    return 0;
}
