#define SUPERBLOCK 1
#define INODE 2
#define FILEEXTENT 3
#define FREE 4
#define MAGICNUMBER 0x44

typedef struct freeblocks {
    int block;
    freeblocks *next;
} freeblocks;

typedef struct superblock {
    int magicNumber;
    int blockNumber;
    freeblocks *firstFreeBlock;
} superblock;

typedef struct inode {
    char *filename; //must support names up to 8 alphanumeric chars
    int filesize;
    freeblocks *inodeBlocks;
} inode;

