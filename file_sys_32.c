#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>
// #include "common.h"
#include "file.h"
#include "file_sys_32.h"

static fat32DE *curr_dir = NULL; //the current directory in the navigation
static fat32BS *bs = NULL;       //bpb holder
static int fd = -1;              //file descriptor for directory
static FSInfo *fs_info = NULL;

void open_device(char *drive_location)
{
    fd = open(drive_location, O_RDONLY);
    if (fd < 0)
    {
        printf("Cannot open %s\n", drive_location);
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("Successfully opened %s\n", drive_location);
    }
}

void load_and_validate_bpb_params()
{
    bs = (fat32BS *)malloc(sizeof(struct fat32BS_struct));
    assert(bs != NULL);
    read_bytes_into_variable(fd, BPB_ROOT, bs, sizeof(struct fat32BS_struct));
    validate_bpb_params();
    fs_info = (FSInfo *)malloc(sizeof(FSInfo));
    read_bytes_into_variable(fd, BPB_ROOT + sizeof(fat32BS), fs_info, sizeof(FSInfo));
}

/*
    Read bytes from a device into a variable.
*/
void read_bytes_into_variable(int fd, uint64_t byte_position, void *destination, uint64_t num_bytes_to_read)
{
    assert(fd != -1);
    if (lseek(fd, byte_position, SEEK_SET) < 0)
    {
        perror("Error lseeking: ");
        exit(EXIT_FAILURE);
    }
    int bytes_read = read(fd, destination, num_bytes_to_read);
    if (bytes_read < 0)
    {
        perror("Error reading bytes");
        exit(EXIT_FAILURE);
    }
}

void validate_bpb_params()
{
    assert(bs != NULL);
    uint64_t RootDirSectors = ((bs->BPB_RootEntCnt * (uint64_t)32) + (bs->BPB_BytesPerSec - 1)) / bs->BPB_BytesPerSec;
    assert(RootDirSectors == FAT32_ROOT_DIR_SECTORS);
    if (RootDirSectors != FAT32_ROOT_DIR_SECTORS)
    {
        printf("Invalid fat type. Please enter a FAT32 Volume");
        exit(EXIT_FAILURE);
    }
    uint64_t CountofClusters = calculate_cluster_count(bs, RootDirSectors);
    // printf("The cluster count is %llu\n", CountofClusters);
    assert(CountofClusters >= MIN_FAT32_CLUSTER_COUNT);
    // if the count of clusters is less than 4085, it is a FAT12 Volume
    if (CountofClusters < MIN_FAT16_CLUSTER_COUNT)
    {
        printf("This Volume is FAT12 . Please enter a FAT32 Volume\n");
        exit(EXIT_FAILURE);
    }
    // if the count of clusters is less than 65525, it is a FAT16 Volume
    else if (CountofClusters < MIN_FAT32_CLUSTER_COUNT)
    {
        printf("This Volume is FAT16. Please enter a FAT32 Volume\n");
        exit(EXIT_FAILURE);
    }
    // it should be a FAT32 Volume but extra checks will be taken
    uint64_t sector_510_bytes = 510;
    uint16_t fat32_signature;
    read_bytes_into_variable(fd, sector_510_bytes, &fat32_signature, sizeof(uint16_t));
    // printf("I got signature %d\n", fat32_signature);
    assert(fat32_signature == FAT32_SIGNATURE);
    if (fat32_signature != FAT32_SIGNATURE)
    {
        printf("Invalid fat32 signature");
        exit(EXIT_FAILURE);
    }
}

void set_root_dir_file_entry()
{
    uint64_t first_cluster_sector_bytes = get_byte_location_from_cluster_number(bs, bs->BPB_RootClus);
    curr_dir = (fat32DE *)malloc(sizeof(struct fat32DE_struct));
    assert(curr_dir != NULL);
    read_bytes_into_variable(fd, first_cluster_sector_bytes, curr_dir, sizeof(struct fat32DE_struct));
}

