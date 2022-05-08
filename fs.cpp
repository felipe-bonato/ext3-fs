#ifndef fs_h
#define fs_h
#include <string>

typedef struct {
    unsigned char IS_USED;             // 0x01 se utilizado, 0x00 se livre
    unsigned char IS_DIR;              // 0x01 se diretorio, 0x00 se arquivo
    char NAME[10];                     // nome do arquivo/dir
    char SIZE;                         // tamanho do arquivo/dir em bytes
    unsigned char DIRECT_BLOCKS[3];    
    unsigned char INDIRECT_BLOCKS[3];
    unsigned char DOUBLE_INDIRECT_BLOCKS[3];
} INODE;

void initFs(std::string fsFileName, int blockSize, int numBlocks, int numInodes)
{

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


#endif /* fs_h */

#include "fs.h"