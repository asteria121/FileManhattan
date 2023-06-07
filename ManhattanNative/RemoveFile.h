#pragma once
#include <Windows.h>
#include <thread>
#include <chrono>
#include <atomic>

#include "ErasureMethod.h"
#include "DiskUtility.h"
#include "Utility.h"

#include "osrng.h"
#include "randpool.h"

#pragma comment(lib, "Shlwapi.lib")

enum class RemoveResult
{
	SUCCESS,
	INVALID_HANDLE,
	SEEK_FAILED,
	WRITE_FAILED,
	EXCEPTION
};

#pragma pack(push, 1)  // 패딩 제거
struct RemoveProgressInfo
{
	double progressRate;
	long long totalBytesWritten;
	LPCSTR currentWork;
	int isFinished;
};
#pragma pack(pop)

typedef void (*RemoveCallback)(RemoveProgressInfo);

struct RemoveCallbackThread
{
	std::atomic<bool>& isRunning;
	RemoveProgressInfo* data;
	RemoveCallback removeUpdateCallback;
};


extern "C"
{
	__declspec(dllexport) RemoveResult SecureRemoveFile(LPCSTR inputFile, RemoveAlgorithm mode, RemoveCallback removeUpdateCallback);
}