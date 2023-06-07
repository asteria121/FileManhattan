#include "ErasureMethod.h"

void GetPassArray(ErasePass* passes, int* passCounts, RemoveAlgorithm mode)
{
	if (mode == RemoveAlgorithm::THREEPASS)
		DoDE(passes, passCounts);
	else if (mode == RemoveAlgorithm::SEVENPASS)
		DoDECE(passes, passCounts);
	else if (mode == RemoveAlgorithm::THIRTYFIVEPASS)
		Gutmann(passes, passCounts);
	else
		OnePass(passes, passCounts);
}

void OnePass(ErasePass* passes, int* count)
{
	passes[0].Method = ErasePassMethod::RANDOM;								/// Phase 1: ·£´ý ¹ÙÀÌÆ®·Î µ¤¾î¾º¿ì±â
	*count = 1;
}

void DoDE(ErasePass* passes, int* count)
{
	passes[0].Method = ErasePassMethod::ONEBYTE; passes[0].OneByte = 0x00;	/// Phase 1: 0x00À¸·Î µ¤¾î¾º¿ì±â
	passes[1].Method = ErasePassMethod::ONEBYTE; passes[1].OneByte = 0xFF;	/// Phase 2: 0xFFÀ¸·Î µ¤¾î¾º¿ì±â
	passes[2].Method = ErasePassMethod::RANDOM;								/// Phase 3: ·£´ý ¹ÙÀÌÆ®·Î µ¤¾î¾º¿ì±â
	*count = 3;
}

void DoDECE(ErasePass* passes, int* count)
{
	passes[0].Method = ErasePassMethod::ONEBYTE; passes[0].OneByte = 0x00;									/// Phase 1: 0x00À¸·Î µ¤¾î¾º¿ì±â
	passes[1].Method = ErasePassMethod::ONEBYTE; passes[1].OneByte = 0xFF;									/// Phase 2: 0x00ÀÇ º¸¼ö 0xFF·Î µ¤¾î¾º¿ì±â
	passes[2].Method = ErasePassMethod::RANDOM;																/// Phase 3: ·£´ý ¹ÙÀÌÆ®·Î µ¤¾î¾º¿ì±â
	passes[3].Method = ErasePassMethod::ONEBYTE; passes[3].OneByte = RandEngine::csprng.GenerateByte();		/// Phase 4: ·£´ý ´ÜÀÏ 1¹ÙÀÌÆ®·Î µ¤¾î¾º¿ì±â
	passes[4].Method = ErasePassMethod::ONEBYTE; passes[4].OneByte = 0x00;									/// Phase 5: 0x00À¸·Î µ¤¾î¾º¿ì±â
	passes[5].Method = ErasePassMethod::ONEBYTE; passes[5].OneByte = 0xFF;									/// Phase 6: 0x00ÀÇ º¸¼ö 0xFF·Î µ¤¾î¾º¿ì±â
	passes[6].Method = ErasePassMethod::RANDOM;																/// Phase 7: ·£´ý ¹ÙÀÌÆ®·Î µ¤¾î¾º¿ì±â
	*count = 7;
}

