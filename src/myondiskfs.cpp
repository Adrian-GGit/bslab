//
// Created by Oliver Waldhorst on 20.03.20.
// Copyright © 2017-2020 Oliver Waldhorst. All rights reserved.
//

#include "myondiskfs.h"

// For documentation of FUSE methods see https://libfuse.github.io/doxygen/structfuse__operations.html

#undef DEBUG

// TODO: Comment lines to reduce debug messages
#define DEBUG
#define DEBUG_METHODS
#define DEBUG_RETURN_VALUES

#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "macros.h"
#include "myfs.h"
#include "myfs-info.h"
#include "blockdevice.h"

/// @brief Constructor of the on-disk file system class.
///
/// You may add your own constructor code here.
MyOnDiskFS::MyOnDiskFS() : MyFS() {
    // create a block device object
    this->blockDevice= new BlockDevice(BLOCK_SIZE);

    //alle Blöcke sind noch frei
    for (int i = 0; i < NUM_BLOCKS; i++) {
        sdfr->dmap->freeBlocks[i] = '0';
    }

}

/// @brief Destructor of the on-disk file system class.
///
/// You may add your own destructor code here.
MyOnDiskFS::~MyOnDiskFS() {
    // free block device object
    delete this->blockDevice;

    // TODO: [PART 2] Add your cleanup code here

}

/// @brief Create a new file.
///
/// Create a new file with given name and permissions.
/// You do not have to check file permissions, but can assume that it is always ok to access the file.
/// \param [in] path Name of the file, starting with "/".
/// \param [in] mode Permissions for file access.
/// \param [in] dev Can be ignored.
/// \return 0 on success, -ERRNO on failure.
int MyOnDiskFS::fuseMknod(const char *path, mode_t mode, dev_t dev) {
    LOGM();

    // TODO: [PART 2] Implement this!

    RETURN(0);
}

/// @brief Delete a file.
///
/// Delete a file with given name from the file system.
/// You do not have to check file permissions, but can assume that it is always ok to access the file.
/// \param [in] path Name of the file, starting with "/".
/// \return 0 on success, -ERRNO on failure.
int MyOnDiskFS::fuseUnlink(const char *path) {
    LOGM();

    // TODO: [PART 2] Implement this!

    RETURN(0);
}

/// @brief Rename a file.
///
/// Rename the file with with a given name to a new name.
/// Note that if a file with the new name already exists it is replaced (i.e., removed
/// before renaming the file.
/// You do not have to check file permissions, but can assume that it is always ok to access the file.
/// \param [in] path Name of the file, starting with "/".
/// \param [in] newpath  New name of the file, starting with "/".
/// \return 0 on success, -ERRNO on failure.
int MyOnDiskFS::fuseRename(const char *path, const char *newpath) {
    LOGM();

    // TODO: [PART 2] Implement this!

    RETURN(0);
}

/// @brief Get file meta data.
///
/// Get the metadata of a file (user & group id, modification times, permissions, ...).
/// \param [in] path Name of the file, starting with "/".
/// \param [out] statbuf Structure containing the meta data, for details type "man 2 stat" in a terminal.
/// \return 0 on success, -ERRNO on failure.
int MyOnDiskFS::fuseGetattr(const char *path, struct stat *statbuf) {
    LOGM();

    // TODO: [PART 2] Implement this!

    RETURN(0);
}

/// @brief Change file permissions.
///
/// Set new permissions for a file.
/// You do not have to check file permissions, but can assume that it is always ok to access the file.
/// \param [in] path Name of the file, starting with "/".
/// \param [in] mode New mode of the file.
/// \return 0 on success, -ERRNO on failure.
int MyOnDiskFS::fuseChmod(const char *path, mode_t mode) {
    LOGM();

    // TODO: [PART 2] Implement this!

    RETURN(0);
}

/// @brief Change the owner of a file.
///
/// Change the user and group identifier in the meta data of a file.
/// You do not have to check file permissions, but can assume that it is always ok to access the file.
/// \param [in] path Name of the file, starting with "/".
/// \param [in] uid New user id.
/// \param [in] gid New group id.
/// \return 0 on success, -ERRNO on failure.
int MyOnDiskFS::fuseChown(const char *path, uid_t uid, gid_t gid) {
    LOGM();

    // TODO: [PART 2] Implement this!

    RETURN(0);
}

