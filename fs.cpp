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

// Type Aliases
using c8 = char;
using u8 = uint8_t;
using i32 = int32_t;
using usize = size_t;
using str = std::string;
using fd = std::fstream;

// Structural Aliases
using ParsedPathParents = std::vector<std::string>;

struct ParsedPath {
	str name;
	ParsedPathParents parents;
};

struct INodeBlocks {
	INodeBlocks() = default;
	INodeBlocks(INODE& iNode) {
		for (int i = 0; i < 3; i++) {
			this->DIRECT_BLOCKS[i] = iNode.DIRECT_BLOCKS[i];
			this->INDIRECT_BLOCKS[i] = iNode.INDIRECT_BLOCKS[i];
			this->DOUBLE_INDIRECT_BLOCKS[i] = iNode.DOUBLE_INDIRECT_BLOCKS[i];
		}
	}

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
		inode.INDIRECT_BLOCKS[i] = blocks.INDIRECT_BLOCKS[i];
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

usize _getBlocksOffSet(MetaData& metaData)
{
	return _getRootIndexOffSet(metaData.numBlocks, metaData.numINodes)
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

INODE _fetchINodeByIndex(fd& fs, usize index, MetaData& metaData)
{
	INODE inode{};
	fs.seekg(_getINodesOffSet(metaData.numBlocks) + index*sizeof(INODE))
		.read(reinterpret_cast<char*>(&inode), sizeof(INODE));
	return inode;
}

void _writeINodeByIndex(fd& fs, INODE inode, usize index, MetaData& metaData)
{
	fs.seekp(_getINodesOffSet(metaData.numBlocks) + index*sizeof(INODE))
		.write(_iNodeToWritable(inode), sizeof(INODE));
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
	buff = value ?
		buff | 0x01 << (position % 8)
		: buff & ~(0x01 << (position % 8));
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
				return byteIndex*8 + bitIndex;
			}
		}
	}
	throw std::runtime_error("No free blocks");
}

INodeBlocks _writeBlocks(std::fstream& fs, str& fileContent)
{
	auto metaData = _fetchMetadata(fs);
	
	INodeBlocks blocks{};

	// Even if thers nothing, every directory has awlways one block
	if(fileContent.size() == 0){
		auto blockIndex = _findEmptyBlockIndex(fs);
		_writeBitMapAt(fs, blockIndex, 0x01, _getBitMapOffSet());
		blocks.DIRECT_BLOCKS[0] = blockIndex;
		return blocks;
	}
	
	auto blocksNeededToStore = _blocksNeededToStore(fileContent.size(), metaData.blockSize);

	for(usize i = 0; i < blocksNeededToStore; i++){
		auto blockIndex = _findEmptyBlockIndex(fs);
		auto blockOffset = _fetchBlocksOffset(fs) + blockIndex*metaData.blockSize;
		fs.seekp(blockOffset);
		for(usize j = 0; j < static_cast<usize>(metaData.blockSize); j++){
			if(i*metaData.blockSize + j < fileContent.size()){
				fs.write(&fileContent[i*metaData.blockSize + j], sizeof(c8));
			}
		}
		_writeBitMapAt(fs, blockIndex, 0x01, _getBitMapOffSet());
		blocks.DIRECT_BLOCKS[i] = blockIndex;
	}

	return blocks;
}

// @returns the index of the new inode
usize _writeINode(fd& fs, INODE& inode, MetaData& metaData)
{
	for(usize i = 0; i < static_cast<usize>(metaData.numINodes); i++){
		INODE inodeBuff{};
		fs.seekg(_getINodesOffSet(metaData.numBlocks) + i*sizeof(INODE))
			.read(_iNodeToWritable(inodeBuff), sizeof(INODE));
		if(inodeBuff.IS_USED == 0){
			fs.seekp(_getINodesOffSet(metaData.numBlocks) + i*sizeof(INODE))
				.write(_iNodeToWritable(inode), sizeof(INODE));
			return i;
		}
	}
	throw std::runtime_error("No free space for inodes");
}

usize _findINodeIndexByName(fd& fs, str& name, MetaData& metaData)
{
	for(usize i = 0; i < static_cast<usize>(metaData.numINodes); i++){
		INODE inodeBuff{};
		fs.seekg(_getINodesOffSet(metaData.numBlocks) + i*sizeof(INODE))
			.read(_iNodeToWritable(inodeBuff), sizeof(INODE));
		if(inodeBuff.IS_USED == 1 && inodeBuff.NAME == name){
			return i;
		}
	}
	throw std::runtime_error("Parent does not exist");
}

