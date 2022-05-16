#include "fs.h"

#include <vector>
#include <cmath>
#include <fstream>
#include <memory>
#include <string>

typedef char c8;
typedef uint8_t u8;
typedef int32_t i32;
typedef size_t usize;
typedef std::string str;

void truncate_file(str& fileName){
	std::fstream file{fileName, std::ios::out | std::ios::trunc};
}

usize _getNumBitMapBytes(const c8 numBlocks)
{
	// Rounded up to the nearest byte;
	return std::ceil(static_cast<double>(numBlocks) / 8.0);
}

usize _getMetaDataByteOffSet() { return 0; }

usize _getBitMapByteOffSet() { return _getMetaDataByteOffSet() + 3; }

usize _getINodesByteOffSet(const c8 numBlocks) {
	// Here we need to get how many bytes are used by the bitmap cause it is dynamic
	return _getBitMapByteOffSet() + _getNumBitMapBytes(numBlocks);
}

usize _getBlockByteOffSet(const c8 numBlocks, const c8 numInodes) {
	return _getINodesByteOffSet(numBlocks)
		+ numInodes*sizeof(INODE);
}

char* _iNodeToWritable(INODE& inode)
{
	return reinterpret_cast<char*>(&inode);
}

void _writeMetaData(std::fstream& file, const c8 blockSize, const c8 numBlocks, const c8 numInodes, const usize byteOffset)

{
	file.seekp(byteOffset)
		.write(&blockSize, sizeof(c8))
		.write(&numBlocks, sizeof(c8))
		.write(&numInodes, sizeof(c8));
}

void _writeBitMap(std::fstream& file, const c8 numBlocks, const std::vector<bool> bitMap, const usize byteOffset)
{
	file.seekp(byteOffset);
	for(size_t i = 0; i < _getNumBitMapBytes(numBlocks); i++){
		char bit = i < bitMap.size() ? (bitMap[i] ? 1 : 0) : 0;
		file.write(&bit, sizeof(c8));
	}
}

// TODO: This io currently overwriting the whole byte
void _writeBitMapAt(std::fstream& file, const usize position, const c8 value, const usize byteOffset)
{
	file.seekp(byteOffset + position);
	file.write(&value, sizeof(c8));
}

void _writeBitMapFill(std::fstream& file, const c8 value, const c8 numBlocks, const usize byteOffset)
{
	file.seekp(byteOffset);
	for(size_t i = 0; i < _getNumBitMapBytes(numBlocks); i++){
		file.write(&value, sizeof(c8));
	}
}

void _writeINodeRoot(std::fstream& file, const c8 numInodes, const usize byteOffset)
{
	INODE root{
		0xdd, // IS_USED
		0x01, // IS_DIR
		"/", // NAME
		0, // SIZE
		{0, 0, 0}, // DIRECT_BLOCKS
		{0, 0, 0}, // INDIRECT_BLOCKS
		{0, 0, 0xcc} // DOUBLE_INDIRECT_BLOCKS
	};

	file.seekp(byteOffset)
		.write(_iNodeToWritable(root), sizeof(INODE));
	// Because we writed we now need to update the bit map

	_writeBitMapAt(file, 0x00, 0x01, _getBitMapByteOffSet());

	// Fill the rest with 0
	INODE empty{
		0xee, // IS_USED
		0, // IS_DIR
		"\0", // NAME
		0, // SIZE
		{0, 0, 0}, // DIRECT_BLOCKS
		{0, 0, 0}, // INDIRECT_BLOCKS
		{0, 0, 0xff} // DOUBLE_INDIRECT_BLOCKS
	};
	
	for(size_t i = 1; i < static_cast<usize>(numInodes); i++){
		file.seekp(byteOffset + i*sizeof(INODE))
			.write(_iNodeToWritable(empty), sizeof(INODE));
	}
}

void _writeBlocksFill(std::fstream& file, const c8 value, const c8 numBlocks, const c8 blockSize, const usize byteOffset)
{
	for(size_t i = 0; i < static_cast<usize>(numBlocks); i++){
		file.seekp(byteOffset + i*blockSize)
			.write(&value, sizeof(c8))
			.write(&value, sizeof(c8));
	}
}

void initFs(std::string fsFileName, int blockSize, int numBlocks, int numInodes)
{
	truncate_file(fsFileName);
	std::fstream file{ fsFileName, std::ios::binary | std::ios::in | std::ios::out };
	_writeMetaData(file, blockSize, numBlocks, numInodes, _getMetaDataByteOffSet());
	_writeBitMapFill(file, 0x00, numBlocks, _getBitMapByteOffSet());
	_writeINodeRoot(file, numInodes, _getINodesByteOffSet(numBlocks));
	// TODO: _writeRootIndex(file, _getRootIndexByteOffSet(numBlocks, numInodes));
	_writeBlocksFill(file, 0x32, numBlocks, blockSize, _getBlockByteOffSet(numBlocks, numInodes));
}

void addFile(std::string fsFileName, std::string filePath, std::string fileContent)
{
	
}

void addDir(std::string fsFileName, std::string dirPath)
{

}

void remove(std::string fsFileName, std::string path)
{
	
}

void move(std::string fsFileName, std::string oldPath, std::string newPath)
{
	
}
