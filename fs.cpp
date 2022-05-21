/**
 * TODO:
 * - Refactor for fetch and get functions accept only file metadata as argument
 */

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
typedef std::fstream fd;

// Structural typedefs
typedef std::vector<std::string> ParsedPathParents;

struct ParsedPath {
	str name;
	ParsedPathParents parents;
};

struct INodeBlocks {
	unsigned char DIRECT_BLOCKS[3];    
	unsigned char INDIRECT_BLOCKS[3];
	unsigned char DOUBLE_INDIRECT_BLOCKS[3];
};

struct MetaData {
	char blockSize;
	char numBlocks;
	char numINodes;
};

// Because the struct in fs.h was declared in a c style instead of a cpp style
// we cannot create an actual constructor
// The only option is this factory
INODE INODE_factory(u8 is_used, u8 is_dir, str name, u8 size, INodeBlocks blocks) {
	INODE inode;
	inode.IS_USED = is_used;
	inode.IS_DIR = is_dir;

	for(usize i = 0; i < 10; i++) {
		inode.NAME[i] = i < name.size() ? name[i] : '\0';
	}

	inode.SIZE = size;
	for (int i = 0; i < 3; i++) {
		inode.DIRECT_BLOCKS[i] = blocks.DIRECT_BLOCKS[i];
	}
	for (int i = 0; i < 3; i++) {
		inode.INDIRECT_BLOCKS[i] = blocks.INDIRECT_BLOCKS[i];
	}
	for (int i = 0; i < 3; i++) {
		inode.DOUBLE_INDIRECT_BLOCKS[i] = blocks.DOUBLE_INDIRECT_BLOCKS[i];
	}
	return inode;
}

void truncate_file(str& fileName){ std::fstream file{fileName, std::ios::out | std::ios::trunc}; }

c8 _getBitNFromByte(c8 byte, usize n){ return byte & (0x01 << n); } //(byte >> position) & 0x1

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

MetaData _fetchMetadata(fd& fs)
{
	MetaData metaData{};
	fs.seekg(_getMetaDataOffSet())
		.read(reinterpret_cast<char*>(&metaData), sizeof(MetaData));
	return metaData;
}

usize _fetchBitMapSize(std::fstream& fs)
{
	return _getBitMapSize(_fetchMetadata(fs).numBlocks);
}

usize _fetchBlocksOffset(fd& fs)
{
	auto metaData = _fetchMetadata(fs);
	return _getBlocksOffSet(metaData.numBlocks, metaData.numINodes);
}

