#include "indexing.h"
#include "structures.h"

#include <memory>
#include <stdio.h>

#include <fmt/core.h>
#include <fmt/format.h>
#include <sys/types.h>
#include <sys/mman.h>

namespace
{
	typedef std::unique_ptr<FILE, decltype(&fclose)> FileCloser;
	
	class MemoryUnmapper
	{
		public:
			MemoryUnmapper(void* addr, size_t len) :
				address(addr), length(len)
			{}
			
			~MemoryUnmapper() {
				munmap(this->address, this->length);
			}
			
		private:
			void* address;
			size_t length;
	};
	
	std::string GetSectionName(PIMAGE_SECTION_HEADER section)
	{
		return std::string(
			(const char*)section->Name,
			strnlen((const char*)section->Name, IMAGE_SIZEOF_SHORT_NAME)
		);
	}
}

std::string HashForPE(const std::filesystem::path& path)
{
	FileCloser file( fopen(path.string().c_str(), "rb"), &fclose );
	if (!file) {
		throw fmt::system_error(errno, "Failed to open file: {}", path.string());
	}
	
	if (fseek(file.get(), 0L, SEEK_END)){
		throw fmt::system_error(errno, "Failed to seek to end of file: {}", path.string());
	}
	
	// Map PE file into memory
	auto fileSize = ftell(file.get());
	void* mapped = mmap(NULL, fileSize, PROT_READ, MAP_PRIVATE, fileno(file.get()), 0);
	if (mapped == MAP_FAILED){
		throw fmt::system_error(errno, "Failed to map file into memory: {}", path.string());
	}
	
	// Ensure the memory is unmapped when we finish
	MemoryUnmapper unmapper(mapped, fileSize);
	
	// Retrieve the module's DOS header
	PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)(mapped);
	if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
		throw std::runtime_error(fmt::format("PE file {} has incorrect DOS signature", path.string()));
	}
	
	// Retrieve the module's NT header
	PIMAGE_NT_HEADERS32 ntHeader = (PIMAGE_NT_HEADERS32)((uint8_t*)(dosHeader) + dosHeader->e_lfanew);
	if (ntHeader->Signature != IMAGE_NT_SIGNATURE) {
		throw std::runtime_error(fmt::format("PE file {} has incorrect NT signature", path.string()));
	}
	
	// Determine the bitness of the module and retrieve the size of the image
	DWORD imageSize = 0;
	if (ntHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
		imageSize = ntHeader->OptionalHeader.SizeOfImage;
	}
	else {
		imageSize = ((PIMAGE_NT_HEADERS64)ntHeader)->OptionalHeader.SizeOfImage;
	}
	
	/*
		Microsoft's `symstore.exe` tool evidently rejects resource-only DLL files, printing a warning like so:
		
		```
		SYMSTORE MESSAGE: Skipping file tzres.dll - not a known file type. ErrorLevel is 13.
		```
		
		For the sake of consistency with official tooling, we detect this case and ignore these DLL files as well.
		This shouldn't impact debugging, since these files only contain resource data and do not contain any code.
	*/
	PIMAGE_SECTION_HEADER firstSection = IMAGE_FIRST_SECTION(ntHeader);
	if (ntHeader->FileHeader.NumberOfSections == 1 && GetSectionName(firstSection) == ".rsrc") {
		return "SENTINEL_IGNORE_FILE";
	}
	
	// Hash is the combination of the TimeDateStamp and the size of the image
	// (Note that the former field is upper case, whereas the latter is lower case)
	return fmt::format(
		"{:08X}{:x}",
		ntHeader->FileHeader.TimeDateStamp,
		imageSize
	);
}

std::string SignatureForPDB(const std::filesystem::path& path)
{
	FileCloser file( fopen(path.string().c_str(), "rb"), &fclose );
	if (!file) {
		throw fmt::system_error(errno, "Cannot open file: {}", path.string());
	}
	
	if (fseek(file.get(), 0L, SEEK_END)){
		throw fmt::system_error(errno, "Failed to seek to end of file: {}", path.string());
	}
	
	// Map PDB file into memory
	auto fileSize = ftell(file.get());
	void* mapped = mmap(NULL, fileSize, PROT_READ, MAP_PRIVATE, fileno(file.get()), 0);
	if (mapped == MAP_FAILED) {
		throw fmt::system_error(errno, "Failed to map file into memory: {}", path.string());
	}
	
	// Ensure the memory is unmapped when we finish
	MemoryUnmapper unmapper(mapped, fileSize);
	
	// MSF "SuperBlock"
	PMSF_SUPERBLOCK superBlock = (PMSF_SUPERBLOCK)mapped;
	
	// File size should match superBlock->NumBlocks * superBlock->BlockSize
	if (fileSize != (superBlock->NumBlocks * superBlock->BlockSize))
	{
		throw std::runtime_error(
			fmt::format(
				"File size {} does not match calculated size {} for PDB file {}",
				fileSize,
				superBlock->NumBlocks * superBlock->BlockSize,
				path.string()
			)
		);
	}
	
	// Calculate the StreamDirectory Offset
	// Take the pointer stream pointer from the 3rd block () and step to the position in the file
	// eg: currentMappedAddress + ( currentMappedAddress + 3 * 4096 ) * 4096
	// (Calculation adapted from the logic here: <https://forum.pellesc.de/index.php?topic=7340.0>)
	PSTREAM_DIRECTORY streamDirectory = (PSTREAM_DIRECTORY)(
		(uint8_t*)mapped + (*(uint32_t*)((uint8_t*)mapped + superBlock->BlockMapAddr * superBlock->BlockSize) * superBlock->BlockSize)
	);
	
	// Calculate the start position of streamDirectory->StreamBlocks[][] after the streamDirectory->StreamSizes[]
	uint32_t* streamBlocks = streamDirectory->StreamSizes + streamDirectory->NumStreams;
	
	// PDB Info Stream Header is located at the start if the streams, start at position 0 of block size and step #blocks * BlockSize
	// (Note that this is specific to Codeview type VC70)
	uint32_t version = *(uint32_t*)((uint8_t*)mapped + (*streamBlocks * superBlock->BlockSize));
	if (version == VC70)
	{
		PPDBSTREAM_HEADER streamHeader = (PPDBSTREAM_HEADER)((uint8_t*)mapped + ((streamBlocks[0] * superBlock->BlockSize)));
		
		auto guid = fmt::format("{:08X}{:04X}{:04X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}", 
			streamHeader->UniqueId.Data1, streamHeader->UniqueId.Data2, streamHeader->UniqueId.Data3, 
			streamHeader->UniqueId.Data4[0], streamHeader->UniqueId.Data4[1], streamHeader->UniqueId.Data4[2], 
			streamHeader->UniqueId.Data4[3], streamHeader->UniqueId.Data4[4], streamHeader->UniqueId.Data4[5], 
			streamHeader->UniqueId.Data4[6], streamHeader->UniqueId.Data4[7]);
		
		return guid + std::to_string(streamHeader->Age);
	}
	else {
		throw std::runtime_error(fmt::format("PDB file {} contains unsupported PDB stream version {}", path.string(), version));
	}
}