usize _findINodeOffsetByName(fd& fs, str& name, MetaData& metaData)
{
	return _getINodesOffSet(metaData.numBlocks) + _findINodeIndexByName(fs, name, metaData)*sizeof(INODE);
}

void _updateParentAddChild(
	fd& fs,
	str& parentName,
	usize inodeIndex,
	INODE& inode,
	MetaData& metaData
){
	INODE parent{};
	auto parentOffset = _findINodeOffsetByName(fs, parentName, metaData);
	fs.seekg(parentOffset)
		.read(_iNodeToWritable(parent), sizeof(INODE));

	c8 inodeIndexAsChar = inodeIndex;
	// Cause every directory awlays has at least one block alocated
	if(parent.SIZE == 0){
		fs.seekp(
			_getBlocksOffSet(metaData.numBlocks, metaData.numINodes)
			+ metaData.blockSize*parent.DIRECT_BLOCKS[0]
		).write(&inodeIndexAsChar, sizeof(c8));
	} else if(parent.SIZE % metaData.blockSize != 0) {
		// This probably has a couple bugs, but it works
		// auto blockWithEmptySpace = parent.DIRECT_BLOCKS[parent.SIZE];
		fs.seekp(
			_getBlocksOffSet(metaData.numBlocks, metaData.numINodes)
			+ (parent.SIZE % metaData.blockSize)
		).write(&inodeIndexAsChar, sizeof(c8));
	} else {
		str indexAsStr{static_cast<char>(inodeIndex)};
		auto blocks = _writeBlocks(fs, indexAsStr);
		for(usize i = parent.SIZE; i < 3; i++){
			parent.DIRECT_BLOCKS[i] = blocks.DIRECT_BLOCKS[i];
		}
	}
	parent.SIZE += 1;

	// Writing the modifiend parent inode back
	fs.seekp(parentOffset)
		.write(_iNodeToWritable(parent), sizeof(INODE));
}

void _removeINode(fd& fs, usize inodeIndex, MetaData& metaData)
{
	INODE empty_inode{};
	fs.seekp(_getINodesOffSet(metaData.numBlocks) + inodeIndex*sizeof(INODE))
		.write(_iNodeToWritable(empty_inode), sizeof(INODE));
}

INodeBlocks _fetchINodeBlocks(fd& fs, usize inodeIndex, MetaData& metaData)
{
	auto iNode = _fetchINodeByIndex(fs, inodeIndex, metaData);
	INodeBlocks blocks{iNode};
	return blocks;
}

void _updateFreeBlocks(fd& fs, INodeBlocks& blocks, MetaData& metaData)
{
	for(usize i = 0; i < 3; i++){
		if(blocks.DIRECT_BLOCKS[i] != 0){
			_writeBitMapAt(fs, blocks.DIRECT_BLOCKS[i], 0x00, _getBitMapOffSet());
		}
	}
}