/// @brief Open a file.
///
/// Open a file for reading or writing. This includes checking the permissions of the current user and incrementing the
/// open file count.
/// You do not have to check file permissions, but can assume that it is always ok to access the file.
/// \param [in] path Name of the file, starting with "/".
/// \param [out] fileInfo Can be ignored in Part 1
/// \return 0 on success, -ERRNO on failure.
int MyOnDiskFS::fuseOpen(const char *path, struct fuse_file_info *fileInfo) {
    LOGM();

    // TODO: [PART 2] Implement this!

    RETURN(0);
}

/// @brief Read from a file.
///
/// Read a given number of bytes from a file starting form a given position.
/// You do not have to check file permissions, but can assume that it is always ok to access the file.
/// Note that the file content is an array of bytes, not a string. I.e., it is not (!) necessarily terminated by '\0'
/// and may contain an arbitrary number of '\0'at any position. Thus, you should not use strlen(), strcpy(), strcmp(),
/// ... on both the file content and buf, but explicitly store the length of the file and all buffers somewhere and use
/// memcpy(), memcmp(), ... to process the content.
/// \param [in] path Name of the file, starting with "/".
/// \param [out] buf The data read from the file is stored in this array. You can assume that the size of buffer is at
/// least 'size'
/// \param [in] size Number of bytes to read
/// \param [in] offset Starting position in the file, i.e., number of the first byte to read relative to the first byte of
/// the file
/// \param [in] fileInfo Can be ignored in Part 1
/// \return The Number of bytes read on success. This may be less than size if the file does not contain sufficient bytes.
/// -ERRNO on failure.
int MyOnDiskFS::fuseRead(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fileInfo) {
    LOGM();

    // TODO: [PART 2] Implement this!

    RETURN(0);
}

/// @brief Write to a file.
///
/// Write a given number of bytes to a file starting at a given position.
/// You do not have to check file permissions, but can assume that it is always ok to access the file.
/// Note that the file content is an array of bytes, not a string. I.e., it is not (!) necessarily terminated by '\0'
/// and may contain an arbitrary number of '\0'at any position. Thus, you should not use strlen(), strcpy(), strcmp(),
/// ... on both the file content and buf, but explicitly store the length of the file and all buffers somewhere and use
/// memcpy(), memcmp(), ... to process the content.
/// \param [in] path Name of the file, starting with "/".
/// \param [in] buf An array containing the bytes that should be written.
/// \param [in] size Number of bytes to write.
/// \param [in] offset Starting position in the file, i.e., number of the first byte to read relative to the first byte of
/// the file.
/// \param [in] fileInfo Can be ignored in Part 1 .
/// \return Number of bytes written on success, -ERRNO on failure.
int MyOnDiskFS::fuseWrite(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fileInfo) {
    LOGM();

    // TODO: [PART 2] Implement this!

    RETURN(0);
}

/// @brief Close a file.
///
/// \param [in] path Name of the file, starting with "/".
/// \param [in] File handel for the file set by fuseOpen.
/// \return 0 on success, -ERRNO on failure.
int MyOnDiskFS::fuseRelease(const char *path, struct fuse_file_info *fileInfo) {
    LOGM();

    // TODO: [PART 2] Implement this!

    RETURN(0);
}

/// @brief Truncate a file.
///
/// Set the size of a file to the new size. If the new size is smaller than the old size, spare bytes are removed. If
/// the new size is larger than the old size, the new bytes may be random.
/// You do not have to check file permissions, but can assume that it is always ok to access the file.
/// \param [in] path Name of the file, starting with "/".
/// \param [in] newSize New size of the file.
/// \return 0 on success, -ERRNO on failure.
int MyOnDiskFS::fuseTruncate(const char *path, off_t newSize) {
    LOGM();

    // TODO: [PART 2] Implement this!

    RETURN(0);
}

