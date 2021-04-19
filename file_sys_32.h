#ifndef FAT32_IMPL_H
#define FAT32_IMPL_H

#include "fat32.h"
#include "file.h"
#include <stdbool.h>

//Maximum Buffer Size for reading Input
#define MAX_BUF 10000

void openDisk(char *drive_location);

void initializeStructs();

void readBytesToVar(int fd, uint64_t byte_position, uint64_t num_bytes_to_read, void *destination);

void validateFAT32BPB();

void setRootDirectory();

void printCharToBuffer(char dest[], char info[], int length);

void deviceInfo();

uint64_t getClusterNumber(uint16_t high, uint16_t low);

uint64_t getByteLocationFromClusterNumb(fat32BootSector *bs, uint64_t clus_num);

void readByteLocationToBuffer(int fd, uint64_t byte_position, char buffer[], uint64_t chars_to_read);

void readByteLocationToFile(int fd, FILE *fp, uint64_t byte_position, uint64_t chars_to_read);

bool isDIRValid(char *dir_name);

bool isPrintableEntry(fat32DE *d);

uint64_t getClusterCount(fat32BootSector *bs, uint64_t RootDirSectors);

uint64_t findNextListing(fat32BootSector *bs, uint64_t next_clus);

bool isReadable(fat32DE *listing);

uint32_t getFatByteStart();

uint32_t getDataSectorStart();

bool isDirectory(uint8_t dir_attr);

bool isHidden(uint8_t dir_attr);

char *trim(char *str, const char *seps);

char *getNames(fat32DE *currFile);

void printContents();

void printDirContents(int level, uint32_t offset, uint32_t cluster);

#endif
