#include "fs.h"

#include <string>
#include <fstream>

#include <cmath>

#include <bitset>

typedef char c8;
typedef uint8_t u8;

struct fileInPos
{
	std::ofstream m_File;
	fileInPos(std::string& fileName, size_t position)
	{
		m_File = std::ofstream{fileName, std::ios::binary};
		m_File.seekp(position);
	}
	~fileInPos()
	{
		m_File.close();
	}
};

void _writeMetadata(std::string fsFileName, int blockSize, int numBlocks, int numInodes) {
	fileInPos file = fileInPos{fsFileName, 0};
	std::ofstream(fsFileName, std::ios::app)
		<< static_cast<u8>(blockSize)
		<< static_cast<u8>(numBlocks)
		<< static_cast<u8>(numInodes);
}

void _writeBitMap(std::string fsFileName, int numBlocks) {
	std::ofstream file(fsFileName, std::ios::binary);
	file.seekp(3); // Position where bitmap starts

	size_t numBitMapBytes = std::ceil(static_cast<double>(numBlocks) / 8.0); // This is rounded up to the nearest byte
	for (size_t byteIndex = 0; byteIndex < numBitMapBytes; byteIndex++)
	{		
		file << 0x00;
	}
	file.close();
}

void _writeInodes(std::string fsFileName, int numInodes) {

}

void _writeRoot(std::string fsFileName, int numInodes) {

}

void _writeBlocks(std::string fsFileName, int numBlocks) {

}

void initFs(std::string fsFileName, int blockSize, int numBlocks, int numInodes)
{
	// Resetting file
	std::ofstream file{fsFileName, std::ios::out};
	file.close(); // We open it and close right after cause cpp

	// Acctually initiating the fs
	_writeMetadata(fsFileName, blockSize, numBlocks, numInodes);
	_writeBitMap(fsFileName, numBlocks);
	
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
