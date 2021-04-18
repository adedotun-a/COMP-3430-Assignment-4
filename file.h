#ifndef FAT32_H
#define FAT32_H

#include <inttypes.h>

//fat 32 verfification
#define FAT32_SIGNATURE 0xAA55
#define FAT32_ROOT_DIR_SECTORS 0
#define MIN_FAT16_CLUSTER_COUNT 4085
#define MIN_FAT32_CLUSTER_COUNT 65525
#define BPB_ROOT 0

#define _FILE_OFFSET_BITS 64
/* boot sector constants */
#define BS_OEMName_LENGTH 8
#define BS_VolLab_LENGTH 11
#define BS_FilSysType_LENGTH 8

#define MIRRORED_FAT_BITS 7
#define FAT_MIRROR_ENABLED_BIT 128

#define BPB_MEDIA_FIXED 0xF8
#define BS_DRIVE_SECTOR_FLOPPY 0x00

#define NEXT_CLUSTER_MASK 0x0FFFFFFF
#define MAX_CLUSTER_NUMBER 0x0FFFFFF7

#pragma pack(push)
#pragma pack(1)
struct fat32BS_struct
{
    // Jump instruction to boot code.
    char BS_jmpBoot[3];
    // “MSWIN4.1”
    char BS_OEMName[BS_OEMName_LENGTH];
    // Count of bytes per sector.
    uint16_t BPB_BytesPerSec;
    // Number of sectors per allocation unit.
    uint8_t BPB_SecPerClus;
    // Number of reserved sectors in the Reserved region of the volume starting at the first sector of the volume. Must not be 0, typically 32 for fat32
    uint16_t BPB_RsvdSecCnt;
    // The count of FAT data structures on the volume.
    uint8_t BPB_NumFATs;
    // For FAT32 volumes, this field must be set to 0.
    uint16_t BPB_RootEntCnt;
    // This field is the old 16-bit total count of sectors on the volume. For FAT32 volumes, this field must be 0
    uint16_t BPB_TotSec16;
    // 0xF8 is the standard value for “fixed” (non-removable) media. For removable media, 0xF0 is frequently used.
    uint8_t BPB_Media;
    //  On FAT32 volumes this field must be 0, and BPB_FATSz32 contains the FAT size count.
    uint16_t BPB_FATSz16;
    //Sectors per track for interrupt 0x13.
    uint16_t BPB_SecPerTrk;
    // Number of heads for interrupt 0x13
    uint16_t BPB_NumHeads;
    // Count of hidden sectors preceding the partition that contains this FAT volume
    uint32_t BPB_HiddSec;
    // This field is the new 32-bit total count of sectors on the volume. This count includes the count of all sectors in all four regions of the volume.
    uint32_t BPB_TotSec32;
    /* FAT32 Specific values
    */
    // This field is the FAT32 32-bit count of sectors occupied by ONE FAT.
    uint32_t BPB_FATSz32;
    /* Bits 0-3 -- Zero-based number of active FAT. Only valid if mirroring is disabled.
    Bits 4-6    -- Reserved.
    Bit 7       -- 0 means the FAT is mirrored at runtime into all FATs. 
                -- 1 means only one FAT is active; it is the one referenced in bits 0-3.
    Bits 8-15   -- Reserved.*/
    uint16_t BPB_ExtFlags;
    /*  High byte is major revision number.
        Low byte is minor revision number.
        This is the version number of the FAT32 volume. */
    uint8_t BPB_FSVerLow;
    uint8_t BPB_FSVerHigh;
    // This is set to the cluster number of the first cluster of the root directory, usually 2 but not required to be 2.
    uint32_t BPB_RootClus;
    // Sector number of FSINFO structure in the reserved area of the FAT32 volume.Usually 1.
    uint16_t BPB_FSInfo;
    // If non-zero, indicates the sector number in the reserved area of the volume of a copy of the boot record.Usually 6. No value other than 6 is recommended.
    uint16_t BPB_BkBootSec;
    // Reserved for future expansion. Code that formats FAT32 volumes should always set all of the bytes of this field to 0.
    char BPB_reserved[12];
    // Int drive number
    uint8_t BS_DrvNum;
    // Reserved (used by Windows NT).
    uint8_t BS_Reserved1;
    // Extended boot signature
    uint8_t BS_BootSig;
    /* Volume serial number. This field, together with BS_VolLab,
    supports volume tracking on removable media. These values allow
    FAT file system drivers to detect that the wrong disk is inserted in a
    removable drive. This ID is usually generated by simply combining
    the current date and time into a 32-bit value.*/
    uint32_t BS_VolID;
    // Volume label. This field matches the 11-byte volume label recorded in the root directory.
    char BS_VolLab[BS_VolLab_LENGTH];
    // Always set to the string ”FAT32 ”.
    char BS_FilSysType[BS_FilSysType_LENGTH];
    char BS_CodeReserved[420];
    uint8_t BS_SigA;
    uint8_t BS_SigB;
};
#pragma pack(pop)

typedef struct fat32BS_struct fat32BS;

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