c8 _fetchBitMapByte(fd& fs, usize byteIndex)
{
	c8 buff{};
	fs.seekg(_getBitMapOffSet() + byteIndex*sizeof(c8))
		.read(&buff, sizeof(c8));
	return buff;
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

ParsedPath _parsePath(str& path)
{
	ParsedPath parsedPath{};

	str strBuff{};
	for(usize i = 0; i < path.size(); i++) {
		if(path[i] == '/') {
			// We root
			if(strBuff.size() == 0){
				strBuff = '/';
			}
			parsedPath.parents.push_back(strBuff);
			strBuff.clear();
		} else {
			strBuff.push_back(path[i]);
		}
	}
	parsedPath.name = strBuff;

	return parsedPath;
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

void _writeBitMapAt(
	std::fstream& fs,
	const usize position,
	const bool value,
	const usize byteOffset
){
	c8 buff{};
	auto byteIndex = position / 8;
	auto bitMapOffset = _getBitMapOffSet() + byteIndex;
	fs.seekg(bitMapOffset)
		.read(&buff, sizeof(c8));
	buff |= static_cast<c8>(value) << (position % 8);
	fs.seekp(bitMapOffset)
		.write(&buff, sizeof(c8));
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

usize _findEmptyBlockIndex(std::fstream& fs)
{
	for(usize byteIndex = 0; byteIndex < _fetchBitMapSize(fs); byteIndex++){
		c8 bitMapByte{_fetchBitMapByte(fs, byteIndex)};
		// We now check each bit
		for(usize bitIndex = 0; bitIndex < 8; bitIndex++){
			c8 bit{_getBitNFromByte(bitMapByte, bitIndex)};
			if(bit == 0){
				return _fetchBlocksOffset(fs) + (byteIndex*8 + bitIndex)*_fetchMetadata(fs).blockSize;
			}
		}
	}
	throw std::runtime_error("No free blocks");
}

INodeBlocks _writeBlocks(std::fstream& fs, str& fileContent)
{
	auto metaData = _fetchMetadata(fs);
	
	auto blocksNeeded{_blocksNeededToStore(fileContent.size(), metaData.blockSize)};
	
	INodeBlocks blocks{};

	for(usize i = 0; i < blocksNeeded; i++){
		auto blockIndex = _findEmptyBlockIndex(fs);
		fs.seekp(blockIndex)
			.write(&fileContent[i*metaData.blockSize], sizeof(c8))
			.write(&fileContent[i*metaData.blockSize+1], sizeof(c8));
		_writeBitMapAt(
			fs,
			(blockIndex - _getBlocksOffSet(metaData.numBlocks, metaData.numINodes)) / metaData.blockSize,
			0x01,
			_getBitMapOffSet()
		);
		blocks.DIRECT_BLOCKS[i] = blockIndex;
	}

	return blocks;
}

// @returns the index of the new inode
usize _writeINode(fd& fs, INODE& inode, MetaData& metaData)
{
	for(usize i = 0; i < static_cast<usize>(_fetchMetadata(fs).numINodes); i++){
		INODE inodeBuff{};
		fs.seekg(_getINodesOffSet(metaData.numBlocks) + i*sizeof(INODE))
			.read(_iNodeToWritable(inodeBuff), sizeof(INODE));
		if(inodeBuff.IS_USED == 0){
			fs.seekp(_getINodesOffSet(metaData.numBlocks) + i*sizeof(INODE))
				.write(_iNodeToWritable(inode), sizeof(INODE));
			return _getINodesOffSet(metaData.numBlocks) + i*sizeof(INODE);
		}
	}
	throw std::runtime_error("No free space for inodes");
}

usize _findParentIndex(fd& fs, str& name, MetaData& metaData)
{
	for(usize i = 0; i < static_cast<usize>(metaData.numINodes); i++){
		INODE inodeBuff{};
		fs.seekg(_getINodesOffSet(metaData.numBlocks) + i*sizeof(INODE))
			.read(_iNodeToWritable(inodeBuff), sizeof(INODE));
		if(inodeBuff.IS_USED == 1 && inodeBuff.IS_DIR == 1 && inodeBuff.NAME == name){
			return _getINodesOffSet(metaData.numBlocks) + i*sizeof(INODE);
		}
	}
	throw std::runtime_error("Parent does not exist");
}

void _writeUpdateParent(
	fd& fs,
	str& parentName,
	usize inodeIndex,
	INODE& inode,
	MetaData& metaData
){
	
	INODE inodeBuff{};
	auto parentIndex = _findParentIndex(fs, parentName, metaData);
	fs.seekg(parentIndex)
		.read(_iNodeToWritable(inodeBuff), sizeof(INODE));
	inodeBuff.SIZE += 1;

	// Writing the children of parent in blocks
	str indexAsStr{static_cast<char>(inodeIndex)};
	auto blocks = _writeBlocks(fs, indexAsStr);
	inodeBuff.DIRECT_BLOCKS[0] = blocks.DIRECT_BLOCKS[0];
	inodeBuff.DIRECT_BLOCKS[1] = blocks.DIRECT_BLOCKS[1];
	inodeBuff.DIRECT_BLOCKS[2] = blocks.DIRECT_BLOCKS[2];

	// Writing the modifiend parent inode back
	fs.seekp(parentIndex)
		.write(_iNodeToWritable(inodeBuff), sizeof(INODE));
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
	auto metaData = _fetchMetadata(fs);

	auto blocksIndex = _writeBlocks(fs, fileContent);
	auto fileStructure = _parsePath(filePath);
	auto inode = INODE_factory(
		1,
		0,
		fileStructure.name,
		fileContent.size(),
		blocksIndex
	);
	auto inodeIndex = _writeINode(fs, inode, metaData);
	_writeUpdateParent(fs, fileStructure.parents.back(), inodeIndex, inode, metaData);
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
