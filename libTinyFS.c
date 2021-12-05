#include <stdlib.h>
#include <string.h>
#include "libTinyFS.h"
#include "libDisk.h"
#include "blockTypes.h"

 #define EXIT_FAILURE -1
#define SUCCESS 1
static char *mounted_tinyfs = NULL;

int tfs_mkfs(char *filename, int nBytes) {
    int o_disk, i;

    // define block -  one block
    char * blck = calloc(1, BLOCKSIZE);

    // Bytes 0 and 1 must be formatter as specified
    // blck[0] = [block type = 1|2|3|4]
    // blck[1] = 0x44
    // blck[2] = [address of another block]
    // blck[3] = [empty]
    // blck[4] = [data starts]
    // blck[5] = [second byte of data]
    // blck[6] = [third byte of data]
    // blck[7] = [fourth byte of data ?] | ...

    blck[0] = FREE; // free - ready for future writes
    blck[1] = MAGICNUMBER; // magic number

    o_disk = openDisk(filename, nBytes);
    //setting magic numbers
    for (i = 0; i < nBytes/BLOCKSIZE; i++) {
        writeBlock(o_disk, i, blck);
    }

    //initalize and writing the superblock and inodes
    freeblocks *head = malloc(sizeof (freeblocks));
    freeblocks *curr;
    //check if head malloc'd
    head->block = INODE;
    head->next = NULL;
    curr = head;

    for (i = 3; i < nBytes/BLOCKSIZE - 2; i++) {
        curr->next = malloc(sizeof(freeblocks));
        curr->next->block = i;
        curr->next->next = NULL;
        curr = curr->next;
    }

    superblock *super_block = malloc(sizeof(superblock));
    super_block->magicNumber = MAGICNUMBER;
    super_block->blockNumber = head->block;
    super_block->firstFreeBlock = head;

    blck[0] = SUPERBLOCK;
    blck[1] = MAGICNUMBER;
    strcpy(blck[2], &super_block); //or memcpy ??
    writeBlock(o_disk, 0, blck);

    inode *inodes = malloc(sizeof(inode));
    inodes->filename = "/";
    inodes->filesize = 1; //or maybe 0?

    blck[0] = INODE;
    blck[1] = MAGICNUMBER;
    strcpy(blck[2], &inodes); //or memcpy ??
    writeBlock(o_disk, 0, blck);

    return SUCCESS;
}

int tfs_mount(char *diskname) {
    char *data = malloc(BLOCKSIZE);
    int i, o_disk;
    
    //unmount whatever is mounted on the file system
    tfs_unmount();
	
	o_disk = openDisk(diskname, 0);
	readBlock(o_disk, 0, data);

	if (data[1] != MAGICNUMBER)   
		return EXIT_FAILURE;

	mounted_tinyfs = diskname;
	return SUCCESS;
}

int tfs_unmount(void) {
    if (mounted_tinyfs == NULL) {
        return EXIT_FAILURE;
    }
    mounted_tinyfs = NULL;
    return SUCCESS;
}

fileDescriptor tfs_openFile(char *name) {

}

int tfs_closeFile(fileDescriptor FD) {

}

int tfs_writeFile(fileDescriptor FD,char *buffer, int size) {

}

int tfs_deleteFile(fileDescriptor FD){

}

int tfs_readByte(fileDescriptor FD, char *buffer) {

}

int tfs_seek(fileDescriptor FD, int offset) {

}
