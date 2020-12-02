//
// Created by Oliver Waldhorst on 20.03.20.
// Copyright © 2017-2020 Oliver Waldhorst. All rights reserved.
//

#include "myondiskfs.h"

// For documentation of FUSE methods see https://libfuse.github.io/doxygen/structfuse__operations.html

#undef DEBUG

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
        sdfr->fat->FATTable[i] = EOF;   //TODO benötigt??!
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

    LOGF("path: %s | count: %d | numdirs: %d\n", path, count, NUM_DIR_ENTRIES);

    int nextFreeBlock = findNextFreeBlock();

    if (count < NUM_DIR_ENTRIES && nextFreeBlock >= 0) { //>= 0 -> mem vorhanden
        index = searchForFile(path);
        if(index >= 0) {
            RETURN(-EEXIST)
        }
        if (strlen(path + 1) > NAME_LENGTH) {
            RETURN(-ENAMETOOLONG)
        }
        MyFsFileInfo* newData = &(sdfr->root->fileInfos[count]);
        copyFileNameIntoArray(path + 1, newData->fileName);
        newData->mode = mode;  //root: read,write,execute; group: read,execute; others:read,execute -> to give everyone all perms: 0777
        newData->a_time = time(NULL);    //current time
        newData->m_time = time(NULL);
        newData->c_time = time(NULL);
        newData->userId = getuid();
        newData->groupId = getgid();
        newData->startBlock = nextFreeBlock;

        sdfr->dmap->freeBlocks[nextFreeBlock] = '1';
        sdfr->fat->FATTable[nextFreeBlock] = EOF; //TODO eigentlich unnötig, da im fat die freien Blöcke immer 0 enthalten
        newData->noBlocks = 1;

        synchronize();

        LOGF("datasize: %d", newData->dataSize);

        count += 1;
        RETURN(0);
    }

    RETURN(-ENOMEM);
}

