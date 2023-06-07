#include <Windows.h>
#include <Shlwapi.h>

#include "osrng.h"
#include "randpool.h"
#include "modes.h"
#include "aes.h"
#include "filters.h"
#include "files.h"
#include "sha.h"

void GenerateRandomFileName(LPSTR buffer, DWORD bufferLength);
void SetRandomFileName(LPCSTR oldFilePath, LPSTR newFilePath, DWORD newFilePathLength, DWORD repeatCount);
void SetRandomFileAttribute(LPCSTR inputFile, DWORD repeatCount);
void SetRandomFileStamp(HANDLE hFile, DWORD repeatCount);
void ChangeFileSize(HANDLE hFile, long long size);
void GenerateRandomBlock(CryptoPP::SecByteBlock* buffer);