/*
    Create a string from an array of characters.
    Null terminate after copying.
*/
void print_chars_into_buffer(char dest[], char info[], int length)
{
    int i;
    for (i = 0; i < length; i++)
    {
        dest[i] = info[i];
    }
    dest[length] = 0;
}

void print_fat32_device_info()
{
    uint32_t to_kb = 1024;
    uint32_t usable_space = (bs->BPB_TotSec32 - (uint32_t)bs->BPB_RsvdSecCnt - ((uint32_t)bs->BPB_NumFATs * bs->BPB_FATSz32)) * (uint32_t)bs->BPB_BytesPerSec;
    uint32_t bytes_per_cluster = (uint32_t)bs->BPB_BytesPerSec * (uint32_t)bs->BPB_SecPerClus;
    uint32_t total_bytes = (uint32_t)bs->BPB_TotSec32 * (uint32_t)bs->BPB_BytesPerSec;
    uint32_t free_clusters = fs_info->FSI_Free_Count;
    uint32_t free_space = (free_clusters * (uint32_t)bs->BPB_SecPerClus * (uint32_t)bs->BPB_BytesPerSec) / to_kb;
    char printBuf[MAX_BUF];
    printf("---Device Info---\n");
    print_chars_into_buffer(printBuf, bs->BS_OEMName, BS_OEMName_LENGTH);
    printf("OEM Name: %s\n", printBuf);
    print_chars_into_buffer(printBuf, bs->BS_VolLab, BS_VolLab_LENGTH);
    printf("Volume Label: %s\n", printBuf);
    printf("Free Space: %d kb\n", free_space);
    printf("Usable Storage: %d bytes\n", usable_space);
    printf("Cluster Size: \n\tNumber of Sectors: %d\n\tNumber of bytes: %d\n", bs->BPB_SecPerClus, bytes_per_cluster);
    printf("Total Bytes on Drive: %d\n", total_bytes);
}

/*
    Uses the high bit and low bit to calculate the next cluster number.
    This function will return 2 if the 
*/
uint64_t convert_high_low_to_cluster_number(uint16_t high, uint16_t low)
{
    uint64_t clus_num = ((uint64_t)high) << 16; //Shift by 8 bits
    clus_num = clus_num | low;
    return clus_num;
}

uint64_t get_byte_location_from_cluster_number(fat32BS *bs, uint64_t clus_num)
{
    uint64_t first_data_sector = bs->BPB_RsvdSecCnt + (bs->BPB_NumFATs * (uint64_t)bs->BPB_FATSz32);
    if (clus_num == 0)
    {
        //0 is not representative of a cluster so we have to ensure we are
        //checking the root cluster
        clus_num = bs->BPB_RootClus;
    }
    uint64_t cluster_sector = ((clus_num - 2) * bs->BPB_SecPerClus) + first_data_sector;
    uint64_t byte_location = cluster_sector * bs->BPB_BytesPerSec;
    return byte_location;
}

/*
    Given a byte location, read the contents into a buffer.
*/
void read_byte_location_into_buffer(int fd, uint64_t byte_position, char buffer[], uint64_t chars_to_read)
{
    assert(fd != -1);
    if (lseek(fd, byte_position, SEEK_SET) < 0)
    {
        perror("Error lseeking: ");
        exit(EXIT_FAILURE);
    }
    int chars_read = read(fd, buffer, chars_to_read);
    if (chars_read < 0)
    {
        perror("Error reading cluster");
        exit(EXIT_FAILURE);
    }
}