/// @brief Truncate a file.
///
/// Set the size of a file to the new size. If the new size is smaller than the old size, spare bytes are removed. If
/// the new size is larger than the old size, the new bytes may be random. This function is called for files that are
/// open.
/// You do not have to check file permissions, but can assume that it is always ok to access the file.
/// \param [in] path Name of the file, starting with "/".
/// \param [in] newSize New size of the file.
/// \param [in] fileInfo Can be ignored in Part 1.
/// \return 0 on success, -ERRNO on failure.
int MyOnDiskFS::fuseTruncate(const char *path, off_t newSize, struct fuse_file_info *fileInfo) {
    LOGM();

    // TODO: [PART 2] Implement this!

    RETURN(0);
}

/// @brief Read a directory.
///
/// Read the content of the (only) directory.
/// You do not have to check file permissions, but can assume that it is always ok to access the directory.
/// \param [in] path Path of the directory. Should be "/" in our case.
/// \param [out] buf A buffer for storing the directory entries.
/// \param [in] filler A function for putting entries into the buffer.
/// \param [in] offset Can be ignored.
/// \param [in] fileInfo Can be ignored.
/// \return 0 on success, -ERRNO on failure.
int MyOnDiskFS::fuseReaddir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fileInfo) {
    LOGM();

    // TODO: [PART 2] Implement this!

    RETURN(0);
}

