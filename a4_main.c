#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/stat.h>
#include "file.h"
#include "file_sys_32.h"

static fat32DE *currDir = NULL;    //the current directory in the navigation
static fat32BootSector *bs = NULL; //bpb holder
static int fd = -1;                //file descriptor for directory
static FSInfo *fsInfo = NULL;
uint32_t dataByteStart;
char filePath[30][250];
char *name;

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
    bs = (fat32BootSector *)malloc(sizeof(fat32BootSector));
    assert(bs != NULL);
    readBytesToVar(fd, BPB_ROOT, sizeof(fat32BootSector), bs);
    validateFAT32BPB();
    fsInfo = (FSInfo *)malloc(sizeof(FSInfo));
    assert(fsInfo != NULL);
    readBytesToVar(fd, BPB_ROOT + sizeof(fat32BootSector), sizeof(FSInfo), fsInfo);
}

// Read bytes from a device into a variable.
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

// validate the header of the FAT32 file
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
    uint64_t CountofClusters = getClusterCount(bs, RootDirSectors);
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
    assert(fat32_signature == FAT32_SIGNATURE);
    if (fat32_signature != FAT32_SIGNATURE)
    {
        printf("Invalid fat32 signature");
        exit(EXIT_FAILURE);
    }
}

void setRootDirectory()
{
    uint64_t first_cluster_sector_bytes = getByteLocationFromClusterNumb(bs, bs->BPB_RootClus);
    currDir = (fat32DE *)malloc(sizeof(fat32DE));
    assert(currDir != NULL);
    readBytesToVar(fd, first_cluster_sector_bytes, sizeof(fat32DE), currDir);
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

uint64_t getByteLocationFromClusterNumb(fat32BootSector *bs, uint64_t clus_num)
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
void readByteLocationToBuffer(int fd, uint64_t byte_position, char buffer[], uint64_t chars_to_read)
{
    assert(fd != -1);
    if (lseek(fd, byte_position, SEEK_SET) < 0)
    {
        printf("Error lseeking: ");
        exit(EXIT_FAILURE);
    }
    int chars_read = read(fd, buffer, chars_to_read);
    if (chars_read < 0)
    {
        printf("Error reading cluster");
        exit(EXIT_FAILURE);
    }
}

/*
    Given a file descriptor and a byte position in a
    ffat32 device, read the contents into the file.
*/
void readByteLocationToFile(int fd, FILE *fp, uint64_t byte_position, uint64_t chars_to_read)
{
    char buffer[chars_to_read];
    assert(fd != -1);
    if (lseek(fd, byte_position, SEEK_SET) < 0)
    {
        printf("Error lseeking: ");
        exit(EXIT_FAILURE);
    }
    int chars_read = read(fd, buffer, chars_to_read);
    if (chars_read < 0)
    {
        printf("Error reading cluster");
        exit(EXIT_FAILURE);
    }
    fwrite(buffer, sizeof(char), sizeof(char) * chars_read, fp);
}

// checks if it is a valid directory

bool isDIRValid(char *dir_name)
{
    return !((uint8_t)(dir_name[0]) == 0x05 || (uint8_t)(dir_name[0]) == 0xE5);
}

bool isPrintableEntry(fat32DE *d)
{
    return isDIRValid(d->DIR_Name) //name should be valid
           // && (d->DIR_Attr & ATTR_READ_ONLY) == 0 //not read only
           && (d->DIR_Attr & ATTR_HIDDEN) == 0     // not hidden
           && (d->DIR_Attr & ATTR_VOLUME_ID) == 0; //not the root directory
}

uint64_t getClusterCount(fat32BootSector *bs, uint64_t RootDirSectors)
{
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

// find next listing based on cluster number
uint64_t findNextListing(fat32BootSector *bs, uint64_t next_clus)
{
    uint64_t FATOffset = next_clus * 4;
    uint64_t ThisFATSecNum = bs->BPB_RsvdSecCnt + (FATOffset / bs->BPB_BytesPerSec);
    uint64_t ThisFATEntOffset = FATOffset % bs->BPB_BytesPerSec;
    uint64_t NextListing = (ThisFATSecNum * bs->BPB_BytesPerSec) + ThisFATEntOffset;
    return NextListing;
}

bool isReadable(fat32DE *listing)
{
    uint8_t dir_attr = listing->DIR_Attr;
    return isDIRValid(listing->DIR_Name) &&
           !isDirectory(dir_attr) && //is not a directory
           !isHidden(dir_attr);      // is not hidden
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

uint32_t getFatByteStart()
{
    return (bs->BPB_RsvdSecCnt * bs->BPB_BytesPerSec);
}

uint32_t getDataSectorStart()
{
    return bs->BPB_RsvdSecCnt + (bs->BPB_NumFATs * bs->BPB_FATSz32);
}

bool compareFileNames(char *pathFileName, char *diskFileName, fat32DE *currFile)
{

    bool result = true; //return true if names are equal

    for (int i = 0; pathFileName[i] != '.' && pathFileName[i] != '\0' && diskFileName[i] != '~'; i++)
    {

        if (pathFileName[i] != diskFileName[i])
        { //if file names are not equal set result to false

            result = false;
        }
    }

    int offset = strlen(pathFileName) - 4;

    //check if file extension is valid
    if (result && currFile->DIR_Name[8] != 0x00 && currFile->DIR_Name[8] != -1 && currFile->DIR_Name[8] != 32)
    {

        for (int j = offset + 1; j < (int)strlen(pathFileName) - 1; j++)
        {

            if (pathFileName[j + 1] != currFile->DIR_Name[8 + j - offset])
            { //if file extensions are not equal set result to false

                result = false;
            }
        }
    }

    return result;
}

bool isDirectory(uint8_t dir_attr)
{
    return (dir_attr & ATTR_DIRECTORY) != false;
}

bool isHidden(uint8_t dir_attr)
{
    return (dir_attr & ATTR_HIDDEN) != false;
}

uint64_t getClusterSize(fat32BootSector *bs)
{
    return bs->BPB_SecPerClus * (uint64_t)bs->BPB_BytesPerSec;
}

char *rtrim(char *str, const char *seps)
{

    if (seps == NULL)
    {

        seps = "\t\n\v\f\r ";
    }

    int i = strlen(str) - 1;
    while (i >= 0 && strchr(seps, str[i]) != NULL)
    {

        str[i] = '\0';
        i--;
    }

    return str;
}

char *printNames(fat32DE *currFile)
{
    // char fileFormat[3];
    char dot[2];

    name = (char *)malloc(sizeof(char) * 11);
    dot[0] = '.';
    dot[1] = '\0';
    memcpy(name, currFile->DIR_Name, 11); //copy name from the disk into name string
    name = rtrim(name, NULL);

    return name;
}

void printContents()
{
    printDirContents(0, getDataSectorStart(), bs->BPB_RootClus);
}

void printDirContents(int level, uint32_t offset, uint32_t cluster)
{

    int seek;
    int currSector;
    int currLevel = level;
    char *dash;
    uint32_t currCluster;
    uint32_t nextClusSector;
    fat32DE *currFile = (fat32DE *)malloc(sizeof(fat32DE)); //current directory
    assert(currFile != NULL);

    int currSeek = lseek(fd, offset * bs->BPB_BytesPerSec, SEEK_SET); //get the current position on disk
    int loop = (bs->BPB_BytesPerSec * (uint64_t)bs->BPB_SecPerClus) / 32;

    for (int i = 0; i < loop; i++)
    {

        seek = lseek(fd, currSeek + i * 32, SEEK_SET); //keep seeking through the disk

        if (seek == -1)
        { //seek failed

            printf("Error: Failed to seek");
            exit(EXIT_FAILURE);
        }

        currSector = read(fd, currFile, sizeof(fat32DE)); //read the current directory from the disk

        if (currSector == -1)
        { //failed to read

            printf("Error: Failed to read directory struct");
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

        if (strncmp(currFile->DIR_Name, ".", 1) != 0 && strncmp(currFile->DIR_Name, "..", 2) != 0)
        { //if we're in a directory

            dash = (char *)malloc(sizeof(char) * 5);
            for (int i = 0; i < currLevel; i++)
            { //for every level in the directory path add a dash

                strcat(dash, "-");
            }

            if (currLevel == 0)
            { //tracker for the path depth

                currLevel++;
            }

            //if directory is readable and contains other files
            if (currFile->DIR_Attr == 0x01 || currFile->DIR_Attr == 0x10 || seek == (int)dataByteStart)
            {

                printf("\n%sDirectory: %s\n", dash, printNames(currFile)); //print directory name
                currCluster = (currFile->DIR_FstClusHI << 16) | currFile->DIR_FstClusLO;
                nextClusSector = (currCluster - 2) * bs->BPB_SecPerClus + getDataSectorStart();
                printDirContents(currLevel + 1, nextClusSector, currCluster); //recursively print
            }
            else if (currFile->DIR_Name[8] != 0x00 && currFile->DIR_Name[8] != -1 && currFile->DIR_Name[8] != 32)
            {

                printf("%s%s\n", dash, printNames(currFile)); //print file names
            }
        }
    }

    free(name);

    uint32_t lookUp = getFatByteStart() + cluster * 4;
    uint32_t nextCluster;

    lseek(fd, lookUp, SEEK_SET); //search next part of disk
    read(fd, &nextCluster, 4);   //read next cluster
    nextCluster = nextCluster & 0x0FFFFFFF;

    if (nextCluster != 0x0FFFFFFF)
    {

        printDirContents(currLevel, (nextCluster - 2) * bs->BPB_SecPerClus + getDataSectorStart(), nextCluster); //recursively print
    }
}
