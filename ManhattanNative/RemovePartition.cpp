#include "RemovePartition.h"

void ProcessPartitonCallback(PartitionCallbackThread param)
{
    while (param.isRunning)
    {
        param.partitionUpdateCallback(*param.data);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

PartitionRemoveResult OverwritePartitionRandom(HANDLE hFile, DWORD clusterSize, long long totalBytes, std::string currentWork, PartitionCallback partitionUpdateCallback)
{
    CryptoPP::SecByteBlock buffer(clusterSize);	// ��ũ Ŭ������ ũ�⸸ŭ ����� ���� ����
    GenerateRandomBlock(&buffer);

    DWORD dwBytesWritten = 0;
    double rate = 0.0;
    long long totalWritten = 0;

    std::atomic<bool> isRunning(true);											    // UI �ݹ� ������ �۵��� �����ϴ� �����ڿ�
    PartitionProgressInfo data = { rate, totalWritten, currentWork.c_str(), 0 };	// �ݹ��Լ����� �������� �Ķ���͸� �����ϱ� ���� ����ü
    PartitionCallbackThread param = { isRunning, &data, partitionUpdateCallback };	// �����忡 �������� �Ķ���͸� �����ϱ� ���� ����ü
    std::thread t(ProcessPartitonCallback, std::ref(param));						// UI �ݹ� ������ ����
    // ������ ũ�� < ����� ũ���϶����� WriteFile() ����
    for (totalWritten = 0; totalWritten < totalBytes; totalWritten += dwBytesWritten)
    {
        if (!WriteFile(hFile, buffer.data(), clusterSize, &dwBytesWritten, NULL))
            return PartitionRemoveResult::WRITE_FAILED;

        rate = static_cast<double>(totalWritten) / totalBytes;		    // ���� �۾� �����Ȳ ������� �ݿ�
        data.progressRate = rate;
        data.totalBytesWritten = totalWritten;
    }
    isRunning = false;													// �����ڿ��� false�� �����Ͽ� ������ �ݺ��� Ż��
    t.join();															// ������ ������� ���
    PartitionProgressInfo finish = { 1.0, totalWritten, currentWork.c_str(), 1 };
    partitionUpdateCallback(finish);								    // �۾� �ϷḦ ��Ÿ���� �ݹ� ���� ����

    LARGE_INTEGER fileSize;
    fileSize.QuadPart = 0;
    if (!SetFilePointerEx(hFile, fileSize, NULL, FILE_BEGIN))
        return PartitionRemoveResult::SEEK_FAILED;

    return PartitionRemoveResult::SUCCESS;
}

PartitionRemoveResult OverwritePartition3Bytes(HANDLE hFile, PBYTE buff, DWORD clusterSize, long long totalBytes, std::string currentWork, PartitionCallback partitionUpdateCallback)
{
    CryptoPP::SecByteBlock buffer(clusterSize * 3);	// ��ũ Ŭ������ ũ�⸸ŭ ����� ���� ����

    for (DWORD i = 0; i < clusterSize * 3; i++)
    {
        buffer.begin()[i] = buff[i % 3];
    }

    DWORD dwBytesWritten = 0;
    double rate = 0.0;
    long long totalWritten = 0;

    std::atomic<bool> isRunning(true);											    // UI �ݹ� ������ �۵��� �����ϴ� �����ڿ�
    PartitionProgressInfo data = { rate, totalWritten, currentWork.c_str(), 0 };	// �ݹ��Լ����� �������� �Ķ���͸� �����ϱ� ���� ����ü
    PartitionCallbackThread param = { isRunning, &data, partitionUpdateCallback };	// �����忡 �������� �Ķ���͸� �����ϱ� ���� ����ü
    std::thread t(ProcessPartitonCallback, std::ref(param));						// UI �ݹ� ������ ����
    // ������ ũ�� < ����� ũ���϶����� WriteFile() ����
    for (totalWritten = 0; totalWritten < totalBytes; totalWritten += dwBytesWritten)
    {
        if (!WriteFile(hFile, buffer.data() + (totalWritten % (clusterSize * 3)), clusterSize, &dwBytesWritten, NULL))
            return PartitionRemoveResult::WRITE_FAILED;

        rate = static_cast<double>(totalWritten) / totalBytes;		    // ���� �۾� �����Ȳ ������� �ݿ�
        data.progressRate = rate;
        data.totalBytesWritten = totalWritten;
    }
    isRunning = false;													// �����ڿ��� false�� �����Ͽ� ������ �ݺ��� Ż��
    t.join();															// ������ ������� ���
    PartitionProgressInfo finish = { 1.0, totalWritten, currentWork.c_str(), 1 };
    partitionUpdateCallback(finish);								    // �۾� �ϷḦ ��Ÿ���� �ݹ� ���� ����

    LARGE_INTEGER fileSize;
    fileSize.QuadPart = 0;
    if (!SetFilePointerEx(hFile, fileSize, NULL, FILE_BEGIN))
        return PartitionRemoveResult::SEEK_FAILED;

    return PartitionRemoveResult::SUCCESS;
}

PartitionRemoveResult OverwritePartition(HANDLE hFile, BYTE buff, DWORD clusterSize, long long totalBytes, std::string currentWork, PartitionCallback partitionUpdateCallback)
{
    CryptoPP::SecByteBlock buffer(clusterSize);	// ��ũ Ŭ������ ũ�⸸ŭ ����� ���� ����
    memset(buffer.begin(), buff, buffer.size());

    DWORD dwBytesWritten = 0;
    double rate = 0.0;
    long long totalWritten = 0;

    std::atomic<bool> isRunning(true);											    // UI �ݹ� ������ �۵��� �����ϴ� �����ڿ�
    PartitionProgressInfo data = { rate, totalWritten, currentWork.c_str(), 0 };	// �ݹ��Լ����� �������� �Ķ���͸� �����ϱ� ���� ����ü
    PartitionCallbackThread param = { isRunning, &data, partitionUpdateCallback };	// �����忡 �������� �Ķ���͸� �����ϱ� ���� ����ü
    std::thread t(ProcessPartitonCallback, std::ref(param));						// UI �ݹ� ������ ����
    // ������ ũ�� < ����� ũ���϶����� WriteFile() ����
    for (totalWritten = 0; totalWritten < totalBytes; totalWritten += dwBytesWritten)
    {
        if (!WriteFile(hFile, buffer.data(), clusterSize, &dwBytesWritten, NULL))
            return PartitionRemoveResult::WRITE_FAILED;

        rate = static_cast<double>(totalWritten) / totalBytes;		    // ���� �۾� �����Ȳ ������� �ݿ�
        data.progressRate = rate;
        data.totalBytesWritten = totalWritten;
    }
    isRunning = false;													// �����ڿ��� false�� �����Ͽ� ������ �ݺ��� Ż��
    t.join();															// ������ ������� ���
    PartitionProgressInfo finish = { 1.0, totalWritten, currentWork.c_str(), 1 };
    partitionUpdateCallback(finish);								    // �۾� �ϷḦ ��Ÿ���� �ݹ� ���� ����

    LARGE_INTEGER fileSize;
    fileSize.QuadPart = 0;
    if (!SetFilePointerEx(hFile, fileSize, NULL, FILE_BEGIN))
        return PartitionRemoveResult::SEEK_FAILED;

    return PartitionRemoveResult::SUCCESS;
}


extern "C"
{
    __declspec(dllexport) PartitionRemoveResult SecureRemovePartition(LPCSTR inputDisk, RemoveAlgorithm mode, PartitionCallback partitionUpdateCallback)
    {
        std::string physicalDrive = std::format("\\\\.\\{}:", inputDisk[0]);
        HANDLE hPartition = CreateFileA(physicalDrive.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0,
            OPEN_EXISTING, FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING, NULL);
        if (hPartition == INVALID_HANDLE_VALUE)
            return PartitionRemoveResult::INVALID_HANDLE;

        DWORD dwBytesPerSectors, dwSectorsPerCluster, dwFreeClusters, dwTotalClusters;
        std::string logicalDrive = std::format("{}:\\", inputDisk[0]);
        if (!GetDiskFreeSpaceA(logicalDrive.c_str(), &dwBytesPerSectors, &dwSectorsPerCluster, &dwFreeClusters, &dwTotalClusters))
        {
            CloseHandle(hPartition);
            return PartitionRemoveResult::RETRIEVE_DISK_INFO_FAILED;
        }

        DWORD clusterSize = dwBytesPerSectors * dwSectorsPerCluster;
        long long totalBytes = dwBytesPerSectors * dwSectorsPerCluster * dwTotalClusters;
        DWORD status;

        // �����ϰ� ��ũ�� ���� ���� ���� ���� ������ ����ϰ� ����Ʈ�� �����Ѵ�.
        // �ش� ��ũ���� ������� ������ ���� ��� OS���� ���� ��� ��û�� �������� ����.
        // ���� ����Ʈ ���� ���̴� ���� Raw data�� ��ũ�� �ۼ��� �� ����.
        if (!DeviceIoControl(hPartition, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &status, NULL))
        {
            CloseHandle(hPartition);
            return PartitionRemoveResult::LOCK_FAILED;
        }

        if (!DeviceIoControl(hPartition, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &status, NULL))
        {
            CloseHandle(hPartition);
            return PartitionRemoveResult::DISMOUNT_FAILED;
        }
            

        ErasePass passes[MAX_OVERWRITE];
        int passCounts;
        GetPassArray(passes, &passCounts, mode);
        std::string currentWork;

        for (int i = 0; i < passCounts; i++)
        {
            currentWork = std::format("{}/{}�ܰ� �����", i + 1, passCounts);
            if (passes[i].Method == ErasePassMethod::ONEBYTE)
            {
                PartitionRemoveResult result = OverwritePartition(hPartition, passes[i].OneByte, clusterSize, totalBytes, currentWork, partitionUpdateCallback);
                if (result != PartitionRemoveResult::SUCCESS)
                {
                    CloseHandle(hPartition);
                    return result;
                }
            }
            else if (passes[i].Method == ErasePassMethod::THREEBYTE)
            {
                PartitionRemoveResult result = OverwritePartition3Bytes(hPartition, passes[i].ThreeByte, clusterSize, totalBytes, currentWork, partitionUpdateCallback);
                if (result != PartitionRemoveResult::SUCCESS)
                {
                    CloseHandle(hPartition);
                    return result;
                }
            }
            else // if (passes[i].Method == ErasePassMethod::RANDOM)
            {
                PartitionRemoveResult result = OverwritePartitionRandom(hPartition, clusterSize, totalBytes, currentWork, partitionUpdateCallback);
                if (result != PartitionRemoveResult::SUCCESS)
                {
                    CloseHandle(hPartition);
                    return result;
                }
            }
        }

        CloseHandle(hPartition);

        return PartitionRemoveResult::SUCCESS;
    }
}