//  testing

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
#define UNLINKFILE "unlinkOpenedFile"
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

//TODO überarbeiten - ist genau gleicher test wie bei itest.cpp
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

/*
 * Part mit write und read
 */

/*
 * write:
 */
TEST_CASE("T-ut5", "[Part_2]") {
    printf("Testcase integrationtest 5: write 1.1.1.1 and 1.1.1.2 and 1.1.2\n");
    const char *buf1 = "aaaaaccccc";
    const char *buf2 = "bbbb";
    const char *buf3 = "aaabbbbccc";
    const char *buf4 = "aaabbbbbbb";
    const char *buf5 = "aaabbbbbbbb";
    char *buf6= new char[strlen(buf3)];
    char *buf7= new char[strlen(buf4)];
    char *buf8= new char[strlen(buf5)];

    int fd;
    unlink(FILENAME);
    fd = open(FILENAME, O_EXCL | O_RDWR | O_CREAT, 0666);
    REQUIRE(fd >= 0);
    REQUIRE(write(fd, buf1, strlen(buf1)) == strlen(buf1));
    REQUIRE(close(fd) >= 0);

    //1.1.1.2
    fd = open(FILENAME, O_EXCL | O_RDWR, 0666);
    REQUIRE(fd >= 0);
    REQUIRE(lseek(fd, 3, SEEK_SET) == 3);
    REQUIRE(write(fd, buf2, strlen(buf2)) == strlen(buf2));
    REQUIRE(close(fd) >= 0);
    fd = open(FILENAME, O_EXCL | O_RDWR, 0666);
    REQUIRE(fd >= 0);
    REQUIRE(read(fd, buf6, strlen(buf3)) == strlen(buf3));
    REQUIRE(memcmp(buf3, buf6, strlen(buf3)) == 0);
    REQUIRE(close(fd) >= 0);

    //1.1.1.1
    fd = open(FILENAME, O_EXCL | O_RDWR, 0666);
    REQUIRE(fd >= 0);
    REQUIRE(lseek(fd, 6, SEEK_SET) == 6);
    REQUIRE(write(fd, buf2, strlen(buf2)) == strlen(buf2));
    REQUIRE(close(fd) >= 0);
    fd = open(FILENAME, O_EXCL | O_RDWR, 0666);
    REQUIRE(fd >= 0);
    REQUIRE(read(fd, buf7, strlen(buf4)) == strlen(buf4));
    REQUIRE(memcmp(buf4, buf7, strlen(buf4)) == 0);
    REQUIRE(close(fd) >= 0);

    //1.1.2
    fd = open(FILENAME, O_EXCL | O_RDWR, 0666);
    REQUIRE(fd >= 0);
    REQUIRE(lseek(fd, 7, SEEK_SET) == 7);
    REQUIRE(write(fd, buf2, strlen(buf2)) == strlen(buf2));
    REQUIRE(close(fd) >= 0);
    fd = open(FILENAME, O_EXCL | O_RDWR, 0666);
    REQUIRE(fd >= 0);
    REQUIRE(read(fd, buf8, strlen(buf5)) == strlen(buf5));
    REQUIRE(memcmp(buf5, buf8, strlen(buf5)) == 0);
    REQUIRE(close(fd) >= 0);

    delete [] buf6;
    delete [] buf7;
    delete [] buf8;
    REQUIRE(unlink(FILENAME) >= 0);
}

TEST_CASE("T-ut6", "[Part_2]") {
    printf("Testcase integrationtest 6: write 1.2.1 and 1.3.1\n");
    const char *buf1 = "aaaaaccccc";
    const char *buf2 = "bbbbb";
    const char *buf3 = "aaaaacccccbbbbb";
    const char *buf4 = "aaaaacccccbbbbb0bbbbb";
    char *buf5= new char[strlen(buf3)];
    char *buf6= new char[strlen(buf4)];

    int fd;
    unlink(FILENAME);
    fd = open(FILENAME, O_EXCL | O_RDWR | O_CREAT, 0666);
    REQUIRE(fd >= 0);
    REQUIRE(write(fd, buf1, strlen(buf1)) == strlen(buf1));
    REQUIRE(close(fd) >= 0);

    //1.2.1
    fd = open(FILENAME, O_EXCL | O_RDWR, 0666);
    REQUIRE(fd >= 0);
    REQUIRE(lseek(fd, 10, SEEK_SET) == 10);
    REQUIRE(write(fd, buf2, strlen(buf2)) == strlen(buf2));
    REQUIRE(close(fd) >= 0);
    fd = open(FILENAME, O_EXCL | O_RDWR, 0666);
    REQUIRE(fd >= 0);
    REQUIRE(read(fd, buf5, strlen(buf3)) == strlen(buf3));
    REQUIRE(memcmp(buf3, buf5, strlen(buf3)) == 0);
    REQUIRE(close(fd) >= 0);

    //1.3.1
    fd = open(FILENAME, O_EXCL | O_RDWR, 0666);
    REQUIRE(fd >= 0);
    REQUIRE(lseek(fd, 16, SEEK_SET) == 16);
    REQUIRE(write(fd, buf2, strlen(buf2)) == strlen(buf2));
    REQUIRE(close(fd) >= 0);
    fd = open(FILENAME, O_EXCL | O_RDWR, 0666);
    REQUIRE(fd >= 0);
    REQUIRE(read(fd, buf6, strlen(buf4)) == strlen(buf4));
    REQUIRE(memcmp(buf4, buf6, strlen(buf4)) == 0);
    REQUIRE(close(fd) >= 0);

    delete [] buf5;
    delete [] buf6;
    REQUIRE(unlink(FILENAME) >= 0);
}

