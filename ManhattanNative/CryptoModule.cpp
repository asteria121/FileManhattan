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
	inputFile->seekg(0, std::ios::end);											// 파일의 끝으로 이동한다
	long long fileSize = static_cast<long long>(inputFile->tellg());			// 64비트 정수형으로 변환
	inputFile->seekg(currentPosition, std::ios::beg);							// 파일 처음 위치로 이동

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

	// 키 스트레칭 100회 반복
	for (int i = 0; i < 100; i++)
		CryptoPP::SHA3_512().CalculateDigest(tmp.begin(), tmp.data(), tmp.size());

	RtlCopyMemory(buffer->begin(), tmp.data(), tmp.size());
}

extern "C"
{
	__declspec(dllexport) CryptoResult Encrypt(LPCSTR inputPath, LPCSTR outputPath, LPCSTR originalKey, CryptoCallback cryptoUpdateCallback)
	{
		size_t originalKeyLen = lstrlenA(originalKey);
		CryptoPP::SecByteBlock salt(SALT_SIZE);								// 암호에 섞을 SALT
		CryptoPP::SecByteBlock saltedKey(salt.size() + originalKeyLen);		// 암호와 SALT를 혼합한 배열
		CryptoPP::SecByteBlock hashedKey(CryptoPP::SHA3_512::DIGESTSIZE);	// 최종적으로 암호 검증에 사용될 SHA512 해시
		CryptoPP::SecByteBlock iv(CryptoPP::AES::BLOCKSIZE);				// 암호화에 사용할 벡터

		try
		{
			GenerateRandomBlock(&salt);												// CSPRNG로 SALT 64바이트 초기화
			GenerateRandomBlock(&iv);												// 벡터 16바이트(AES 블록 크기) 초기화

			RtlCopyMemory(saltedKey.begin(), salt.data(), salt.size());								// SALT와 Key 혼합
			RtlCopyMemory(saltedKey.begin() + salt.size(), originalKey, originalKeyLen);			// SALT뒤에 Key 배열 복사
			GenerateHash(&hashedKey, &saltedKey);													// 비밀번호 검증에 활용할 해시 생성

			CryptoPP::SecByteBlock key(CryptoPP::SHA3_512::DIGESTSIZE);								// SHA512 DIGESTSIZE = AES MAXKEYLENGTH
			CryptoPP::SHA3_512().CalculateDigest(key.begin(), saltedKey.data(), saltedKey.size());	// 암호화에 사용될 최종 64바이트 (256 * 2 Bit(XTS)) 배열 생성

			CryptoPP::XTS<CryptoPP::AES>::Encryption encryption;
			//encryption.SetKeyWithIV(key.data(), key.size(), iv.data());
			encryption.SetKeyWithIV(key.data(), key.size(), iv.data());

			// 암복호화 작업 전에 파일을 읽거나 써야하는데 이렇게 안하면 FileSink랑 FileSource 객체에서 파일 입출력 객체를 생성하는데
			// GetStream으로 읽어와서 암복호화 작업전에 읽기 쓰기에 사용하는데 이러면 오류 생김
			std::ifstream inputFile(inputPath, std::ios::binary);
			if (!inputFile.is_open())
				return CryptoResult::INPUT_NOT_EXISTS;

			std::ofstream outputFile(outputPath, std::ios::binary);
			if (!outputFile.is_open())
				return CryptoResult::OUTPUT_NOT_CREATABLE;

			long long fileSize = GetFileSize(&inputFile);

			outputFile.write(HEADER_TEXT, HEADER_SIZE);														// 암호화 파일 헤더 시작 텍스트 16B
			outputFile.write(reinterpret_cast<LPCSTR>(hashedKey.data()), CryptoPP::SHA3_512::DIGESTSIZE);	// 암호 검증 해시 64B
			outputFile.write(reinterpret_cast<LPCSTR>(iv.data()), iv.size());								// 벡터 16B
			outputFile.write(reinterpret_cast<LPCSTR>(salt.data()), salt.size());							// SALT 64B

			CryptoPP::StreamTransformationFilter encryptor(encryption, NULL, CryptoPP::StreamTransformationFilter::NO_PADDING);	// 암호화 객체 생성
			CryptoPP::FileSource source(inputFile, false);						// 원본 파일 객체 생성
			CryptoPP::FileSink sink(outputFile);								// 암호화 파일 객체 생성
			CryptoPP::MeterFilter meter;										// 원본 파일 객체와 상호작용하는 객체

			source.Attach(new CryptoPP::Redirector(encryptor));					// 원본 파일과 암호화 파일, 작업 진행 상황을 라이브러리로 연결한다.
			encryptor.Attach(new CryptoPP::Redirector(meter));
			meter.Attach(new CryptoPP::Redirector(sink));						// 원본 파일과 암호화 파일, 작업 진행 상황을 라이브러리로 연결한다.

			double rate = 0.0;
			std::atomic<bool> isRunning(true);											// UI 콜백 스레드 작동을 관리하는 공유자원
			CryptoProgressInfo data = { rate, 0 };										// 콜백함수에서 여러개의 파라미터를 전달하기 위한 구조체
			CryptoCallbackThread param = { isRunning, &data, cryptoUpdateCallback };	// 스레드에 여러개의 파라미터를 전달하기 위한 구조체
			std::thread t(ProcessCryptoCallback, std::ref(param));						// UI 콜백 스레드 시작
			while (!EndOfFile(source) && !source.SourceExhausted())						// 파일의 끝까지 진행
			{
				source.Pump(static_cast<long long>(1024 * 1024 ));						// 1MB씩 읽어서 암호화 진행.
				encryptor.Flush(false);

				rate = static_cast<double>(meter.GetTotalBytes()) / fileSize;			// 현재 작업 진행상황 백분율로 반영		
				data.progressRate = rate;
				data.totalBytesWritten = meter.GetTotalBytes();
			}
			isRunning = false;															// 공유자원을 false로 변경하여 스레드 반복문 탈출
			t.join();																	// 스레드 종료까지 대기
			encryptor.MessageEnd();														// 암호화 작업 종료

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
			char headerText[17];													// 헤더파일 텍스트
			RtlZeroMemory(headerText, sizeof(headerText));

			CryptoPP::SecByteBlock salt(SALT_SIZE);									// 암호에 섞을 SALT
			CryptoPP::SecByteBlock saltedKey(salt.size() + originalKeyLen);			// 암호와 SALT를 혼합한 배열
			CryptoPP::SecByteBlock hashedKey(CryptoPP::SHA3_512::DIGESTSIZE);		// 파일의 진짜 해시
			CryptoPP::SecByteBlock userHashedKey(CryptoPP::SHA3_512::DIGESTSIZE);	// 사용자가 입력한 암호의 해시
			CryptoPP::SecByteBlock iv(CryptoPP::AES::BLOCKSIZE);					// 암호화에 사용할 벡터

			// 암복호화 작업 전에 파일을 읽거나 써야하는데 이렇게 안하면 FileSink랑 FileSource 객체에서 파일 입출력 객체를 생성하는데
			// GetStream으로 읽어와서 암복호화 작업전에 읽기 쓰기에 사용하는데 이러면 오류 생김
			std::ifstream inputFile(inputPath, std::ios::binary);
			if (!inputFile.is_open())												// 전달받은 input 파일이 열리지 않을 경우
				return CryptoResult::INPUT_NOT_EXISTS;

			long long fileSize = GetFileSize(&inputFile);

			inputFile.read(headerText, HEADER_SIZE);								// 헤더 텍스트 16B
			if (strncmp(headerText, HEADER_TEXT, HEADER_SIZE))						// 000FILEMANHATTAN 헤더가 없을 경우 종료
				return CryptoResult::WRONG_FILE;

			inputFile.read(reinterpret_cast<LPSTR>(hashedKey.data()), CryptoPP::SHA3_512::DIGESTSIZE);	// 암호 검증용 SHA512 해시
			inputFile.read(reinterpret_cast<LPSTR>(iv.data()), CryptoPP::AES::BLOCKSIZE);				// 벡터
			inputFile.read(reinterpret_cast<LPSTR>(salt.data()), salt.size());							// SALT

			RtlCopyMemory(saltedKey.begin(), salt.data(), salt.size());									// SALT와 Key 혼합
			RtlCopyMemory(saltedKey.begin() + salt.size(), originalKey, originalKeyLen);				// SALT뒤에 Key 배열 복사 (최종적으로 SALT + 원본 key)
			GenerateHash(&userHashedKey, &saltedKey);													// 비밀번호 검증에 활용할 해시 생성

			if (userHashedKey != hashedKey)															// 비밀번호 틀릴 경우
				return CryptoResult::WRONG_PASSWORD;

			std::ofstream outputFile(outputPath, std::ios::binary);									// 복호화 파일 생성
			if (!outputFile.is_open())
				return CryptoResult::OUTPUT_NOT_CREATABLE;

			CryptoPP::SecByteBlock key(CryptoPP::SHA3_512::DIGESTSIZE);								// SHA512 DIGESTSIZE = AES MAXKEYLENGTH
			CryptoPP::SHA3_512().CalculateDigest(key.begin(), saltedKey.data(), saltedKey.size());	// 복호화에 사용될 최종 32바이트 (256 Bit) 배열 생성

			CryptoPP::XTS<CryptoPP::AES>::Decryption decryption;
			decryption.SetKeyWithIV(key.data(), key.size(), iv.data());

			CryptoPP::StreamTransformationFilter decryptor(decryption, NULL, CryptoPP::StreamTransformationFilter::NO_PADDING);	// 암호화 객체 생성
			CryptoPP::FileSource source(inputFile, false);								// 원본 파일 객체 생성
			CryptoPP::FileSink sink(outputFile);										// 암호화 파일 객체 생성
			CryptoPP::MeterFilter meter;												// 원본 파일 객체와 상호작용하는 객체

			source.Attach(new CryptoPP::Redirector(decryptor));							// 암호화된 파일과 복호화 파일, 작업 진행 상황을 라이브러리로 연결한다.
			decryptor.Attach(new CryptoPP::Redirector(meter));
			meter.Attach(new CryptoPP::Redirector(sink));								// 암호화된 파일과 복호화 파일, 작업 진행 상황을 라이브러리로 연결한다.

			double rate = 0.0;
			std::atomic<bool> isRunning(true);											// UI 콜백 스레드 작동을 관리하는 공유자원
			CryptoProgressInfo data = { rate, 0 };										// 콜백함수에서 여러개의 파라미터를 전달하기 위한 구조체
			CryptoCallbackThread param = { isRunning, &data, cryptoUpdateCallback };	// 스레드에 여러개의 파라미터를 전달하기 위한 구조체
			std::thread t(ProcessCryptoCallback, std::ref(param));						// UI 콜백 스레드 시작
			while (!EndOfFile(source) && !source.SourceExhausted())						// 파일의 끝까지 진행
			{
				source.Pump(static_cast<long long>(1024 * 1024));						// 1MB씩 읽어서 복호화 진행.
				decryptor.Flush(false);

				rate = static_cast<double>(meter.GetTotalBytes()) / fileSize;			// 현재 작업 진행상황 백분율로 반영		
				data.progressRate = rate;
				data.totalBytesWritten = meter.GetTotalBytes();
			}
			isRunning = false;															// 공유자원을 false로 변경하여 스레드 반복문 탈출
			t.join();																	// 스레드 종료까지 대기
			decryptor.MessageEnd();														// 복호화 작업 종료

			return CryptoResult::SUCCESS;
		}
		catch (const CryptoPP::Exception& ex)
		{
			std::cerr << ex.what();
			return CryptoResult::EXCEPTION;
		}
	}
}