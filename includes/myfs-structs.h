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

// TODO: Add structures of your file system here

struct MyFsFileInfo {
    char fileName[NAME_LENGTH];
    size_t blockSize = BLOCK_SIZE;  //vielfaches von BLOCK_SIZE
    size_t dataSize = 0;            //evtl 0 weg
    unsigned int userId;
    unsigned int groupId;

    mode_t mode;

    time_t a_time;
    time_t m_time;
    time_t c_time;

    char* data = "";        //evtl "" weg
};

#endif /* myfs_structs_h */
