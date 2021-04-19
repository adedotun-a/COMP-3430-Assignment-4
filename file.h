#ifndef FAT32_H
#define FAT32_H

#include <inttypes.h>
#include "fat32.h"

//fat 32 verfification
#define FAT32_SIGNATURE 0xAA55
#define FAT32_ROOT_DIR_SECTORS 0
#define MIN_FAT16_CLUSTER_COUNT 4085
#define MIN_FAT32_CLUSTER_COUNT 65525
#define BPB_ROOT 0

#define _FILE_OFFSET_BITS 64

#define MIRRORED_FAT_BITS 7
#define FAT_MIRROR_ENABLED_BIT 128

#define BPB_MEDIA_FIXED 0xF8
#define BS_DRIVE_SECTOR_FLOPPY 0x00

#define NEXT_CLUSTER_MASK 0x0FFFFFFF
#define MAX_CLUSTER_NUMBER 0x0FFFFFF7

// FAT32 FSInfo Sector Structure and Backup Boot Sector
#pragma pack(push)
#pragma pack(1)
struct fat32FSInfo_struct
{
    // This lead signature is used to validate that this is in fact an FSInfo sector.
    uint32_t FSI_LeadSig;
    // FAT32 format code should always initialize all bytes of this field to 0. Bytes in this field must currently never be used.
    uint8_t FSI_Reserved1[480];
    // FAT32 format code should always initialize all bytes of this field to 0. Bytes in this field must currently never be used.
    uint32_t FSI_StrucSig;
    // FAT32 format code should always initialize all bytes of this field to 0. Bytes in this field must currently never be used.
    uint32_t FSI_Free_Count;
    // FAT32 format code should always initialize all bytes of this field to 0. Bytes in this field must currently never be used.
    uint32_t FSI_Nxt_Free;
    // FAT32 format code should always initialize all bytes of this field to 0. Bytes in this field must currently never be used.
    uint8_t FSI_Reserved2[12];
    // FAT32 format code should always initialize all bytes of this field to 0. Bytes in this field must currently never be used.
    uint32_t FSI_TrailSig;
};
#pragma pack(pop)

typedef struct fat32FSInfo_struct FSInfo;

// Directory File attributes constants
// Indicates that writes to the file should fail.
#define ATTR_READ_ONLY 0x01
// Indicates that normal directory listings should not show this file.
#define ATTR_HIDDEN 0x02
// Indicates that this is an operating system file.
#define ATTR_SYSTEM 0x04
/*There should only be one “file” on the volume that has this attribute set,
and that file must be in the root directory. This name of this file is 
actually the label for the volume.
*/
#define ATTR_VOLUME_ID 0x08
// Indicates that this file is actually a container for other files.
#define ATTR_DIRECTORY 0x10
/* This attribute supports backup utilities. This bit is set by the FAT file
system driver when a file is created, renamed, or written to.
*/
#define ATTR_ARCHIVE 0x20
//  indicates that the “file” is actually part of the long name entry for some other file.
#define ATTR_LONG_NAME 0x0F
#define DIR_Name_LENGTH 11

#pragma pack(push)
#pragma pack(1)
// FAT 32 Byte Directory Entry Structure
// TODO
struct fat32DE_struct
{
    // Short name.
    char DIR_Name[DIR_Name_LENGTH];
    // Directory File attributes.
    uint8_t DIR_Attr;
    // for use by Windows NT. Set value to 0 when a file is created and never modify or look at it after that.
    uint8_t DIR_NTRes;
    // Millisecond stamp at file creation time.
    uint8_t DIR_CrtTimeTenth;
    // Time file was created.
    uint16_t DIR_CrtTime;
    //  Date file was created.
    uint16_t DIR_CrtDate;
    // Last access date. In the case of a write, this should be set to the same date as DIR_WrtDate.
    uint16_t DIR_LstAccDate;
    // High word of this entry’s first cluster number (always 0 for a FAT12 or FAT16 volume).
    uint16_t DIR_FstClusHI;
    // Time of last write. Note that file creation is considered a write.
    uint16_t DIR_WrtTime;
    // Date of last write. Note that file creation is considered a write.s
    uint16_t DIR_WrtDate;
    // Low word of this entry’s first cluster number.
    uint16_t DIR_FstClusLO;
    // 32-bit DWORD holding this file’s size in bytes.
    uint32_t DIR_FileSize;
};
#pragma pack(pop)

typedef struct fat32DE_struct fat32DE;
#endif
