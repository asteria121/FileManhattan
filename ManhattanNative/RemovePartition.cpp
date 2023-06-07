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
    CryptoPP::SecByteBlock buffer(clusterSize);	// 디스크 클러스터 크기만큼 덮어씌울 버퍼 생성
    GenerateRandomBlock(&buffer);

    DWORD dwBytesWritten = 0;
    double rate = 0.0;
    long long totalWritten = 0;

    std::atomic<bool> isRunning(true);											    // UI 콜백 스레드 작동을 관리하는 공유자원
    PartitionProgressInfo data = { rate, totalWritten, currentWork.c_str(), 0 };	// 콜백함수에서 여러개의 파라미터를 전달하기 위한 구조체
    PartitionCallbackThread param = { isRunning, &data, partitionUpdateCallback };	// 스레드에 여러개의 파라미터를 전달하기 위한 구조체
    std::thread t(ProcessPartitonCallback, std::ref(param));						// UI 콜백 스레드 시작
    // 파일의 크기 < 덮어씌운 크기일때까지 WriteFile() 진행
    for (totalWritten = 0; totalWritten < totalBytes; totalWritten += dwBytesWritten)
    {
        if (!WriteFile(hFile, buffer.data(), clusterSize, &dwBytesWritten, NULL))
            return PartitionRemoveResult::WRITE_FAILED;

        rate = static_cast<double>(totalWritten) / totalBytes;		    // 현재 작업 진행상황 백분율로 반영
        data.progressRate = rate;
        data.totalBytesWritten = totalWritten;
    }
    isRunning = false;													// 공유자원을 false로 변경하여 스레드 반복문 탈출
    t.join();															// 스레드 종료까지 대기
    PartitionProgressInfo finish = { 1.0, totalWritten, currentWork.c_str(), 1 };
    partitionUpdateCallback(finish);								    // 작업 완료를 나타내는 콜백 정보 전송

    LARGE_INTEGER fileSize;
    fileSize.QuadPart = 0;
    if (!SetFilePointerEx(hFile, fileSize, NULL, FILE_BEGIN))
        return PartitionRemoveResult::SEEK_FAILED;

    return PartitionRemoveResult::SUCCESS;
}

PartitionRemoveResult OverwritePartition3Bytes(HANDLE hFile, PBYTE buff, DWORD clusterSize, long long totalBytes, std::string currentWork, PartitionCallback partitionUpdateCallback)
{
    CryptoPP::SecByteBlock buffer(clusterSize * 3);	// 디스크 클러스터 크기만큼 덮어씌울 버퍼 생성

    for (DWORD i = 0; i < clusterSize * 3; i++)
    {
        buffer.begin()[i] = buff[i % 3];
    }

    DWORD dwBytesWritten = 0;
    double rate = 0.0;
    long long totalWritten = 0;

    std::atomic<bool> isRunning(true);											    // UI 콜백 스레드 작동을 관리하는 공유자원
    PartitionProgressInfo data = { rate, totalWritten, currentWork.c_str(), 0 };	// 콜백함수에서 여러개의 파라미터를 전달하기 위한 구조체
    PartitionCallbackThread param = { isRunning, &data, partitionUpdateCallback };	// 스레드에 여러개의 파라미터를 전달하기 위한 구조체
    std::thread t(ProcessPartitonCallback, std::ref(param));						// UI 콜백 스레드 시작
    // 파일의 크기 < 덮어씌운 크기일때까지 WriteFile() 진행
    for (totalWritten = 0; totalWritten < totalBytes; totalWritten += dwBytesWritten)
    {
        if (!WriteFile(hFile, buffer.data() + (totalWritten % (clusterSize * 3)), clusterSize, &dwBytesWritten, NULL))
            return PartitionRemoveResult::WRITE_FAILED;

        rate = static_cast<double>(totalWritten) / totalBytes;		    // 현재 작업 진행상황 백분율로 반영
        data.progressRate = rate;
        data.totalBytesWritten = totalWritten;
    }
    isRunning = false;													// 공유자원을 false로 변경하여 스레드 반복문 탈출
    t.join();															// 스레드 종료까지 대기
    PartitionProgressInfo finish = { 1.0, totalWritten, currentWork.c_str(), 1 };
    partitionUpdateCallback(finish);								    // 작업 완료를 나타내는 콜백 정보 전송

    LARGE_INTEGER fileSize;
    fileSize.QuadPart = 0;
    if (!SetFilePointerEx(hFile, fileSize, NULL, FILE_BEGIN))
        return PartitionRemoveResult::SEEK_FAILED;

    return PartitionRemoveResult::SUCCESS;
}

PartitionRemoveResult OverwritePartition(HANDLE hFile, BYTE buff, DWORD clusterSize, long long totalBytes, std::string currentWork, PartitionCallback partitionUpdateCallback)
{
    CryptoPP::SecByteBlock buffer(clusterSize);	// 디스크 클러스터 크기만큼 덮어씌울 버퍼 생성
    memset(buffer.begin(), buff, buffer.size());

    DWORD dwBytesWritten = 0;
    double rate = 0.0;
    long long totalWritten = 0;

    std::atomic<bool> isRunning(true);											    // UI 콜백 스레드 작동을 관리하는 공유자원
    PartitionProgressInfo data = { rate, totalWritten, currentWork.c_str(), 0 };	// 콜백함수에서 여러개의 파라미터를 전달하기 위한 구조체
    PartitionCallbackThread param = { isRunning, &data, partitionUpdateCallback };	// 스레드에 여러개의 파라미터를 전달하기 위한 구조체
    std::thread t(ProcessPartitonCallback, std::ref(param));						// UI 콜백 스레드 시작
    // 파일의 크기 < 덮어씌운 크기일때까지 WriteFile() 진행
    for (totalWritten = 0; totalWritten < totalBytes; totalWritten += dwBytesWritten)
    {
        if (!WriteFile(hFile, buffer.data(), clusterSize, &dwBytesWritten, NULL))
            return PartitionRemoveResult::WRITE_FAILED;

        rate = static_cast<double>(totalWritten) / totalBytes;		    // 현재 작업 진행상황 백분율로 반영
        data.progressRate = rate;
        data.totalBytesWritten = totalWritten;
    }
    isRunning = false;													// 공유자원을 false로 변경하여 스레드 반복문 탈출
    t.join();															// 스레드 종료까지 대기
    PartitionProgressInfo finish = { 1.0, totalWritten, currentWork.c_str(), 1 };
    partitionUpdateCallback(finish);								    // 작업 완료를 나타내는 콜백 정보 전송

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

        // 안전하게 디스크를 덮어 쓰기 위해 먼저 볼륨을 잠금하고 마운트를 해제한다.
        // 해당 디스크에서 사용중인 파일이 있을 경우 OS에서 볼륨 잠금 요청을 수행하지 않음.
        // 볼륨 마운트 해제 없이는 직접 Raw data를 디스크에 작성할 수 없음.
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
            currentWork = std::format("{}/{}단계 덮어쓰기", i + 1, passCounts);
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