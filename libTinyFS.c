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
    int o_disk, i, w_disk;

    // define block -  one block
    char *data = calloc(1, BLOCKSIZE);

    // Bytes 0 and 1 must be formatter as specified
    // blck[0] = [block type = 1|2|3|4]
    // blck[1] = 0x44
    // blck[2] = [address of another block]
    // blck[3] = [empty]
    // blck[4] = [data starts]
    // blck[5] = [second byte of data]
    // blck[6] = [third byte of data]
    // blck[7] = [fourth byte of data ?] | ...

    data[0] = FREE; // free - ready for future writes
    data[1] = MAGICNUMBER; // magic number

    o_disk = openDisk(filename, nBytes);
    if (o_disk < 0) {
        return ERROPENDISK;
    }
    
    //setting magic numbers
    for (i = 0; i < nBytes/BLOCKSIZE; i++) {
        w_disk = writeBlock(o_disk, i, data);
        if (w_disk < 0) {
            return ERRWRITEBLCK;
        }
    }

    data[0] = SUPERBLOCK;
    data[1] = MAGICNUMBER;
    data[2] = SUPERBLOCK;

    w_disk = writeBlock(o_disk, 0, data);
    if (w_disk < 0) {
        return ERRWRITEBLCK;
    }

    // blck[0] = INODE;
    // blck[1] = MAGICNUMBER;
    // blck[2] = INODE;
    // if (writeBlock(o_disk, 0, blck) < 0) {
    //     return ERRWRITEBLCK;
    // }
    printf("3. (tfs_mkfs) Checking data...\n");
    for (i = 0; i < 5; i++) {
        printf("data[%d]: %d\n", i, data[i]);
    }
    return SUCCESS;
}

int tfs_mount(char *diskname) {
    char data[BLOCKSIZE];
    int o_disk, r_disk;
    
    //unmount whatever is mounted on the file system
    tfs_unmount();
    printf("Unmounted. Waiting for new mounting...\n");
	
    printf("Opening disk...\n");
    o_disk = openDisk(diskname, 0);
	if(o_disk < 0) {
        return ERROPENDISK;
    }
    
    printf("\nReading block...\n");
	r_disk = readBlock(o_disk, 0, data);
    if (r_disk < 0) {
        return ERRREADBLCK;
    }

    if (data[1] != MAGICNUMBER) {
        printf("Error: Magic Number not here!\n");
        //give an error
    }

    printf("(tfs_mount) Checking data...\n");
    for (int i = 0; i < 5; i++) {
        printf("data[%d]: %d\n", i, data[i]);
    }

	mounted_tinyfs = diskname;
    printf("File system mounted: %s\n", mounted_tinyfs);
	return SUCCESS;
}

int tfs_unmount(void) {
    printf("Unmounting...\n");
    if (mounted_tinyfs == NULL) {
        return NOMOUNTEDFILE;
    }
    mounted_tinyfs = NULL;
    return SUCCESS;
}