void _updateParentRemoveChild(
	fd& fs,
	usize parentIndex,
	usize childIndex,
	MetaData& metaData
){
	// Modifique o pai P de F da seguinte forma:
	//     Seja B a lista de filhos de P armazenado na região de blocos. 
	//     Se B[k] = F e 0 ≤ k < P.SIZE -1 (ou seja, F não é o último filho de P), faça
	//         B[j] = B[j+1] para j=k, k+1, …, P.SIZE - 2 (copie da esq para direita)
	//     Decremente P.SIZE
	//     Se um bloco foi desocupado, marque-o como livre no mapa de bits

	INODE parent = _fetchINodeByIndex(fs, parentIndex, metaData);
	
	c8 childToBeRemovedBlockIndex{-1};
	c8 childIndexInParentINode{-1};
	bool flagExit{false};
	for(usize blockIndex = 0; blockIndex < 3 /* direct blocks size */ && !flagExit; blockIndex++){
		for(usize byteIndex = 0; byteIndex < static_cast<usize>(metaData.blockSize); byteIndex++){
			fs.seekg(
				_getBlocksOffSet(metaData.numBlocks, metaData.numINodes)
					+ parent.DIRECT_BLOCKS[blockIndex]*metaData.blockSize
					+ byteIndex
			).read(&childToBeRemovedBlockIndex, sizeof(c8));
			if(static_cast<usize>(childToBeRemovedBlockIndex) == childIndex){
				childIndexInParentINode = blockIndex;
				flagExit = true;
				break;
			}
		}
	}

	
	for(usize i = childIndex; i <= static_cast<usize>(parent.SIZE - 1); i++){
		usize curBlockIndexInINode = floor(i / metaData.blockSize);
		usize curBlockIndex = parent.DIRECT_BLOCKS[curBlockIndexInINode];
		usize curByteIndex = i % metaData.blockSize;
		usize curBlockOffSet = _getBlocksOffSet(metaData.numBlocks, metaData.numINodes)
			+ curBlockIndex*metaData.blockSize
			+ curByteIndex;

		usize nextBlockIndexInINode = floor((i + 1) / metaData.blockSize);
		usize nextBlockIndex = parent.DIRECT_BLOCKS[nextBlockIndexInINode];
		usize nextByteIndex = (i + 1) % metaData.blockSize;
		usize nextBlockOffSet = _getBlocksOffSet(metaData.numBlocks, metaData.numINodes)
			+ nextBlockIndex*metaData.blockSize
			+ nextByteIndex;

		c8 next{};
		fs.seekg(curBlockOffSet).read(&next, sizeof(c8));
		fs.seekp(nextBlockOffSet).write(&next, sizeof(c8));
	}
	
	
	if(parent.IS_DIR != 1 and parent.SIZE != 0){
		_writeBitMapAt(fs, parent.DIRECT_BLOCKS[childIndexInParentINode], 0, _getBitMapOffSet());
	}
	fs.flush();
	parent.SIZE--;
	//parent.DIRECT_BLOCKS[childIndexInParentINode] = 0;
	_writeINodeByIndex(fs, parent, parentIndex, metaData);
	fs.flush();

}

void initFs(std::string fsFileName, int blockSize, int numBlocks, int numInodes)
{
	truncate_file(fsFileName);
	std::fstream fs{ fsFileName, std::ios::binary | std::ios::in | std::ios::out };
	_writeMetaData(fs, blockSize, numBlocks, numInodes, _getMetaDataOffSet());
	_writeBitMapFill(fs, 0, numBlocks, _getBitMapOffSet());
	_writeINodeRoot(fs, numInodes, _getINodesOffSet(numBlocks));
	_writeRootIndex(fs, _getRootIndexOffSet(numBlocks, numInodes));
	_writeBlocksFill(fs, 0, numBlocks, blockSize, _getBlocksOffSet(numBlocks, numInodes));
}

void addFile(std::string fsFileName, std::string filePath, std::string fileContent)
{
	std::fstream fs{ fsFileName, std::ios::binary | std::ios::in | std::ios::out };
	auto metaData = _fetchMetadata(fs);
	
	auto blocksIndex = _writeBlocks(fs, fileContent);
	auto fileStructure = _parsePath(filePath);
	auto inode = INODE_factory(1, 0, fileStructure.name, fileContent.size(), blocksIndex);
	auto inodeIndex = _writeINode(fs, inode, metaData);

	_updateParentAddChild(fs, fileStructure.parents.back(), inodeIndex, inode, metaData);
}

void addDir(std::string fsFileName, std::string dirPath)
{
	std::fstream fs{ fsFileName, std::ios::binary | std::ios::in | std::ios::out };
	auto metaData = _fetchMetadata(fs);


	str empty{""}; // Cause every directory must have at least one block alocated
	auto blocksIndex = _writeBlocks(fs, empty);
	
	auto dirStructure = _parsePath(dirPath);
	auto inode = INODE_factory(1, 1, dirStructure.name,	0, blocksIndex);
	auto inodeIndex = _writeINode(fs, inode, metaData);

	_updateParentAddChild(fs, dirStructure.parents.back(), inodeIndex, inode, metaData);
}

void remove(std::string fsFileName, std::string path)
{
	std::fstream fs{ fsFileName, std::ios::binary | std::ios::in | std::ios::out };
	auto metaData = _fetchMetadata(fs);

	auto dirStructure = _parsePath(path);
	auto iNodeIndex = _findINodeIndexByName(fs, dirStructure.name, metaData);
	auto iNodeBlocks = _fetchINodeBlocks(fs, iNodeIndex, metaData);
	_updateFreeBlocks(fs, iNodeBlocks, metaData);
	_removeINode(fs, iNodeIndex, metaData);

	auto parentName = dirStructure.parents.back();
	_updateParentRemoveChild(fs, _findINodeIndexByName(fs, parentName, metaData), iNodeIndex, metaData);
}

void move(std::string fsFileName, std::string oldPath, std::string newPath)
{
	
}
