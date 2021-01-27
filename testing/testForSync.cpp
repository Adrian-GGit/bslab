//  testing

//IMPORTANT: manuelle Ausführung der einzelnen Tests!!!
//erst Test 1 zum schreiben -> unmounten -> neu mounten -> Test 2 zum nochmaligen lesen nach neuem mount

#include "catch.hpp"
#include <cstdio>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#define FILENAME "file"
#define NEWFILENAME "file2"
#define SMALL_SIZE 1024
#define LARGE_SIZE 20*1024*1024

TEST_CASE("T-utSync1.1", "[Part_1]") {
    printf("Testcase unittest1: Mount and write\n");

    int fd;
    char* r= new char[SMALL_SIZE];
    memset(r, 0, SMALL_SIZE);
    char *buf= new char[SMALL_SIZE];


    // remove file (just to be sure)
    unlink(FILENAME);

    // Create file
    fd = open(FILENAME, O_EXCL | O_RDWR | O_CREAT, 0666);
    REQUIRE(fd >= 0);
    // Write to the file
    REQUIRE(write(fd, r, SMALL_SIZE) == SMALL_SIZE);

    // Close file
    REQUIRE(close(fd) >= 0);

    // Open file again
    fd = open(FILENAME, O_EXCL | O_RDWR, 0666);
    REQUIRE(fd >= 0);

    // Read from the file
    REQUIRE(read(fd, buf, SMALL_SIZE) == SMALL_SIZE);
    REQUIRE(memcmp(r, buf, SMALL_SIZE) == 0);

    // Close file
    REQUIRE(close(fd) >= 0);

    delete[] r;
    delete[] buf;
}

//unmount after this and remount

TEST_CASE("T-utSync1.2", "[Part_1]") {
    printf("Testcase unittest1: Mount and read again\n");

    int fd;
    char* r= new char[SMALL_SIZE];
    memset(r, 0, SMALL_SIZE);
    char *buf= new char[SMALL_SIZE];

    // Open file again
    fd = open(FILENAME, O_EXCL | O_RDWR, 0666);
    REQUIRE(fd >= 0);

    // Read from the file
    REQUIRE(read(fd, buf, SMALL_SIZE) == SMALL_SIZE);
    REQUIRE(memcmp(r, buf, SMALL_SIZE) == 0);

    // Close file
    REQUIRE(close(fd) >= 0);

    delete[] r;
    delete[] buf;

    // remove file
    REQUIRE(unlink(FILENAME) >= 0);
}