fileDescriptor tfs_openFile(char *name) {
    dynamicResourceTable *curr = DRT_head, *newNode;
    fileDescriptor o_disk, r_disk, w_disk;
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
        o_disk = openDisk(mounted_tinyfs, 0);
        if (o_disk < 0) {
            fprintf(stderr, "1. Error opening disk\n");
            return ERROPENDISK;
        }
        // return fd;
    } else {
        return NOMOUNTEDFILE;
    }
    int b = DEFAULT_DISK_SIZE/BLOCKSIZE;
    for (i = 0; i < b; i++) {
        if (!exists) {
            // printf("Mounted Disk: %s\n", mounted_tinyfs);
            // printf("o_disk: %d\n", o_disk);
            r_disk = readBlock(o_disk, i, data);
            if (r_disk < 0){
                fprintf(stderr, "1. Error reading disk\n");
                return ERRREADBLCK;
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
            r_disk = readBlock(o_disk, i, data);
            if (r_disk < 0){
                fprintf(stderr, "2. Error reading disk\n");
                return ERRREADBLCK;
            }
            if(data[0] == FREE){
                break;
            }
        }
        data[0] = INODE;
        data[3] = SUPERBLOCK;
        strncpy(data+4, name, strlen(name));
        w_disk = writeBlock(o_disk, i, data);
        if (w_disk < 0) {
            fprintf(stderr, "1. Error writing to disk\n");
            return ERRWRITEBLCK;
        }

        //now add file to dynamic table
        if (DRT_head == NULL) {
            curr = calloc(1, sizeof(dynamicResourceTable));
            printf("%lu\n", strlen(name));
            curr->filename = (char*)malloc(sizeof(char) * strlen(name) + 1);
            strcpy(curr->filename, name);
            curr->id = o_disk;
            curr->next = NULL;
            DRT_head = curr;
        } else {
            newNode = calloc(1, sizeof(dynamicResourceTable));
            newNode->filename = (char*)malloc(sizeof(char) * strlen(name) + 1);
            strcpy(newNode->filename, name);
            curr = DRT_head;

            while (curr->next != NULL) {
                curr = curr->next;
            }
            newNode->id = curr->id;
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
    fileDescriptor o_disk, r_disk, w_disk;
    int b_num = size/BLOCKSIZE - 1, b, exists = 0, i, inode, start, end;
    dynamicResourceTable *curr = DRT_head;
    char *filename = NULL, data[BLOCKSIZE];
    
    if (!mounted_tinyfs) {
        return NOMOUNTEDFILE;
    } 

    o_disk = openDisk(mounted_tinyfs, 0); 
    if (o_disk < 0) {
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

    filename = (char*)malloc(sizeof(char) * strlen(curr->filename) + 1);
    strcpy(filename, curr->filename);

    b = DEFAULT_DISK_SIZE/BLOCKSIZE;
    for (i = 0; i < b; i++) {
        if (!exists) {
            r_disk = readBlock(o_disk, i, data);
            if (r_disk < 0){
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
        r_disk = readBlock(o_disk, start, data);
        if (r_disk < 0){
            return ERRREADBLCK;
        }
        if(data[0] == FREE) {
            printf("---IN---\n");
            for (end = start; end < start + b_num; end++) {
                r_disk = readBlock(o_disk, end, data);
                if (r_disk < 0){
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
    printf("start: %d\nend: %d\n", start, end);
    if (!exists){
        return ERRFSCORRUPT;
    }
    exists = 0;
    data[0] = FILEEXTENT;
    data[1] = MAGICNUMBER;
    data[3] = inode;

    printf("-----\nBefore loop\n");
	printf("start: %d\nend: %d\n", start, end);

    for (i = start; i <= end; i++) {
        if (i != end) {
            data[2] = start + 1;
            printf("1. data[2]: %d\n", data[2]);
        } else {
            data[2] = 0;
            printf("2. data[2]: %d\n", data[2]);
        }
        strncpy(data+4, buffer, BLOCKSIZE-4);
        w_disk = writeBlock(o_disk, i, data);
        if (w_disk < 0){
            return ERRWRITEBLCK;
        }
    }

    r_disk = readBlock(o_disk, inode, data);
    if (r_disk < 0) {
        return ERRREADBLCK;
    }
    
    data[2] = start;
    w_disk = writeBlock(o_disk, inode, data);
    if (w_disk < 0){
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
    fileDescriptor o_disk, r_disk, w_disk;
    int b, i, exists = 0;
    dynamicResourceTable *curr = DRT_head;
    char *filename = NULL, *data = malloc(BLOCKSIZE);

    if (!mounted_tinyfs) {
        return NOMOUNTEDFILE;
    } 

    o_disk = openDisk(mounted_tinyfs, 0);
    if(o_disk < 0) {
        return ERROPENDISK;
    }

    if (curr == NULL) {
        fprintf(stderr, "No file to be deleted!\n");
        return EMPTYDIRECTORY;
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
    
    filename = (char*)malloc(sizeof(char) * strlen(curr->filename) + 1);
    strcpy(filename, curr->filename);

    b = DEFAULT_DISK_SIZE/BLOCKSIZE;
    for (i = 0; i < b; i++) {
        if (!exists) {
            r_disk = readBlock(o_disk, i, data);
            if (r_disk < 0){
                return ERRREADBLCK;
            }
            if (data[0] == INODE) {
                if(strcmp(filename, data+4) == 0) {
                    exists = 1;
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

    w_disk = writeBlock(o_disk, i, data);
    if (w_disk < 0){
        return ERRWRITEBLCK;
    }

    tfs_closeFile(FD);
    printf("(tfs_deleteFile) Checking data...\n");
    for (i = 0; i < 5; i++) {
        printf("data[%d]: %d\n", i, data[i]);
    }
    return SUCCESS;
}

int tfs_readByte(fileDescriptor FD, char *buffer) {
    fileDescriptor o_disk, r_disk, w_disk;
    int b = DEFAULT_DISK_SIZE/BLOCKSIZE, i, exists = 0, current_block, saveBlock, file_pointer;
    dynamicResourceTable *curr = DRT_head;
    char *filename = NULL, data[BLOCKSIZE];

    if (mounted_tinyfs == NULL) {
        return NOMOUNTEDFILE;
    } 

    o_disk = openDisk(mounted_tinyfs, 0);
    if(o_disk < 0) {
        return ERRREADBLCK;
    }

    if (curr == NULL) {
        return EMPTYDIRECTORY;
    }

    printf("The FD to read is: %d\n", FD);
    while (curr != NULL) {
        printf("filename: %s\nid: %d\n----\n", curr->filename, curr->id);
        if (curr->id == FD) {
            break;
        }
        curr = curr->next;
    }
    if (curr == NULL) {
        fprintf(stderr, "File not found!\n");
        return EXITFAILURE;
    }

    filename = (char*)malloc(sizeof(char) * strlen(curr->filename) + 1);
    strcpy(filename, curr->filename);

    for (i = 0; i < b; i++) {
        if (!exists) {
            r_disk = readBlock(o_disk, i, data);
            if (r_disk < 0){
                return ERRREADBLCK;
            }
            if (data[0] == INODE) {
                if(strcmp(filename, data+4) == 0) {
                    exists = 1;
                    w_disk = writeBlock(o_disk, i, data);
                    if (w_disk < 0) {
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

    r_disk = readBlock(o_disk, current_block + saveBlock, data);
    if (r_disk < 0) {
        return ERRREADBLCK;
    }
    
    if (data[0] != FILEEXTENT){
        printf("(tfs_readByte) Checking data...\n");
        for (i = 0; i < 5; i++) {
            printf("data[%d]: %d\n", i, data[i]);
        }
        return data[0];
    }
    *buffer = data[file_pointer+4];
    curr->filePointer++;
    
    printf("(tfs_readByte) Checking data...\n");
    for (i = 0; i < 5; i++) {
        printf("data[%d]: %d\n", i, data[i]);
    }
    return SUCCESS;
}

int tfs_seek(fileDescriptor FD, int offset) {
    fileDescriptor o_disk, r_disk, w_disk;
    int b, i, exists = 0;
    dynamicResourceTable *curr = DRT_head;
    char *filename = NULL, *data = malloc(BLOCKSIZE);

    if (!mounted_tinyfs) {
        return EXITFAILURE;
    } 

    o_disk = openDisk(mounted_tinyfs, 0);
    if(o_disk < 0){
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
    filename = (char*)malloc(sizeof(char) * strlen(curr->filename) + 1);
    strcpy(filename, curr->filename);

    b = DEFAULT_DISK_SIZE/BLOCKSIZE;
    for (i = 0; i < b; i++) {
        if (!exists) {
            r_disk = readBlock(o_disk, i, data);
            if (r_disk < 0){
                return ERRREADBLCK;
            }
            if (data[0] == INODE) {
                if(strcmp(filename, data+4) == 0) {
                    exists = 1;
                    w_disk = writeBlock(o_disk, i, data);
                    if (w_disk < 0){
                        return ERRWRITEBLCK;
                    }
                    break;
                }
            }
        }
    }
    return SUCCESS;
}

void viewDRT() {
    dynamicResourceTable *curr = DRT_head;
    printf("\n");
    while (curr != NULL) {
        printf("filename: %s\nid: %d\n", curr->filename, curr->id);
        curr = curr->next;
    }
    printf("\n");
}
/** TODO: REMOVE THIS! */

int tfs_displayFragments() {
    printf("Displaying fragments...\n");

	int i, o_disk, count = 0, r_disk;
	char buff[BLOCKSIZE];

	if(mounted_tinyfs) 
		o_disk = openDisk(mounted_tinyfs, 0);
	
    else
		return ERRNOTEMPTY;
	for(i = 0; i < DEFAULT_DISK_SIZE / BLOCKSIZE; i++){

        r_disk = readBlock(o_disk, i, buff);
		if(r_disk < 0) {
			return ERRNOTEMPTY;
        }

		if(buff[0] == 1){
			printf("|S|");
			count++;
		}
		else if(buff[0] == 2){
			printf("|I|");
			count++;
		}
		else if(buff[0] == 3){
			printf("|D|");
			count++;
		}
		else if(buff[0] == 4){
			printf("| |");
			count++;
		}
		if(count == 4 || i == DEFAULT_DISK_SIZE / BLOCKSIZE - 1) {
			printf("\n");
			count = 0;
		}
		else
			printf(" -> ");
	}
	printf("\n");
	return 1;
}