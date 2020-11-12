//
//  myfs-structs.h
//  myfs
//
//  Created by Oliver Waldhorst on 07.09.17.
//  Copyright © 2017 Oliver Waldhorst. All rights reserved.
//

#ifndef myfs_structs_h
#define myfs_structs_h

#define NAME_LENGTH 255
#define BLOCK_SIZE 512
#define NUM_DIR_ENTRIES 64
#define NUM_OPEN_FILES 64

#define NUM_BLOCKS 100000       //TODO prüfe ob 100k reichen für 20MB data + SDFR Blöcke (Superblock, DMAP, FAT, Root)

#define NUM_SDFR 4

/*#define BLOCKSIZE_SUPERBLOCK 512
#define BLOCKSIZE_DMAP 4608
#define BLOCKSIZE_FAT 512
#define BLOCKSIZE_ROOT 19968*/


// TODO: Add structures of your file system here

struct MyFsFileInfo {
    char fileName[NAME_LENGTH];
    size_t dataSize = 0;            //beschreibt bei ondisk wo Bytes im Block aufhören
    unsigned int start = 0;         //beschreibt bei ondisk wo Bytes im Block beginnen
    unsigned int startBlock;        //beschreibt bei ondisk in welchem Block Datei startet
    unsigned int userId;
    unsigned int groupId;

    mode_t mode;

    time_t a_time;
    time_t m_time;
    time_t c_time;

    char* data;                 //bleibt bei ondisk immer null
};

struct mySuperblock {
    unsigned int mySuperblockindex;     //start von Superblock
    unsigned int myDMAPindex;           //start von DMAP
    unsigned int myFATindex;           //start von FAT
    unsigned int myRootindex;          //start von Root
    unsigned int myDATAindex;          //start von Data
};

struct myDMAP {
    unsigned char freeBlocks[NUM_BLOCKS];   //0 is free, 1 is full
};

struct myFAT {
    unsigned int FATTable[NUM_DIR_ENTRIES];     //kommt noch in VL
};

struct myRoot {
    MyFsFileInfo fileInfos[NUM_DIR_ENTRIES];
};

#endif /* myfs_structs_h */