TEST_CASE("T-ut7", "[Part_2]") {
    printf("Testcase integrationtest 7: write 2.1.1.1 and 2.1.1.2 and 2.1.2\n");
    char buf1[514]; memset(buf1, 'a', 514);
    char *buf2 = "bb";
    char *buf21 = "bbb";
    char *buf22 = "bbbb";
    char *buf3 = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaabba";
    REQUIRE(strlen(buf3) == 514);
    char *buf4 = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaabbb";
    REQUIRE(strlen(buf4) == 514);
    char *buf5 = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaabbbb";
    REQUIRE(strlen(buf5) == 515);
    char *buf6= new char[515];

    int fd;
    unlink(FILENAME);
    fd = open(FILENAME, O_EXCL | O_RDWR | O_CREAT, 0666);
    REQUIRE(fd >= 0);
    REQUIRE(write(fd, buf1, strlen(buf1)) == strlen(buf1));
    REQUIRE(close(fd) >= 0);

    //2.1.1.1
    fd = open(FILENAME, O_EXCL | O_RDWR, 0666);
    REQUIRE(fd >= 0);
    REQUIRE(lseek(fd, 511, SEEK_SET) == 511);
    REQUIRE(write(fd, buf2, strlen(buf2)) == strlen(buf2));
    REQUIRE(close(fd) >= 0);
    fd = open(FILENAME, O_EXCL | O_RDWR, 0666);
    REQUIRE(fd >= 0);
    REQUIRE(read(fd, buf6, 514) == 514);
    REQUIRE(memcmp(buf3, buf6, 514) == 0);
    REQUIRE(close(fd) >= 0);

    //2.1.1.2
    fd = open(FILENAME, O_EXCL | O_RDWR, 0666);
    REQUIRE(fd >= 0);
    REQUIRE(lseek(fd, 512, SEEK_SET) == 512);
    REQUIRE(write(fd, buf2, strlen(buf2)) == strlen(buf2));
    REQUIRE(close(fd) >= 0);
    fd = open(FILENAME, O_EXCL | O_RDWR, 0666);
    REQUIRE(fd >= 0);
    REQUIRE(read(fd, buf6, 514) == 514);
    REQUIRE(memcmp(buf4, buf6, 514) == 0);
    REQUIRE(close(fd) >= 0);

    //2.1.2
    fd = open(FILENAME, O_EXCL | O_RDWR, 0666);
    REQUIRE(fd >= 0);
    REQUIRE(lseek(fd, 513, SEEK_SET) == 513);
    REQUIRE(write(fd, buf2, strlen(buf2)) == strlen(buf2));
    REQUIRE(close(fd) >= 0);
    fd = open(FILENAME, O_EXCL | O_RDWR, 0666);
    REQUIRE(fd >= 0);
    REQUIRE(read(fd, buf6, 515) == 515);
    REQUIRE(memcmp(buf5, buf6, 515) == 0);
    REQUIRE(close(fd) >= 0);

    delete [] buf6;
    REQUIRE(unlink(FILENAME) >= 0);
}

TEST_CASE("T-ut8", "[Part_2]") {
    printf("Testcase integrationtest 8: write 2.2.1 and 2.3.1\n");
    char buf1[510]; memset(buf1, 'a', 510);
    const char *buf2 = "bbbbb";
    char buf3[515]; memset(buf3, 'a', 510); memcpy(buf3 + 510, buf2, strlen(buf2));

    const char *buf4 = "0bbbbb";
    char buf5[521]; memset(buf5, 'a', 510); memcpy(buf5 + 510, buf2, strlen(buf2)); memcpy(buf5 + 515, buf4, strlen(buf4));

    char *buf6= new char[515];
    char *buf7= new char[521];

    int fd;
    unlink(FILENAME);
    fd = open(FILENAME, O_EXCL | O_RDWR | O_CREAT, 0666);
    REQUIRE(fd >= 0);
    REQUIRE(write(fd, buf1, strlen(buf1)) == strlen(buf1));
    REQUIRE(close(fd) >= 0);

    //2.2.1
    fd = open(FILENAME, O_EXCL | O_RDWR, 0666);
    REQUIRE(fd >= 0);
    REQUIRE(lseek(fd, 510, SEEK_SET) == 510);
    REQUIRE(write(fd, buf2, strlen(buf2)) == strlen(buf2));
    REQUIRE(close(fd) >= 0);
    fd = open(FILENAME, O_EXCL | O_RDWR, 0666);
    REQUIRE(fd >= 0);
    REQUIRE(read(fd, buf6, 515) == 515);
    REQUIRE(memcmp(buf3, buf6, 515) == 0);
    REQUIRE(close(fd) >= 0);

    //2.3.1
    fd = open(FILENAME, O_EXCL | O_RDWR, 0666);
    REQUIRE(fd >= 0);
    REQUIRE(lseek(fd, 516, SEEK_SET) == 516);
    REQUIRE(write(fd, buf2, strlen(buf2)) == strlen(buf2));
    REQUIRE(close(fd) >= 0);
    fd = open(FILENAME, O_EXCL | O_RDWR, 0666);
    REQUIRE(fd >= 0);
    INFO("5: " << buf5);
    INFO("7: " << buf7);
    REQUIRE(read(fd, buf7, 521) == 521);
    REQUIRE(memcmp(buf5, buf7, 521) == 0);
    REQUIRE(close(fd) >= 0);

    delete [] buf6;
    delete [] buf7;
    REQUIRE(unlink(FILENAME) >= 0);
}

