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

static fat32DE *currDir = NULL; //the current directory in the navigation
static fat32BS *bs = NULL;      //bpb holder
static int fd = -1;             //file descriptor for directory
static FSInfo *fsInfo = NULL;
uint32_t dataByteStart;
char filePath[30][250];

void openDisk(char *drive_location)
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

// load the BBoot Sector and BPB Structure and FAT32 FSInfo Sector, then call helper function to validate BPB parameters
void initializeStructs()
{
    bs = (fat32BS *)malloc(sizeof(fat32BS));
    assert(bs != NULL);
    readBytesToVar(fd, BPB_ROOT, sizeof(fat32BS), bs);
    validateFAT32BPB();
    fsInfo = (FSInfo *)malloc(sizeof(FSInfo));
    assert(fsInfo != NULL);
    readBytesToVar(fd, BPB_ROOT + sizeof(fat32BS), sizeof(FSInfo), fsInfo);
}

/*
    Read bytes from a device into a variable.
*/
void readBytesToVar(int fd, uint64_t byte_position, uint64_t num_bytes_to_read, void *destination)
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

void validateFAT32BPB()
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
    readBytesToVar(fd, sector_510_bytes, sizeof(uint16_t), &fat32_signature);
    // printf("I got signature %d\n", fat32_signature);
    assert(fat32_signature == FAT32_SIGNATURE);
    if (fat32_signature != FAT32_SIGNATURE)
    {
        printf("Invalid fat32 signature");
        exit(EXIT_FAILURE);
    }
}

void setRootDirectory()
{
    uint64_t first_cluster_sector_bytes = get_byte_location_from_cluster_number(bs, bs->BPB_RootClus);
    currDir = (fat32DE *)malloc(sizeof(struct fat32DE_struct));
    assert(currDir != NULL);
    readBytesToVar(fd, first_cluster_sector_bytes, sizeof(struct fat32DE_struct), currDir);
}

/*
    Create a string from an array of characters.
    Null terminate after copying.
*/
void printCharToBuffer(char dest[], char info[], int length)
{
    int i;
    for (i = 0; i < length; i++)
    {
        dest[i] = info[i];
    }
    dest[length] = 0;
}