/// @brief Delete a file.
///
/// Delete a file with given name from the file system.
/// You do not have to check file permissions, but can assume that it is always ok to access the file.
/// \param [in] path Name of the file, starting with "/".
/// \return 0 on success, -ERRNO on failure.
int MyOnDiskFS::fuseUnlink(const char *path) {
    LOGM();

    index = searchForFile(path);
    LOGF("index: %d", index);
    int numBlocks = 1;

    if (index >= 0) {
        MyFsFileInfo* file = &(sdfr->root->fileInfos[index]);
        size_t s = file->dataSize;
        if (s != 0)
            numBlocks = s % BLOCK_SIZE == 0 ? s / BLOCK_SIZE : (s / BLOCK_SIZE) + 1;
        int blocks[numBlocks];
        int currentBlock = file->startBlock;
        for (int i = 0; i < numBlocks; i++) {
            blocks[i] = currentBlock;
            currentBlock = sdfr->fat->FATTable[currentBlock];
        }
        //reset dmap and fat
        fillFatAndDmap(blocks, sizeof(blocks) / sizeof(blocks[0]), false);
        //reset element in root and fill gap
        //starting from position where file to delete is to the last file (count - 1 -> < count)
        //bedenke hinterstes Element wird doppelt vorhanden sein -> sollte aber nicht gefunden werden können da searchForFile nur bis count nachguckt
        for (int i = index; i < count; i++) {
            sdfr->root->fileInfos[i] = sdfr->root->fileInfos[i + 1];
        }

        count--;
        RETURN(0);
    }

    RETURN(-ENOENT);
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

    index = searchForFile(path);
    statbuf->st_uid = sdfr->root->fileInfos[index].userId;
    statbuf->st_gid = sdfr->root->fileInfos[index].groupId;
    updateTime(index, 1);
    statbuf->st_atime = sdfr->root->fileInfos[index].a_time;
    statbuf->st_mtime = sdfr->root->fileInfos[index].m_time;

    int ret= 0;

    if ( strcmp( path, "/" ) == 0 )
    {
        statbuf->st_mode = S_IFDIR | 0755;
        statbuf->st_nlink = 2; // Why "two" hardlinks instead of "one"? The answer is here: http://unix.stackexchange.com/a/101536
        RETURN(ret);
    }

    if(index >= 0) {
        statbuf->st_mode = sdfr->root->fileInfos[index].mode;
        statbuf->st_nlink = 1;
        statbuf->st_size = sdfr->root->fileInfos[index].dataSize;
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

    index = searchForFile(path);
    if(index >= 0) {
        sdfr->root->fileInfos[index].mode = mode;
        updateTime(index, 1);
        RETURN(0);
    }

    RETURN(index);
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

    index = searchForFile(path);
    if(index >= 0) {
        sdfr->root->fileInfos[index].userId = uid;
        sdfr->root->fileInfos[index].groupId = gid;
        updateTime(index, 1);
        RETURN(0);
    }

    RETURN(index);
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

    index = searchForFile(path);
    if (openFiles >= NUM_OPEN_FILES || index < 0) {
        RETURN(-ENOENT);
    }

    puffer = new char[BLOCK_SIZE];
    fileInfo->fh = -1;  //nothing relevant in the puffer ->

    openFiles++;
    updateTime(index, 0);
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

    //TODO fragen wie das mit read mitten aus einer file funktionieren soll: size ist immer 4096

    //TODO test this!

    LOGF( "--> Trying to read %s, %lu, %lu\n", path, (unsigned long) offset, size );
    index = searchForFile(path);
    if(index >= 0) {

        unsigned int startInFirstBlock = offset % BLOCK_SIZE;   //wo die Bytes angefangen werden zu lesen
        //unsigned int endInLastBlock = (startInFirstBlock + size) % BLOCK_SIZE;  //wo die Bytes aufgehört werden zu lesen

        size_t dataSize = sdfr->root->fileInfos[index].dataSize;
        size_t toRead;
        size_t numReadingBlocks;

        unsigned int numBlocksForward = offset / BLOCK_SIZE;    //number of blocks that need to be read

        unsigned int startingBlock = sdfr->root->fileInfos[index].startBlock;
        startingBlock = getStartingBlock(startingBlock, numBlocksForward);  //getting first Block relative to offset

        //LOGF("num1: %d | size: %d | startinfirstblock: %d", (startInFirstBlock + size) / BLOCK_SIZE, size, startInFirstBlock);
        //TODO ternärer operator
        if (size < dataSize - offset) {
            toRead = size;
        } else{
            toRead = dataSize - offset;
        }
        numReadingBlocks = dataSize % BLOCK_SIZE == 0 ?
                (dataSize - numBlocksForward) / BLOCK_SIZE : (dataSize - numBlocksForward) / BLOCK_SIZE + 1;

        //char puf[toRead];
        //readOnDisk(startingBlock, puf, numReadingBlocks, toRead, offset, false, fileInfo);
        char blockBuffer[BLOCK_SIZE];
        size_t currentSize;
        unsigned int counter = 0;

        for (int i = 0; i < numReadingBlocks; i++) {
            //TODO von puffer auslesen von fileInfo
            currentSize = toRead - counter >= BLOCK_SIZE ? BLOCK_SIZE : toRead - counter;
            blockDevice->read(startingBlock, blockBuffer);
            memcpy(buf + counter, blockBuffer + offset, currentSize);
            //LOGF("blockBuffer[512]: %s | blockBuffer[512] + offset: %s | buf: %s | buf + counter: %s", blockBuffer, blockBuffer + offset, buf, buf + counter);
            LOGF("buf: %s | buf + counter: %s", buf, buf + counter);
            LOGF("startingBlock: %d | numWritingBlock: %d | currentSize: %d | counter: %d | size: %d | dataSize: %d | offset: %d",
                 startingBlock, numReadingBlocks, currentSize, counter, size, dataSize, offset);
            startingBlock = sdfr->fat->FATTable[startingBlock];
            offset = 0;
            counter += BLOCK_SIZE;
        }

        LOG("---------------------------------------------------------------------------------------");
        LOGF("buffinal: %s", buf);
        LOG("---------------------------------------------------------------------------------------");

        updateTime(index, 0);

        RETURN(toRead);
    }
    return index;
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

    LOGF("--> Trying to write %s, %lu, %lu\n", path, (unsigned long) offset, size);
    LOGF("buf: %s", buf);
    index = searchForFile(path);        //TODO funktion anders formen, da bei rekursion immer wieder unnötigerweise searchforfile aufgerufen wird
    size_t totalSize = 0;
    if (index >= 0) {
        MyFsFileInfo *file = &(sdfr->root->fileInfos[index]);

        //falls offset größer als die dateigrößer selber ist -> dazwischen ist freier platz welcher allokiert und mit 0en aufgefüllt wird
        if (offset > file->dataSize) {
            size_t missing = offset - file->dataSize;
            char puf[missing];
            memset(puf, '0', missing);
            totalSize += fuseWrite(path, puf, missing, offset - missing, fileInfo);
        }

        unsigned int startInFirstBlock = offset % BLOCK_SIZE;   //wo die Bytes angefangen werden zu lesen
        //unsigned int endInLastBlock = (startInFirstBlock + size) % BLOCK_SIZE;  //wo die Bytes aufgehört werden zu lesen
        unsigned int numBlocksForward = offset / BLOCK_SIZE;    //number of blocks that need to be read
        unsigned int startingBlock = sdfr->root->fileInfos[index].startBlock;   //the first block of the file

        //hole neuen Block falls aktueller Block voll ist
        if (numBlocksForward == file->noBlocks) {
            startingBlock = getStartingBlock(startingBlock, numBlocksForward - 1);
            int temp = startingBlock;
            //TODO falls startBlock < 0 -> nomem
            sdfr->fat->FATTable[temp] = startingBlock = findNextFreeBlock();
            sdfr->fat->FATTable[startingBlock] = EOF;
            sdfr->dmap->freeBlocks[startingBlock] = '1';
            file->noBlocks++;
        } else{
            startingBlock = getStartingBlock(startingBlock, numBlocksForward);  //getting first Block relative to offset
        }

        unsigned int freeSizeInCurrentBlock = BLOCK_SIZE - startInFirstBlock;
        char blockBuffer[BLOCK_SIZE];
        blockDevice->read(startingBlock, blockBuffer);
        //falls alle zu schreibenden Bytes in den aktuellen Block passen -> hole bestehende Bytes bis offset -> rest werden neue Bytes
        if (size <= freeSizeInCurrentBlock) {
            unsigned int pufferSize = startInFirstBlock + size;
            char puffer[BLOCK_SIZE];
            memcpy(puffer, blockBuffer, startInFirstBlock);
            memcpy(puffer + startInFirstBlock, buf, size);

            totalSize += write(file, puffer, pufferSize, offset, startingBlock, fileInfo);
        } else {
            char puffer[BLOCK_SIZE];
            memcpy(puffer, blockBuffer, startInFirstBlock);
            memcpy(puffer + startInFirstBlock, buf, freeSizeInCurrentBlock);

            totalSize += write(file, puffer, BLOCK_SIZE, offset, startingBlock, fileInfo);
            totalSize += fuseWrite(path, buf + BLOCK_SIZE, size - freeSizeInCurrentBlock, offset + BLOCK_SIZE, fileInfo);
        }

        file->dataSize = size + offset;
        return totalSize;
    }
    return index;
}

int MyOnDiskFS::write(MyFsFileInfo *file, char *buf, size_t size, off_t offset, unsigned int blockNumber, struct fuse_file_info *fileInfo) {
    blockDevice->write(blockNumber, buf);

    synchronize();
    updateTime(index, 0);

    RETURN(size);
}

/// @brief Close a file.
///
/// \param [in] path Name of the file, starting with "/".
/// \param [in] File handel for the file set by fuseOpen.
/// \return 0 on success, -ERRNO on failure.
int MyOnDiskFS::fuseRelease(const char *path, struct fuse_file_info *fileInfo) {
    LOGM();

    index = searchForFile(path);
    LOGF("index: %d | openFiles: %d", index, openFiles);
    if (index >= 0) {
        delete[] puffer;
        openFiles--;
        updateTime(index, 0);
    }

    RETURN(index);
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
            //TODO EVTL fillFatAndDmap
            readContainer();
        } else if(ret == -ENOENT) {
            LOG("Container file does not exist, creating a new one");

            ret = this->blockDevice->create(((MyFsInfo *) fuse_get_context()->private_data)->contFile);

            if (ret >= 0) {

                this->blockDevice->create("/home/user/bslab/container.bin");

                setIndexes();
                //baue Struktur auf:
                fillFatAndDmapWhileBuild();
                buildStructure();
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
        //TODO indexes nötig??? -> statt indexes[i] einfach sdfr->getIndex(i);
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
            LOGF("GEFUNDEN %s", path + 1);
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

int MyOnDiskFS::findNextFreeBlock(int lastBlock) {  //TODO benötigt?!?
    lastBlock++; //current as parameter is the current block which is definitely not free

    while(true) {
        if (lastBlock >= NUM_BLOCKS) {
            RETURN(-ENOMEM);
        } else {
            if (sdfr->dmap->freeBlocks[lastBlock] == '0') {
                return lastBlock;
            } else {
                lastBlock++;
            }
        }
    }
}

void MyOnDiskFS::fillFatAndDmapWhileBuild() {
    for (int i = 0; i < NUM_SDFR - 1; i++) {
        int blocks[indexes[i + 1] - indexes[i]];
        for (int j = indexes[i]; j < indexes[i + 1]; j++) {
            blocks[j - indexes[i]] = j;
        }
        fillFatAndDmap(blocks, sizeof(blocks) / sizeof(blocks[0]), true);
    }
}

void MyOnDiskFS::fillFatAndDmap(int blocks[], size_t sizeArray, bool fill) {
    for (int i = 0; i < sizeArray; i++) {
        if (i == sizeArray - 1) {
                sdfr->fat->FATTable[blocks[sizeArray - 1]] = EOF;
            if (fill) {
                sdfr->dmap->freeBlocks[blocks[sizeArray - 1]] = '1';
            } else{
                sdfr->dmap->freeBlocks[blocks[sizeArray - 1]] = '0';
            }
        } else{
            if (fill) {
                sdfr->fat->FATTable[blocks[i]] = blocks[i + 1];
                sdfr->dmap->freeBlocks[blocks[i]] = '1';
            } else{
                sdfr->fat->FATTable[blocks[i]] = EOF;
                sdfr->dmap->freeBlocks[blocks[i]] = '0';
            }
        }
    }
}

void MyOnDiskFS::synchronize() {
    //alle sdfr blöcke werden aktualisiert
    buildStructure();
}

unsigned int MyOnDiskFS::getStartingBlock(unsigned int startingBlock, unsigned int numBlocksForward) {
    for (int i = 0; i < numBlocksForward; i++) {
        startingBlock = sdfr->fat->FATTable[startingBlock];
    }
    return startingBlock;
}

// TODO: [PART 2] You may add your own additional methods here!

//TODO manche hilfsfunktionen sind dieselben wie bei inmemoryfs -> gleiche funktionen in basisklasse myfs

void MyOnDiskFS::buildStructure() {
    for (int i = 0; i < NUM_SDFR - 1; i++) {
        size_t s = sdfr->getSize(i);
        char buf[s];
        memcpy(buf, sdfr->getStruct(i), s);
        writeOnDisk(sdfr->getIndex(i), buf, indexes[i + 1] - indexes[i], s, 0, true, nullptr);;
    }

    /*LOGF("SB index: %d | DMAP index: %d | fat index: %d | root index: %d | data index: %d",
         sdfr->superBlock->mySuperblockindex, sdfr->superBlock->myDMAPindex, sdfr->superBlock->myFATindex, sdfr->superBlock->myRootindex, sdfr->superBlock->myDATAindex);
    LOGF("len dmap: %d | len fat: %d | len root: %d",
         sizeof(sdfr->dmap->freeBlocks), sizeof(sdfr->fat->FATTable), sizeof(sdfr->root->fileInfos));
    for (int i = 0; i <= sdfr->superBlock->myDATAindex; i++) {
        LOGF("dmap %d: %c", i, sdfr->dmap->freeBlocks[i]);
    }
    for (int i = 0; i <= sdfr->superBlock->myDATAindex; i++) {
        LOGF("fat: %d: %d", i, sdfr->fat->FATTable[i]);
    }*/
}

//TODO immer wenn an einer Datei was geändert wird müssen auch die sdfr Blöcke verändert werden in der .bin -> funktion synchronize()
//TODO anfangs sind keine Blöcke belegt -> writeondisk sollte am besten immer nächstliegenden Block nehmen -> keine eigene Funktion für building, da sdfr blöcke dann garantiert nebeneinander liegen
//write on disk mit nebeneinander liegenden blocks - erstmal nur für structure builden
void MyOnDiskFS::writeOnDisk(unsigned int startBlock, const char* pufAll, unsigned int numBlocks, size_t size, off_t offset, bool building, struct fuse_file_info *fileInfo) {
    char buf[BLOCK_SIZE];
    size_t currentSize;
    unsigned int counter = 0;

    for (int i = 0; i < numBlocks; i++) {
        if (!building) {
            /*currentSize = size - counter >= BLOCK_SIZE ? BLOCK_SIZE : size - counter;
            //TODO evtl mal hier debuggen (wegen pufALlNew + counter, size ist newSize)
            if (offset != 0) {
                blockDevice->read(startBlock, buf);
            }
            LOGF("Davor: buf: %s | buf + offset: %s | pufAll + counter: %s | currentSize - offset: %d", buf, buf + offset, pufAll + counter, currentSize - offset);
            memcpy(buf + offset, pufAll + counter, currentSize);
            LOGF("buf: %s | buf + offset: %s | pufAll + counter: %s | currentSize - offset: %d", buf, buf + offset, pufAll + counter, currentSize - offset);
            blockDevice->write(startBlock, buf);

            if (i != numBlocks - 1) {
                bool needNewBlock = sdfr->fat->FATTable[startBlock] == EOF;

                if (!needNewBlock) {
                    startBlock = sdfr->fat->FATTable[startBlock];
                } else{
                    int temp = startBlock;
                    //TODO falls startBlock < 0 -> nomem
                    sdfr->fat->FATTable[temp] = startBlock = findNextFreeBlock();
                    sdfr->fat->FATTable[startBlock] = EOF;
                    sdfr->dmap->freeBlocks[startBlock] = '1';
                }
            }*/
            //TODO evtl statt startBlock auch über fat -> vermeiden von redundanten code
        } else{
            currentSize = size - counter >= BLOCK_SIZE ? BLOCK_SIZE : size - counter;
            memcpy(buf, pufAll + counter, currentSize);
            blockDevice->write(startBlock, buf);
            startBlock++;
        }
        offset = 0;
        counter += BLOCK_SIZE;
    }
}

void MyOnDiskFS::readContainer() {
    for (int i = 0; i < NUM_SDFR - 1; i++) {
        size_t s = sdfr->getSize(i);
        char puffer[s];
        readOnDisk(indexes[i], puffer, indexes[i + 1] - indexes[i], s, 0, true, nullptr);
        memcpy(sdfr->getStruct(i), puffer, s);
    }

    /*LOGF("SB index: %d | DMAP index: %d | fat index: %d | root index: %d | data index: %d",
         sdfr->superBlock->mySuperblockindex, sdfr->superBlock->myDMAPindex, sdfr->superBlock->myFATindex, sdfr->superBlock->myRootindex, sdfr->superBlock->myDATAindex);
    LOGF("len dmap: %d | len fat: %d | len root: %d",
         sizeof(sdfr->dmap->freeBlocks), sizeof(sdfr->fat->FATTable), sizeof(sdfr->root->fileInfos));
    for (int i = 0; i <= sdfr->superBlock->myDATAindex; i++) {
        LOGF("dmap %d: %c", i, sdfr->dmap->freeBlocks[i]);
    }
    for (int i = 0; i <= sdfr->superBlock->myDATAindex; i++) {
        LOGF("fat: %d: %d", i, sdfr->fat->FATTable[i]);
    }
    for (int i = sdfr->superBlock->myDATAindex + 1; i <= sdfr->superBlock->myDATAindex + 20; i++) {
        LOGF("fat: %d: %d", i, sdfr->fat->FATTable[i]);
    }*/
}

//TODO evtl sogar write und read zusammen packen zu einer funktion mit unterscheidung write oder read
void MyOnDiskFS::readOnDisk(unsigned int startBlock, char* puf, unsigned int numBlocks, size_t size, off_t offset, bool building, struct fuse_file_info *fileInfo) {
    char buf[BLOCK_SIZE];
    size_t currentSize;
    unsigned int counter = 0;

   for (int i = 0; i < numBlocks; i++) {
       if (!building) {     //TODO evlt mit &&
           /*if (startBlock == fileInfo->fh) {
               memcpy(puf + counter, puffer + offset, BLOCK_SIZE);
           } else{
               currentSize = size - counter >= BLOCK_SIZE ? BLOCK_SIZE : size - counter;
               blockDevice->read(startBlock, buf);
               memcpy(puf + counter, buf + offset, currentSize);
           }
           startBlock = sdfr->fat->FATTable[startBlock];*/
       } else{
           currentSize = size - counter >= BLOCK_SIZE ? BLOCK_SIZE : size - counter;
           blockDevice->read(startBlock, buf);
           memcpy(puf + counter, buf, currentSize);
           startBlock++;
       }
       offset = 0;
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