/*
    Given a file descriptor and a byte position in a
    ffat32 device, read the contents into the file.
*/
void read_byte_location_into_file(int fd, FILE *fp, uint64_t byte_position, uint64_t chars_to_read)
{
    char buffer[chars_to_read];
    assert(fd != -1);
    if (lseek(fd, byte_position, SEEK_SET) < 0)
    {
        perror("Error lseeking: ");
        exit(EXIT_FAILURE);
    }
    int chars_read = read(fd, buffer, chars_to_read);
    if (chars_read < 0)
    {
        perror("Error reading cluster");
        exit(EXIT_FAILURE);
    }
    fwrite(buffer, sizeof(char), sizeof(char) * chars_read, fp);
    // read_byte_location_into_buffer(fd,byte_position,);
}

bool is_dir_name_valid(char *dir_name)
{
    return !((uint8_t)(dir_name[0]) == 0x05 || (uint8_t)(dir_name[0]) == 0xE5);
}

bool is_printable_entry(fat32DE *d)
{
    return is_dir_name_valid(d->DIR_Name) //name should be valid
           // && (d->DIR_Attr & ATTR_READ_ONLY) == 0 //not read only
           && (d->DIR_Attr & ATTR_HIDDEN) == 0     // not hidden
           && (d->DIR_Attr & ATTR_VOLUME_ID) == 0; //not the root directory
}

void print_directory_details()
{
    assert(curr_dir != NULL); //Make sure this has been initialized
    assert(bs != NULL);
    char printBuf[MAX_BUF];
    long read_size = bs->BPB_SecPerClus * (uint64_t)bs->BPB_BytesPerSec; // TODO Refactor
    char contents[read_size];
    // printf("Low and high %d and %d \n", curr_dir->DIR_FstClusHI, curr_dir->DIR_FstClusLO);
    uint64_t next_cluster = convert_high_low_to_cluster_number(curr_dir->DIR_FstClusHI, curr_dir->DIR_FstClusLO);
    uint64_t file_byte_position = get_byte_location_from_cluster_number(bs, next_cluster);
    read_byte_location_into_buffer(fd, file_byte_position, contents, read_size);

    fat32DE *listing = (fat32DE *)contents;
    memcpy(curr_dir, listing, sizeof(struct fat32DE_struct));

    print_chars_into_buffer(printBuf, bs->BS_VolLab, BS_VolLab_LENGTH);
    // printf("Volume: %s\n", printBuf);
    uint64_t num_bytes_in_cluster = bs->BPB_BytesPerSec * (uint64_t)bs->BPB_SecPerClus;
    int total_lines = num_bytes_in_cluster / 32;
    while (true)
    {
        int lines_read = 0; //We shouldn't read more than a certain number of lines per sector

        //We print out all directory entries in the current cluster
        while (listing->DIR_Name[0] && (lines_read++) < total_lines)
        {
            // bool entry_valid = is_printable_entry(listing);
            if (is_printable_entry(listing))
            {

                char *printableEntryName = convert_file_entry_name(listing->DIR_Name);
                char *file_end = is_attr_directory(listing->DIR_Attr) ? "/" : "";
                printf("%-16s %d%s\n", printableEntryName, listing->DIR_FileSize, file_end);
                free(printableEntryName);
            }
            listing++;
        }
        //Get the next cluster entry for the file
        uint64_t cluster_entry_bytes = calculate_fat_entry_for_cluster(bs, next_cluster);
        read_bytes_into_variable(fd, cluster_entry_bytes, &next_cluster, sizeof(uint64_t));
        next_cluster = next_cluster & NEXT_CLUSTER_MASK;
        if (next_cluster >= MAX_CLUSTER_NUMBER)
            break; //break if there is no more cluster

        //update to the next cluster for the directory
        file_byte_position = get_byte_location_from_cluster_number(bs, next_cluster);
        read_byte_location_into_buffer(fd, file_byte_position, contents, read_size);
        listing = (fat32DE *)contents;
    }
}

// uint64_t calculate_root_dir_sectors(fat32BS *bs)
// {
//     uint64_t RootDirSectors = ((bs->BPB_RootEntCnt * (uint64_t)32) + (bs->BPB_BytesPerSec - 1)) / bs->BPB_BytesPerSec;
//     return RootDirSectors;
// }

