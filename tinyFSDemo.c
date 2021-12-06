#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libTinyFS.h"
#include "tinyFS_errno.h"

int main(void) {
    printf("tfs_mkfs testing:\n");
    printf("\nMaking a blanks TinyFS\nReturns: %d\n",tfs_mkfs(DEFAULT_DISK_NAME, DEFAULT_DISK_SIZE));
	
    printf("Unmounting...\n");
    printf("No file system to unmount!\nReturns: %d\n", tfs_unmount());

    printf("Mouting a tiny file system...\n");
    printf("File System mounted!\nReturns: %d", tfs_mount(DEFAULT_DISK_NAME));
    return 0;
}
