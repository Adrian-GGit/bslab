//
// Created by Oliver Waldhorst on 20.03.20.
// Copyright © 2017-2020 Oliver Waldhorst. All rights reserved.
//

#ifndef MYFS_MYONDISKFS_H
#define MYFS_MYONDISKFS_H

#include "myfs.h"

/// @brief On-disk implementation of a simple file system.
class MyOnDiskFS : public MyFS {
protected:
    // BlockDevice blockDevice;

public:
    static MyOnDiskFS *Instance();

    // TODO: [PART 1] Add attributes of your file system here

    char puffer[BLOCK_SIZE];

    SDFR* sdfr;

    MyOnDiskFS();
    ~MyOnDiskFS();

    static void SetInstance();

    // --- Methods called by FUSE ---
    // For Documentation see https://libfuse.github.io/doxygen/structfuse__operations.html
    virtual int fuseGetattr(const char *path, struct stat *statbuf);
    virtual int fuseMknod(const char *path, mode_t mode, dev_t dev);
    virtual int fuseUnlink(const char *path);
    virtual int fuseRename(const char *path, const char *newpath);
    virtual int fuseChmod(const char *path, mode_t mode);
    virtual int fuseChown(const char *path, uid_t uid, gid_t gid);
    virtual int fuseTruncate(const char *path, off_t newSize);
    virtual int fuseOpen(const char *path, struct fuse_file_info *fileInfo);
    virtual int fuseRead(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fileInfo);
    virtual int fuseWrite(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fileInfo);
    virtual int fuseRelease(const char *path, struct fuse_file_info *fileInfo);
    virtual void* fuseInit(struct fuse_conn_info *conn);
    virtual int fuseReaddir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fileInfo);
    virtual int fuseTruncate(const char *path, off_t offset, struct fuse_file_info *fileInfo);
    virtual void fuseDestroy();

    // TODO: Add methods of your file system here
    void buildStructure();
    void readContainer();
    void setIndexes();
    int searchForFile(const char *path);
    void updateTime(int index, int timeIndex);
    int findNextFreeBlock();
    void fillFatAndDmap(int blocks[], size_t sizeArray, bool fill);
    void synchronizeSuperBlock();
    void calcBlocksAndSynchronize(int sdfrBlock, unsigned int indexOfInRamArray);
    unsigned int getStartingBlock(unsigned int startingBlock, unsigned int numBlocksForward);
    unsigned int write(MyFsFileInfo *file, const char *buf, size_t size, off_t offset, fuse_file_info *fileInfo, int build);
    unsigned int read(size_t dataSize, char *buf, size_t size, off_t offset, fuse_file_info *fileInfo, int build);
    bool enoughStorage(int index, size_t neededStorage);
    void checkAndCloseFile(MyFsFileInfo* file);
};

#endif //MYFS_MYONDISKFS_H
