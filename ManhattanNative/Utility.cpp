#include "Utility.h"
#include "RandEngine.h"

void GenerateRandomFileName(LPSTR buffer, DWORD bufferLength)
{
	try
	{
		for (DWORD i = 0; i < bufferLength - 1; i++)
		{
			// ������ ���̱��� ����, �빮��, �ҹ��ڸ� �������� �����Ѵ�.
			switch (RandEngine::csprng.GenerateByte() % 3)
			{
				// 0 ~ 9 ����
			case 0:
				buffer[i] = 0x30 + RandEngine::csprng.GenerateByte() % 10;
				break;
				// �빮�� A ~ Z ����
			case 1:
				buffer[i] = 0x41 + RandEngine::csprng.GenerateByte() % 26;
				break;
				// �ҹ��� a ~ z ����
			case 2:
			default:
				buffer[i] = 0x61 + RandEngine::csprng.GenerateByte() % 26;
				break;
			}
		}
		buffer[bufferLength - 1] = '\0';
	}
	catch (CryptoPP::Exception& ex)
	{
		std::cerr << ex.what();
	}
}

void SetRandomFileName(LPCSTR oldFilePath, LPSTR newFilePath, DWORD newFilePathLength, DWORD repeatCount)
{
	std::string newPath = oldFilePath;
	std::string fileName = PathFindFileNameA(oldFilePath);

	size_t pos = newPath.find(fileName);
	if (pos != std::string::npos)
	{
		std::string targetPath = oldFilePath;
		for (DWORD i = 0; i < repeatCount; i++)
		{
			char randFileName[MAX_PATH];
			GenerateRandomFileName(randFileName, static_cast<DWORD>(fileName.length()));
			newPath.replace(pos, fileName.length(), randFileName);

			// MoveFile()�� MoveFileEx()�� ����
			// MoveFile�� �������� ��ũ �̵��� ���ݵǳ�, MoveFileEx�� ��ũ�� ��ϵ� ��ġ���� ������ �õ���.
			if (!MoveFileExA(targetPath.c_str(), newPath.c_str(), MOVEFILE_REPLACE_EXISTING))
			{
				newFilePath = NULL;
				return;
			}
			else
			{
				targetPath = newPath;
				strcpy_s(newFilePath, newFilePathLength, newPath.c_str());
			}
		}
	}
}

void SetRandomFileAttribute(LPCSTR inputFile, DWORD repeatCount)
{
	for (DWORD i = 0; i < repeatCount - 1; i++)
	{
		DWORD attribute = NULL;
		if (RandEngine::csprng.GenerateByte() % 2 == 0)
			attribute |= FILE_ATTRIBUTE_HIDDEN;
		if (RandEngine::csprng.GenerateByte() % 2 == 0)
			attribute |= FILE_ATTRIBUTE_SYSTEM;
		if (RandEngine::csprng.GenerateByte() % 2 == 0)
			attribute |= FILE_ATTRIBUTE_READONLY;

		if (RandEngine::csprng.GenerateByte() % 3 != 0)
			SetFileAttributesA(inputFile, attribute);
		else
			SetFileAttributesA(inputFile, FILE_ATTRIBUTE_NORMAL);
	}
	SetFileAttributesA(inputFile, FILE_ATTRIBUTE_NORMAL);
}

void SetRandomFileStamp(HANDLE hFile, DWORD repeatCount)
{
	for (DWORD i = 0; i < repeatCount; i++)
	{
		SYSTEMTIME newTime = { 0 };
		newTime.wYear = static_cast<WORD>(RandEngine::csprng.GenerateWord32()) % 50 + 1970;
		newTime.wMonth = static_cast<WORD>(RandEngine::csprng.GenerateWord32()) % 12 + 1;
		newTime.wDay = static_cast<WORD>(RandEngine::csprng.GenerateWord32()) % 31;
		newTime.wHour = static_cast<WORD>(RandEngine::csprng.GenerateWord32()) % 24;
		newTime.wMinute = static_cast<WORD>(RandEngine::csprng.GenerateWord32()) % 60;
		newTime.wSecond = static_cast<WORD>(RandEngine::csprng.GenerateWord32()) % 60;
		newTime.wMilliseconds = static_cast<WORD>(RandEngine::csprng.GenerateWord32()) % 1000;

		FILETIME newFileTime = { 0 };
		SystemTimeToFileTime(&newTime, &newFileTime);
		SetFileTime(hFile, &newFileTime, &newFileTime, &newFileTime);
	}
}

void ChangeFileSize(HANDLE hFile, long long size)
{
	LARGE_INTEGER fileSize;
	fileSize.QuadPart = size;
	if (!SetFilePointerEx(hFile, fileSize, NULL, FILE_BEGIN))
		return;

	if (!SetEndOfFile(hFile))
		return;
}

void GenerateRandomBlock(CryptoPP::SecByteBlock* buffer)
{
	try
	{
		RandEngine::csprng.GenerateBlock(buffer->begin(), buffer->size());
	}
	catch (CryptoPP::Exception& ex)
	{
		std::cerr << ex.what();
	}
}