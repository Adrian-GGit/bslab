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
        sdfr->dmap->freeBlocks[i] = 0;
        sdfr->fat->FATTable[i] = EOF;
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

    LOGF("path: %s | existingFiles: %d | numdirs: %d | openFiles: %d\n", path, sdfr->superBlock->existingFiles, NUM_DIR_ENTRIES, openFiles);

    if (sdfr->superBlock->existingFiles < NUM_DIR_ENTRIES) {//&& nextFreeBlock >= 0) { //>= 0 -> mem vorhanden um neue Datei anzulegen
        index = searchForFile(path);
        if(index >= 0) {
            RETURN(-EEXIST)
        }
        if (strlen(path + 1) > NAME_LENGTH) {
            RETURN(-ENAMETOOLONG)
        }
        MyFsFileInfo* newData = &(sdfr->root->fileInfos[sdfr->superBlock->existingFiles]);
        strcpy(newData->fileName, path + 1);
        newData->mode = mode;  //root: read,write,execute; group: read,execute; others:read,execute -> to give everyone all perms: 0777
        newData->a_time = time(NULL);    //current time
        newData->m_time = time(NULL);
        newData->c_time = time(NULL);
        newData->userId = getuid();
        newData->groupId = getgid();

        sdfr->superBlock->existingFiles += 1;
        calcBlocksAndSynchronize(SUPERBLOCK, 0);
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

    if (index >= 0) {
        MyFsFileInfo* file = &(sdfr->root->fileInfos[index]);
        checkAndCloseFile(file);
        unsigned int numBlocks = sdfr->root->fileInfos[index].numBlocks;
        int blocks[numBlocks];
        int currentBlock = file->startBlock;
        for (int i = 0; i < numBlocks; i++) { //füllt array blocks mit allen Indizes von fat bzw dmap auf
            blocks[i] = currentBlock;
            currentBlock = sdfr->fat->FATTable[currentBlock];
        }
        //reset dmap and fat
        fillFatAndDmap(blocks, sizeof(blocks) / sizeof(blocks[0]), false);
        //reset element in root and fill gap
        for (int i = index; i < sdfr->superBlock->existingFiles; i++) {
            //überschreibe letztes Element mit leerer MyFsFileInfo
            sdfr->root->fileInfos[i] = i == sdfr->superBlock->existingFiles - 1 ? MyFsFileInfo() : sdfr->root->fileInfos[i + 1];
            calcBlocksAndSynchronize(ROOT, i);
        }
        sdfr->superBlock->existingFiles--;
        calcBlocksAndSynchronize(SUPERBLOCK, 0);
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
        strcpy(sdfr->root->fileInfos[index].fileName, newpath + 1);
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

    LOGF( "\tAttributes of %s requested\n", path );

    // GNU's definitions of the attributes (http://www.gnu.org/software/libc/manual/html_node/Attribute-Meanings.html):
    // 		st_uid: 	The user ID of the file’s owner.
    //		st_gid: 	The group ID of the file.
    //		st_atime: 	This is the last access time for the file.
    //		st_mtime: 	This is the time of the last modification to the contents of the file.
    //		st_mode: 	Specifies the mode of the file. This includes file type information (see Testing File Type) and
    //		            the file permission bits (see Permission Bits).
    //		st_nlink: 	The number of hard links to the file. This existingFiles keeps track of how many directories have
    //	             	entries for this file. If the existingFiles is ever decremented to zero, then the file itself is
    //	             	discarded as soon as no process still holds it open. Symbolic links are not counted in the
    //	             	total.
    //		st_size:	This specifies the size of a regular file in bytes. For files that are really devices this field
    //		            isn’t usually meaningful. For symbolic links this specifies the length of the file name the link
    //		            refers to.

    index = searchForFile(path);

    int ret= 0;

    if ( strcmp( path, "/" ) == 0 )
    {
        statbuf->st_uid = getuid();
        statbuf->st_gid = getgid();
        statbuf->st_atime = time(NULL);
        statbuf->st_mtime = time(NULL);

        statbuf->st_mode = S_IFDIR | 0755;
        statbuf->st_nlink = 2; // Why "two" hardlinks instead of "one"? The answer is here: http://unix.stackexchange.com/a/101536
        RETURN(ret);
    }

    if(index >= 0) {
        statbuf->st_uid = sdfr->root->fileInfos[index].userId;
        statbuf->st_gid = sdfr->root->fileInfos[index].groupId;
        updateTime(index, 1);
        statbuf->st_atime = sdfr->root->fileInfos[index].a_time;
        statbuf->st_mtime = sdfr->root->fileInfos[index].m_time;

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
/// open file existingFiles.
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

    fileInfo->fh = -1;  //nothing relevant in the puffer

    sdfr->root->fileInfos[index].open = true;
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
    LOGF( "--> Trying to read %s, %lu, %lu\n", path, (unsigned long) offset, size );
    index = searchForFile(path);
    size_t finalSize;
    if(index >= 0) {
        MyFsFileInfo *file = &(sdfr->root->fileInfos[index]);
        finalSize = read(file->dataSize, buf, size, offset, fileInfo, -1);
        updateTime(index, 0);
        RETURN(finalSize);
    }
    RETURN(index);
}

unsigned int MyOnDiskFS::read(size_t dataSize, char *buf, size_t size, off_t offset, struct fuse_file_info *fileInfo, int build) {
    if (offset > dataSize || sdfr->root->fileInfos[index].numBlocks == 0) {
        RETURN(0);
    }

    unsigned int totalSize = 0;
    unsigned int startInFirstBlock = offset % BLOCK_SIZE;   //wo die Bytes angefangen werden zu lesen
    unsigned int numBlocksForward = offset / BLOCK_SIZE;    //number of blocks that need to be read
    unsigned int startingBlock = build < 0 ? sdfr->root->fileInfos[index].startBlock : sdfr->getIndex(build);
    startingBlock = getStartingBlock(startingBlock, numBlocksForward);

    unsigned int bytesInFileAfterOffset = dataSize - (numBlocksForward * BLOCK_SIZE);    //Anzahl Bytes die hinter offset in der Datei stehen
    unsigned int bytesToReadAfterOffset = bytesInFileAfterOffset > size ? size: bytesInFileAfterOffset;    //entweder begrenzt bytesAfterOffset oder size die Anzahl zu lesender Bytes

    char blockBuffer[BLOCK_SIZE];
    unsigned int count = 0;

    while (true) {
        if (build < 0 && startingBlock == fileInfo->fh) {   //im falle von build<0 gibt es möglicherweise einen puffer in welchem bereits gelesene/geschriebene Daten drin gespeichert wurden
            memcpy(blockBuffer, puffer, BLOCK_SIZE);
        } else{
            blockDevice->read(startingBlock, blockBuffer);
        }
        if (bytesToReadAfterOffset > BLOCK_SIZE) {
            if (count == 0) {   //anfangs muss ein Teil des 512er Blocks abgeschnitten werden abhängig von startInFirstBlock
                memcpy(buf, blockBuffer + startInFirstBlock, BLOCK_SIZE - startInFirstBlock);
                totalSize += BLOCK_SIZE - startInFirstBlock;
            } else{
                memcpy(buf + count, blockBuffer, BLOCK_SIZE);
                totalSize += BLOCK_SIZE;
            }
        } else{
            memcpy(buf + count, blockBuffer, bytesToReadAfterOffset);
            totalSize += bytesToReadAfterOffset;
            RETURN(totalSize);
        }
        if (build < 0) {
            startingBlock = sdfr->fat->FATTable[startingBlock];
        } else{
            startingBlock++;
        }
        bytesToReadAfterOffset -= BLOCK_SIZE;
        count += BLOCK_SIZE;
    }
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
    index = searchForFile(path);
    if (!enoughStorage(index, size))
        return -ENOMEM;
    size_t finalSize = 0;
    size_t missing = 0;
    if (index >= 0) {
        MyFsFileInfo *file = &(sdfr->root->fileInfos[index]);
        //falls offset größer als die dateigrößer selber ist -> dazwischen ist freier platz welcher allokiert und mit 0en aufgefüllt wird
        if (offset > file->dataSize) {
            missing = offset - file->dataSize;
            char puf[missing + size];
            memset(puf, '0', missing);
            memcpy(puf + missing, buf, size);
            finalSize += write(file, puf, missing + size, offset - missing, fileInfo, -1);    //neues offset muss um missing viele Bytes nach vorne verschoben werden, dass die 0en auch geschrieben werden und somit keine Lücke bleibt
        } else{
            finalSize += write(file, buf, size, offset, fileInfo, -1);
        }

        file->dataSize = offset + size > file->dataSize ? offset + size : file->dataSize;   //falls was überschrieben wird -> dataSize bleibt gleich, falls was dazukam offset + size
        finalSize -= missing;   //TODO nötig??? eigentlich werden die 0en auch noch geschrieben
        updateTime(index, 0);
        return finalSize;
    }
    return index;
}

unsigned int MyOnDiskFS::write(MyFsFileInfo *file, const char *buf, size_t size, off_t offset, struct fuse_file_info *fileInfo, int build) {
    unsigned int totalSize = 0;
    unsigned int startInFirstBlock = offset % BLOCK_SIZE;   //wo die Bytes angefangen werden zu lesen
    unsigned int numBlocksForward = offset / BLOCK_SIZE;    //number of blocks that need to be read
    unsigned int startingBlock;
    if (file->numBlocks == 0) {
        startingBlock = file->startBlock = findNextFreeBlock();
        file->numBlocks = 1;
        calcBlocksAndSynchronize(ROOT, index);
        sdfr->dmap->freeBlocks[startingBlock] = 1;   //allokiere ersten Block -> Rest allokiert write
        calcBlocksAndSynchronize(DMAP, startingBlock);
    } else{
        startingBlock = build < 0 ? file->startBlock : sdfr->getIndex(build);
    }

    //hole neuen Block falls aktueller Block voll ist
    if (numBlocksForward == file->numBlocks) {
        int previousBlock = getStartingBlock(startingBlock, numBlocksForward - 1);  //numBlocksForward-1 da numBlocksForward den Block repräsentiert, der erst noch allokiert werden muss
        startingBlock = findNextFreeBlock();
        int blocks[] = {previousBlock, (int) startingBlock};
        fillFatAndDmap(blocks, 2, true);
        file->numBlocks++;
    } else{
        startingBlock = getStartingBlock(startingBlock, numBlocksForward);  //getting first Block relative to offset
    }

    unsigned int freeSizeInCurrentBlock = BLOCK_SIZE - startInFirstBlock;
    char buffer[BLOCK_SIZE];

    //startInFirstBlock nur beim ersten Block ungleich 0
    if (startInFirstBlock != 0 && fileInfo != nullptr) {     //falls im ersten Block Bytes nicht überschrieben werden, müssen diese gespeichert werden
        //speichert die nicht zu überschreibenden Bytes in den Puffer, was nur ganz am Anfang wenn überhaupt benötigt wird
        if (startingBlock == fileInfo->fh) {
            memcpy(buffer, puffer, startInFirstBlock);
        } else{
            blockDevice->read(startingBlock, buffer);
            fileInfo->fh = startingBlock;
        }
    }

    //falls alle zu schreibenden Bytes in den aktuellen Block passen -> hole bestehende Bytes bis offset -> rest werden neue Bytes
    if (size > freeSizeInCurrentBlock) {
        memcpy(buffer + startInFirstBlock, buf, freeSizeInCurrentBlock);
        blockDevice->write(startingBlock, buffer);
        totalSize += freeSizeInCurrentBlock + write(file, buf + freeSizeInCurrentBlock, size - freeSizeInCurrentBlock,
                                                    offset + freeSizeInCurrentBlock, fileInfo, build);
    } else {
        blockDevice->read(startingBlock, buffer);
        if (build < 0 && fileInfo != nullptr)
            fileInfo->fh = startingBlock;
        memcpy(buffer + startInFirstBlock, buf, size);
        blockDevice->write(startingBlock, buffer);
        totalSize += size;
    }
    return totalSize;
}

/// @brief Close a file.
///
/// \param [in] path Name of the file, starting with "/".
/// \param [in] File handel for the file set by fuseOpen.
/// \return 0 on success, -ERRNO on failure.
int MyOnDiskFS::fuseRelease(const char *path, struct fuse_file_info *fileInfo) {
    LOGM();

    index = searchForFile(path);
    if (index >= 0) {
        checkAndCloseFile(&(sdfr->root->fileInfos[index]));
        fileInfo->fh = -1;
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
    RETURN(fuseTruncate(path, newSize, nullptr));
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

    index = searchForFile(path);
    if (index >= 0) {
        MyFsFileInfo *file = &(sdfr->root->fileInfos[index]);
        int numBlocksNew = newSize % BLOCK_SIZE == 0 ? newSize / BLOCK_SIZE : newSize / BLOCK_SIZE + 1;
        if(newSize > file->dataSize) {
            int missing = newSize - file->dataSize;
            char puf[missing];
            memset(puf, '\0', missing);
            if (!enoughStorage(index, missing))
                return -ENOMEM;
            write(file, puf, missing, file->dataSize, fileInfo, -1);
        } else {
            int toDelete = file->numBlocks - numBlocksNew;
            int current = file->startBlock;
            for (int i = 0; i < file->numBlocks; i++) {
                int next = sdfr->fat->FATTable[current];
                if (i >= file->numBlocks - toDelete - 1) {     //letzter Block der Datei muss EOF haben -> toDelete - 1
                    sdfr->fat->FATTable[current] = EOF;
                    calcBlocksAndSynchronize(FAT, current);
                }
                if (i >= file->numBlocks - toDelete) {
                    sdfr->dmap->freeBlocks[current] = 0;
                    calcBlocksAndSynchronize(DMAP, current);
                }
                current = next;
            }
        }

        file->dataSize = newSize;
        file->numBlocks = numBlocksNew;
        updateTime(index, 1);
    }

    RETURN(index);
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

    LOGF( "--> Getting The List of Files of %s\n", path );

    filler( buf, ".", NULL, 0 ); // Current Directory
    filler( buf, "..", NULL, 0 ); // Parent Directory

    if ( strcmp( path, "/" ) == 0 ) // If the user is trying to show the files/directories of the root directory show the following
    {
        for (int i = 0; i < sdfr->superBlock->existingFiles; i++) {
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
        } else if(ret == -ENOENT) {
            LOG("Container file does not exist, creating a new one");

            ret = this->blockDevice->create(((MyFsInfo *) fuse_get_context()->private_data)->contFile);

            if (ret >= 0) {

                this->blockDevice->create("/home/user/bslab/container.bin");

                setIndexes();
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

    delete _instance; //TODO nötig?!?!?!

}

void MyOnDiskFS::setIndexes() {
    int currentIndex;
    unsigned int numBlocks = 0;

    for (int i = 0; i < NUM_SDFR; i++) {
        currentIndex = sdfr->getIndex(i);
        sdfr->setIndex(i, currentIndex + numBlocks);
        size_t s = sdfr->getSize(i);
        numBlocks = s % BLOCK_SIZE == 0 ? s / BLOCK_SIZE : (s / BLOCK_SIZE) + 1;
        LOGF("index %d: %d", i, sdfr->getIndex(i));
    }
}

int MyOnDiskFS::searchForFile(const char* path) {
    LOGM();
    for (int i = 0; i < sdfr->superBlock->existingFiles; i++) {
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
    calcBlocksAndSynchronize(ROOT, index);
}

int MyOnDiskFS::findNextFreeBlock() {
    int currentBlock = 0;
    while(true) {
        if (currentBlock >= NUM_BLOCKS) {
            RETURN(-ENOMEM);
        } else {
            if (sdfr->dmap->freeBlocks[currentBlock] == 0) {
                return currentBlock;
            } else {
                currentBlock++;
            }
        }
    }
}

//Funktion um Fat und Dmap an allen indizes zu füllen welche in blocks stehen
void MyOnDiskFS::fillFatAndDmap(int blocks[], size_t sizeArray, bool fill) {
    for (int i = 0; i < sizeArray; i++) {
        if (i == sizeArray - 1) {
            fillFat(blocks[i], EOF);
            if (fill) {
                fillDmap(blocks[i], 1);
            } else {
                fillDmap(blocks[i], 0);
            }
        } else {
            if (fill) {
                fillFat(blocks[i], blocks[i + 1]);
                fillDmap(blocks[i], 1);
            } else {
                fillFat(blocks[i], EOF);
                fillDmap(blocks[i], 0);
            }
        }
    }
}

void MyOnDiskFS::fillFat(int index, int toInsert) {
    sdfr->fat->FATTable[index] = toInsert;
    calcBlocksAndSynchronize(FAT, index);
}

void MyOnDiskFS::fillDmap(int index, bool toInsert) {
    sdfr->dmap->freeBlocks[index] = toInsert;
    calcBlocksAndSynchronize(DMAP, index);
}

///used to calculate which block has to be replaced with which buffer and synchronize
void MyOnDiskFS::calcBlocksAndSynchronize(int sdfrBlock, unsigned int indexInArray) {
    //LOG("Synchronize...");
    if (sdfrBlock == 0) {
        char buf[BLOCK_SIZE];
        memcpy(buf, sdfr->getStruct(SUPERBLOCK), BLOCK_SIZE);
        blockDevice->write(sdfr->superBlock->mySuperblockindex, buf);
    } else{
        float numBlocks = sdfr->getIndex(sdfrBlock + 1) - sdfr->getIndex(sdfrBlock);    //Anzahl an realen Blöcke die der struct einnimmt
        float oneBlock = sdfrBlock == ROOT ? NUM_DIR_ENTRIES / numBlocks : NUM_BLOCKS / numBlocks;    //die Anzahl an Array Einträgen die in einen 512er Block passen

        float floatStartBlock = (indexInArray / oneBlock);
        int startBlock = (int)(floatStartBlock);
        float floatLastBlock = ((indexInArray + 1) / oneBlock);
        int lastBlock = (int)(floatLastBlock);

        if (floatLastBlock - lastBlock == (float)0) { //falls floatLastBlock keine Nachkommastelle besitzt wird das Ende des gefragten Index noch vom vorherigen Block verwaltet -> --
            lastBlock--;
        }

        size_t size = sdfr->getSize(sdfrBlock);
        char buf[size];
        memcpy(buf, sdfr->getStruct(sdfrBlock), size);
        char puffer[BLOCK_SIZE];

        for (int i = startBlock; i <= lastBlock; i++) {
            memcpy(puffer, buf + i * BLOCK_SIZE, BLOCK_SIZE);
            blockDevice->write(sdfr->getIndex(sdfrBlock) + i, puffer);
        }
    }
    //LOG("End of synchronize...");
}

unsigned int MyOnDiskFS::getStartingBlock(unsigned int startingBlock, unsigned int numBlocksForward) {
    for (int i = 0; i < numBlocksForward; i++) {
        startingBlock = sdfr->fat->FATTable[startingBlock];
    }
    return startingBlock;
}

bool MyOnDiskFS::enoughStorage(int index, size_t neededStorage) {
    MyFsFileInfo* file = &(sdfr->root->fileInfos[index]);
    size_t missingStorageInLastBlock = BLOCK_SIZE - (file->dataSize % BLOCK_SIZE); //berechnet wie viel Platz noch im letzten von der file allokierten Block verfügbar ist
    size_t storageToAlloc = neededStorage - missingStorageInLastBlock;
    int numNewBlocks = storageToAlloc % BLOCK_SIZE == 0 ? storageToAlloc / BLOCK_SIZE : storageToAlloc / BLOCK_SIZE + 1;
    int currentBlock = sdfr->superBlock->myDATAindex;
    int counter = 0;
    while(true) {
        if (sdfr->dmap->freeBlocks[currentBlock] == 0) {
            counter++;
        }
        currentBlock++;
        if (counter >= numNewBlocks)
            return true;
        if (currentBlock >= NUM_BLOCKS)
            return false;
    }
}

void MyOnDiskFS::checkAndCloseFile(MyFsFileInfo* file) {
    if (file->open) {   //schließe Datei falls diese noch offen ist
        sdfr->root->fileInfos[index].open = false;
        openFiles--;
        updateTime(index, 0);
    }
}

void MyOnDiskFS::buildStructure() {
    for (int i = 0; i < NUM_SDFR - 1; i++) {
        size_t s = sdfr->getSize(i);
        char buf[s];
        memcpy(buf, sdfr->getStruct(i), s);
        MyFsFileInfo *file = new MyFsFileInfo;
        sdfr->dmap->freeBlocks[sdfr->getIndex(i)] = 1;   //allokiere ersten Block -> Rest allokiert write
        calcBlocksAndSynchronize(DMAP, sdfr->getIndex(i));
        file->numBlocks = 1;
        write(file, buf, s, 0, nullptr, i);
        delete file;
    }
}

void MyOnDiskFS::readContainer() {
    for (int i = 0; i < NUM_SDFR - 1; i++) {
        size_t s = sdfr->getSize(i);
        char buf[s];
        read(s, buf, s, 0, nullptr, i);
        memcpy(sdfr->getStruct(i), buf, s);
    }
}


// DO NOT EDIT ANYTHING BELOW THIS LINE!!!

/// @brief Set the static instance of the file system.
///
/// Do not edit this method!
void MyOnDiskFS::SetInstance() {
    MyFS::_instance= new MyOnDiskFS();
}