/*
 * read:
 */

TEST_CASE("T-ut9", "[Part_2]") {
    printf("Testcase integrationtest 9: read 1.1.1 and 1.1.2\n");
    const char *buf1 = "aaaaabbbbbccccc";

    const char *buf2 = "aabb";
    const char *buf3 = "aabbbbbccccc";

    char *buf4= new char[strlen(buf2)];
    char *buf5= new char[strlen(buf3)];

    int fd;
    unlink(FILENAME);
    fd = open(FILENAME, O_EXCL | O_RDWR | O_CREAT, 0666);
    REQUIRE(fd >= 0);
    REQUIRE(write(fd, buf1, strlen(buf1)) == strlen(buf1));
    REQUIRE(close(fd) >= 0);

    //1.1.1
    fd = open(FILENAME, O_EXCL | O_RDWR, 0666);
    REQUIRE(fd >= 0);
    REQUIRE(lseek(fd, 3, SEEK_SET) == 3);
    REQUIRE(read(fd, buf4, strlen(buf2)) == strlen(buf2));
    REQUIRE(memcmp(buf2, buf4, strlen(buf2)) == 0);
    REQUIRE(close(fd) >= 0);

    //1.1.2
    fd = open(FILENAME, O_EXCL | O_RDWR, 0666);
    REQUIRE(fd >= 0);
    REQUIRE(lseek(fd, 3, SEEK_SET) == 3);
    REQUIRE(read(fd, buf5, strlen(buf3)) == strlen(buf3));
    REQUIRE(memcmp(buf3, buf5, strlen(buf3)) == 0);
    REQUIRE(close(fd) >= 0);

    delete [] buf4;
    delete [] buf5;
    REQUIRE(unlink(FILENAME) >= 0);
}

TEST_CASE("T-ut10", "[Part_2]") {
    printf("Testcase integrationtest 10: read 2.1.1 and 2.1.2\n");
    char buf1[520]; memset(buf1, 'a', 512);
    const char *buf2 = "bbbbbccc";
    memcpy(buf1 + 512, buf2, strlen(buf2));

    const char *buf3 = "aaaaabbbbb";
    const char *buf4 = "aaaaabbbbbccc";

    char *buf5= new char[10];
    char *buf6= new char[15];

    int fd;
    unlink(FILENAME);
    fd = open(FILENAME, O_EXCL | O_RDWR | O_CREAT, 0666);
    REQUIRE(fd >= 0);
    REQUIRE(write(fd, buf1, strlen(buf1)) == strlen(buf1));
    REQUIRE(close(fd) >= 0);

    //1.1.1
    fd = open(FILENAME, O_EXCL | O_RDWR, 0666);
    REQUIRE(fd >= 0);
    INFO("3: " << buf3);
    INFO("5: " << buf5);
    REQUIRE(lseek(fd, 507, SEEK_SET) == 507);
    REQUIRE(read(fd, buf5, 10) == 10);
    REQUIRE(memcmp(buf3, buf5, 10) == 0);
    REQUIRE(close(fd) >= 0);

    //1.1.2
    fd = open(FILENAME, O_EXCL | O_RDWR, 0666);
    REQUIRE(fd >= 0);
    REQUIRE(lseek(fd, 507, SEEK_SET) == 507);
    REQUIRE(read(fd, buf6, 15) == 13);
    REQUIRE(memcmp(buf4, buf6, 13) == 0);
    REQUIRE(close(fd) >= 0);

    delete [] buf5;
    delete [] buf6;
    REQUIRE(unlink(FILENAME) >= 0);
}

/*TEST_CASE("T-ut11", "[Part_2]") {
    printf("Testcase integrationtest 11: unlink file although its open\n");
    int fd;
    char* w= new char[SMALL_SIZE];
    memset(w, 0, SMALL_SIZE);

    unlink(UNLINKFILE);
    fd = open(UNLINKFILE, O_EXCL | O_RDWR | O_CREAT, 0666);
    REQUIRE(fd >= 0);
    REQUIRE(unlink(UNLINKFILE) >= 0);
}*/