/// Initialize a file system.
///
/// This function is called when the file system is mounted. You may add some initializing code here.
/// \param [in] conn Can be ignored.
/// \return 0.
void* MyOnDiskFS::fuseInit(struct fuse_conn_info *conn) {






    struct SDFRtest {
        struct mySuperblock {
            unsigned int mySuperblockindex = 0;     //start von Superblock
            unsigned int myDMAPindex;           //start von DMAP
            unsigned int myFATindex;           //start von FAT
            unsigned int myRootindex;          //start von Root
            unsigned int myDATAindex;          //start von Data
        };
        mySuperblock* superBlock = new mySuperblock;      //in fuseDestory werden alle allokierten Variablen mit delete wieder gelöscht

        struct myDMAP {
            unsigned char freeBlocks[NUM_BLOCKS];   //0 is free, 1 is full
        };
        myDMAP* dmap = new myDMAP;

        struct myFAT {
            unsigned int FATTable[NUM_BLOCKS];     //kommt noch in VL
        };
        myFAT* fat = new myFAT;

        struct myRoot {
            MyFsFileInfo fileInfos[NUM_DIR_ENTRIES];
        };
        myRoot* root = new myRoot;

        size_t getSize(int i) {
            switch (i) {
                case 0:
                    //printf("Size superblock: %d", sizeof(mySuperblock));
                    return sizeof(mySuperblock);
                case 1:
                    //printf("Size dmap: %d", sizeof(myDMAP));
                    return sizeof(myDMAP);
                case 2:
                    //printf("Size fat: %d", sizeof(myFAT));
                    return sizeof(myFAT);
                case 3:
                    //printf("Size root: %d", sizeof(myRoot));
                    return sizeof(myRoot);
            }
        }

        void* getStruct (int i) {
            switch (i) {
                case 0:
                    return superBlock;
                case 1:
                    return dmap;
                case 2:
                    return fat;
                case 3:
                    return root;
            }
        }

        void setStruct (int i, void* puffer) {
            switch (i) {
                case 0:
                    superBlock = static_cast<mySuperblock *>(puffer);
                    break;
                case 1:
                    dmap = static_cast<myDMAP *>(puffer);;
                    break;
                case 2:
                    fat = static_cast<myFAT *>(puffer);;
                    break;
                case 3:
                    root = static_cast<myRoot *>(puffer);;
                    break;
            }
        }

        unsigned int getLastIndex (int i) {
            switch (i) {
                case 0:
                    return 0;
                case 1:
                    return superBlock->mySuperblockindex;
                case 2:
                    return superBlock->myDMAPindex;
                case 3:
                    return superBlock->myFATindex;
                case 4:
                    return superBlock->myRootindex;
            }
        }

        void setIndex (int i, int index) {
            switch (i) {
                case 0:
                    superBlock->mySuperblockindex = index;
                case 1:
                    superBlock->myDMAPindex = index;
                case 2:
                    superBlock->myFATindex = index;
                case 3:
                    superBlock->myRootindex = index;
                case 4:
                    superBlock->myDATAindex = index;
            }
        }
    };


    SDFRtest* s = new SDFRtest;



    struct kott {
        struct test {
            int i = 0;
        };
        test* t = new test;

        void* getStruct() {
            return this->t;
        }

        void convertToBuffer(char* buffer) {
            buffer[0] = t->i;
        }

        void convertToStruct(char* buffer) {
            t->i = buffer[0];
        }

        size_t getSize() {
            return sizeof(test);
        }

        void setInt(int newi) {
            t->i = newi;
        }
    };

    kott* k = new kott;


    // Open logfile
    this->logFile= fopen(((MyFsInfo *) fuse_get_context()->private_data)->logFile, "w+");
    if(this->logFile == NULL) {
        fprintf(stderr, "ERROR: Cannot open logfile %s\n", ((MyFsInfo *) fuse_get_context()->private_data)->logFile);
    } else {
        // turn of logfile buffering
        setvbuf(this->logFile, NULL, _IOLBF, 0);

        LOG("Starting logging...\n");

        LOG("Using on-disk mode");

        LOGF("Container file name: %s", ((MyFsInfo *) fuse_get_context()->private_data)->contFile);

        int ret= this->blockDevice->open(((MyFsInfo *) fuse_get_context()->private_data)->contFile);

        if(ret >= 0) {
            LOG("Container file does exist, reading");

            // TODO: [PART 2] Read existing structures form file
            readContainer();

            //LOGF("oldint: %d", s->superBlock->mySuperblockindex);
            /*char buffer[sizeof(sdfr->superBlock)];
            blockDevice->read(0, buffer);
            sdfr->setStruct(0, buffer);
            LOGF("newint: %d %d %d %d", sdfr->superBlock->myDATAindex, sdfr->superBlock->myDMAPindex, sdfr->superBlock->myFATindex, sdfr->superBlock->myRootindex);*/

            /*char buf[BLOCK_SIZE];
            blockDevice->read(0, buf);
            memcpy(k->t, buf, k->getSize());
            LOGF("newsint: %d", k->t->i);*/

        } else if(ret == -ENOENT) {
            LOG("Container file does not exist, creating a new one");

            ret = this->blockDevice->create(((MyFsInfo *) fuse_get_context()->private_data)->contFile);

            if (ret >= 0) {

                // TODO: [PART 2] Create empty structures in file

                this->blockDevice->create("/home/user/bslab/container.bin");

                //baue Struktur auf:
                buildStructure();

                /*LOGF("oldint: %d", k->t->i);
                k->setInt(12345);
                char buf[BLOCK_SIZE];
                memcpy(buf, k->t, k->getSize());
                blockDevice->write(0, buf);*/
                /*char buf[BLOCK_SIZE];
                blockDevice->read(0, buf);
                //k->convertToStruct(buf);
                memcpy(k->t, buf, k->getSize());
                LOGF("newsint: %d", k->t->i);*/
            }
        }

        if(ret < 0) {
            LOGF("ERROR: Access to container file failed with error %d", ret);
        }
     }

    return 0;
}

/// @brief Clean up a file system.
///
/// This function is called when the file system is unmounted. You may add some cleanup code here.
void MyOnDiskFS::fuseDestroy() {
    LOGM();

    delete sdfr->superBlock;
    delete sdfr->dmap;
    delete sdfr->fat;
    delete sdfr->root;

}

// TODO: [PART 2] You may add your own additional methods here!

