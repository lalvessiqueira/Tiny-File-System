#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libDisk.h"
#include "libTinyFS.h"
#include "blockTypes.h"
#include "tinyFS_errno.h"

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

    if((o_disk = openDisk(filename, nBytes) < 0)) {
        return ERROPENDISK;
    }
    
    //setting magic numbers
    for (i = 0; i < nBytes/BLOCKSIZE; i++) {
        writeBlock(o_disk, i, blck);
    }
    // printf("1\n");

    blck[0] = SUPERBLOCK;
    blck[1] = MAGICNUMBER;
    blck[2] = SUPERBLOCK;
    if (writeBlock(o_disk, 0, blck) < 0) {
        return ERRWRITEBLCK;
    }
    // printf("2\n");

    blck[0] = INODE;
    blck[1] = MAGICNUMBER;
    blck[2] = INODE;
    if (writeBlock(o_disk, 0, blck) < 0) {
        return ERRWRITEBLCK;
    }
    printf("(tfs_mkfs) Checking data...\n");
    for (i = 0; i < 5; i++) {
        printf("data[%d]: %d\n", i, blck[i]);
    }
    return SUCCESS;
}

int tfs_mount(char *diskname) {
    char data[BLOCKSIZE];
    int o_disk;
    
    //unmount whatever is mounted on the file system
    printf("Unmounting...\n");
    tfs_unmount();
    printf("Unmounted. Waiting for new mounting...\n");
	
    printf("Opening disk...\n");
	if((o_disk = openDisk(diskname, 0) < 0)) {
        return ERROPENDISK;
    }
    
    printf("\nReading block...\no_disk = %d\ndata = %s\n\n",o_disk, data);
	readBlock(o_disk, 0, data);

    printf("(tfs_mount) Checking data...\n");
    for (int i = 0; i < 5; i++) {
        printf("data[%d]: %d\n", i, data[i]);
    }

    printf("I have to strcpy here! DUH!\n");
	mounted_tinyfs = diskname;
    printf("File system mounted: %s\n", mounted_tinyfs);
	return SUCCESS;
}

int tfs_unmount(void) {
    if (mounted_tinyfs == NULL) {
        return NOMOUNTEDFILE;
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
        if ((fd = openDisk(mounted_tinyfs, 0)) < 0) {
            return ERRREADBLCK;
        }
        // return fd;
    } else {
        return NOMOUNTEDFILE;
    }
    int b = DEFAULT_DISK_SIZE/BLOCKSIZE;
    for (i = 0; i < b; i++) {
        if (!exists) {
            if (readBlock(fd, i, data) < 0){
                return ERRFSCORRUPT;
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
                return ERRFSCORRUPT;
            }
            if(data[0] == FREE){
                break;
            }
        }
        data[0] = INODE;
        data[3] = SUPERBLOCK;
        strncpy(data+4, name, strlen(name));
        if (writeBlock(fd, i, data) < 0) {
            return ERRWRITEBLCK;
        }

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
    }
    return curr->id;
}