//  Calculates and prints the fd info
void deviceInfo()
{
    uint32_t to_kb = 1000;
    uint32_t usable_space = (bs->BPB_TotSec32 - (uint32_t)bs->BPB_RsvdSecCnt - ((uint32_t)bs->BPB_NumFATs * bs->BPB_FATSz32)) * (uint32_t)bs->BPB_BytesPerSec;
    uint32_t bytes_per_cluster = (uint32_t)bs->BPB_BytesPerSec * (uint32_t)bs->BPB_SecPerClus;
    uint32_t total_bytes = (uint32_t)bs->BPB_TotSec32 * (uint32_t)bs->BPB_BytesPerSec;
    uint32_t free_clusters = fsInfo->FSI_Free_Count;
    uint32_t free_space = (free_clusters * (uint32_t)bs->BPB_SecPerClus * (uint32_t)bs->BPB_BytesPerSec) / to_kb;
    char printBuf[MAX_BUF];
    printf("---Device Info---\n");
    printCharToBuffer(printBuf, bs->BS_OEMName, BS_OEMName_LENGTH);
    printf("OEM Name: %s\n", printBuf);
    printCharToBuffer(printBuf, bs->BS_VolLab, BS_VolLab_LENGTH);
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
uint64_t getClusterNumber(uint16_t high, uint16_t low)
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
    assert(currDir != NULL); //Make sure this has been initialized
    assert(bs != NULL);

    char printBuf[MAX_BUF];
    long read_size = bs->BPB_SecPerClus * (uint64_t)bs->BPB_BytesPerSec; // TODO Refactor
    char contents[read_size];
    // printf("Low and high %d and %d \n", currDir->DIR_FstClusHI, currDir->DIR_FstClusLO);
    uint64_t next_cluster = getClusterNumber(currDir->DIR_FstClusHI, currDir->DIR_FstClusLO);
    uint64_t file_byte_position = get_byte_location_from_cluster_number(bs, next_cluster);
    read_byte_location_into_buffer(fd, file_byte_position, contents, read_size);

    fat32DE *listing = (fat32DE *)contents;
    memcpy(currDir, listing, sizeof(struct fat32DE_struct));

    printCharToBuffer(printBuf, bs->BS_VolLab, BS_VolLab_LENGTH);
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
        readBytesToVar(fd, cluster_entry_bytes, sizeof(uint64_t), &next_cluster);
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

void downloadFile(fat32DE *listing, char *f_name)
{
    // Check if file has already been downloaded
    if (access(f_name, F_OK) != -1)
    {
        printf("%s has already been downloaded\n", f_name);
    }
    else
    {
        printf("Downloading file: %s\n", f_name);
        uint32_t currClus = getClusterNumber(listing->DIR_FstClusHI, listing->DIR_FstClusLO);
        uint32_t startRead = dataByteStart + (currClus - 2) * bs->BPB_SecPerClus * bs->BPB_BytesPerSec; //where to start reading from
        uint32_t toRead = getClusterSize(bs);
        bool keepReading = true;

        FILE *fp;
        fp = fopen(f_name, "w"); // Open file
        if (fp == NULL)
        {
            perror("Error opening file: ");
            exit(EXIT_FAILURE);
        }
        // Search fd and check if it worked
        assert(fd != -1);
        if (lseek(fd, startRead, SEEK_SET) < 0)
        {
            perror("Error lseeking: ");
            exit(EXIT_FAILURE);
        }

        while (keepReading)
        {
            char buffer[toRead];
            int charsRead = read(fd, buffer, toRead); // Read chars from fd and copy to buffer
            if (charsRead < 0)
            {
                perror("Error reading cluster");
                exit(EXIT_FAILURE);
            }

            if (buffer[0] == 0x00)
            { // Check if buffer is empty
                keepReading = false;
            }
            else
            {
                fwrite(buffer, sizeof(char), sizeof(char) * charsRead, fp); //write from buffer into file
            }
        }
        printf("File downloaded in directory\n");
        fclose(fp);
    }
}

void findFile(char pathFileName[10][250], int pathDepth, uint32_t offset, uint32_t cluster)
{

    int seek;
    int currSector;
    uint32_t currCluster;
    uint32_t nextClusSector;
    fat32DE *currFile = (fat32DE *)malloc(sizeof(fat32DE)); //current directory
    assert(currFile != NULL);

    int currSeek = lseek(fd, offset * bs->BPB_BytesPerSec, SEEK_SET); //get the current position on fd
    int loop = (bs->BPB_BytesPerSec * bs->BPB_SecPerClus) / 32;

    for (int i = 0; i < loop; i++)
    {

        seek = lseek(fd, currSeek + i * 32, SEEK_SET); //keep seeking through the fd

        if (seek == -1)
        { //check if seek failed

            printf("Error moving the pointer, cannot seek");
            exit(EXIT_FAILURE);
        }

        currSector = read(fd, currFile, sizeof(fat32DE)); //read the current directory from the fd

        if (currSector == -1)
        { //check if read failed

            printf("Error reading into directory struct");
            exit(EXIT_FAILURE);
        }

        if (currFile->DIR_Name[0] == -27)
        { //check if directory is empty or has contents

            continue;
        }
        else if (currFile->DIR_Name[0] == 0x00)
        {

            return;
        }

        if (!strncmp(currFile->DIR_Name, ".", 1) == 0 && !strncmp(currFile->DIR_Name, "..", 2) == 0)
        { //if we're in a directory

            //check if file name from path is equal to file name from fd
            if (compareFileNames(pathFileName[pathDepth], (char *)currFile->DIR_Name, currFile))
            {

                currCluster = (currFile->DIR_FstClusHI << 16) | currFile->DIR_FstClusLO;
                nextClusSector = (currCluster - 2) * bs->BPB_SecPerClus + dataSectorStart;

                //if file extensions are valid download file
                if (currFile->DIR_Ext[0] != 0x00 && currFile->DIR_Ext[0] != -1 && currFile->DIR_Ext[0] != 32)
                {

                    downloadFile(currFile, pathFileName[pathDepth]); //download file
                    close(fd);
                    exit(0);
                }
                pathDepth++;                                                //increase depth
                findFile(filePath, pathDepth, nextClusSector, currCluster); //recursively search for file
            }
        }
    }

    uint32_t lookUp = fatByteStart + cluster * 4;
    uint32_t nextCluster;

    lseek(fd, lookUp, SEEK_SET); //search next part of fd
    read(fd, &nextCluster, 4);   //read next cluster
    nextCluster = nextCluster & 0x0FFFFFFF;

    if (nextCluster != 0x0FFFFFFF)
    {

        findFile(filePath, pathDepth, (nextCluster - 2) * bs->BPB_SecPerClus + dataSectorStart, nextCluster); //recursively print
    }
    else if (nextCluster == 0x0FFFFFFF || (currFile->DIR_Name[0] & 0x00) == 0x00)
    { //no more directory entries

        printf("Error: File cannot be found");
    }
}

void get_file_from_current_directory(char *f_name)
{
    long read_size = bs->BPB_BytesPerSec * bs->BPB_SecPerClus;
    char contents[read_size];
    uint64_t next_cluster = getClusterNumber(currDir->DIR_FstClusHI, currDir->DIR_FstClusLO);
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
                    downloadFile(listing, f_name);
                    return;
                }
                free(compBuf);
            }
            listing++;
        }
        //Get the next cluster entry for the file
        uint64_t cluster_entry_bytes = calculate_fat_entry_for_cluster(bs, next_cluster);
        readBytesToVar(fd, cluster_entry_bytes, sizeof(uint64_t), &next_cluster);
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

uint64_t getClusterSize(fat32BS *bs)
{
    return bs->BPB_SecPerClus * (uint64_t)bs->BPB_BytesPerSec;
}
