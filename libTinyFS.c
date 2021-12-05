#include <stdlib.h>
#include <string.h>
#include "libTinyFS.h"
#include "libDisk.h"
#include "blockTypes.h"

#define EXIT_FAILURE -1
#define SUCCESS 1

dynamicResourceTable *DRT_head = NULL;
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
    dynamicResourceTable *curr = DRT_head, *newNode;
    fileDescriptor fd;
    int i, exists = 0;
    char *data = malloc(BLOCKSIZE);

    //check if disk is mounted
    if (mounted_tinyfs) {
        //first check if the file exists in dynamic resource table
        while (curr != NULL) {
            if (strcmp(name, curr->filename) == 0){
                return curr->id;
            }
            curr = curr->next;
        }
        fd = openDisk(mounted_tinyfs, 0);
        // return fd;
    } else {
        return EXIT_FAILURE;
    }
    int b = DEFAULT_DISK_SIZE/BLOCKSIZE;
    for (i = 0; i < b; i++) {
        if (!exists) {
            if (readBlock(fd, i, data) < 0){
                return EXIT_FAILURE;
            }
            if (data[0] == INODE) {
                if(strcmp(name, data+4) == 0) {
                    exists = 1;
                    break;
                }
            }
        }
    }

    //if the file still doesn't exist
    if (!exists) {
        for (i = 0; i < b; i++) {
            if (readBlock(fd, i, data) < 0){
                return EXIT_FAILURE;
            }
            if(data[0] == FREE){
                break;
            }
        }
        data[0] = INODE;
        data[3] = SUPERBLOCK;
        strncpy(data+4, name, strlen(name));
        writeBlock(fd, i, data);

        //now add file to dynamic table
        if (DRT_head == NULL) {
            curr = calloc(1, sizeof(dynamicResourceTable));
            strcpy(curr->filename, name);
            curr->id = fd;
            curr->next = NULL;
            DRT_head = curr;
        } else {
            newNode = calloc(1, sizeof(dynamicResourceTable));
            //usze strdup?
            strcpy(newNode->filename, name);
            curr = DRT_head;

            while (curr->next != NULL) {
                curr = curr->next;
            }
            newNode->id = curr->id + 1;
            curr->next = newNode;
        }
        return curr->id;
    }
}

int tfs_closeFile(fileDescriptor FD) {
    //just needs to itereate and remove from linked list
    dynamicResourceTable *curr = DRT_head, *curr2;

    if (DRT_head == NULL) {
        return EXIT_FAILURE;
    }

    //check if its the head 
    if (curr->id == FD) {
        if (curr->next == NULL){
            DRT_head = NULL;
        } else {
            DRT_head = curr->next;
        }
    } else {
        curr2 = curr;
        curr = curr->next;
        while (curr != NULL) {
            if (curr->id == FD) {
                break;
            }
            curr = curr->next;
            curr2 = curr2->next;
        }
        if (curr == NULL) {
            return EXIT_FAILURE;
        }
        if (curr->next == NULL) {
            curr2->next = NULL;
        } else {
            curr2->next = curr->next;
        }
    }
    free(curr);
    return SUCCESS;
}

int tfs_writeFile(fileDescriptor FD,char *buffer, int size) {
    //first check if disk is mounted
    fileDescriptor fd;
    int b_num, b, exists = 0, i, inode, start, end;
    dynamicResourceTable *curr = DRT_head;
    char *filename, *data = malloc(BLOCKSIZE);
    
    if (!mounted_tinyfs) {
        return EXIT_FAILURE;
    } 
    fd = openDisk(mounted_tinyfs, 0);
    b_num = size/BLOCKSIZE;

    while (curr != NULL) {
        if (curr->id == FD) {
            break;
        }
        curr = curr->next;
    }
    if (curr == NULL) {
        return EXIT_FAILURE;
    }
    strcpy(filename, curr->filename);

    b = DEFAULT_DISK_SIZE/BLOCKSIZE;
    for (i = 0; i < b; i++) {
        if (!exists) {
            if (readBlock(fd, i, data) < 0){
                return EXIT_FAILURE;
            }
            if (data[0] == INODE) {
                if(strcmp(filename, data+4) == 0) {
                    exists = 1;
                    inode = i;
                    break;
                }
            }
        }
    }
    if (!exists){
        return EXIT_FAILURE;
    }
    for (start = 0; start < b; start++) {
        if (readBlock(fd, i, data) < 0){
            return EXIT_FAILURE;
        }
        if(data[0] == FREE) {
            for (end = start; end + b_num-1; end++) {
                if (readBlock(fd, end, data) < 0) {
                    return EXIT_FAILURE;
                }
                if (data[0] != FREE) {
                    exists = 0;
                }
            }
            if (exists) {
                break;
            }
        }
    }
    if (!exists){
        return EXIT_FAILURE;
    }
    exists = 0;
    for (i = start; i <= end; i++) {
        if (i != end) {
            data[2] = start + 1;
        } else {
            data[2] = 0;
        }
        strncpy(data+4, buffer, BLOCKSIZE-4);
        writeBlock(fd, i, data);
    }

    readBlock(fd, inode, data);
    data[2] = start;
    writeBlock(fd, inode, data);

    return SUCCESS;
}

int tfs_deleteFile(fileDescriptor FD){

}

int tfs_readByte(fileDescriptor FD, char *buffer) {

}

int tfs_seek(fileDescriptor FD, int offset) {

}