void Gutmann(ErasePass* passes, int* count)
{
	BYTE pass1[3] = { 0x92, 0x49, 0x24 };
	BYTE pass2[3] = { 0x49, 0x24, 0x92 };
	BYTE pass3[3] = { 0x24, 0x92, 0x49 };
	BYTE pass4[3] = { 0x6D, 0xB6, 0xDB };
	BYTE pass5[3] = { 0xB6, 0xDB, 0x6D };
	BYTE pass6[3] = { 0xDB, 0xB6, 0xB6 };

	passes[0].Method = ErasePassMethod::RANDOM;																/// Phase 1: ·£´ý ¹ÙÀÌÆ®·Î µ¤¾î¾º¿ì±â
	passes[1].Method = ErasePassMethod::RANDOM;																/// Phase 2: ·£´ý ¹ÙÀÌÆ®·Î µ¤¾î¾º¿ì±â
	passes[2].Method = ErasePassMethod::RANDOM;																/// Phase 3: ·£´ý ¹ÙÀÌÆ®·Î µ¤¾î¾º¿ì±â
	passes[3].Method = ErasePassMethod::RANDOM;																/// Phase 4: ·£´ý ¹ÙÀÌÆ®·Î µ¤¾î¾º¿ì±â
	passes[4].Method = ErasePassMethod::ONEBYTE; passes[0].OneByte = 0x55;									/// Phase 5: 0x55·Î µ¤¾î¾º¿ì±â
	passes[5].Method = ErasePassMethod::ONEBYTE; passes[1].OneByte = 0xAA;									/// Phase 6: 0xAA·Î µ¤¾î¾º¿ì±â
	passes[6].Method = ErasePassMethod::THREEBYTE; RtlCopyMemory(passes[6].ThreeByte, pass1, sizeof(pass1));// Phase 7: 0x92, 0x49, 0x24 ¼ø¼­´ë·Î µ¤¾î¾º¿ì±â
	passes[7].Method = ErasePassMethod::THREEBYTE; RtlCopyMemory(passes[7].ThreeByte, pass2, sizeof(pass2));/// Phase 8: 0x49, 0x24, 0x92 ¼ø¼­´ë·Î µ¤¾î¾º¿ì±â
	passes[8].Method = ErasePassMethod::THREEBYTE; RtlCopyMemory(passes[8].ThreeByte, pass3, sizeof(pass3));/// Phase 9: 0x24, 0x92, 0x49 ¼ø¼­´ë·Î µ¤¾î¾º¿ì±â
	passes[9].Method = ErasePassMethod::ONEBYTE; passes[9].OneByte = 0x00;									/// Phase 10: 0x00À¸·Î µ¤¾î¾º¿ì±â
	passes[10].Method = ErasePassMethod::ONEBYTE; passes[10].OneByte = 0x11;								/// Phase 11: 0x11À¸·Î µ¤¾î¾º¿ì±â
	passes[11].Method = ErasePassMethod::ONEBYTE; passes[11].OneByte = 0x22;								/// Phase 12: 0x22À¸·Î µ¤¾î¾º¿ì±â
	passes[12].Method = ErasePassMethod::ONEBYTE; passes[12].OneByte = 0x33;								/// Phase 13: 0x33À¸·Î µ¤¾î¾º¿ì±â
	passes[13].Method = ErasePassMethod::ONEBYTE; passes[13].OneByte = 0x44;								/// Phase 14: 0x44À¸·Î µ¤¾î¾º¿ì±â
	passes[14].Method = ErasePassMethod::ONEBYTE; passes[14].OneByte = 0x55;								/// Phase 15: 0x55À¸·Î µ¤¾î¾º¿ì±â
	passes[15].Method = ErasePassMethod::ONEBYTE; passes[15].OneByte = 0x66;								/// Phase 16: 0x66À¸·Î µ¤¾î¾º¿ì±â
	passes[16].Method = ErasePassMethod::ONEBYTE; passes[16].OneByte = 0x77;								/// Phase 17: 0x77À¸·Î µ¤¾î¾º¿ì±â
	passes[17].Method = ErasePassMethod::ONEBYTE; passes[17].OneByte = 0x88;								/// Phase 18: 0x88À¸·Î µ¤¾î¾º¿ì±â
	passes[18].Method = ErasePassMethod::ONEBYTE; passes[18].OneByte = 0x99;								/// Phase 19: 0x99À¸·Î µ¤¾î¾º¿ì±â
	passes[19].Method = ErasePassMethod::ONEBYTE; passes[19].OneByte = 0xAA;								/// Phase 20: 0xAAÀ¸·Î µ¤¾î¾º¿ì±â
	passes[20].Method = ErasePassMethod::ONEBYTE; passes[20].OneByte = 0xBB;								/// Phase 21: 0xBBÀ¸·Î µ¤¾î¾º¿ì±â
	passes[21].Method = ErasePassMethod::ONEBYTE; passes[21].OneByte = 0xCC;								/// Phase 22: 0xCCÀ¸·Î µ¤¾î¾º¿ì±â
	passes[22].Method = ErasePassMethod::ONEBYTE; passes[22].OneByte = 0xDD;								/// Phase 23: 0xDDÀ¸·Î µ¤¾î¾º¿ì±â
	passes[23].Method = ErasePassMethod::ONEBYTE; passes[23].OneByte = 0xEE;								/// Phase 24: 0xEEÀ¸·Î µ¤¾î¾º¿ì±â
	passes[24].Method = ErasePassMethod::ONEBYTE; passes[24].OneByte = 0xFF;								/// Phase 25: 0xFFÀ¸·Î µ¤¾î¾º¿ì±â
	passes[25].Method = ErasePassMethod::THREEBYTE; RtlCopyMemory(passes[25].ThreeByte, pass1, sizeof(pass1));/// Phase 26: 0x92, 0x49, 0x24 ¼ø¼­´ë·Î µ¤¾î¾º¿ì±â
	passes[26].Method = ErasePassMethod::THREEBYTE; RtlCopyMemory(passes[26].ThreeByte, pass2, sizeof(pass2));/// Phase 27: 0x49, 0x24, 0x92 ¼ø¼­´ë·Î µ¤¾î¾º¿ì±â
	passes[27].Method = ErasePassMethod::THREEBYTE; RtlCopyMemory(passes[27].ThreeByte, pass3, sizeof(pass3));/// Phase 28: 0x24, 0x92, 0x49 ¼ø¼­´ë·Î µ¤¾î¾º¿ì±â
	passes[28].Method = ErasePassMethod::THREEBYTE; RtlCopyMemory(passes[28].ThreeByte, pass4, sizeof(pass4));/// Phase 29: 0x6D, 0xB6, 0xDB ¼ø¼­´ë·Î µ¤¾î¾º¿ì±â
	passes[29].Method = ErasePassMethod::THREEBYTE; RtlCopyMemory(passes[29].ThreeByte, pass5, sizeof(pass5));/// Phase 30: 0xB6, 0xDB, 0x6D ¼ø¼­´ë·Î µ¤¾î¾º¿ì±â
	passes[30].Method = ErasePassMethod::THREEBYTE; RtlCopyMemory(passes[30].ThreeByte, pass6, sizeof(pass6));/// Phase 31: 0xDB, 0xB6, 0xB6 ¼ø¼­´ë·Î µ¤¾î¾º¿ì±â
	passes[31].Method = ErasePassMethod::RANDOM;															/// Phase 32: ·£´ý ¹ÙÀÌÆ®·Î µ¤¾î¾º¿ì±â
	passes[32].Method = ErasePassMethod::RANDOM;															/// Phase 33: ·£´ý ¹ÙÀÌÆ®·Î µ¤¾î¾º¿ì±â
	passes[33].Method = ErasePassMethod::RANDOM;															/// Phase 34: ·£´ý ¹ÙÀÌÆ®·Î µ¤¾î¾º¿ì±â
	passes[34].Method = ErasePassMethod::RANDOM;															/// Phase 35: ·£´ý ¹ÙÀÌÆ®·Î µ¤¾î¾º¿ì±â
	*count = 35;
}