uint64_t calculate_cluster_count(fat32BS *bs, uint64_t RootDirSectors)
{
    // uint64_t RootDirSectors = ((bs->BPB_RootEntCnt * (uint64_t)32) + (bs->BPB_BytesPerSec - 1)) / bs->BPB_BytesPerSec;
    uint64_t FATSz;
    if (bs->BPB_FATSz16 != 0)
    {
        FATSz = bs->BPB_FATSz16;
    }
    else
    {
        FATSz = bs->BPB_FATSz32;
    }
    uint64_t TotSec;
    // check if the count of sect for FAT16 is not set to zero, if so set the count of sect to that value
    if (bs->BPB_TotSec16 != 0)
    {
        TotSec = bs->BPB_TotSec16;
    }
    // if the count of sect for FAT16 is set to zero set the the count of sect for FAT32 to be out count of sect
    else
    {
        TotSec = bs->BPB_TotSec32;
    }
    //Not order of casting. Must be 64 bit
    uint64_t DataSec = TotSec - (bs->BPB_RsvdSecCnt + (bs->BPB_NumFATs * (uint64_t)FATSz) + RootDirSectors);
    uint64_t CountofClusters = DataSec / bs->BPB_SecPerClus;
    return CountofClusters;
}

uint64_t calculate_fat_entry_for_cluster(fat32BS *bs, uint64_t next_clus)
{
    uint64_t FATOffset = next_clus * 4;
    uint64_t ThisFATSecNum = bs->BPB_RsvdSecCnt + (FATOffset / bs->BPB_BytesPerSec);
    uint64_t ThisFATEntOffset = FATOffset % bs->BPB_BytesPerSec;
    uint64_t NextListing = (ThisFATSecNum * bs->BPB_BytesPerSec) + ThisFATEntOffset;
    return NextListing;
}

bool listing_is_readable_file(fat32DE *listing)
{
    uint8_t dir_attr = listing->DIR_Attr;
    return is_dir_name_valid(listing->DIR_Name) &&
           !is_attr_directory(dir_attr) && //is not a directory
           !is_attr_hidden(dir_attr);      // is not hidden
}

char *convert_file_entry_name(char entry_name[])
{
    char *retBuf = (char *)malloc(sizeof(char) * DIR_Name_LENGTH);
    int i;
    int p_count = 0;
    for (i = 0; i < 8 && entry_name[i] != ' '; i++)
    {
        retBuf[i] = entry_name[i];
        p_count++;
    }
    if (entry_name[8] != ' ')
        retBuf[p_count++] = '.';
    for (i = 8; i < 11 && entry_name[i] != ' '; i++)
    {
        retBuf[p_count++] = entry_name[i];
    }
    retBuf[p_count++] = 0;
    return retBuf;
}

