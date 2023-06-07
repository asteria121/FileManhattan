#include "DiskUtility.h"

extern "C"
{
	// 관리자권한 필요 없음
	int GetDiskClusterSize(LPCSTR path)
	{
		std::string originalPath = path;
		std::string parentPath = originalPath.substr(0, 3);

		DWORD sectPerCluster, dwBytesPerSector, dwFreeClusters, dwTotalClusters;
		BOOL result = GetDiskFreeSpaceA(parentPath.c_str(), &sectPerCluster, &dwBytesPerSector, &dwFreeClusters, &dwTotalClusters);
		if (result == 0)
			return 0;
		else
			return sectPerCluster * dwBytesPerSector;
	}

	__declspec(dllexport) bool IsSolidStateDrive(int driveNo)
	{
		DWORD bytesReturned;
		std::string physDisk = "\\\\.\\PhysicalDrive";
		std::string physNo = std::to_string(driveNo);
		physDisk.append(physNo);

		//As an example, let's test 1st physical drive
		HANDLE hDevice = CreateFileA(physDisk.c_str(),
			GENERIC_READ | GENERIC_WRITE,       //We need write access to send ATA command to read RPMs
			FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
			OPEN_EXISTING, 0, NULL);
		if (hDevice != INVALID_HANDLE_VALUE)
		{
			// Seek Panalty 항목을 검사한다. HDD는 탐색 패널티를 발생시킴.
			STORAGE_PROPERTY_QUERY spqSeekP;
			spqSeekP.PropertyId = (STORAGE_PROPERTY_ID)StorageDeviceSeekPenaltyProperty;
			spqSeekP.QueryType = PropertyStandardQuery;

			bytesReturned = 0;
			DEVICE_SEEK_PENALTY_DESCRIPTOR dspd = { 0 };
			bool seekPanalty = false;
			if (DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY, &spqSeekP, sizeof(spqSeekP), &dspd, sizeof(dspd), &bytesReturned, NULL) && bytesReturned == sizeof(dspd))
			{
				if (dspd.IncursSeekPenalty)
					seekPanalty = true;
			}

			// 해당 드라이브의 RPM을 구한다. SSD는 쿼리값이 없거나 RPM이 1이 나와야함.
			ATAIdentifyDeviceQuery id_query;
			memset(&id_query, 0, sizeof(id_query));

			id_query.header.Length = sizeof(id_query.header);
			id_query.header.AtaFlags = ATA_FLAGS_DATA_IN;
			id_query.header.DataTransferLength = sizeof(id_query.data);
			id_query.header.TimeOutValue = 5;   //Timeout in seconds
			id_query.header.DataBufferOffset = offsetof(ATAIdentifyDeviceQuery, data[0]);
			id_query.header.CurrentTaskFile[6] = 0xec; // ATA IDENTIFY DEVICE

			bytesReturned = 0;
			UINT rpm = 0;
			if (DeviceIoControl(hDevice, IOCTL_ATA_PASS_THROUGH,
				&id_query, sizeof(id_query), &id_query, sizeof(id_query), &bytesReturned, NULL) &&
				bytesReturned == sizeof(id_query))
			{
				//Got it

				//Index of nominal media rotation rate
				//SOURCE: http://www.t13.org/documents/UploadedDocuments/docs2009/d2015r1a-ATAATAPI_Command_Set_-_2_ACS-2.pdf
				//          7.18.7.81 Word 217
				//QUOTE: Word 217 indicates the nominal media rotation rate of the device and is defined in table:
				//          Value           Description
				//          --------------------------------
				//          0000h           Rate not reported
				//          0001h           Non-rotating media (e.g., solid state device)
				//          0002h-0400h     Reserved
				//          0401h-FFFEh     Nominal media rotation rate in rotations per minute (rpm)
				//                                  (e.g., 7 200 rpm = 1C20h)
				//          FFFFh           Reserved
#define kNominalMediaRotRateWordIndex 217
				rpm = (UINT)id_query.data[kNominalMediaRotRateWordIndex];
			}
			CloseHandle(hDevice);
			if (seekPanalty == false && (rpm == 0 || rpm == 1))
				return true;
			else
				return false;
		}
		return false;
	}
}