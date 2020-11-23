//
//  test-myfs.cpp
//  testing
//
//  Created by Oliver Waldhorst on 15.12.17.
//  Copyright © 2017 Oliver Waldhorst. All rights reserved.
//

#include "catch.hpp"

// TODO: Implement your helper functions here!
#include <cstdio>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "catch.hpp"
#include "tools.hpp"

#define FILENAME "file"
#define NEWFILENAME "file2"
#define SMALL_SIZE 1024
#define LARGE_SIZE 20*1024*1024

TEST_CASE("T-ut1", "[Part_1]") {
    printf("Testcase unittest1: Rename file\n");

    int fd;

    // remove file (just to be sure)
    unlink(FILENAME);

    // Create file
    fd = open(FILENAME, O_EXCL | O_RDWR | O_CREAT, 0666);
    REQUIRE(fd >= 0);

    REQUIRE(rename(FILENAME, NEWFILENAME) >= 0);

    // Close file
    REQUIRE(close(fd) >= 0);

    // remove file
    REQUIRE(unlink(NEWFILENAME) >= 0);
}

TEST_CASE("T-ut2", "[Part_1]") {
    printf("Testcase unittest2: Change owner\n");

    int fd;

    // remove file (just to be sure)
    unlink(FILENAME);

    // Create file
    fd = open(FILENAME, O_EXCL | O_RDWR | O_CREAT, 0666);
    REQUIRE(fd >= 0);

    REQUIRE(chown(FILENAME, 1001, 1001) >= 0);

    // Close file
    REQUIRE(close(fd) >= 0);

    // remove file
    REQUIRE(unlink(FILENAME) >= 0);
}

TEST_CASE("T-ut3", "[Part_1]") {
    printf("Testcase unittest3: Truncate a file and read and compare content\n");

    int fd;

    // remove file (just to be sure)
    unlink(FILENAME);

    // set up read & write buffer
    char* r= new char[SMALL_SIZE];
    memset(r, 0, SMALL_SIZE);
    char* w= new char[SMALL_SIZE];
    memset(w, 0, SMALL_SIZE);
    gen_random(w, SMALL_SIZE);

    char *buforiginal = new char[SMALL_SIZE];
    char *buforiginalHalf = new char[SMALL_SIZE / 2];
    memcpy(buforiginalHalf, w, SMALL_SIZE / 2);
    char *buforiginalQuarter = new char[SMALL_SIZE / 4];
    memcpy(buforiginalQuarter, w, SMALL_SIZE / 4);
    char *buftrunc1 = new char[SMALL_SIZE / 2];
    char *buftrunc2 = new char[SMALL_SIZE / 4];

    // Create file
    fd = open(FILENAME, O_EXCL | O_RDWR | O_CREAT, 0666);
    REQUIRE(fd >= 0);

    // Write to the file
    REQUIRE(write(fd, w, SMALL_SIZE) == SMALL_SIZE);

    // Close file
    REQUIRE(close(fd) >= 0);

    // Open file again
    fd = open(FILENAME, O_EXCL | O_RDWR, 0666);
    REQUIRE(fd >= 0);

    // Read from the file
    REQUIRE(read(fd, buforiginal, SMALL_SIZE) == SMALL_SIZE);
    REQUIRE(memcmp(buforiginal, w, SMALL_SIZE) == 0);

    // Close file
    REQUIRE(close(fd) >= 0);

    fd = open(FILENAME, O_EXCL | O_RDWR, 0666);
    REQUIRE(fd >= 0);

    // Truncate open file
    REQUIRE(ftruncate(fd, SMALL_SIZE / 2) == 0);
    REQUIRE(read(fd, buftrunc1, SMALL_SIZE / 2) == SMALL_SIZE / 2);
    REQUIRE(memcmp(buftrunc1, buforiginalHalf, SMALL_SIZE / 2) == 0);

    // Close file
    REQUIRE(close(fd) >= 0);

    // Truncate closed file
    REQUIRE(truncate(FILENAME, SMALL_SIZE / 4) == 0);

    fd = open(FILENAME, O_EXCL | O_RDWR, 0666);
    REQUIRE(fd >= 0);

    REQUIRE(read(fd, buftrunc2, SMALL_SIZE / 4) == SMALL_SIZE / 4);
    REQUIRE(memcmp(buftrunc2, buforiginalQuarter, SMALL_SIZE / 4) == 0);

    // Close file
    REQUIRE(close(fd) >= 0);

    // remove file
    REQUIRE(unlink(FILENAME) >= 0);

    delete [] r;
    delete [] w;
}

TEST_CASE("T-ut4", "[Part_1]") {
    printf("Testcase unittest4: Read in middle of file\n");

    const char *buf1= "abcdexyz";
    char *buf2= new char[strlen(buf1) / 2];

    int fd;
    ssize_t b;

    // remove file (just to be sure)
    unlink(FILENAME);

    // Create file
    fd = open(FILENAME, O_EXCL | O_RDWR | O_CREAT, 0666);
    REQUIRE(fd >= 0);

    // Write to the file
    REQUIRE(write(fd, buf1, strlen(buf1)) == strlen(buf1));

    // Close file
    REQUIRE(close(fd) >= 0);

    // Open file again
    fd = open(FILENAME, O_EXCL | O_RDWR, 0666);
    REQUIRE(fd >= 0);

    // Read from the file
    REQUIRE(read(fd, buf2, strlen(buf1) / 2) == strlen(buf1) / 2);
    REQUIRE(memcmp(buf1, buf2, strlen(buf1) / 2) == 0);

    // Close file
    REQUIRE(close(fd) >= 0);

    // remove file
    REQUIRE(unlink(FILENAME) >= 0);

    delete [] buf2;
}