int tfs_closeFile(fileDescriptor FD) {
    //just needs to itereate and remove from linked list
    dynamicResourceTable *curr = DRT_head, *curr2;

    if (DRT_head == NULL) {
        return EXITFAILURE;
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
            return NOFILETOCLOSE;
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
    char *filename = NULL, *data = malloc(BLOCKSIZE);
    
    if (!mounted_tinyfs) {
        return NOMOUNTEDFILE;
    } 
    if ((fd = openDisk(mounted_tinyfs, 0)) < 0) {
        return ERRREADBLCK;
    }
    b_num = size/BLOCKSIZE;

    while (curr != NULL) {
        if (curr->id == FD) {
            break;
        }
        curr = curr->next;
    }
    if (curr == NULL) {
        return ERRFSCORRUPT;
    }
    strcpy(filename, curr->filename);

    b = DEFAULT_DISK_SIZE/BLOCKSIZE;
    for (i = 0; i < b; i++) {
        if (!exists) {
            if (readBlock(fd, i, data) < 0){
                return EXITFAILURE;
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
        return ERRFSCORRUPT;
    }
    for (start = 0; start < b; start++) {
        if (readBlock(fd, i, data) < 0){
            return ERRREADBLCK;
        }
        if(data[0] == FREE) {
            for (end = start; end + b_num-1; end++) {
                if (readBlock(fd, end, data) < 0) {
                    return ERRREADBLCK;
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
        return ERRFSCORRUPT;
    }
    exists = 0;
    data[0] = FILEEXTENT;
    data[1] = MAGICNUMBER;
    data[3] = inode;

    for (i = start; i <= end; i++) {
        if (i != end) {
            data[2] = start + 1;
        } else {
            data[2] = 0;
        }
        strncpy(data+4, buffer, BLOCKSIZE-4);
        if (writeBlock(fd, i, data) < 0){
            return ERRWRITEBLCK;
        }
    }

    if (readBlock(fd, inode, data) < 0) {
        return ERRREADBLCK;
    }
    
    data[2] = start;
    if (writeBlock(fd, inode, data) < 0){
        return ERRWRITEBLCK;
    }
    curr->filePointer = 0;

    printf("(tfs_writeFile) Checking data...\n");
    for (i = 0; i < 5; i++) {
        printf("data[%d]: %d\n", i, data[i]);
    }

    return SUCCESS;
}

int tfs_deleteFile(fileDescriptor FD){
    fileDescriptor fd;
    int b, i, exists = 0, saveBlock;
    dynamicResourceTable *curr = DRT_head;
    char *filename = NULL, *data = malloc(BLOCKSIZE);

    if (!mounted_tinyfs) {
        return EXITFAILURE;
    } 
    if((fd = openDisk(mounted_tinyfs, 0)) < 0) {
        return ERROPENDISK;
    }

    while (curr != NULL) {
        if (curr->id == FD) {
            break;
        }
        curr = curr->next;
    }
    if (curr == NULL) {
        return ERRFSCORRUPT;
    }
    strcpy(filename, curr->filename);

    b = DEFAULT_DISK_SIZE/BLOCKSIZE;
    for (i = 0; i < b; i++) {
        if (!exists) {
            if (readBlock(fd, i, data) < 0){
                return ERRREADBLCK;
            }
            if (data[0] == INODE) {
                if(strcmp(filename, data+4) == 0) {
                    if (data[3] == 0){
                        return ERRNOTEMPTY;
                    }
                    exists = 1;
                    saveBlock = data[2];
                    break;
                }
            }
        }
    }
    if (!exists){
        return EXITFAILURE;
    }
    //mark its blocks as free on disk
    data[0] = FREE;

    if (writeBlock(fd, i, data) < 0){
        return ERRWRITEBLCK;
    }
    // for (i = saveBlock; i < save)
    tfs_closeFile(FD);
    return SUCCESS;
}

int tfs_readByte(fileDescriptor FD, char *buffer) {
    fileDescriptor fd;
    int b, i, exists = 0, current_block, saveBlock, file_pointer;
    dynamicResourceTable *curr = DRT_head;
    char *filename = NULL, *data = malloc(BLOCKSIZE);

    if (!mounted_tinyfs) {
        return NOMOUNTEDFILE;
    } 
    if( (fd = openDisk(mounted_tinyfs, 0)) < 0) {
        return ERRREADBLCK;
    }

    while (curr != NULL) {
        if (curr->id == FD) {
            break;
        }
        curr = curr->next;
    }
    if (curr == NULL) {
        return EXITFAILURE;
    }
    strcpy(filename, curr->filename);

    b = DEFAULT_DISK_SIZE/BLOCKSIZE;
    for (i = 0; i < b; i++) {
        if (!exists) {
            if (readBlock(fd, i, data) < 0){
                return ERRREADBLCK;
            }
            if (data[0] == INODE) {
                if(strcmp(filename, data+4) == 0) {
                    exists = 1;
                    if (writeBlock(fd, i, data) < 0){
                        return ERRWRITEBLCK;
                    }
                    saveBlock = data[2];
                    break;
                }
            }
        }
    }
    if (!exists){
        return EXITFAILURE;
    }
    current_block = curr->filePointer + 1/ BLOCKSIZE;
    file_pointer = curr->filePointer - (BLOCKSIZE * current_block);

    if (readBlock(fd, current_block + saveBlock, data) < 0) {
        return ERRREADBLCK;
    }
    
    if (data[0] != FILEEXTENT){
        return ERRFSCORRUPT;
    }
    *buffer = data[file_pointer+4];
    curr->filePointer++;
    return SUCCESS;
}

int tfs_seek(fileDescriptor FD, int offset) {
    fileDescriptor fd;
    int b, i, exists = 0;
    dynamicResourceTable *curr = DRT_head;
    char *filename = NULL, *data = malloc(BLOCKSIZE);

    if (!mounted_tinyfs) {
        return EXITFAILURE;
    } 
    if((fd = openDisk(mounted_tinyfs, 0)) < 0){
        return ERRREADBLCK;
    }

    while (curr != NULL) {
        if (curr->id == FD) {
            break;
        }
        curr = curr->next;
    }
    if (curr == NULL) {
        return ERRFSCORRUPT;
    }
    curr->filePointer = offset;
    strcpy(filename, curr->filename);

    b = DEFAULT_DISK_SIZE/BLOCKSIZE;
    for (i = 0; i < b; i++) {
        if (!exists) {
            if (readBlock(fd, i, data) < 0){
                return ERRREADBLCK;
            }
            if (data[0] == INODE) {
                if(strcmp(filename, data+4) == 0) {
                    exists = 1;
                    if (writeBlock(fd, i, data) < 0){
                        return ERRWRITEBLCK;
                    }
                    break;
                }
            }
        }
    }
    return SUCCESS;
}
