#pragma once

#include <Windows.h>
#include <thread>
#include <chrono>
#include <atomic>


#include "Utility.h"

#include "modes.h"
#include "xts.h"
#include "aes.h"
#include "filters.h"
#include "files.h"
#include "sha3.h"

//패딩 제거
#pragma pack(push, 1)
struct CryptoProgressInfo
{
	double progressRate;
	long long totalBytesWritten;
};
#pragma pack(pop)

typedef void (*CryptoCallback)(CryptoProgressInfo);

struct CryptoCallbackThread
{
	std::atomic<bool>& isRunning;
	CryptoProgressInfo* data;
	CryptoCallback cryptoUpdateCallback;
};

enum class CryptoResult
{
	SUCCESS,
	WRONG_FILE,
	WRONG_PASSWORD,
	INPUT_NOT_EXISTS,
	OUTPUT_NOT_CREATABLE,
	EXCEPTION
};

#define SALT_SIZE 64
#define HEADER_SIZE 16
#define TOTAL_HEADER_SIZE 160
#define HEADER_TEXT "000FILEMANHATTAN"

extern "C"
{
	__declspec(dllexport) CryptoResult Encrypt(LPCSTR inputPath, LPCSTR outputPath, LPCSTR originalKey, CryptoCallback cryptoUpdateCallback);
	__declspec(dllexport) CryptoResult Decrypt(LPCSTR inputPath, LPCSTR outputPath, LPCSTR originalKey, CryptoCallback cryptoUpdateCallback);
}