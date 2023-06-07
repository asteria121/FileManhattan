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
	CryptoPP::SecByteBlock buffer(bufferSize);	// ��ũ Ŭ������ ũ�⸸ŭ ����� ���� ����
	GenerateRandomBlock(&buffer);

	DWORD dwBytesWritten = 0;
	double rate = 0.0;
	long long totalWritten = 0;

	std::atomic<bool> isRunning(true);											// UI �ݹ� ������ �۵��� �����ϴ� �����ڿ�
	RemoveProgressInfo data = { rate, totalWritten, currentWork.c_str(), 0 };	// �ݹ��Լ����� �������� �Ķ���͸� �����ϱ� ���� ����ü
	RemoveCallbackThread param = { isRunning, &data, removeUpdateCallback };	// �����忡 �������� �Ķ���͸� �����ϱ� ���� ����ü
	std::thread t(ProcessRemoveCallback, std::ref(param));						// UI �ݹ� ������ ����
	// ������ ũ�� < ����� ũ���϶����� WriteFile() ����
	for (totalWritten = 0; totalWritten < totalFileSize; totalWritten += dwBytesWritten)
	{
		if (!WriteFile(hFile, buffer.data(), bufferSize, &dwBytesWritten, NULL))
			return RemoveResult::WRITE_FAILED;

		rate = static_cast<double>(totalWritten) / totalFileSize;		// ���� �۾� �����Ȳ ������� �ݿ�
		data.progressRate = rate;
		data.totalBytesWritten = totalWritten;
	}
	isRunning = false;													// �����ڿ��� false�� �����Ͽ� ������ �ݺ��� Ż��
	t.join();															// ������ ������� ���
	RemoveProgressInfo finish = { 1.0, totalWritten, currentWork.c_str(), 1 };
	removeUpdateCallback(finish);										// �۾� �ϷḦ ��Ÿ���� �ݹ� ���� ����

	LARGE_INTEGER fileSize;
	fileSize.QuadPart = 0;
	if (!SetFilePointerEx(hFile, fileSize, NULL, FILE_BEGIN))
		return RemoveResult::SEEK_FAILED;

	return RemoveResult::SUCCESS;
}

RemoveResult Overwrite3Bytes(HANDLE hFile, PBYTE buff, DWORD bufferSize, long long totalFileSize, std::string currentWork, RemoveCallback removeUpdateCallback)
{
	CryptoPP::SecByteBlock buffer(bufferSize * 3);	// ��ũ Ŭ������ ũ�⸸ŭ ����� ���� ����
	
	for (DWORD i = 0; i < bufferSize * 3; i++)
	{
		buffer.begin()[i] = buff[i % 3];
	}

	DWORD dwBytesWritten = 0;
	double rate = 0.0;
	long long totalWritten = 0;

	std::atomic<bool> isRunning(true);											// UI �ݹ� ������ �۵��� �����ϴ� �����ڿ�
	RemoveProgressInfo data = { rate, totalWritten, currentWork.c_str(), 0 };	// �ݹ��Լ����� �������� �Ķ���͸� �����ϱ� ���� ����ü
	RemoveCallbackThread param = { isRunning, &data, removeUpdateCallback };	// �����忡 �������� �Ķ���͸� �����ϱ� ���� ����ü
	std::thread t(ProcessRemoveCallback, std::ref(param));						// UI �ݹ� ������ ����
	// ������ ũ�� < ����� ũ���϶����� WriteFile() ����
	for (totalWritten = 0; totalWritten < totalFileSize; totalWritten += dwBytesWritten)
	{
		if (!WriteFile(hFile, buffer.data() + (totalWritten % (static_cast<long long>(bufferSize) * 3)), bufferSize, &dwBytesWritten, NULL))
			return RemoveResult::WRITE_FAILED;

		rate = static_cast<double>(totalWritten) / totalFileSize;		// ���� �۾� �����Ȳ ������� �ݿ�
		data.progressRate = rate;
		data.totalBytesWritten = totalWritten;
	}
	isRunning = false;													// �����ڿ��� false�� �����Ͽ� ������ �ݺ��� Ż��
	t.join();															// ������ ������� ���
	RemoveProgressInfo finish = { 1.0, totalWritten, currentWork.c_str(), 1 };
	removeUpdateCallback(finish);										// �۾� �ϷḦ ��Ÿ���� �ݹ� ���� ����

	LARGE_INTEGER fileSize;
	fileSize.QuadPart = 0;
	if (!SetFilePointerEx(hFile, fileSize, NULL, FILE_BEGIN))
		return RemoveResult::SEEK_FAILED;

	return RemoveResult::SUCCESS;
}

