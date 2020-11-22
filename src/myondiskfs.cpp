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
    this->sdfr = new SDFR;

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

    delete sdfr->superBlock;
    delete sdfr->dmap;
    delete sdfr->fat;
    delete sdfr->root;
    delete sdfr;

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

    index = searchForFile(path);
    if(index >= 0) {
        copyFileNameIntoArray(newpath + 1, sdfr->root->fileInfos[index].fileName);
        updateTime(index, 1);
        RETURN(0);
    }
    RETURN(index);
}

/// @brief Get file meta data.
///
/// Get the metadata of a file (user & group id, modification times, permissions, ...).
/// \param [in] path Name of the file, starting with "/".
/// \param [out] statbuf Structure containing the meta data, for details type "man 2 stat" in a terminal.
/// \return 0 on success, -ERRNO on failure.
int MyOnDiskFS::fuseGetattr(const char *path, struct stat *statbuf) {
    LOGM();

    //TODO test this

    LOGF( "\tAttributes of %s requested\n", path );

    // GNU's definitions of the attributes (http://www.gnu.org/software/libc/manual/html_node/Attribute-Meanings.html):
    // 		st_uid: 	The user ID of the file’s owner.
    //		st_gid: 	The group ID of the file.
    //		st_atime: 	This is the last access time for the file.
    //		st_mtime: 	This is the time of the last modification to the contents of the file.
    //		st_mode: 	Specifies the mode of the file. This includes file type information (see Testing File Type) and
    //		            the file permission bits (see Permission Bits).
    //		st_nlink: 	The number of hard links to the file. This count keeps track of how many directories have
    //	             	entries for this file. If the count is ever decremented to zero, then the file itself is
    //	             	discarded as soon as no process still holds it open. Symbolic links are not counted in the
    //	             	total.
    //		st_size:	This specifies the size of a regular file in bytes. For files that are really devices this field
    //		            isn’t usually meaningful. For symbolic links this specifies the length of the file name the link
    //		            refers to.

    LOG("$");
    index = searchForFile(path);
    LOG("H");
    statbuf->st_uid = sdfr->root->fileInfos[index].userId;
    statbuf->st_gid = sdfr->root->fileInfos[index].groupId;
    updateTime(index, 1);
    statbuf->st_atime = sdfr->root->fileInfos[index].a_time;
    statbuf->st_mtime = sdfr->root->fileInfos[index].m_time;
    LOG("HI");

    int ret= 0;

    if ( strcmp( path, "/" ) == 0 )
    {
        statbuf->st_mode = S_IFDIR | 0755;
        statbuf->st_nlink = 2; // Why "two" hardlinks instead of "one"? The answer is here: http://unix.stackexchange.com/a/101536
        LOG("HIi");
        RETURN(ret);
    }

    if(index >= 0) {
        LOG("HIii");
        statbuf->st_mode = sdfr->root->fileInfos[index].mode;
        statbuf->st_nlink = 1;
        statbuf->st_size = sdfr->root->fileInfos[index].dataSize;
        LOG("HIiii");
        RETURN(ret);
    }


    RETURN(index);
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

    //TODO test this

    LOGF( "--> Getting The List of Files of %s\n", path );

    filler( buf, ".", NULL, 0 ); // Current Directory
    filler( buf, "..", NULL, 0 ); // Parent Directory

    if ( strcmp( path, "/" ) == 0 ) // If the user is trying to show the files/directories of the root directory show the following
    {
        for (int i = 0; i < count; i++) {
            filler(buf, sdfr->root->fileInfos[i].fileName, NULL, 0);
        }
        RETURN(0);
    }

    RETURN(-ENOENT);
}

