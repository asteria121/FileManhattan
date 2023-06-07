#include "RemoveEmptyDisk.h"

void ProcessEmptyCallback(EmptyCallbackThread param)
{
	while (param.isRunning)
	{
		param.emptyUpdateCallback(*param.data);
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

RemoveEmptyResult OverwriteEmptyRandom(LPCSTR inputDisk, int clusterSize, std::string currentWork, EmptyCallback emptyUpdateCallback)
{
	std::string tmpFile = inputDisk;
	char randFileName[MAX_PATH];
	GenerateRandomFileName(randFileName, 16);
	tmpFile.append(randFileName);
	tmpFile.append(".tmp");

	HANDLE hFile = CreateFileA(tmpFile.c_str(), GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return RemoveEmptyResult::INVALID_HANDLE;

	CryptoPP::SecByteBlock buffer(clusterSize);	// 디스크 클러스터 크기만큼 덮어씌울 버퍼 생성
	GenerateRandomBlock(&buffer);

	DWORD dwBytesWritten;
	ULARGE_INTEGER lpTotalNumberOfFreeBytes;
	if (!GetDiskFreeSpaceExA(inputDisk, NULL, NULL, &lpTotalNumberOfFreeBytes))
	{
		CloseHandle(hFile);
		return RemoveEmptyResult::RETRIEVE_EMPTY_SPACE_FAILED;
	}
		

	long long currentFreeSpace = lpTotalNumberOfFreeBytes.QuadPart;

	long totalWritten = 0;
	double rate = 0.0;

	std::atomic<bool> isRunning(true);											// UI 콜백 스레드 작동을 관리하는 공유자원
	EmptyProgressInfo data = { rate, totalWritten, currentWork.c_str(), 0 };	// 콜백함수에서 여러개의 파라미터를 전달하기 위한 구조체
	EmptyCallbackThread param = { isRunning, &data, emptyUpdateCallback };		// 스레드에 여러개의 파라미터를 전달하기 위한 구조체
	std::thread t(ProcessEmptyCallback, std::ref(param));						// UI 콜백 스레드 시작
	// 디스크 빈공간이 0이 될때까지 진행
	while (lpTotalNumberOfFreeBytes.QuadPart >= clusterSize)
	{
		if (!WriteFile(hFile, buffer.data(), clusterSize, &dwBytesWritten, NULL))
		{
			CloseHandle(hFile);
			return RemoveEmptyResult::WRITE_FAILED;
		}
			
		if (!GetDiskFreeSpaceExA(inputDisk, NULL, NULL, &lpTotalNumberOfFreeBytes))	// 실시간으로 남은 디스크 공간 갱신
		{
			CloseHandle(hFile);
			return RemoveEmptyResult::RETRIEVE_EMPTY_SPACE_FAILED;
		}

		totalWritten += dwBytesWritten;
		rate = static_cast<double>(totalWritten) / currentFreeSpace;	// 현재 작업 진행상황 백분율로 반영
		data.progressRate = rate;
		data.totalBytesWritten = totalWritten;
		if (totalWritten % (1024 * 1024 * 100) == 0)
			FlushFileBuffers(hFile);
	}
	FlushFileBuffers(hFile);											// 마지막까지 버퍼 플러시
	isRunning = false;													// 공유자원을 false로 변경하여 스레드 반복문 탈출
	t.join();															// 스레드 종료까지 대기
	EmptyProgressInfo finish = { 1.0, totalWritten, currentWork.c_str(), 1 };
	emptyUpdateCallback(finish);										// 작업 완료를 나타내는 콜백 정보 전송

	SetRandomFileStamp(hFile, 1);
	ChangeFileSize(hFile, 0);
	CloseHandle(hFile);
	DeleteFileA(tmpFile.c_str());

	return RemoveEmptyResult::SUCCESS;
}

RemoveEmptyResult OverwriteEmpty3Bytes(LPCSTR inputDisk, PBYTE buff, int clusterSize, std::string currentWork, EmptyCallback emptyUpdateCallback)
{
	std::string tmpFile = inputDisk;
	char randFileName[MAX_PATH];
	GenerateRandomFileName(randFileName, 16);
	tmpFile.append(randFileName);
	tmpFile.append(".tmp");

	HANDLE hFile = CreateFileA(tmpFile.c_str(), GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return RemoveEmptyResult::INVALID_HANDLE;

	CryptoPP::SecByteBlock buffer(clusterSize);	// 디스크 클러스터 크기만큼 덮어씌울 버퍼 생성

	for (int i = 0; i < clusterSize; i++)
		buffer.begin()[i] = buff[i % 3];

	DWORD dwBytesWritten;
	ULARGE_INTEGER lpTotalNumberOfFreeBytes;
	if (!GetDiskFreeSpaceExA(inputDisk, NULL, NULL, &lpTotalNumberOfFreeBytes))
	{
		CloseHandle(hFile);
		return RemoveEmptyResult::RETRIEVE_EMPTY_SPACE_FAILED;
	}

	long long currentFreeSpace = lpTotalNumberOfFreeBytes.QuadPart;

	long totalWritten = 0;
	double rate = 0.0;

	std::atomic<bool> isRunning(true);											// UI 콜백 스레드 작동을 관리하는 공유자원
	EmptyProgressInfo data = { rate, totalWritten, currentWork.c_str(), 0 };	// 콜백함수에서 여러개의 파라미터를 전달하기 위한 구조체
	EmptyCallbackThread param = { isRunning, &data, emptyUpdateCallback };		// 스레드에 여러개의 파라미터를 전달하기 위한 구조체
	std::thread t(ProcessEmptyCallback, std::ref(param));							// UI 콜백 스레드 시작
	// 디스크 빈공간이 0이 될때까지 진행
	while (lpTotalNumberOfFreeBytes.QuadPart >= clusterSize)
	{
		if (!WriteFile(hFile, buffer.data() + (totalWritten % (clusterSize * 3)), clusterSize, &dwBytesWritten, NULL))
		{
			CloseHandle(hFile);
			return RemoveEmptyResult::WRITE_FAILED;
		}
			
		if (!GetDiskFreeSpaceExA(inputDisk, NULL, NULL, &lpTotalNumberOfFreeBytes))	// 실시간으로 남은 디스크 공간 갱신
		{
			CloseHandle(hFile);
			return RemoveEmptyResult::RETRIEVE_EMPTY_SPACE_FAILED;
		}

		totalWritten += dwBytesWritten;
		rate = static_cast<double>(totalWritten) / currentFreeSpace;	// 현재 작업 진행상황 백분율로 반영
		data.progressRate = rate;
		data.totalBytesWritten = totalWritten;
		if (totalWritten % (1024 * 1024 * 100) == 0)
			FlushFileBuffers(hFile);
	}
	FlushFileBuffers(hFile);											// 마지막까지 버퍼 플러시
	isRunning = false;													// 공유자원을 false로 변경하여 스레드 반복문 탈출
	t.join();															// 스레드 종료까지 대기
	EmptyProgressInfo finish = { 1.0, totalWritten, currentWork.c_str(), 1 };
	emptyUpdateCallback(finish);										// 작업 완료를 나타내는 콜백 정보 전송

	SetRandomFileStamp(hFile, 1);
	ChangeFileSize(hFile, 0);
	CloseHandle(hFile);
	DeleteFileA(tmpFile.c_str());

	return RemoveEmptyResult::SUCCESS;
}

RemoveEmptyResult OverwriteEmpty(LPCSTR inputDisk, BYTE buff, int clusterSize, std::string currentWork, EmptyCallback emptyUpdateCallback)
{
	std::string tmpFile = inputDisk;
	char randFileName[MAX_PATH];
	GenerateRandomFileName(randFileName, 16);
	tmpFile.append(randFileName);
	tmpFile.append(".tmp");

	HANDLE hFile = CreateFileA(tmpFile.c_str(), GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return RemoveEmptyResult::INVALID_HANDLE;

	CryptoPP::SecByteBlock buffer(clusterSize);	// 디스크 클러스터 크기만큼 덮어씌울 버퍼 생성
	memset(buffer.begin(), buff, buffer.size());

	DWORD dwBytesWritten;
	ULARGE_INTEGER lpTotalNumberOfFreeBytes;
	if (!GetDiskFreeSpaceExA(inputDisk, NULL, NULL, &lpTotalNumberOfFreeBytes))
	{
		CloseHandle(hFile);
		return RemoveEmptyResult::RETRIEVE_EMPTY_SPACE_FAILED;
	}

	long long currentFreeSpace = lpTotalNumberOfFreeBytes.QuadPart;

	long totalWritten = 0;
	double rate = 0.0;

	std::atomic<bool> isRunning(true);											// UI 콜백 스레드 작동을 관리하는 공유자원
	EmptyProgressInfo data = { rate, totalWritten, currentWork.c_str(), 0 };	// 콜백함수에서 여러개의 파라미터를 전달하기 위한 구조체
	EmptyCallbackThread param = { isRunning, &data, emptyUpdateCallback };		// 스레드에 여러개의 파라미터를 전달하기 위한 구조체
	std::thread t(ProcessEmptyCallback, std::ref(param));							// UI 콜백 스레드 시작
	// 디스크 빈공간이 0이 될때까지 진행
	while (lpTotalNumberOfFreeBytes.QuadPart >= clusterSize)
	{
		if (!WriteFile(hFile, buffer.data() + (totalWritten % (clusterSize * 3)), clusterSize, &dwBytesWritten, NULL))
		{
			CloseHandle(hFile);
			return RemoveEmptyResult::WRITE_FAILED;
		}
			
		if (!GetDiskFreeSpaceExA(inputDisk, NULL, NULL, &lpTotalNumberOfFreeBytes))	// 실시간으로 남은 디스크 공간 갱신
		{
			CloseHandle(hFile);
			return RemoveEmptyResult::RETRIEVE_EMPTY_SPACE_FAILED;
		}

		totalWritten += dwBytesWritten;
		rate = static_cast<double>(totalWritten) / currentFreeSpace;	// 현재 작업 진행상황 백분율로 반영
		data.progressRate = rate;
		data.totalBytesWritten = totalWritten;
		if (totalWritten % (1024 * 1024 * 100) == 0)
			FlushFileBuffers(hFile);
	}
	FlushFileBuffers(hFile);											// 마지막까지 버퍼 플러시
	isRunning = false;													// 공유자원을 false로 변경하여 스레드 반복문 탈출
	t.join();															// 스레드 종료까지 대기
	EmptyProgressInfo finish = { 1.0, totalWritten, currentWork.c_str(), 1 };
	emptyUpdateCallback(finish);										// 작업 완료를 나타내는 콜백 정보 전송

	SetRandomFileStamp(hFile, 1);
	ChangeFileSize(hFile, 0);
	CloseHandle(hFile);
	DeleteFileA(tmpFile.c_str());

	return RemoveEmptyResult::SUCCESS;
}

RemoveClusterTipResult OverwriteTipRandom(HANDLE hFile, int clusterTipSize, long long originalFileSize)
{
	CryptoPP::SecByteBlock buffer(clusterTipSize);	// 디스크 클러스터 크기만큼 덮어씌울 버퍼 생성
	GenerateRandomBlock(&buffer);

	DWORD dwBytesWritten = 0;
	// 파일의 크기 < 덮어씌운 크기일때까지 WriteFile() 진행
	if (!WriteFile(hFile, buffer.data(), clusterTipSize, &dwBytesWritten, NULL))
		return RemoveClusterTipResult::WRITE_FAILED;

	LARGE_INTEGER fileSize;
	fileSize.QuadPart = originalFileSize;
	if (!SetFilePointerEx(hFile, fileSize, NULL, FILE_BEGIN))
		return RemoveClusterTipResult::SEEK_FAILED;

	return RemoveClusterTipResult::SUCCESS;
}

RemoveClusterTipResult OverwriteTip3Bytes(HANDLE hFile, PBYTE buff, int clusterTipSize, long long originalFileSize)
{
	CryptoPP::SecByteBlock buffer(clusterTipSize);	// 디스크 클러스터 크기만큼 덮어씌울 버퍼 생성

	for (int i = 0; i < clusterTipSize; i++)
		buffer.begin()[i] = buff[i % 3];

	DWORD dwBytesWritten = 0;
	// 파일의 크기 < 덮어씌운 크기일때까지 WriteFile() 진행
	if (!WriteFile(hFile, buffer.data(), clusterTipSize, &dwBytesWritten, NULL))
		return RemoveClusterTipResult::WRITE_FAILED;

	LARGE_INTEGER fileSize;
	fileSize.QuadPart = originalFileSize;
	if (!SetFilePointerEx(hFile, fileSize, NULL, FILE_BEGIN))
		return RemoveClusterTipResult::SEEK_FAILED;

	return RemoveClusterTipResult::SUCCESS;
}

RemoveClusterTipResult OverwriteTip(HANDLE hFile, BYTE buff, int clusterTipSize, long long originalFileSize)
{
	CryptoPP::SecByteBlock buffer(clusterTipSize);	// 디스크 클러스터 크기만큼 덮어씌울 버퍼 생성
	memset(buffer.begin(), buff, buffer.size());

	DWORD dwBytesWritten = 0;
	// 파일의 크기 < 덮어씌운 크기일때까지 WriteFile() 진행
	if (!WriteFile(hFile, buffer.data(), clusterTipSize, &dwBytesWritten, NULL))
		return RemoveClusterTipResult::WRITE_FAILED;

	LARGE_INTEGER fileSize;
	fileSize.QuadPart = originalFileSize;
	if (!SetFilePointerEx(hFile, fileSize, NULL, FILE_BEGIN))
		return RemoveClusterTipResult::SEEK_FAILED;

	return RemoveClusterTipResult::SUCCESS;
}

extern "C"
{
	__declspec(dllexport) RemoveMFTResult WipeMFT(LPCSTR inputDisk)
	{
		// NTFS 파일 시스템일 경우 동적으로 MFT에 존재하는 파일 정보 개수를 구해 그 수보다 더 많은 정크파일을 생성 후 삭제한다.
		NTFS_VOLUME_DATA_BUFFER volumeData;
		DWORD bytesReturned;
		HANDLE hVol = CreateFileA(std::format("\\\\.\\{}:", inputDisk[0]).c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
		if (hVol == INVALID_HANDLE_VALUE)
		{
			return RemoveMFTResult::INVALID_HANDLE;
		}
		if (!DeviceIoControl(hVol, FSCTL_GET_NTFS_VOLUME_DATA, NULL, 0, &volumeData, sizeof(volumeData), &bytesReturned, NULL))
		{
			CloseHandle(hVol);
			return RemoveMFTResult::GET_NTFS_VOLUME_FAILED;
		}
		CloseHandle(hVol);

		// MFT 길이에 비례하여 각 폴더당 250개의 정크파일을 생성 후 삭제한다.
		long long mftEntryCount = volumeData.MftValidDataLength.QuadPart / volumeData.BytesPerFileRecordSegment;
		long long directoryCount = (mftEntryCount / 250) + 1;
		std::string* directories = new std::string[directoryCount];
		for (long long i = 0; i < directoryCount; i++)
		{
			directories[i] = inputDisk;
			char randFileName[MAX_PATH];
			GenerateRandomFileName(randFileName, 20);
			directories[i].append(randFileName);
			if (GetFileAttributesA(directories[i].c_str()) == -1)
				CreateDirectoryA(directories[i].c_str(), NULL);
			
			for (int j = 0; j < 250; j++)
			{
				std::string tmpFilePath = directories[i];
				GenerateRandomFileName(randFileName, 220);
				tmpFilePath.append("\\");
				tmpFilePath.append(randFileName);

				HANDLE hFile = CreateFileA(tmpFilePath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
				if (hFile != INVALID_HANDLE_VALUE)
				{
					char newFilePath[MAX_PATH];
					SetRandomFileStamp(hFile, 5);
					CloseHandle(hFile);
					SetRandomFileName(tmpFilePath.c_str(), newFilePath, MAX_PATH, 5);
					SetRandomFileAttribute(tmpFilePath.c_str(), 5);
				}
			}
		}

		for (long long i = 0; i < directoryCount; i++)
		{
			std::filesystem::path p(directories[i]);
			try
			{
				std::filesystem::remove_all(p);
			}
			catch (const std::exception& e)
			{

			}
		}

		delete[] directories;
		return RemoveMFTResult::SUCCESS;
	}

	// 고용량 파일을 생성하여 클러스터 크기 단위로 디스크 빈공간을 채움.
	__declspec(dllexport) RemoveEmptyResult RemoveEmptySpace(LPCSTR inputDisk, RemoveAlgorithm mode, EmptyCallback emptyUpdateCallback)
	{
		int clusterSize = GetDiskClusterSize(inputDisk);
		if (clusterSize == 0) clusterSize = 4096;

		ErasePass passes[MAX_OVERWRITE];
		int passCounts;
		GetPassArray(passes, &passCounts, mode);
		std::string currentWork;

		for (int i = 0; i < passCounts; i++)
		{
			currentWork = std::format("{}/{}단계 덮어쓰기", i + 1, passCounts);
			if (passes[i].Method == ErasePassMethod::ONEBYTE)
			{
				RemoveEmptyResult result = OverwriteEmpty(inputDisk, passes[i].OneByte, clusterSize, currentWork, emptyUpdateCallback);
				if (result != RemoveEmptyResult::SUCCESS) return result;
			}
			else if (passes[i].Method == ErasePassMethod::THREEBYTE)
			{
				RemoveEmptyResult result = OverwriteEmpty3Bytes(inputDisk, passes[i].ThreeByte, clusterSize, currentWork, emptyUpdateCallback);
				if (result != RemoveEmptyResult::SUCCESS) return result;
			}
			else // if (passes[i].Method == ErasePassMethod::RANDOM)
			{
				RemoveEmptyResult result = OverwriteEmptyRandom(inputDisk, clusterSize, currentWork, emptyUpdateCallback);
				if (result != RemoveEmptyResult::SUCCESS) return result;
			}
		}

		return RemoveEmptyResult::SUCCESS;
	}

	// 파일 용량이 디스크 클러스터 크기보다 작아도 실제로는 디스크 클러스터 크기 단위로 디스크를 점유함.
	// 디스크 빈공간 삭제를 완벽히 진행하기 위해서는 모든 파일을 열어서 남는 디스크 클러스터 크기만큼 파일의 끝에 덮어 씌워야함.
	__declspec(dllexport) RemoveClusterTipResult RemoveClusterTip(LPCSTR inputFile, RemoveAlgorithm mode)
	{
		int clusterSize = GetDiskClusterSize(inputFile);
		if (clusterSize == 0) clusterSize = 4096;

		HANDLE hFile = CreateFileA(inputFile, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			return RemoveClusterTipResult::INVALID_HANDLE;
		}

		LARGE_INTEGER fileSize;								// 원본 파일의 크기를 백업 후 원래대로 되돌린다.
		GetFileSizeEx(hFile, &fileSize);

		if (fileSize.QuadPart % clusterSize == 0)
		{
			CloseHandle(hFile);
			return RemoveClusterTipResult::SUCCESS;			// 클러스터 사이즈로 나누어 떨어지는 경우 덮어씌울 필요가 없음.
		}

		long long overflowSize = 0;
		for (overflowSize = fileSize.QuadPart; overflowSize > clusterSize; overflowSize -= clusterSize);	// 클러스터 팁 크기 계산
		int clusterTipSize = clusterSize - static_cast<int>(overflowSize);

		LARGE_INTEGER zero;
		zero.QuadPart = 0;
		if (!SetFilePointerEx(hFile, zero, NULL, FILE_END))
		{
			CloseHandle(hFile);
			return RemoveClusterTipResult::SEEK_FAILED;
		}

		ErasePass passes[MAX_OVERWRITE];
		int passCounts;
		GetPassArray(passes, &passCounts, mode);
		std::string currentWork;

		for (int i = 0; i < passCounts; i++)
		{
			currentWork = std::format("{}/{}단계 덮어쓰기", i + 1, passCounts);
			if (passes[i].Method == ErasePassMethod::ONEBYTE)
			{
				RemoveClusterTipResult result = OverwriteTip(hFile, passes[i].OneByte, clusterTipSize, fileSize.QuadPart);
				if (result != RemoveClusterTipResult::SUCCESS)
				{
					CloseHandle(hFile);
					return result;
				}
			}
			else if (passes[i].Method == ErasePassMethod::THREEBYTE)
			{
				RemoveClusterTipResult result = OverwriteTip3Bytes(hFile, passes[i].ThreeByte, clusterTipSize, fileSize.QuadPart);
				if (result != RemoveClusterTipResult::SUCCESS)
				{
					CloseHandle(hFile);
					return result;
				}
			}
			else // if (passes[i].Method == ErasePassMethod::RANDOM)
			{
				RemoveClusterTipResult result = OverwriteTipRandom(hFile, clusterTipSize, fileSize.QuadPart);
				if (result != RemoveClusterTipResult::SUCCESS)
				{
					CloseHandle(hFile);
					return result;
				}
			}
			FlushFileBuffers(hFile);
		}
		
		ChangeFileSize(hFile, fileSize.QuadPart);			// 클러스터 팁을 덮어씌우고 원래 크기로 되돌린다.
		CloseHandle(hFile);

		return RemoveClusterTipResult::SUCCESS;
	}
}