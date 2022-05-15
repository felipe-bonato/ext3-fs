#include "fs.h"

#include <cmath>
#include <fstream>
#include <memory>
#include <string>

typedef char c8;
typedef uint8_t u8;
typedef int32_t i32;
typedef size_t usize;
typedef std::string str;

class FileSystemManager {
public:
	FileSystemManager(std::string fsFileName, int blockSize, int numBlocks, int numInodes)
	{
		this->file = std::ofstream(fsFileName, std::ios::binary | std::ios::in | std::ios::out);
		if(!file.is_open()){
			throw std::runtime_error("Could not open file");
		}

		this->blockSize = blockSize;
		this->numBlocks = numBlocks;
		this->numInodes = numInodes;
	};

	~FileSystemManager()
	{
		this->file.close();
	};

	void initFs()
	{
		this->_writeInitMetadata();
		this->_writeBitMap();
		this->_writeRootINode();
		this->_writeZeroBlock();
	}

private:
	std::ofstream file;
	int blockSize;
	int numBlocks;
	int numInodes;

    /* Helpers */
	usize _getNumBitMapBytes()
	{
		// This is rounded up to the nearest byte;
		return std::ceil(static_cast<double>(this->numBlocks) / 8.0);
	}

	usize _getMetaDataByteOffSet() { return 0; }

	usize _getBitMapByteOffSet() { return _getMetaDataByteOffSet() + 3; }

	usize _getINodesByteOffSet() { return _getBitMapByteOffSet() + _getNumBitMapBytes(); }

	usize _getBlockByteOffSet() { return _getINodesByteOffSet() + this->numBlocks * this->blockSize; }

	void _writeInitMetadata() {
		this->file.seekp(this->_getMetaDataByteOffSet())
			<< static_cast<c8>(this->blockSize)
			<< static_cast<c8>(this->numBlocks)
			<< static_cast<c8>(this->numInodes);
	}

	void _writeBitMap() {
		this->file.seekp(this->_getBitMapByteOffSet());
		for(size_t byteIndex = 0; byteIndex < this->_getNumBitMapBytes(); byteIndex++){		
			file << static_cast<c8>(0);
		}
	}

	void _writeRootINode(){
		INODE root = {
			.IS_USED = 0x01,
			.IS_DIR = 0x01,
			// No aggregate initializer for the name cause gcc is bugged.
			// See link: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=55227.
			// Ps.: Should have used clang
			"/",
			.SIZE = 0,
			.DIRECT_BLOCKS = { 0, 0, 0 },
			.INDIRECT_BLOCKS = { 0, 0, 0 },
			.DOUBLE_INDIRECT_BLOCKS = { 0, 0, 0 }
		};

		this->file.seekp(this->_getINodesByteOffSet())
			<< reinterpret_cast<char*>(&root);
	}

	void _writeZeroBlock() {
		this->file.seekp(this->_getBlockByteOffSet());
		for(usize byteIndex = 0; byteIndex < static_cast<usize>(this->numBlocks); byteIndex++){
			file << static_cast<c8>(0);
		}
	}

};

void initFs(std::string fsFileName, int blockSize, int numBlocks, int numInodes)
{
	FileSystemManager fs(fsFileName, blockSize, numBlocks, numInodes);
	fs.initFs();
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