void MyOnDiskFS::buildStructure() {
    unsigned int numBlocks;
    unsigned int blockSize;
    int currentIndex = 0;

    for (int i = 0; i < NUM_SDFR; i++) {
        currentIndex = sdfr->getLastIndex(i);
        LOGF("index: %d", currentIndex + numBlocks);
        sdfr->setIndex(i, currentIndex + numBlocks);
        if (i != NUM_SDFR - 1) {
            size_t s = sdfr->getSize(i);
            numBlocks = s % BLOCK_SIZE == 0 ? s / BLOCK_SIZE : (s / BLOCK_SIZE) + 1;
            blockSize = numBlocks * BLOCK_SIZE;
            char puffer[blockSize];
            memcpy(puffer, (sdfr->getStruct(i)), s);
            writeOnDisk(sdfr->getLastIndex(i + 1), puffer, numBlocks, s);
        }
    }

    LOGF("SB index: %d | DMAP index: %d | fat index: %d | root index: %d | data index: %d",
         sdfr->superBlock->mySuperblockindex, sdfr->superBlock->myDMAPindex, sdfr->superBlock->myFATindex, sdfr->superBlock->myRootindex, sdfr->superBlock->myDATAindex);

}

//write on disk mit nebeneinander liegenden blocks - erstmal nur für structure builden
void MyOnDiskFS::writeOnDisk(unsigned int blockNumber, char* pufAll, unsigned int numBlocks, size_t size) {
    char buf[BLOCK_SIZE];
    size_t currentSize;
    unsigned int counter = 0;

    for (int i = 0; i < numBlocks; i++) {
        currentSize = i == numBlocks - 1 ? (size - ((numBlocks - 1) * BLOCK_SIZE)) : BLOCK_SIZE; //oder currentSize = size - counter >= BLOCK_SIZE ? BLOCK_SIZE : size - counter;
        memcpy(buf, pufAll + counter, currentSize);
        blockDevice->write(blockNumber, buf);
        blockNumber++;
        counter += BLOCK_SIZE;
    }
}

//TODO funktioniert noch nicht ganz!!! -> Debug
void MyOnDiskFS::readContainer() {
    unsigned int numBlocks;
    unsigned int blockSize;
    unsigned int currentIndex;
    unsigned int blockNumber = 0;

    for (int i = 0; i < NUM_SDFR; i++) {
        if (i != NUM_SDFR - 1) {
            size_t s = sdfr->getSize(i);
            numBlocks = s % BLOCK_SIZE == 0 ? s / BLOCK_SIZE : (s / BLOCK_SIZE) + 1;
            blockSize = numBlocks * BLOCK_SIZE;
            char puffer[blockSize];
            readOnDisk(blockNumber, puffer, numBlocks, s);
            sdfr->setStruct(i, puffer);
            memcpy(sdfr->getStruct(i), puffer, s);
        }
        currentIndex = sdfr->getLastIndex(i);
        LOGF("index: %d", currentIndex + numBlocks);
        blockNumber = currentIndex + numBlocks;
    }

    LOGF("SB index: %d | DMAP index: %d | fat index: %d | root index: %d | data index: %d",
         sdfr->superBlock->mySuperblockindex, sdfr->superBlock->myDMAPindex, sdfr->superBlock->myFATindex, sdfr->superBlock->myRootindex, sdfr->superBlock->myDATAindex);
}

//TODO evtl sogar write und read zusammen packen zu einer funktion mit unterscheidung write oder read
void MyOnDiskFS::readOnDisk(unsigned int blockNumber, char* puf, unsigned int numBlocks, size_t size) {
    char buf[BLOCK_SIZE];
    size_t currentSize;
    unsigned int counter = 0;

    for (int i = 0; i < numBlocks; i++) {
        currentSize = i == numBlocks - 1 ? (size - ((numBlocks - 1) * BLOCK_SIZE)) : BLOCK_SIZE; //oder currentSize = size - counter >= BLOCK_SIZE ? BLOCK_SIZE : size - counter;
        blockDevice->read(blockNumber, buf);
        memcpy(puf + counter, buf, currentSize);
        blockNumber++;
        counter += BLOCK_SIZE;
    }
}


// DO NOT EDIT ANYTHING BELOW THIS LINE!!!

/// @brief Set the static instance of the file system.
///
/// Do not edit this method!
void MyOnDiskFS::SetInstance() {
    MyFS::_instance= new MyOnDiskFS();
}
