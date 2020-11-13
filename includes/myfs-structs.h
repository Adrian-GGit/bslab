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
    unsigned int end;               //beschreibt bei welchem Byte im letzten Block Datei endet
    unsigned int endBlock;          //beschreibt bei welchem Block Datei endet
    unsigned int userId;
    unsigned int groupId;

    mode_t mode;

    time_t a_time;
    time_t m_time;
    time_t c_time;

    char* data;                 //bleibt bei ondisk immer null
};

struct SDFR {
    struct mySuperblock {
        unsigned int mySuperblockindex;     //start von Superblock
        unsigned int myDMAPindex;           //start von DMAP
        unsigned int myFATindex;           //start von FAT
        unsigned int myRootindex;          //start von Root
        unsigned int myDATAindex;          //start von Data
    };
    mySuperblock superBlock;

    struct myDMAP {
        unsigned char freeBlocks[NUM_BLOCKS];   //0 is free, 1 is full
    };
    myDMAP dmap;

    struct myFAT {
        unsigned int FATTable[NUM_DIR_ENTRIES];     //kommt noch in VL
    };
    myFAT fat;

    struct myRoot {
        MyFsFileInfo fileInfos[NUM_DIR_ENTRIES];
    };
    myRoot root;

    inline size_t operator << (int i) {
        switch (i) {
            case 0:
                return sizeof(mySuperblock);
            case 1:
                return sizeof(myDMAP);
            case 2:
                return sizeof(myFAT);
            case 3:
                return sizeof(myRoot);
        }
    }

    inline void* operator >> (int i) {
        switch (i) {
            case 0:
                return &superBlock;
            case 1:
                return &dmap;
            case 2:
                return &fat;
            case 3:
                return &root;
        }
    }
};

#endif /* myfs_structs_h */
