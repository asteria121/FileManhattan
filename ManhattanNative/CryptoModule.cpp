#include "CryptoModule.h"

void ProcessCryptoCallback(CryptoCallbackThread param)
{
	while (param.isRunning)
	{
		param.cryptoUpdateCallback(*param.data);
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

inline long long GetFileSize(std::ifstream* inputFile)
{
	std::streampos currentPosition = inputFile->tellg();
	inputFile->seekg(0, std::ios::end);											// ������ ������ �̵��Ѵ�
	long long fileSize = static_cast<long long>(inputFile->tellg());			// 64��Ʈ ���������� ��ȯ
	inputFile->seekg(currentPosition, std::ios::beg);							// ���� ó�� ��ġ�� �̵�

	return fileSize;
}

inline bool EndOfFile(const CryptoPP::FileSource& file)
{
	std::istream* stream = const_cast<CryptoPP::FileSource&>(file).GetStream();
	return stream->eof();
}

void GenerateHash(CryptoPP::SecByteBlock* buffer, CryptoPP::SecByteBlock* input)
{
	CryptoPP::SecByteBlock tmp(CryptoPP::SHA3_512::DIGESTSIZE);
	CryptoPP::SHA3_512().CalculateDigest(tmp.begin(), input->data(), input->size());

	// Ű ��Ʈ��Ī 100ȸ �ݺ�
	for (int i = 0; i < 100; i++)
		CryptoPP::SHA3_512().CalculateDigest(tmp.begin(), tmp.data(), tmp.size());

	RtlCopyMemory(buffer->begin(), tmp.data(), tmp.size());
}

extern "C"
{
	__declspec(dllexport) CryptoResult Encrypt(LPCSTR inputPath, LPCSTR outputPath, LPCSTR originalKey, CryptoCallback cryptoUpdateCallback)
	{
		size_t originalKeyLen = lstrlenA(originalKey);
		CryptoPP::SecByteBlock salt(SALT_SIZE);								// ��ȣ�� ���� SALT
		CryptoPP::SecByteBlock saltedKey(salt.size() + originalKeyLen);		// ��ȣ�� SALT�� ȥ���� �迭
		CryptoPP::SecByteBlock hashedKey(CryptoPP::SHA3_512::DIGESTSIZE);	// ���������� ��ȣ ������ ���� SHA512 �ؽ�
		CryptoPP::SecByteBlock iv(CryptoPP::AES::BLOCKSIZE);				// ��ȣȭ�� ����� ����

		try
		{
			GenerateRandomBlock(&salt);												// CSPRNG�� SALT 64����Ʈ �ʱ�ȭ
			GenerateRandomBlock(&iv);												// ���� 16����Ʈ(AES ��� ũ��) �ʱ�ȭ

			RtlCopyMemory(saltedKey.begin(), salt.data(), salt.size());								// SALT�� Key ȥ��
			RtlCopyMemory(saltedKey.begin() + salt.size(), originalKey, originalKeyLen);			// SALT�ڿ� Key �迭 ����
			GenerateHash(&hashedKey, &saltedKey);													// ��й�ȣ ������ Ȱ���� �ؽ� ����

			CryptoPP::SecByteBlock key(CryptoPP::SHA3_512::DIGESTSIZE);								// SHA512 DIGESTSIZE = AES MAXKEYLENGTH
			CryptoPP::SHA3_512().CalculateDigest(key.begin(), saltedKey.data(), saltedKey.size());	// ��ȣȭ�� ���� ���� 64����Ʈ (256 * 2 Bit(XTS)) �迭 ����

			CryptoPP::XTS<CryptoPP::AES>::Encryption encryption;
			//encryption.SetKeyWithIV(key.data(), key.size(), iv.data());
			encryption.SetKeyWithIV(key.data(), key.size(), iv.data());

			// �Ϻ�ȣȭ �۾� ���� ������ �аų� ����ϴµ� �̷��� ���ϸ� FileSink�� FileSource ��ü���� ���� ����� ��ü�� �����ϴµ�
			// GetStream���� �о�ͼ� �Ϻ�ȣȭ �۾����� �б� ���⿡ ����ϴµ� �̷��� ���� ����
			std::ifstream inputFile(inputPath, std::ios::binary);
			if (!inputFile.is_open())
				return CryptoResult::INPUT_NOT_EXISTS;

			std::ofstream outputFile(outputPath, std::ios::binary);
			if (!outputFile.is_open())
				return CryptoResult::OUTPUT_NOT_CREATABLE;

			long long fileSize = GetFileSize(&inputFile);

			outputFile.write(HEADER_TEXT, HEADER_SIZE);														// ��ȣȭ ���� ��� ���� �ؽ�Ʈ 16B
			outputFile.write(reinterpret_cast<LPCSTR>(hashedKey.data()), CryptoPP::SHA3_512::DIGESTSIZE);	// ��ȣ ���� �ؽ� 64B
			outputFile.write(reinterpret_cast<LPCSTR>(iv.data()), iv.size());								// ���� 16B
			outputFile.write(reinterpret_cast<LPCSTR>(salt.data()), salt.size());							// SALT 64B

			CryptoPP::StreamTransformationFilter encryptor(encryption, NULL, CryptoPP::StreamTransformationFilter::NO_PADDING);	// ��ȣȭ ��ü ����
			CryptoPP::FileSource source(inputFile, false);						// ���� ���� ��ü ����
			CryptoPP::FileSink sink(outputFile);								// ��ȣȭ ���� ��ü ����
			CryptoPP::MeterFilter meter;										// ���� ���� ��ü�� ��ȣ�ۿ��ϴ� ��ü

			source.Attach(new CryptoPP::Redirector(encryptor));					// ���� ���ϰ� ��ȣȭ ����, �۾� ���� ��Ȳ�� ���̺귯���� �����Ѵ�.
			encryptor.Attach(new CryptoPP::Redirector(meter));
			meter.Attach(new CryptoPP::Redirector(sink));						// ���� ���ϰ� ��ȣȭ ����, �۾� ���� ��Ȳ�� ���̺귯���� �����Ѵ�.

			double rate = 0.0;
			std::atomic<bool> isRunning(true);											// UI �ݹ� ������ �۵��� �����ϴ� �����ڿ�
			CryptoProgressInfo data = { rate, 0 };										// �ݹ��Լ����� �������� �Ķ���͸� �����ϱ� ���� ����ü
			CryptoCallbackThread param = { isRunning, &data, cryptoUpdateCallback };	// �����忡 �������� �Ķ���͸� �����ϱ� ���� ����ü
			std::thread t(ProcessCryptoCallback, std::ref(param));						// UI �ݹ� ������ ����
			while (!EndOfFile(source) && !source.SourceExhausted())						// ������ ������ ����
			{
				source.Pump(static_cast<long long>(1024 * 1024 ));						// 1MB�� �о ��ȣȭ ����.
				encryptor.Flush(false);

				rate = static_cast<double>(meter.GetTotalBytes()) / fileSize;			// ���� �۾� �����Ȳ ������� �ݿ�		
				data.progressRate = rate;
				data.totalBytesWritten = meter.GetTotalBytes();
			}
			isRunning = false;															// �����ڿ��� false�� �����Ͽ� ������ �ݺ��� Ż��
			t.join();																	// ������ ������� ���
			encryptor.MessageEnd();														// ��ȣȭ �۾� ����

			return CryptoResult::SUCCESS;
		}
		catch (const CryptoPP::Exception& ex)
		{
			std::cerr << ex.what();
			return CryptoResult::EXCEPTION;
		}
	}

	__declspec(dllexport) CryptoResult Decrypt(LPCSTR inputPath, LPCSTR outputPath, LPCSTR originalKey, CryptoCallback cryptoUpdateCallback)
	{
		try
		{
			size_t originalKeyLen = lstrlenA(originalKey);
			char headerText[17];													// ������� �ؽ�Ʈ
			RtlZeroMemory(headerText, sizeof(headerText));

			CryptoPP::SecByteBlock salt(SALT_SIZE);									// ��ȣ�� ���� SALT
			CryptoPP::SecByteBlock saltedKey(salt.size() + originalKeyLen);			// ��ȣ�� SALT�� ȥ���� �迭
			CryptoPP::SecByteBlock hashedKey(CryptoPP::SHA3_512::DIGESTSIZE);		// ������ ��¥ �ؽ�
			CryptoPP::SecByteBlock userHashedKey(CryptoPP::SHA3_512::DIGESTSIZE);	// ����ڰ� �Է��� ��ȣ�� �ؽ�
			CryptoPP::SecByteBlock iv(CryptoPP::AES::BLOCKSIZE);					// ��ȣȭ�� ����� ����

			// �Ϻ�ȣȭ �۾� ���� ������ �аų� ����ϴµ� �̷��� ���ϸ� FileSink�� FileSource ��ü���� ���� ����� ��ü�� �����ϴµ�
			// GetStream���� �о�ͼ� �Ϻ�ȣȭ �۾����� �б� ���⿡ ����ϴµ� �̷��� ���� ����
			std::ifstream inputFile(inputPath, std::ios::binary);
			if (!inputFile.is_open())												// ���޹��� input ������ ������ ���� ���
				return CryptoResult::INPUT_NOT_EXISTS;

			long long fileSize = GetFileSize(&inputFile);

			inputFile.read(headerText, HEADER_SIZE);								// ��� �ؽ�Ʈ 16B
			if (strncmp(headerText, HEADER_TEXT, HEADER_SIZE))						// 000FILEMANHATTAN ����� ���� ��� ����
				return CryptoResult::WRONG_FILE;

			inputFile.read(reinterpret_cast<LPSTR>(hashedKey.data()), CryptoPP::SHA3_512::DIGESTSIZE);	// ��ȣ ������ SHA512 �ؽ�
			inputFile.read(reinterpret_cast<LPSTR>(iv.data()), CryptoPP::AES::BLOCKSIZE);				// ����
			inputFile.read(reinterpret_cast<LPSTR>(salt.data()), salt.size());							// SALT

			RtlCopyMemory(saltedKey.begin(), salt.data(), salt.size());									// SALT�� Key ȥ��
			RtlCopyMemory(saltedKey.begin() + salt.size(), originalKey, originalKeyLen);				// SALT�ڿ� Key �迭 ���� (���������� SALT + ���� key)
			GenerateHash(&userHashedKey, &saltedKey);													// ��й�ȣ ������ Ȱ���� �ؽ� ����

			if (userHashedKey != hashedKey)															// ��й�ȣ Ʋ�� ���
				return CryptoResult::WRONG_PASSWORD;

			std::ofstream outputFile(outputPath, std::ios::binary);									// ��ȣȭ ���� ����
			if (!outputFile.is_open())
				return CryptoResult::OUTPUT_NOT_CREATABLE;

			CryptoPP::SecByteBlock key(CryptoPP::SHA3_512::DIGESTSIZE);								// SHA512 DIGESTSIZE = AES MAXKEYLENGTH
			CryptoPP::SHA3_512().CalculateDigest(key.begin(), saltedKey.data(), saltedKey.size());	// ��ȣȭ�� ���� ���� 32����Ʈ (256 Bit) �迭 ����

			CryptoPP::XTS<CryptoPP::AES>::Decryption decryption;
			decryption.SetKeyWithIV(key.data(), key.size(), iv.data());

			CryptoPP::StreamTransformationFilter decryptor(decryption, NULL, CryptoPP::StreamTransformationFilter::NO_PADDING);	// ��ȣȭ ��ü ����
			CryptoPP::FileSource source(inputFile, false);								// ���� ���� ��ü ����
			CryptoPP::FileSink sink(outputFile);										// ��ȣȭ ���� ��ü ����
			CryptoPP::MeterFilter meter;												// ���� ���� ��ü�� ��ȣ�ۿ��ϴ� ��ü

			source.Attach(new CryptoPP::Redirector(decryptor));							// ��ȣȭ�� ���ϰ� ��ȣȭ ����, �۾� ���� ��Ȳ�� ���̺귯���� �����Ѵ�.
			decryptor.Attach(new CryptoPP::Redirector(meter));
			meter.Attach(new CryptoPP::Redirector(sink));								// ��ȣȭ�� ���ϰ� ��ȣȭ ����, �۾� ���� ��Ȳ�� ���̺귯���� �����Ѵ�.

			double rate = 0.0;
			std::atomic<bool> isRunning(true);											// UI �ݹ� ������ �۵��� �����ϴ� �����ڿ�
			CryptoProgressInfo data = { rate, 0 };										// �ݹ��Լ����� �������� �Ķ���͸� �����ϱ� ���� ����ü
			CryptoCallbackThread param = { isRunning, &data, cryptoUpdateCallback };	// �����忡 �������� �Ķ���͸� �����ϱ� ���� ����ü
			std::thread t(ProcessCryptoCallback, std::ref(param));						// UI �ݹ� ������ ����
			while (!EndOfFile(source) && !source.SourceExhausted())						// ������ ������ ����
			{
				source.Pump(static_cast<long long>(1024 * 1024));						// 1MB�� �о ��ȣȭ ����.
				decryptor.Flush(false);

				rate = static_cast<double>(meter.GetTotalBytes()) / fileSize;			// ���� �۾� �����Ȳ ������� �ݿ�		
				data.progressRate = rate;
				data.totalBytesWritten = meter.GetTotalBytes();
			}
			isRunning = false;															// �����ڿ��� false�� �����Ͽ� ������ �ݺ��� Ż��
			t.join();																	// ������ ������� ���
			decryptor.MessageEnd();														// ��ȣȭ �۾� ����

			return CryptoResult::SUCCESS;
		}
		catch (const CryptoPP::Exception& ex)
		{
			std::cerr << ex.what();
			return CryptoResult::EXCEPTION;
		}
	}
}