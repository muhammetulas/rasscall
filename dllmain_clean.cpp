#include "pch.h"
#include <Windows.h>

static HMODULE g_Real = NULL;
static FARPROC g_F[17] = {0};

extern "C" {
    __declspec(naked) void proxy_0()  { __asm { jmp dword ptr [g_F +  0] } }
    __declspec(naked) void proxy_1()  { __asm { jmp dword ptr [g_F +  4] } }
    __declspec(naked) void proxy_2()  { __asm { jmp dword ptr [g_F +  8] } }
    __declspec(naked) void proxy_3()  { __asm { jmp dword ptr [g_F + 12] } }
    __declspec(naked) void proxy_4()  { __asm { jmp dword ptr [g_F + 16] } }
    __declspec(naked) void proxy_5()  { __asm { jmp dword ptr [g_F + 20] } }
    __declspec(naked) void proxy_6()  { __asm { jmp dword ptr [g_F + 24] } }
    __declspec(naked) void proxy_7()  { __asm { jmp dword ptr [g_F + 28] } }
    __declspec(naked) void proxy_8()  { __asm { jmp dword ptr [g_F + 32] } }
    __declspec(naked) void proxy_9()  { __asm { jmp dword ptr [g_F + 36] } }
    __declspec(naked) void proxy_10() { __asm { jmp dword ptr [g_F + 40] } }
    __declspec(naked) void proxy_11() { __asm { jmp dword ptr [g_F + 44] } }
    __declspec(naked) void proxy_12() { __asm { jmp dword ptr [g_F + 48] } }
    __declspec(naked) void proxy_13() { __asm { jmp dword ptr [g_F + 52] } }
    __declspec(naked) void proxy_14() { __asm { jmp dword ptr [g_F + 56] } }
    __declspec(naked) void proxy_15() { __asm { jmp dword ptr [g_F + 60] } }
    __declspec(naked) void proxy_16() { __asm { jmp dword ptr [g_F + 64] } }
}

BOOL WINAPI DllMain(HINSTANCE h, DWORD r, LPVOID) {
    if (r == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(h);
        char sys[MAX_PATH];
        GetSystemDirectoryA(sys, MAX_PATH);
        lstrcatA(sys, "\\version.dll");
        g_Real = LoadLibraryA(sys);
        if (g_Real) {
            const char* n[] = {
                "GetFileVersionInfoA","GetFileVersionInfoByHandle",
                "GetFileVersionInfoExA","GetFileVersionInfoExW",
                "GetFileVersionInfoSizeA","GetFileVersionInfoSizeExA",
                "GetFileVersionInfoSizeExW","GetFileVersionInfoSizeW",
                "GetFileVersionInfoW","VerFindFileA","VerFindFileW",
                "VerInstallFileA","VerInstallFileW","VerLanguageNameA",
                "VerLanguageNameW","VerQueryValueA","VerQueryValueW"
            };
            for (int i = 0; i < 17; i++) g_F[i] = GetProcAddress(g_Real, n[i]);
        }
    }
    return TRUE;
}
