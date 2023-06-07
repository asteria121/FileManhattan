#include "RemoveFile.h"

void ProcessRemoveCallback(RemoveCallbackThread param)
{
	while (param.isRunning)
	{
		param.removeUpdateCallback(*param.data);
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

RemoveResult OverwriteRandom(HANDLE hFile, DWORD bufferSize, long long totalFileSize, std::string currentWork, RemoveCallback removeUpdateCallback)
{
	CryptoPP::SecByteBlock buffer(bufferSize);	// 디스크 클러스터 크기만큼 덮어씌울 버퍼 생성
	GenerateRandomBlock(&buffer);

	DWORD dwBytesWritten = 0;
	double rate = 0.0;
	long long totalWritten = 0;

	std::atomic<bool> isRunning(true);											// UI 콜백 스레드 작동을 관리하는 공유자원
	RemoveProgressInfo data = { rate, totalWritten, currentWork.c_str(), 0 };	// 콜백함수에서 여러개의 파라미터를 전달하기 위한 구조체
	RemoveCallbackThread param = { isRunning, &data, removeUpdateCallback };	// 스레드에 여러개의 파라미터를 전달하기 위한 구조체
	std::thread t(ProcessRemoveCallback, std::ref(param));						// UI 콜백 스레드 시작
	// 파일의 크기 < 덮어씌운 크기일때까지 WriteFile() 진행
	for (totalWritten = 0; totalWritten < totalFileSize; totalWritten += dwBytesWritten)
	{
		if (!WriteFile(hFile, buffer.data(), bufferSize, &dwBytesWritten, NULL))
			return RemoveResult::WRITE_FAILED;

		rate = static_cast<double>(totalWritten) / totalFileSize;		// 현재 작업 진행상황 백분율로 반영
		data.progressRate = rate;
		data.totalBytesWritten = totalWritten;
	}
	isRunning = false;													// 공유자원을 false로 변경하여 스레드 반복문 탈출
	t.join();															// 스레드 종료까지 대기
	RemoveProgressInfo finish = { 1.0, totalWritten, currentWork.c_str(), 1 };
	removeUpdateCallback(finish);										// 작업 완료를 나타내는 콜백 정보 전송

	LARGE_INTEGER fileSize;
	fileSize.QuadPart = 0;
	if (!SetFilePointerEx(hFile, fileSize, NULL, FILE_BEGIN))
		return RemoveResult::SEEK_FAILED;

	return RemoveResult::SUCCESS;
}

RemoveResult Overwrite3Bytes(HANDLE hFile, PBYTE buff, DWORD bufferSize, long long totalFileSize, std::string currentWork, RemoveCallback removeUpdateCallback)
{
	CryptoPP::SecByteBlock buffer(bufferSize * 3);	// 디스크 클러스터 크기만큼 덮어씌울 버퍼 생성
	
	for (DWORD i = 0; i < bufferSize * 3; i++)
	{
		buffer.begin()[i] = buff[i % 3];
	}

	DWORD dwBytesWritten = 0;
	double rate = 0.0;
	long long totalWritten = 0;

	std::atomic<bool> isRunning(true);											// UI 콜백 스레드 작동을 관리하는 공유자원
	RemoveProgressInfo data = { rate, totalWritten, currentWork.c_str(), 0 };	// 콜백함수에서 여러개의 파라미터를 전달하기 위한 구조체
	RemoveCallbackThread param = { isRunning, &data, removeUpdateCallback };	// 스레드에 여러개의 파라미터를 전달하기 위한 구조체
	std::thread t(ProcessRemoveCallback, std::ref(param));						// UI 콜백 스레드 시작
	// 파일의 크기 < 덮어씌운 크기일때까지 WriteFile() 진행
	for (totalWritten = 0; totalWritten < totalFileSize; totalWritten += dwBytesWritten)
	{
		if (!WriteFile(hFile, buffer.data() + (totalWritten % (static_cast<long long>(bufferSize) * 3)), bufferSize, &dwBytesWritten, NULL))
			return RemoveResult::WRITE_FAILED;

		rate = static_cast<double>(totalWritten) / totalFileSize;		// 현재 작업 진행상황 백분율로 반영
		data.progressRate = rate;
		data.totalBytesWritten = totalWritten;
	}
	isRunning = false;													// 공유자원을 false로 변경하여 스레드 반복문 탈출
	t.join();															// 스레드 종료까지 대기
	RemoveProgressInfo finish = { 1.0, totalWritten, currentWork.c_str(), 1 };
	removeUpdateCallback(finish);										// 작업 완료를 나타내는 콜백 정보 전송

	LARGE_INTEGER fileSize;
	fileSize.QuadPart = 0;
	if (!SetFilePointerEx(hFile, fileSize, NULL, FILE_BEGIN))
		return RemoveResult::SEEK_FAILED;

	return RemoveResult::SUCCESS;
}

RemoveResult Overwrite(HANDLE hFile, BYTE buff, DWORD bufferSize, long long totalFileSize, std::string currentWork, RemoveCallback removeUpdateCallback)
{
	CryptoPP::SecByteBlock buffer(bufferSize);	// 디스크 클러스터 크기만큼 덮어씌울 버퍼 생성
	memset(buffer.begin(), buff, buffer.size());

	DWORD dwBytesWritten = 0;
	double rate = 0.0;
	long long totalWritten = 0;

	std::atomic<bool> isRunning(true);											// UI 콜백 스레드 작동을 관리하는 공유자원
	RemoveProgressInfo data = { rate, totalWritten, currentWork.c_str(), 0 };	// 콜백함수에서 여러개의 파라미터를 전달하기 위한 구조체
	RemoveCallbackThread param = { isRunning, &data, removeUpdateCallback };	// 스레드에 여러개의 파라미터를 전달하기 위한 구조체
	std::thread t(ProcessRemoveCallback, std::ref(param));						// UI 콜백 스레드 시작
	// 파일의 크기 < 덮어씌운 크기일때까지 WriteFile() 진행
	for (totalWritten = 0; totalWritten < totalFileSize; totalWritten += dwBytesWritten)
	{
		if (!WriteFile(hFile, buffer.data(), bufferSize, &dwBytesWritten, NULL))
			return RemoveResult::WRITE_FAILED;

		rate = static_cast<double>(totalWritten) / totalFileSize;		// 현재 작업 진행상황 백분율로 반영
		data.progressRate = rate;
		data.totalBytesWritten = totalWritten;
	}
	isRunning = false;													// 공유자원을 false로 변경하여 스레드 반복문 탈출
	t.join();															// 스레드 종료까지 대기
	RemoveProgressInfo finish = { 1.0, totalWritten, currentWork.c_str(), 1 };
	removeUpdateCallback(finish);										// 작업 완료를 나타내는 콜백 정보 전송

	LARGE_INTEGER fileSize;
	fileSize.QuadPart = 0;
	if (!SetFilePointerEx(hFile, fileSize, NULL, FILE_BEGIN))
		return RemoveResult::SEEK_FAILED;

	return RemoveResult::SUCCESS;
}

extern "C"
{
	__declspec(dllexport) RemoveResult SecureRemoveFile(LPCSTR inputFile, RemoveAlgorithm mode, RemoveCallback removeUpdateCallback)
	{
		if (mode == NORMALDELETE)
		{
			DeleteFileA(inputFile);
			return RemoveResult::SUCCESS;
		}

		HANDLE hFile = CreateFileA(inputFile, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			return RemoveResult::INVALID_HANDLE;
		}

		LARGE_INTEGER fileSize;
		GetFileSizeEx(hFile, &fileSize);
		int clusterSize = GetDiskClusterSize(inputFile);
		if (clusterSize == 0) clusterSize = 4096;			// 클러스터 크기 구하기에 실패하면 보편적 크기인 4096으로 클러스터 크기 설정

		ErasePass passes[MAX_OVERWRITE];
		int passCounts;
		GetPassArray(passes, &passCounts, mode);
		std::string currentWork;

		for (int i = 0; i < passCounts; i++)
		{
			currentWork = std::format("{}/{}단계 덮어쓰기", i + 1, passCounts);
			if (passes[i].Method == ErasePassMethod::ONEBYTE)
			{
				RemoveResult result = Overwrite(hFile, passes[i].OneByte, clusterSize, fileSize.QuadPart, currentWork, removeUpdateCallback);
				if (result != RemoveResult::SUCCESS)
				{
					CloseHandle(hFile);
					return result;
				}
			}
			else if (passes[i].Method == ErasePassMethod::THREEBYTE)
			{
				RemoveResult result = Overwrite3Bytes(hFile, passes[i].ThreeByte, clusterSize, fileSize.QuadPart, currentWork, removeUpdateCallback);
				if (result != RemoveResult::SUCCESS)
				{
					CloseHandle(hFile);
					return result;
				}
			}
			else // if (passes[i].Method == ErasePassMethod::RANDOM)
			{
				RemoveResult result = OverwriteRandom(hFile, clusterSize, fileSize.QuadPart, currentWork, removeUpdateCallback);
				if (result != RemoveResult::SUCCESS)
				{
					CloseHandle(hFile);
					return result;
				}
			}
		}

		SetRandomFileStamp(hFile, passCounts);								// 타임 스탬프 변경
		ChangeFileSize(hFile, 0);											// 파일 크기 0으로 변경
		CloseHandle(hFile);													// 이름 및 속성 변경전에 파일 핸들 해제

		char newFilePath[MAX_PATH];											// 파일 이름 변경 후 경로로 삭제하기 위해 이 배열에 저장
		SetRandomFileAttribute(inputFile, passCounts);						// 파일 속성 변경
		SetRandomFileName(inputFile, newFilePath, MAX_PATH, passCounts);	// 파일 이름 변경
		DeleteFileA(newFilePath);											// 최종적으로 파일 삭제

		return RemoveResult::SUCCESS;
	}
}