#pragma once

#include <Windows.h>

#include "ErasureMethod.h"
#include "DiskUtility.h"
#include "Utility.h"

#include "osrng.h"
#include "randpool.h"

#pragma pack(push, 1)  // 패딩 제거
struct PartitionProgressInfo
{
	double progressRate;
	long long totalBytesWritten;
	LPCSTR currentWork;
	int isFinished;
};
#pragma pack(pop)

typedef void (*PartitionCallback)(PartitionProgressInfo);

struct PartitionCallbackThread
{
	std::atomic<bool>& isRunning;
	PartitionProgressInfo* data;
	PartitionCallback partitionUpdateCallback;
};

enum class PartitionRemoveResult
{
	SUCCESS,
	INVALID_HANDLE,
	RETRIEVE_DISK_INFO_FAILED,
	LOCK_FAILED,
	DISMOUNT_FAILED,
	SEEK_FAILED,
	WRITE_FAILED,
	EXCEPTION
};

extern "C"
{
	__declspec(dllexport) PartitionRemoveResult SecureRemovePartition(LPCSTR inputDisk, RemoveAlgorithm mode, PartitionCallback partitionUpdateCallback);
}