/// Initialize a file system.
///
/// This function is called when the file system is mounted. You may add some initializing code here.
/// \param [in] conn Can be ignored.
/// \return 0.
void* MyOnDiskFS::fuseInit(struct fuse_conn_info *conn) {
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

            setIndexes();
            //read Container
            readContainer();
            //TODO belege alle Blöcke in DMAP bis dataindex (evtl in FAT noch was rein dass es konsistent ist)
            //TODO write test to confirm that structures are build and read right

        } else if(ret == -ENOENT) {
            LOG("Container file does not exist, creating a new one");

            ret = this->blockDevice->create(((MyFsInfo *) fuse_get_context()->private_data)->contFile);

            if (ret >= 0) {

                // TODO: [PART 2] Create empty structures in file

                this->blockDevice->create("/home/user/bslab/container.bin");

                setIndexes();
                //baue Struktur auf:
                fillFatAndDmapWhileBuid();
                buildStructure();
                //TODO belege alle Blöcke in DMAP bis dataindex (evtl in FAT noch was rein dass es konsistent ist)
                //TODO write test to confirm that structures are build and read right

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

    delete _instance; //TODO nötig?!??!?!

}

void MyOnDiskFS::setIndexes() {
    int currentIndex = 0;
    unsigned int numBlocks = 0;

    for (int i = 0; i < NUM_SDFR; i++) {
        currentIndex = sdfr->getIndex(i);
        sdfr->setIndex(i, currentIndex + numBlocks);
        indexes[i] = currentIndex + numBlocks;
        size_t s = sdfr->getSize(i);
        numBlocks = s % BLOCK_SIZE == 0 ? s / BLOCK_SIZE : (s / BLOCK_SIZE) + 1;
    }
}

int MyOnDiskFS::searchForFile(const char* path) {
    LOGM();
    LOGF("count: %d", count);
    for (int i = 0; i < count; i++) {
        if (strcmp(path + 1, sdfr->root->fileInfos[i].fileName) == 0) {
            RETURN(i);
        }
    }
    RETURN(-ENOENT)
}

void MyOnDiskFS::updateTime(int index, int timeIndex) {
    //time == 0: a_time
    //time == 1: a_time + m_time
    //time == 2: a_time + m_time + c_time
    time_t update = time(NULL);
    if (timeIndex >= 0) {
        sdfr->root->fileInfos[index].a_time = update;
        if (timeIndex >= 1) {
            sdfr->root->fileInfos[index].m_time = update;
            if (timeIndex >= 2) {
                sdfr->root->fileInfos[index].c_time = update;
            }
        }
    }
}

void MyOnDiskFS::copyFileNameIntoArray(const char *fileName, char fileArray[]) {
    index = 0;

    while(*fileName != '\0') {
        fileArray[index] = *fileName;
        fileName++;
        index++;
    }

    fileArray[index] = '\0';
}

int MyOnDiskFS::findNextFreeBlock(int current = -1) {
    current++; //current as parameter is the current block which is definitely not free

    while(true) {
        if (current >= NUM_BLOCKS) {
            RETURN(-ENOMEM);
        } else {
            if (sdfr->dmap->freeBlocks[current] == 0) {
                return current;
            } else {
                current++;
            }
        }
    }
}

void MyOnDiskFS::fillFatAndDmapWhileBuid() {
    for (int i = 0; i < NUM_SDFR - 1; i++) {
        //LOGF("index i: %d | index i + 1: %d", indexes[i], indexes[i + 1]);
        int blocks[indexes[i + 1] - indexes[i]];
        //LOGF("sizearray: %d", sizeof(blocks) / sizeof(blocks[0]));
        for (int j = indexes[i]; j < indexes[i + 1]; j++) {
            blocks[j - indexes[i]] = j;
            //LOGF("block %d[%d] = %d", i, j - indexes[i], j);
        }
        //LOGF("sizearray: %d", sizeof(blocks) / sizeof(blocks[0]));
        fillFatAndDmap(blocks, sizeof(blocks) / sizeof(blocks[0]));
    }
}

void MyOnDiskFS::fillFatAndDmap(int blocks[], size_t size) {
    //LOG("------------------------------------");
    //LOGF("size: %d | size - 1: %d", size, size - 1);
    for (int i = 0; i < size; i++) {
        if (i == size - 1) {
            //LOGF("blocksfat[%d]=%d", blocks[size - 1], 0);
            //LOGF("blocksdmap[%d]='1'", blocks[size - 1]);
            sdfr->fat->FATTable[blocks[size - 1]] = 0;
            sdfr->dmap->freeBlocks[blocks[size - 1]] = '1';
        } else{
            //LOGF("blocksfat[%d]=%d", blocks[i], blocks[i + 1]);
            //LOGF("blocksdmap[%d]='1'", blocks[i]);
            sdfr->fat->FATTable[blocks[i]] = blocks[i + 1];
            sdfr->dmap->freeBlocks[blocks[i]] = '1';
        }
    }
}

// TODO: [PART 2] You may add your own additional methods here!

void MyOnDiskFS::buildStructure() {
    for (int i = 0; i < NUM_SDFR - 1; i++) {
        size_t s = sdfr->getSize(i);
        char buf[s];
        memcpy(buf, sdfr->getStruct(i), s);
        writeOnDisk(sdfr->getIndex(i), buf, indexes[i + 1] - indexes[i], s);;
    }

    LOGF("SB index: %d | DMAP index: %d | fat index: %d | root index: %d | data index: %d",
         sdfr->superBlock->mySuperblockindex, sdfr->superBlock->myDMAPindex, sdfr->superBlock->myFATindex, sdfr->superBlock->myRootindex, sdfr->superBlock->myDATAindex);
    LOGF("len dmap: %d | len fat: %d | len root: %d",
         sizeof(sdfr->dmap->freeBlocks), sizeof(sdfr->fat->FATTable), sizeof(sdfr->root->fileInfos));
    for (int i = 0; i <= sdfr->superBlock->myDATAindex; i++) {
        LOGF("dmap %d: %c", i, sdfr->dmap->freeBlocks[i]);
    }
    for (int i = 0; i <= sdfr->superBlock->myDATAindex; i++) {
        LOGF("fat: %d: %d", i, sdfr->fat->FATTable[i]);
    }
}

//TODO immer wenn an einer Datei was geändert wird müssen auch die sdfr Blöcke verändert werden in der .bin -> funktion synchronize()
//TODO anfangs sind keine Blöcke belegt -> writeondisk sollte am besten immer nächstliegenden Block nehmen -> keine eigene Funktion für building, da sdfr blöcke dann garantiert nebeneinander liegen

//write on disk mit nebeneinander liegenden blocks - erstmal nur für structure builden
void MyOnDiskFS::writeOnDisk(unsigned int blockNumber, char* pufAll, unsigned int numBlocks, size_t size) {
    char buf[BLOCK_SIZE];
    size_t currentSize;
    unsigned int counter = 0;

    for (int i = 0; i < numBlocks; i++) {
        currentSize = size - counter >= BLOCK_SIZE ? BLOCK_SIZE : size - counter;
        memcpy(buf, pufAll + counter, currentSize);
        blockDevice->write(blockNumber, buf);
        blockNumber++;
        counter += BLOCK_SIZE;
    }
}

//TODO funktioniert noch nicht ganz!!! -> Debug
void MyOnDiskFS::readContainer() {
    for (int i = 0; i < NUM_SDFR - 1; i++) {
        size_t s = sdfr->getSize(i);
        char puffer[s];
        readOnDisk(indexes[i], puffer, indexes[i + 1] - indexes[i], s);
        memcpy(sdfr->getStruct(i), puffer, s);
    }

    LOGF("SB index: %d | DMAP index: %d | fat index: %d | root index: %d | data index: %d",
         sdfr->superBlock->mySuperblockindex, sdfr->superBlock->myDMAPindex, sdfr->superBlock->myFATindex, sdfr->superBlock->myRootindex, sdfr->superBlock->myDATAindex);
    LOGF("len dmap: %d | len fat: %d | len root: %d",
         sizeof(sdfr->dmap->freeBlocks), sizeof(sdfr->fat->FATTable), sizeof(sdfr->root->fileInfos));
    for (int i = 0; i <= sdfr->superBlock->myDATAindex; i++) {
        LOGF("dmap %d: %c", i, sdfr->dmap->freeBlocks[i]);
    }
    for (int i = 0; i <= sdfr->superBlock->myDATAindex; i++) {
        LOGF("fat: %d: %d", i, sdfr->fat->FATTable[i]);
    }
}

//TODO evtl sogar write und read zusammen packen zu einer funktion mit unterscheidung write oder read
void MyOnDiskFS::readOnDisk(unsigned int blockNumber, char* puf, unsigned int numBlocks, size_t size) {
    char buf[BLOCK_SIZE];
    size_t currentSize;
    unsigned int counter = 0;

    for (int i = 0; i < numBlocks; i++) {
        currentSize = size - counter >= BLOCK_SIZE ? BLOCK_SIZE : size - counter;
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
