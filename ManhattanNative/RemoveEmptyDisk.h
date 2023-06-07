#pragma once

#include <Windows.h>
#include <format>
#include <random>
#include <thread>
#include <chrono>
#include <atomic>
#include <filesystem>

#include "DiskUtility.h"
#include "Utility.h"
#include "ErasureMethod.h"

#include "osrng.h"
#include "randpool.h"
#include "modes.h"
#include "aes.h"
#include "filters.h"
#include "files.h"
#include "sha.h"

enum class RemoveEmptyResult
{
	SUCCESS,
	INVALID_HANDLE,
	SEEK_FAILED,
	WRITE_FAILED,
	RETRIEVE_EMPTY_SPACE_FAILED,
	EXCEPTION
};

enum class RemoveMFTResult
{
	SUCCESS,
	INVALID_HANDLE,
	GET_NTFS_VOLUME_FAILED,
	EXCEPTION
};

enum class RemoveClusterTipResult
{
	SUCCESS,
	INVALID_HANDLE,
	SEEK_FAILED,
	WRITE_FAILED,
	EXCEPTION
};

#pragma pack(push, 1)  // 패딩 제거
struct EmptyProgressInfo
{
	double progressRate;
	long long totalBytesWritten;
	LPCSTR currentWork;
	int isFinished;
};
#pragma pack(pop)

typedef void (*EmptyCallback)(EmptyProgressInfo);

struct EmptyCallbackThread
{
	std::atomic<bool>& isRunning;
	EmptyProgressInfo* data;
	EmptyCallback emptyUpdateCallback;
};

extern "C"
{
	__declspec(dllexport) RemoveMFTResult WipeMFT(LPCSTR inputDisk);
	__declspec(dllexport) RemoveEmptyResult RemoveEmptySpace(LPCSTR inputDisk, RemoveAlgorithm mode, EmptyCallback emptyUpdateCallback);
	__declspec(dllexport) RemoveClusterTipResult RemoveClusterTip(LPCSTR inputFile, RemoveAlgorithm mode);
}