void download_file(fat32DE *listing, char *f_name)
{
    printf("Downloading %s\n", f_name);
    uint64_t size = listing->DIR_FileSize;
    uint64_t curr_clus = convert_high_low_to_cluster_number(listing->DIR_FstClusHI, listing->DIR_FstClusLO);
    FILE *fp;
    fp = fopen(f_name, "w");
    if (fp == NULL)
    {
        perror("Error opening file: ");
        exit(EXIT_FAILURE);
    }
    uint64_t CLUSTER_SIZE_BYTES = get_cluster_size_bytes(bs);
    // printf("I shall be reading %llu \n", CLUSTER_SIZE_BYTES);
    while (size > 0 && curr_clus < MAX_CLUSTER_NUMBER)
    {

        //we are reading this amount of characters
        uint64_t to_read = CLUSTER_SIZE_BYTES;
        if (size < to_read)
        {
            to_read = size;
        }
        size -= to_read;
        //We start reading from this cluster
        uint64_t byte_location = get_byte_location_from_cluster_number(bs, curr_clus);

        // printf("First cluster is %llu\n", curr_clus);
        //get the next cluster
        uint64_t next_clus_bytes = calculate_fat_entry_for_cluster(bs, curr_clus);
        // printf("Next cluster bytes is %llu \n", next_clus_bytes);
        uint64_t next_clus;
        read_bytes_into_variable(fd, next_clus_bytes, &next_clus, sizeof(uint64_t));
        next_clus = next_clus & NEXT_CLUSTER_MASK;
        //THIS PART HANDLES READING IN OF SEQUENTIAL CLUSTERS
        while (next_clus < MAX_CLUSTER_NUMBER && next_clus == curr_clus + 1)
        {
            // printf("Reading an extra cluster %llu\n", next_clus);
            curr_clus = next_clus;
            assert(size >= 0); //We can't have to read an empty cluster
            if (size >= CLUSTER_SIZE_BYTES)
            {
                size -= CLUSTER_SIZE_BYTES;
                to_read += CLUSTER_SIZE_BYTES;
            }
            else
            {
                to_read += size;
                size = 0;
            }
            next_clus_bytes = calculate_fat_entry_for_cluster(bs, next_clus);
            read_bytes_into_variable(fd, next_clus_bytes, &next_clus, sizeof(uint64_t));
            next_clus = next_clus & NEXT_CLUSTER_MASK;
        }
        curr_clus = next_clus;
        read_byte_location_into_file(fd, fp, byte_location, to_read);
    }
    printf("File write successful\n");
    fclose(fp);
}

void get_file_from_current_directory(char *f_name)
{

    long read_size = bs->BPB_BytesPerSec * bs->BPB_SecPerClus;
    char contents[read_size];
    uint64_t next_cluster = convert_high_low_to_cluster_number(curr_dir->DIR_FstClusHI, curr_dir->DIR_FstClusLO);
    uint64_t file_byte_position = get_byte_location_from_cluster_number(bs, next_cluster);
    read_byte_location_into_buffer(fd, file_byte_position, contents, read_size);

    fat32DE *listing = (fat32DE *)contents;
    //TODO Refactor
    while (true)
    {
        while (listing->DIR_Name[0])
        {
            if (listing_is_readable_file(listing))
            {
                char *compBuf = convert_file_entry_name(listing->DIR_Name);
                if (strcmp(f_name, compBuf) == 0)
                {
                    // printf("Found file %s\n", printBuf);
                    // printf("File start %d %d\n", listing->DIR_FstClusHI, listing->DIR_FstClusLO);
                    download_file(listing, f_name);
                    return;
                }
                free(compBuf);
            }
            listing++;
        }
        //Get the next cluster entry for the file
        uint64_t cluster_entry_bytes = calculate_fat_entry_for_cluster(bs, next_cluster);
        read_bytes_into_variable(fd, cluster_entry_bytes, &next_cluster, sizeof(uint64_t));
        next_cluster = next_cluster & NEXT_CLUSTER_MASK;
        if (next_cluster >= MAX_CLUSTER_NUMBER)
            break; //break if there is no more cluster

        //update to the next cluster for the directory
        file_byte_position = get_byte_location_from_cluster_number(bs, next_cluster);
        read_byte_location_into_buffer(fd, file_byte_position, contents, read_size);
        listing = (fat32DE *)contents;
    }
    printf("File %s doesn't exist in current directory\n", f_name);
}

bool is_attr_directory(uint8_t dir_attr)
{
    return (dir_attr & ATTR_DIRECTORY) != false;
}

bool is_attr_hidden(uint8_t dir_attr)
{
    return (dir_attr & ATTR_HIDDEN) != false;
}

uint64_t get_cluster_size_bytes(fat32BS *bs)
{
    return bs->BPB_SecPerClus * (uint64_t)bs->BPB_BytesPerSec;
}
