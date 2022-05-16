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

usize _getINodesByteOffSet(const c8 numBlocks)
{
	// Here we need to get how many bytes are used by the bitmap cause it is dynamic
	return _getBitMapByteOffSet() + _getNumBitMapBytes(numBlocks);
}

usize _getRootIndexByteOffSet(const c8 numBlocks, const c8 numInodes)
{
	return _getINodesByteOffSet(numBlocks) + numInodes*sizeof(INODE);
}

usize _getBlockByteOffSet(const c8 numBlocks, const c8 numInodes) {
	return _getRootIndexByteOffSet(numBlocks, numInodes)
		+ sizeof(c8); // Cause pointer is c8
}

char* _iNodeToWritable(INODE& inode)
{
	return reinterpret_cast<char*>(&inode);
}

void _writeMetaData(
	std::fstream& fs,
	const c8 blockSize,
	const c8 numBlocks,
	const c8 numInodes,
	const usize byteOffset
){
	fs.seekp(byteOffset)
		.write(&blockSize, sizeof(c8))
		.write(&numBlocks, sizeof(c8))
		.write(&numInodes, sizeof(c8));
}

void _writeBitMap(
	std::fstream& fs,
	const c8 numBlocks,
	const std::vector<bool> bitMap,
	const usize byteOffset
){
	fs.seekp(byteOffset);
	for(size_t i = 0; i < _getNumBitMapBytes(numBlocks); i++){
		char bit = i < bitMap.size() ? (bitMap[i] ? 1 : 0) : 0;
		fs.write(&bit, sizeof(c8));
	}
}

// TODO: This io currently overwriting the whole byte
void _writeBitMapAt(
	std::fstream& fs,
	const usize position,
	const c8 value,
	const usize byteOffset
){
	fs.seekp(byteOffset + position);
	fs.write(&value, sizeof(c8));
}

void _writeBitMapFill(
	std::fstream& fs,
	const c8 value,
	const c8 numBlocks,
	const usize byteOffset
){
	fs.seekp(byteOffset);
	for(size_t i = 0; i < _getNumBitMapBytes(numBlocks); i++){
		fs.write(&value, sizeof(c8));
	}
}

void _writeINodeRoot(std::fstream& fs, const c8 numInodes, const usize byteOffset)
{
	INODE root{
		0x01, // IS_USED
		0x01, // IS_DIR
		"/", // NAME
		0, // SIZE
		{0, 0, 0}, // DIRECT_BLOCKS
		{0, 0, 0}, // INDIRECT_BLOCKS
		{0, 0, 0} // DOUBLE_INDIRECT_BLOCKS
	};

	fs.seekp(byteOffset)
		.write(_iNodeToWritable(root), sizeof(INODE));
	// Because we writed we now need to update the bit map

	_writeBitMapAt(fs, 0x00, 0x01, _getBitMapByteOffSet());

	// Fill the rest with 0
	INODE empty{
		0, // IS_USED
		0, // IS_DIR
		"\0", // NAME
		0, // SIZE
		{0, 0, 0}, // DIRECT_BLOCKS
		{0, 0, 0}, // INDIRECT_BLOCKS
		{0, 0, 0} // DOUBLE_INDIRECT_BLOCKS
	};
	
	for(size_t i = 1; i < static_cast<usize>(numInodes); i++){
		fs.seekp(byteOffset + i*sizeof(INODE))
			.write(_iNodeToWritable(empty), sizeof(INODE));
	}
}

void _writeRootIndex(std::fstream& fs, const usize byteOffset)
{
	c8 zeroIndex = '\0';
	fs.seekp(byteOffset)
		.write(&zeroIndex, sizeof(c8));
}

void _writeBlocksFill(
	std::fstream& fs,
	const c8 value,
	const c8 numBlocks,
	const c8 blockSize,
	const usize byteOffset
){
	for(size_t i = 0; i < static_cast<usize>(numBlocks*blockSize); i++){
		fs.seekp(byteOffset + i*sizeof(c8))
			.write(&value, sizeof(c8));
	}
}

void initFs(std::string fsFileName, int blockSize, int numBlocks, int numInodes)
{
	truncate_file(fsFileName);
	std::fstream fs{ fsFileName, std::ios::binary | std::ios::in | std::ios::out };
	_writeMetaData(fs, blockSize, numBlocks, numInodes, _getMetaDataByteOffSet());
	_writeBitMapFill(fs, 0x00, numBlocks, _getBitMapByteOffSet());
	_writeINodeRoot(fs, numInodes, _getINodesByteOffSet(numBlocks));
	_writeRootIndex(fs, _getRootIndexByteOffSet(numBlocks, numInodes));
	_writeBlocksFill(fs, 0x00, numBlocks, blockSize, _getBlockByteOffSet(numBlocks, numInodes));
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