RemoveResult Overwrite(HANDLE hFile, BYTE buff, DWORD bufferSize, long long totalFileSize, std::string currentWork, RemoveCallback removeUpdateCallback)
{
	CryptoPP::SecByteBlock buffer(bufferSize);	// ��ũ Ŭ������ ũ�⸸ŭ ����� ���� ����
	memset(buffer.begin(), buff, buffer.size());

	DWORD dwBytesWritten = 0;
	double rate = 0.0;
	long long totalWritten = 0;

	std::atomic<bool> isRunning(true);											// UI �ݹ� ������ �۵��� �����ϴ� �����ڿ�
	RemoveProgressInfo data = { rate, totalWritten, currentWork.c_str(), 0 };	// �ݹ��Լ����� �������� �Ķ���͸� �����ϱ� ���� ����ü
	RemoveCallbackThread param = { isRunning, &data, removeUpdateCallback };	// �����忡 �������� �Ķ���͸� �����ϱ� ���� ����ü
	std::thread t(ProcessRemoveCallback, std::ref(param));						// UI �ݹ� ������ ����
	// ������ ũ�� < ����� ũ���϶����� WriteFile() ����
	for (totalWritten = 0; totalWritten < totalFileSize; totalWritten += dwBytesWritten)
	{
		if (!WriteFile(hFile, buffer.data(), bufferSize, &dwBytesWritten, NULL))
			return RemoveResult::WRITE_FAILED;

		rate = static_cast<double>(totalWritten) / totalFileSize;		// ���� �۾� �����Ȳ ������� �ݿ�
		data.progressRate = rate;
		data.totalBytesWritten = totalWritten;
	}
	isRunning = false;													// �����ڿ��� false�� �����Ͽ� ������ �ݺ��� Ż��
	t.join();															// ������ ������� ���
	RemoveProgressInfo finish = { 1.0, totalWritten, currentWork.c_str(), 1 };
	removeUpdateCallback(finish);										// �۾� �ϷḦ ��Ÿ���� �ݹ� ���� ����

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
		if (clusterSize == 0) clusterSize = 4096;			// Ŭ������ ũ�� ���ϱ⿡ �����ϸ� ������ ũ���� 4096���� Ŭ������ ũ�� ����

		ErasePass passes[MAX_OVERWRITE];
		int passCounts;
		GetPassArray(passes, &passCounts, mode);
		std::string currentWork;

		for (int i = 0; i < passCounts; i++)
		{
			currentWork = std::format("{}/{}�ܰ� �����", i + 1, passCounts);
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

		SetRandomFileStamp(hFile, passCounts);								// Ÿ�� ������ ����
		ChangeFileSize(hFile, 0);											// ���� ũ�� 0���� ����
		CloseHandle(hFile);													// �̸� �� �Ӽ� �������� ���� �ڵ� ����

		char newFilePath[MAX_PATH];											// ���� �̸� ���� �� ��η� �����ϱ� ���� �� �迭�� ����
		SetRandomFileAttribute(inputFile, passCounts);						// ���� �Ӽ� ����
		SetRandomFileName(inputFile, newFilePath, MAX_PATH, passCounts);	// ���� �̸� ����
		DeleteFileA(newFilePath);											// ���������� ���� ����

		return RemoveResult::SUCCESS;
	}
}