#include "fs.h"

#include <string>
#include <fstream>

typedef char c8;

void _writeMetadata(std::string fsFileName, int blockSize, int numBlocks, int numInodes) {
	std::ofstream(fsFileName, std::ios::binary) << blockSize << numBlocks << numInodes;
	//file.write((c8*)&blockSize, sizeof(c8));
	//file.write((c8*)&numBlocks, sizeof(c8));
	//file.write((c8*)&numInodes, sizeof(c8));
	
}

void _writeBitMap(std::string fsFileName, int numBlocks, int numInodes) {

}

void _writeInodes(std::string fsFileName, int numInodes) {

}

void _writeRoot(std::string fsFileName, int numInodes) {

}

void _writeBlocks(std::string fsFileName, int numBlocks) {

}

void initFs(std::string fsFileName, int blockSize, int numBlocks, int numInodes)
{ 
	_writeMetadata(fsFileName, blockSize, numBlocks, numInodes);
	
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
