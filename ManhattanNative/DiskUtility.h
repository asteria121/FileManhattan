#pragma once
#include <string>
#include <format>
#include <Windows.h>
#include <WinIoCtl.h>
#include <Ntddscsi.h>

#ifndef StorageDeviceSeekPenaltyProperty
#define StorageDeviceSeekPenaltyProperty 7
#endif

struct ATAIdentifyDeviceQuery
{
    ATA_PASS_THROUGH_EX header;
    WORD data[256];
};

extern "C"
{
    __declspec(dllexport) int GetDiskClusterSize(LPCSTR path);
    __declspec(dllexport) bool IsSolidStateDrive(int driveNo);
}