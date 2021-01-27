//
// Created by user on 27.01.21.
//

#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

#include "catch.hpp"
#include "tools.hpp"

#include "myondiskfs.h"

#define FILENAME "file"
#define SMALL_SIZE 1024
#define LARGE_SIZE 20*1024*1024

TEST_CASE("T-unittest1", "[Part_2]") {
    printf("Testcase unittest for enoughStorage\n");
    MyOnDiskFS *modfs = new MyOnDiskFS();
    SDFR *sdfr = modfs->sdfr;

    for (int i = 2; i < NUM_BLOCKS; i++) {
        sdfr->dmap->freeBlocks[i] = 1;
    }

    MyFsFileInfo* newData = &(sdfr->root->fileInfos[sdfr->superBlock->existingFiles]);
    strcpy(newData->fileName, "/file" + 1);
    newData->startBlock = 0;
    sdfr->dmap->freeBlocks[0] = 1;
    newData->numBlocks = 1;
    sdfr->superBlock->existingFiles += 1;
    REQUIRE(modfs->enoughStorage(0, 1025) == false);    //very first block 512 bytes left and one unalloced block (1) 512 bytes left but for 1 byte there is no storage left -> false
}

TEST_CASE("T-unittest2", "[Part_2]") {
    printf("Testcase unittest for getStartingBlock\n");
    MyOnDiskFS *modfs = new MyOnDiskFS();
    SDFR *sdfr = modfs->sdfr;

    for (int i = 0; i < 5; i++) {
        sdfr->fat->FATTable[i] = i + 1;
        if (i == 4)
            sdfr->fat->FATTable[i] = EOF;
    }

    REQUIRE(modfs->getStartingBlock(0, 3) == 3);
}

TEST_CASE("T-unittest3", "[Part_2]") {
    printf("Testcase unittest for fillFatAndDmap\n");
    MyOnDiskFS *modfs = new MyOnDiskFS();
    SDFR *sdfr = modfs->sdfr;

    for (int i = 0; i < 10; i++) {
        sdfr->dmap->freeBlocks[i] = 0;
        sdfr->fat->FATTable[i] = EOF;
    }

    modfs->setIndexes();

    int blocks[] = {4, 5, 6, 7};
    modfs->fillFatAndDmap(blocks, 4, true);

    for (int i = 0; i < 10; i++) {
        if (i == 4 || i == 5 || i == 6 || i == 7) {
            if (i == 7) {
                REQUIRE(sdfr->fat->FATTable[i] == EOF);
            } else{
                REQUIRE(sdfr->fat->FATTable[i] == i + 1);
            }
            REQUIRE(sdfr->dmap->freeBlocks[i] == 1);
        } else{
            REQUIRE(sdfr->fat->FATTable[i] == EOF);
            REQUIRE(sdfr->dmap->freeBlocks[i] == 0);
        }
    }
}

TEST_CASE("T-unittest4", "[Part_2]") {
    printf("Testcase unittest for findNextFreeBlock\n");
    MyOnDiskFS *modfs = new MyOnDiskFS();
    SDFR *sdfr = modfs->sdfr;

    for (int i = 0; i < NUM_BLOCKS; i++) {
        sdfr->dmap->freeBlocks[i] = 0;
    }

    for (int i = 0; i < 5; i++) {
        sdfr->dmap->freeBlocks[i] = 1;
    }

    REQUIRE(modfs->findNextFreeBlock() == 5);

    for (int i = 0; i < NUM_BLOCKS; i++) {
        sdfr->dmap->freeBlocks[i] = 1;
    }

    REQUIRE(modfs->findNextFreeBlock() == -ENOMEM);
}

TEST_CASE("T-unittest5", "[Part_2]") {
    printf("Testcase unittest for searchForFile\n");
    MyOnDiskFS *modfs = new MyOnDiskFS();
    SDFR *sdfr = modfs->sdfr;

    strcpy(sdfr->root->fileInfos[0].fileName, "/file0");
    sdfr->superBlock->existingFiles++;
    strcpy(sdfr->root->fileInfos[0].fileName, "/file1");
    sdfr->superBlock->existingFiles++;
    strcpy(sdfr->root->fileInfos[0].fileName, "/file2");
    sdfr->superBlock->existingFiles++;
    strcpy(sdfr->root->fileInfos[0].fileName, "/file3");
    sdfr->superBlock->existingFiles++;

    modfs->searchForFile("file2");
}

TEST_CASE("T-unittest6", "[Part_2]") {
    printf("Testcase unittest for buildStructure\n");
    MyOnDiskFS *modfs = new MyOnDiskFS();
    SDFR *sdfr = modfs->sdfr;

    modfs->setIndexes();

    modfs->buildStructure();

    for (int i = 0; i < NUM_BLOCKS; i++) {
        if (i < sdfr->superBlock->myDATAindex) {
            if (i == sdfr->superBlock->myDATAindex - 1
             || i == sdfr->superBlock->myDMAPindex - 1
             || i == sdfr->superBlock->myFATindex - 1
             || i == sdfr->superBlock->myRootindex - 1) {
                REQUIRE(sdfr->fat->FATTable[i] == EOF);
            } else{
                REQUIRE(sdfr->fat->FATTable[i] == i + 1);
            }
            REQUIRE(sdfr->dmap->freeBlocks[i] = 1);
        } else{
            REQUIRE(sdfr->fat->FATTable[i] == EOF);
            REQUIRE(sdfr->dmap->freeBlocks[i] == 0);
        }
    }
}

