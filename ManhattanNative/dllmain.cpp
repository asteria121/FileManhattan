// dllmain.cpp : DLL 애플리케이션의 진입점을 정의합니다.
#include <Windows.h>
#include <random>
#include <format>
#include "RandEngine.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        // CSPRNG 시드 초기화
        byte p[4];
        std::random_device rd;
        RandEngine::seed = rd();
        p[0] = (byte)((RandEngine::seed >> 24) & 0xFF);
        p[1] = (byte)((RandEngine::seed >> 16) & 0xFF);
        p[2] = (byte)((RandEngine::seed >> 8) & 0xFF);
        p[3] = (byte)(RandEngine::seed & 0xFF);
        RandEngine::csprng.IncorporateEntropy(p, sizeof(p));
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

