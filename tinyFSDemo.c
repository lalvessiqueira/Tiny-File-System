#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include "libDisk.c"
// #include "libTinyFS.c"
#include "libTinyFS.h"
#include "tinyFS_errno.h"

int main(void) {
    int file1, file2;
    char bufferBlock[BLOCKSIZE] = {"This is the last assignment of the quarter!"};
    char bufferBlock2[BLOCKSIZE] = {"Almost Done with this?"};
    char read;

    printf("tfs_mkfs testing:\n");
    printf("\nMaking a blanks TinyFS\nReturns: %d\n\n",tfs_mkfs(DEFAULT_DISK_NAME, DEFAULT_DISK_SIZE));
	
    printf("\n----------------------------\n");

    printf("No file system to unmount!\nReturns: %d\n\n", tfs_unmount());

    printf("\n----------------------------\n");

    printf("Mouting a tiny file system...\n");
    printf("File System mounted!\nReturns: %d\n", tfs_mount(DEFAULT_DISK_NAME));
    tfs_displayFragments();

    printf("\n----------------------------\n");
    file1 = tfs_openFile("test1");
	printf("tfs_openFile('test1') returns: %d\n", file1);
	printf("After opening a new file, 'tinyFsDisk' looks like...\n");
	tfs_displayFragments();

    printf("\n----------------------------\n");
    printf("Let's write some data to 'file1'...\n");
    printf("So the write should write the content of the buffer ‘block’ to that location of file1\n");
	printf("tfs_writeFile returns %d\n",tfs_writeFile(file1, bufferBlock, BLOCKSIZE));
	printf("After writing to 'test1', 'tinyFsDisk' looks like...\n");
	tfs_displayFragments();

    printf("\n----------------------------\n");
    file2 = tfs_openFile("test2");
    printf("tfs_openFile('test2') returns: %d\n", file2);
    printf("After opening a new file, 'tinyFsDisk' looks like...\n");
    printf("tfs_writeFile returns %d\n",tfs_writeFile(file2, bufferBlock2, BLOCKSIZE));
	printf("After writing to 'test2', 'tinyFsDisk' looks like...\n");
	tfs_displayFragments();

    printf("\n----------------------------\n");
    printf("View Dynamic Resource Table:\n");
    viewDRT();

    printf("\n----------------------------\n");
    printf("Let's delete test11...\n");
	printf("tfs_deleteFile(file1) returns: %d\n",tfs_deleteFile(file1));

	viewDRT();
	tfs_displayFragments();
    printf("\n----------------------------\n");

    printf("And now just to make sure let's read the first byte from that file...\n");
	printf("tfs_readByte returns: %d\n", tfs_readByte(file2, &read));
	printf("The byte we got back is %c\n", read);
    viewDRT();
    printf("\n----------------------------\n");
    return 0;
}
