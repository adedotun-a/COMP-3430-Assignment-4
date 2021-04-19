#pragma once

#include <inttypes.h>

/**
 * All of the values and byte/bit sizes for each of these values comes directly
 * from the various descriptions of the FAT32 file system.
 */

/* boots sector constants */
#define BS_OEMName_LENGTH 8
#define BS_VolLab_LENGTH 11
#define BS_FilSysType_LENGTH 8

/**
 * The pragmas here are specifically making sure that your compiler does not
 * try to do "smart things" like reordering the struct members; the properties
 * are ordered in this specific order because it matches the layout of the boot
 * sector of a FAT32 file system.
 */
#pragma pack(push)
#pragma pack(1)
struct fat32BootSector_struct
{
	char BS_jmpBoot[3];
	char BS_OEMName[BS_OEMName_LENGTH];
	uint16_t BPB_BytesPerSec;
	uint8_t BPB_SecPerClus;
	uint16_t BPB_RsvdSecCnt;
	uint8_t BPB_NumFATs;
	uint16_t BPB_RootEntCnt;
	uint16_t BPB_TotSec16;
	uint8_t BPB_Media;
	uint16_t BPB_FATSz16;
	uint16_t BPB_SecPerTrk;
	uint16_t BPB_NumHeads;
	uint32_t BPB_HiddSec;
	uint32_t BPB_TotSec32;
	uint32_t BPB_FATSz32;
	uint16_t BPB_ExtFlags;
	uint8_t BPB_FSVerLow;
	uint8_t BPB_FSVerHigh;
	uint32_t BPB_RootClus;
	uint16_t BPB_FSInfo;
	uint16_t BPB_BkBootSec;
	char BPB_reserved[12];
	uint8_t BS_DrvNum;
	uint8_t BS_Reserved1;
	uint8_t BS_BootSig;
	uint32_t BS_VolID;
	char BS_VolLab[BS_VolLab_LENGTH];
	char BS_FilSysType[BS_FilSysType_LENGTH];
	char BS_CodeReserved[420];
	uint8_t BS_SigA;
	uint8_t BS_SigB;
};
#pragma pack(pop)

typedef struct fat32BootSector_struct fat32BootSector;
