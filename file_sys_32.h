#ifndef FAT32_IMPL_H
#define FAT32_IMPL_H

#include "file.h"
#include <stdbool.h>

//Maximum Buffer Size for reading Input
#define MAX_BUF 10000

void read_bytes_into_variable(int fd, uint64_t byte_position, void *destination, uint64_t num_bytes_to_read);

void validate_bpb_params();

void load_and_validate_bpb_params();

void set_root_dir_file_entry();

void print_directory_details();

void deviceInfo();

void open_device(char *drive_location);

void get_file_from_current_directory(char *f_name);

uint64_t get_byte_location_from_cluster_number(fat32BS *bs, uint64_t clus_num);

char *convert_file_entry_name(char entry_name[]);

bool is_printable_entry(fat32DE *d);

bool is_dir_name_valid(char *dir_name);

uint64_t convert_high_low_to_cluster_number(uint16_t high, uint16_t low);

uint64_t calculate_fat_entry_for_cluster(fat32BS *bs, uint64_t next_clus);

bool listing_is_readable_file(fat32DE *listing);

uint64_t calculate_cluster_count(fat32BS *bs, uint64_t RootDirSectors);

uint64_t get_cluster_size_bytes(fat32BS *bs);

bool is_attr_directory(uint8_t dir_attr);

bool is_attr_hidden(uint8_t dir_attr);

#endif
