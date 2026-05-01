#pragma once

#include <cstdint>
#include <minwindef.h>
#undef min
#undef max

/*
	Constants and struct definitions below are adapted from the "The MSF File Format" page of the LLVM documentation:
	<https://llvm.org/docs/PDB/MsfFile.html>
*/

// Size in bytes of the superblock magic string for the newer PDB type ("Microsoft C/C++ MSF 7.00\r\n\032DS\0\0")
#define PDB_VC70 32

typedef struct _MSF_SUPERBLOCK {
	char FileMagic[PDB_VC70];
	uint32_t BlockSize;
	uint32_t FreeBlockMapBlock;
	uint32_t NumBlocks;
	uint32_t NumDirectoryBytes;
	uint32_t Unknown;
	uint32_t BlockMapAddr;
} MSF_SUPERBLOCK, *PMSF_SUPERBLOCK;

// Length at runtime unknown
typedef struct _STREAM_DIRECTORY {
	uint32_t NumStreams;
	uint32_t StreamSizes[1];
	uint32_t StreamBlocks[1][1];
} STREAM_DIRECTORY, *PSTREAM_DIRECTORY;


/*
	Constants and struct definitions below are adapted from the "The PDB Info Stream (aka the PDB Stream)" page of the LLVM documentation:
	<https://llvm.org/docs/PDB/PdbStream.html>
*/

// PDB Stream Versions
#define VC2     19941610
#define VC4     19950623
#define VC41    19950814
#define VC50    19960307
#define VC98    19970604
#define VC70Dep 19990604
#define VC70    20000404
#define VC80    20030901
#define VC110   20091201
#define VC140   20140508

typedef struct _PDBSTREAM_HEADER {
	uint32_t Version;
	uint32_t Signature;
	uint32_t Age;
	GUID UniqueId;
} PDBSTREAM_HEADER,*PPDBSTREAM_HEADER;
