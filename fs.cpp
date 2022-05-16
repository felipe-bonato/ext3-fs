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

typedef struct {
	unsigned char DIRECT_BLOCKS[3];    
    unsigned char INDIRECT_BLOCKS[3];
    unsigned char DOUBLE_INDIRECT_BLOCKS[3];
} INODE_BLOCKS;

void truncate_file(str& fileName){ std::fstream file{fileName, std::ios::out | std::ios::trunc}; }

usize _getBitMapSize(const c8 numBlocks)
{
	// Rounded up to the nearest byte;
	return std::ceil(static_cast<double>(numBlocks) / 8.0);
}

usize _getMetaDataOffSet() { return 0; }

usize _getBlockSizeOffSet() { return _getMetaDataOffSet(); }

usize _getNumBlocksOffSet() { return _getMetaDataOffSet() + 1; }

usize _getNumINodesOffSet() { return _getMetaDataOffSet() + 2; }

usize _getBitMapOffSet() { return _getMetaDataOffSet() + 3; }

usize _getINodesOffSet(const c8 numBlocks)
{
	// Here we need to get how many bytes are used by the bitmap cause it is dynamic
	return _getBitMapOffSet() + _getBitMapSize(numBlocks);
}

usize _getRootIndexOffSet(const c8 numBlocks, const c8 numInodes)
{
	return _getINodesOffSet(numBlocks) + numInodes*sizeof(INODE);
}

usize _getBlocksOffSet(const c8 numBlocks, const c8 numInodes)
{
	return _getRootIndexOffSet(numBlocks, numInodes)
		+ sizeof(c8); // Cause pointer is c8
}

usize _blocksNeededToStore(usize content, c8 blockSize)
{
	return std::ceil(static_cast<double>(content) / static_cast<double>(blockSize));
}

c8 _fetchBlockSize(std::fstream& fs)
{
	c8 blockSize;
	fs.seekg(_getBlockSizeOffSet())
		.read(&blockSize, sizeof(c8));
	return blockSize;
}

c8 _fetchNumBlocks(std::fstream& fs)
{
	c8 numBlocks;
	fs.seekg(_getNumBlocksOffSet())
		.read(&numBlocks, sizeof(c8));
	return numBlocks;
}

c8 _fetchBitMapSize(std::fstream& fs)
{
	return _getBitMapSize(_fetchNumBlocks(fs));
}

c8* _iNodeToWritable(INODE& inode) { return reinterpret_cast<c8*>(&inode);}

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
	for(size_t i = 0; i < _getBitMapSize(numBlocks); i++){
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
	for(size_t i = 0; i < _getBitMapSize(numBlocks); i++){
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

	_writeBitMapAt(fs, 0x00, 0x01, _getBitMapOffSet());

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

c8 _findEmptyBlockIndex(std::fstream& fs)
{
	std::vector<c8> bitMap{};
	for(usize i = 0; i < _fetchBitMapSize(fs); i++){

	}
}

INODE_BLOCKS _writeBlocks(std::fstream& fs, std::fstream& file, str& fileContent)
{
	c8 blockSize{};
	fs.seekg(_getBlockSizeOffSet())
		.read(&blockSize, sizeof(c8));
	
	c8 blocksNeeded{_blocksNeededToStore(fileContent.size(), blockSize)};
	
	for(usize i = 0; i < blocksNeeded; i++){
		fs.seekp(_findEmptyBlockIndex(fs))
			.write(&(fileContent.c_str()[i*blockSize]), blockSize);
	}
}

void initFs(std::string fsFileName, int blockSize, int numBlocks, int numInodes)
{
	truncate_file(fsFileName);
	std::fstream fs{ fsFileName, std::ios::binary | std::ios::in | std::ios::out };
	_writeMetaData(fs, blockSize, numBlocks, numInodes, _getMetaDataOffSet());
	_writeBitMapFill(fs, 0x00, numBlocks, _getBitMapOffSet());
	_writeINodeRoot(fs, numInodes, _getINodesOffSet(numBlocks));
	_writeRootIndex(fs, _getRootIndexOffSet(numBlocks, numInodes));
	_writeBlocksFill(fs, 0x00, numBlocks, blockSize, _getBlocksOffSet(numBlocks, numInodes));
}

void addFile(std::string fsFileName, std::string filePath, std::string fileContent)
{
	std::fstream fs{ fsFileName, std::ios::binary | std::ios::in | std::ios::out };
	std::fstream file{ filePath, std::ios::binary | std::ios::in | std::ios::out };
	auto blocksIndex = _writeBlocks(fs, file, fileContent);
	/*auto [parent, name] = parseFileName(filePath);
	_writeInode(fs, parent, INODE{
		0x01, // IS_USED
		0x00, // IS_DIR
		name, // NAME
		static_cast<c8>(fileContent.size()), // SIZE
		blocksIndex.directBlocks, // DIRECT_BLOCKS
		blocksIndex.indirectBlocks, // INDIRECT_BLOCKS
		blocksIndex.doubleIndirectBlocks // DOUBLE_INDIRECT_BLOCKS
	});*/
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
