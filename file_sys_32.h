#ifndef FAT32_IMPL_H
#define FAT32_IMPL_H

#include "file.h"
#include <stdbool.h>

//Maximum Buffer Size for reading Input
#define MAX_BUF 10000

void readBytesToVar(int fd, uint64_t byte_position, uint64_t num_bytes_to_read, void *destination);

void validateFAT32BPB();

void initializeStructs();

void setRootDirectory();

void printCharToBuffer(char dest[], char info[], int length);

void print_directory_details();

void deviceInfo();

void openDisk(char *drive_location);

void get_file_from_current_directory(char *f_name);

uint64_t get_byte_location_from_cluster_number(fat32BS *bs, uint64_t clus_num);

char *convert_file_entry_name(char entry_name[]);

bool is_printable_entry(fat32DE *d);

bool is_dir_name_valid(char *dir_name);

uint64_t getClusterNumber(uint16_t high, uint16_t low);

uint64_t calculate_fat_entry_for_cluster(fat32BS *bs, uint64_t next_clus);

bool listing_is_readable_file(fat32DE *listing);

uint64_t calculate_cluster_count(fat32BS *bs, uint64_t RootDirSectors);

uint64_t getClusterSize(fat32BS *bs);

bool is_attr_directory(uint8_t dir_attr);

bool is_attr_hidden(uint8_t dir_attr);

#endif
