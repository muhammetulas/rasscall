#include "pch.h"
#include <Windows.h>
#include <string.h>

// ====================================================================
// VERSION.DLL PROXY + BAN BYPASS (CreateThread YOK - ilk cagrida init)
// ====================================================================

#define LOG_PATH "C:\\rasscall\\hook_diag.log"
static CRITICAL_SECTION g_LogCs;

static void DiagLog(const char* msg) {
    EnterCriticalSection(&g_LogCs);
    HANDLE h = CreateFileA(LOG_PATH, FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h != INVALID_HANDLE_VALUE) {
        DWORD w = 0;
        WriteFile(h, msg, (DWORD)strlen(msg), &w, NULL);
        WriteFile(h, "\r\n", 2, &w, NULL);
        CloseHandle(h);
    }
    LeaveCriticalSection(&g_LogCs);
}

/* Kilitsiz - phase2 kazanan thread CriticalSection'dan once crash yiyorsa bu yazilir */
static void RawLog(const char* msg) {
    HANDLE h = CreateFileA(LOG_PATH, FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h != INVALID_HANDLE_VALUE) {
        DWORD w = 0;
        WriteFile(h, msg, (DWORD)strlen(msg), &w, NULL);
        WriteFile(h, "\r\n", 2, &w, NULL);
        CloseHandle(h);
    }
}

static void DiagLogHex(const char* prefix, void* p) {
    char buf[80];
    buf[0] = 0;
    lstrcatA(buf, prefix);
    char hex[16];
    wsprintfA(hex, "0x%p", p);
    lstrcatA(buf, hex);
    DiagLog(buf);
}

static HMODULE g_RealVersion = NULL;
extern "C" FARPROC g_OrigFuncs[17] = {0};
static volatile long g_InitDone = 0;

static void LoadRealVersion() {
    if (g_RealVersion) return;
    char sys[MAX_PATH];
    GetSystemDirectoryA(sys, MAX_PATH);
    lstrcatA(sys, "\\version.dll");
    g_RealVersion = LoadLibraryA(sys);
    if (!g_RealVersion) return;
    const char* names[] = {
        "GetFileVersionInfoA", "GetFileVersionInfoByHandle",
        "GetFileVersionInfoExA", "GetFileVersionInfoExW",
        "GetFileVersionInfoSizeA", "GetFileVersionInfoSizeExA",
        "GetFileVersionInfoSizeExW", "GetFileVersionInfoSizeW",
        "GetFileVersionInfoW",
        "VerFindFileA", "VerFindFileW", "VerInstallFileA", "VerInstallFileW",
        "VerLanguageNameA", "VerLanguageNameW", "VerQueryValueA", "VerQueryValueW",
    };
    for (int i = 0; i < 17; i++)
        g_OrigFuncs[i] = GetProcAddress(g_RealVersion, names[i]);
}

// --- Ban bypass (IAT only, no thread) ---
typedef UINT_PTR SOCKET;
typedef int (WINAPI* fnSend)(SOCKET, const char*, int, int);
typedef int (WINAPI* fnRecv)(SOCKET, char*, int, int);

static fnSend g_RealSend = nullptr;
static fnRecv g_RealRecv = nullptr;

/* --- NtQuerySystemInformation hook: process listesinden CE/dbg araclari gizle --- */
typedef LONG NTSTATUS;
#define NT_SUCCESS(s) (((NTSTATUS)(s)) >= 0)
#define SystemProcessInformation 5
typedef NTSTATUS (NTAPI *fnNtQuerySystemInformation)(ULONG, PVOID, ULONG, PULONG);
static fnNtQuerySystemInformation g_RealNtQSI = nullptr;
#define ProcessDebugPort 7
typedef NTSTATUS (NTAPI *fnNtQueryInformationProcess)(HANDLE, ULONG, PVOID, ULONG, PULONG);
static fnNtQueryInformationProcess g_RealNtQIP = nullptr;
static NTSTATUS NTAPI hkNtQueryInformationProcess(HANDLE h, ULONG cls, PVOID buf, ULONG len, PULONG ret) {
    NTSTATUS st = g_RealNtQIP(h, cls, buf, len, ret);
    if (NT_SUCCESS(st) && cls == ProcessDebugPort && buf && len >= sizeof(ULONG_PTR))
        *(ULONG_PTR*)buf = 0;
    return st;
}

/* IsWindowVisible: her zaman 0 don - gorunurluk kontrolunde hicbir pencere gorunur gorunmesin */
typedef BOOL (WINAPI *fnIsWindowVisible)(HWND);
static fnIsWindowVisible g_RealIsWindowVisible = nullptr;
static BOOL WINAPI hkIsWindowVisible(HWND hwnd) {
    (void)hwnd;
    return 0;
}
/* x86 SYSTEM_PROCESS_INFORMATION: NextEntryOffset=0, ImageName UNICODE_STRING=+0x44 (Length,MaxLen,Buffer) */
#define OFF_NextEntry  0
#define OFF_ImageName  0x44
static bool IsBlacklistedProcess(const WCHAR* buf, UINT len) {
    if (!buf || len == 0) return false;
    char tmp[128];
    UINT i;
    for (i = 0; i < len && i < 127; i++) tmp[i] = (char)(buf[i] & 0xFF);
    tmp[i < 127 ? i : 127] = 0;
    for (i = 0; tmp[i]; i++) if (tmp[i] >= 'A' && tmp[i] <= 'Z') tmp[i] += 32;
    const char* list[] = { "cheat engine", "cheatengine", "x64dbg", "x32dbg", "ollydbg", "process hacker", "processhacker", "ida", "ida64", "ida32", "dnspy", "de4dot", "dotpeek", "reshade", "inject", "debugger", NULL };
    for (int j = 0; list[j]; j++) {
        const char* p = list[j];
        size_t plen = strlen(p);
        for (i = 0; tmp[i]; i++) {
            int k = 0;
            while (p[k] && tmp[i+k]) {
                char c = p[k]; if (c >= 'A' && c <= 'Z') c += 32;
                char t = tmp[i+k]; if (t >= 'A' && t <= 'Z') t += 32;
                if (c != t) break;
                k++;
            }
            if (!p[k]) return true;
        }
    }
    return false;
}
static NTSTATUS NTAPI hkNtQuerySystemInformation(ULONG cls, PVOID buf, ULONG len, PULONG ret) {
    NTSTATUS st = g_RealNtQSI(cls, buf, len, ret);
    if (!NT_SUCCESS(st) || cls != SystemProcessInformation || !buf || len < 0x60) return st;
    __try {
        BYTE* cur = (BYTE*)buf;
        BYTE* prev = NULL;
        ULONG prevNext = 0;
        while (1) {
            ULONG nextOff = *(ULONG*)(cur + OFF_NextEntry);
            UINT nameLen = *(USHORT*)(cur + OFF_ImageName);
            WCHAR* nameBuf = *(WCHAR**)(cur + OFF_ImageName + 4);
            if (nameBuf && nameLen && IsBlacklistedProcess(nameBuf, nameLen / sizeof(WCHAR))) {
                if (prev == NULL) {
                    if (nextOff == 0) break;
                    memmove(buf, cur + nextOff, len - (ULONG)(cur - (BYTE*)buf) - nextOff);
                    if (ret) *ret -= nextOff;
                    cur = (BYTE*)buf;
                    continue;
                }
                *(ULONG*)(prev + OFF_NextEntry) = (prevNext ? prevNext + nextOff : nextOff);
                if (nextOff == 0) break;
                cur += nextOff;
                continue;
            }
            prev = cur;
            prevNext = nextOff;
            if (nextOff == 0) break;
            cur += nextOff;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) { }
    return st;
}

#define DATA_XOR_KEY        0x55
#define FAKE_PACKET_LEN     172
#define FAKE_PACKET_YOL_LEN 192

static const unsigned char g_EncFakeClean[172] = {
    0x1D,0x01,0x01,0x05,0x7A,0x64,0x7B,0x64,0x75,0x67,0x65,0x65,0x75,0x1A,0x1E,0x58,
    0x5F,0x11,0x34,0x21,0x30,0x6F,0x75,0x18,0x3A,0x3B,0x79,0x75,0x65,0x67,0x75,0x18,
    0x34,0x27,0x75,0x67,0x65,0x67,0x63,0x75,0x64,0x63,0x6F,0x66,0x6D,0x6F,0x65,0x61,
    0x75,0x12,0x18,0x01,0x58,0x5F,0x06,0x30,0x27,0x23,0x30,0x27,0x6F,0x75,0x14,0x25,
    0x34,0x36,0x3D,0x30,0x7A,0x67,0x7B,0x61,0x7B,0x60,0x67,0x75,0x7D,0x00,0x37,0x20,
    0x3B,0x21,0x20,0x7C,0x58,0x5F,0x16,0x3A,0x3B,0x21,0x30,0x3B,0x21,0x78,0x19,0x30,
    0x3B,0x32,0x21,0x3D,0x6F,0x75,0x67,0x61,0x58,0x5F,0x16,0x3A,0x3B,0x21,0x30,0x3B,
    0x21,0x78,0x01,0x2C,0x25,0x30,0x6F,0x75,0x21,0x30,0x2D,0x21,0x7A,0x3D,0x21,0x38,
    0x39,0x6E,0x75,0x36,0x3D,0x34,0x27,0x26,0x30,0x21,0x68,0x00,0x01,0x13,0x78,0x6D,
    0x58,0x5F,0x58,0x5F,0x24,0x13,0x66,0x17,0x37,0x32,0x66,0x34,0x6C,0x65,0x25,0x3A,
    0x1C,0x37,0x14,0x11,0x03,0x1D,0x39,0x3A,0x65,0x32,0x68,0x68
};
static const unsigned char g_EncFakeYolayrimi[192] = {
    0x1D,0x01,0x01,0x05,0x7A,0x64,0x7B,0x64,0x75,0x67,0x65,0x65,0x75,0x1A,0x1E,0x58,
    0x5F,0x11,0x34,0x21,0x30,0x6F,0x75,0x01,0x20,0x30,0x79,0x75,0x65,0x66,0x75,0x18,
    0x34,0x27,0x75,0x67,0x65,0x67,0x63,0x75,0x65,0x65,0x6F,0x67,0x6D,0x6F,0x60,0x60,
    0x75,0x12,0x18,0x01,0x58,0x5F,0x06,0x30,0x27,0x23,0x30,0x27,0x6F,0x75,0x14,0x25,
    0x34,0x36,0x3D,0x30,0x7A,0x67,0x7B,0x61,0x7B,0x60,0x67,0x75,0x7D,0x00,0x37,0x20,
    0x3B,0x21,0x20,0x7C,0x58,0x5F,0x16,0x3A,0x3B,0x21,0x30,0x3B,0x21,0x78,0x19,0x30,
    0x3B,0x32,0x21,0x3D,0x6F,0x75,0x61,0x61,0x58,0x5F,0x16,0x3A,0x3B,0x21,0x30,0x3B,
    0x21,0x78,0x01,0x2C,0x25,0x30,0x6F,0x75,0x21,0x30,0x2D,0x21,0x7A,0x3D,0x21,0x38,
    0x39,0x6E,0x75,0x36,0x3D,0x34,0x27,0x26,0x30,0x21,0x68,0x00,0x01,0x13,0x78,0x6D,
    0x58,0x5F,0x58,0x5F,0x04,0x1A,0x22,0x3C,0x60,0x61,0x3E,0x7A,0x63,0x34,0x04,0x24,
    0x7E,0x31,0x32,0x61,0x04,0x7A,0x33,0x2C,0x3B,0x1A,0x16,0x3B,0x0F,0x3B,0x20,0x1C,
    0x36,0x06,0x24,0x1D,0x01,0x20,0x37,0x02,0x1F,0x6D,0x3A,0x17,0x63,0x7A,0x14,0x68
};
static const unsigned char g_EncBanPattern[16] = {
    0x3B,0x30,0x37,0x34,0x3E,0x3C,0x2C,0x3A,0x3B,0x34,0x3B,0x31,0x34,0x23,0x34,0x39
};
static const unsigned char g_EncYolPattern[10] = {
    0x2C,0x3A,0x39,0x34,0x2C,0x27,0x3C,0x38,0x3C,0x68
};

static void DecryptToStack(void* dst, const void* enc, int len) {
    for (int i = 0; i < len; i++)
        ((unsigned char*)dst)[i] = ((const unsigned char*)enc)[i] ^ DATA_XOR_KEY;
}

static bool BufferContainsEncrypted(const char* buf, int bufLen, const unsigned char* encPat, int patLen) {
    if (!buf || bufLen < patLen) return false;
    int scan = bufLen > 2048 ? 2048 : bufLen;
    unsigned char dec[32];
    DecryptToStack(dec, encPat, patLen);
    for (int i = 0; i <= scan - patLen; i++)
        if (memcmp(buf + i, dec, patLen) == 0) { memset(dec, 0, 32); return true; }
    memset(dec, 0, 32);
    return false;
}

static long g_RecvBypass=0;  static SOCKET g_RecvSock=0;  static SOCKET g_RecvInj=0;
static long g_YolBypass=0;   static SOCKET g_YolSock=0;   static SOCKET g_YolInj=0;

static int WINAPI hkSend(SOCKET s, const char* buf, int len, int flags) {
    __try {
        if (!g_RealSend) return -1;
        if (buf && len > 0 && len <= 0x10000 && BufferContainsEncrypted(buf, len, g_EncBanPattern, 16)) {
            g_RecvSock = s; InterlockedExchange(&g_RecvBypass, 1); return len;
        }
        if (buf && len > 0 && len <= 0x10000 && BufferContainsEncrypted(buf, len, g_EncYolPattern, 10)) {
            g_YolSock = s; InterlockedExchange(&g_YolBypass, 1); return len;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) { }
    return g_RealSend(s, buf, len, flags);
}

static int WINAPI hkRecv(SOCKET s, char* buf, int len, int flags) {
    __try {
        if (g_YolBypass && s==g_YolSock && buf && len>=FAKE_PACKET_YOL_LEN) {
            DecryptToStack(buf, g_EncFakeYolayrimi, FAKE_PACKET_YOL_LEN);
            InterlockedExchange(&g_YolBypass, 0); g_YolSock=0; g_YolInj=s;
            return FAKE_PACKET_YOL_LEN;
        }
        if (g_YolInj && s==g_YolInj) { g_YolInj=0; return 0; }
        if (g_RecvBypass && s==g_RecvSock && buf && len>=FAKE_PACKET_LEN) {
            DecryptToStack(buf, g_EncFakeClean, FAKE_PACKET_LEN);
            InterlockedExchange(&g_RecvBypass, 0); g_RecvSock=0; g_RecvInj=s;
            return FAKE_PACKET_LEN;
        }
        if (g_RecvInj && s==g_RecvInj) { g_RecvInj=0; return 0; }
    } __except (EXCEPTION_EXECUTE_HANDLER) { }
    if (!g_RealRecv) return -1;
    return g_RealRecv(s, buf, len, flags);
}

/* Inline hook: ws2_32.send/recv ilk 5 byte -> JMP bizim handler. Trampoline = orijinal 5 byte + JMP target+5.
   Oyun IAT'ina dokunmuyoruz, Rascal IAT kontrolu atlanir. */
static int InlineHook5(void* target, void* hook, void** pTrampoline) {
    __try {
        BYTE* p = (BYTE*)target;
        DWORD old = 0;
        if (!VirtualProtect(p, 5, PAGE_EXECUTE_READWRITE, &old)) return 0;
        void* tramp = VirtualAlloc(NULL, 16, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        if (!tramp) { VirtualProtect(p, 5, old, &old); return 0; }
        memcpy(tramp, p, 5);
        *(BYTE*)((BYTE*)tramp + 5) = 0xE9;  /* JMP rel32 */
        *(DWORD*)((BYTE*)tramp + 6) = (DWORD)((BYTE*)p + 5) - (DWORD)((BYTE*)tramp + 10);  /* target+5 */
        *p = 0xE9;
        *(DWORD*)(p + 1) = (DWORD)hook - (DWORD)(p + 5);
        VirtualProtect(p, 5, old, &old);
        if (pTrampoline) *pTrampoline = tramp;
        return 1;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) { return 0; }
}

/* Tek asamali init. IAT yerine inline hook - oyun exe IAT'i degismez. */
extern "C" void DoInitOnce() {
    if (g_InitDone == 2) return;
    if (g_InitDone == 1) {
        for (int i = 0; i < 200; i++) { Sleep(10); if (g_InitDone == 2) return; }
        return;
    }
    if (InterlockedCompareExchange(&g_InitDone, 1, 0) != 0) return;

    LoadRealVersion();
    if (!g_RealVersion) {
        InterlockedExchange(&g_InitDone, 0);
        return;
    }

#define SKIP_INLINE_HOOK 0  /* 0 = send/recv inline hook (ban bypass) */
#if !SKIP_INLINE_HOOK
    HMODULE hWs2 = GetModuleHandleA("ws2_32.dll");
    if (hWs2) {
        void* pSend = (void*)GetProcAddress(hWs2, "send");
        void* pRecv = (void*)GetProcAddress(hWs2, "recv");
        if (pSend) InlineHook5(pSend, (void*)hkSend, (void**)&g_RealSend);
        if (pRecv) InlineHook5(pRecv, (void*)hkRecv, (void**)&g_RealRecv);
    }
    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    if (hNtdll) {
        void* pNtQSI = (void*)GetProcAddress(hNtdll, "NtQuerySystemInformation");
        void* pNtQIP = (void*)GetProcAddress(hNtdll, "NtQueryInformationProcess");
        if (pNtQSI) InlineHook5(pNtQSI, (void*)hkNtQuerySystemInformation, (void**)&g_RealNtQSI);
        if (pNtQIP) InlineHook5(pNtQIP, (void*)hkNtQueryInformationProcess, (void**)&g_RealNtQIP);
    }
    HMODULE hUser32 = GetModuleHandleA("user32.dll");
    if (hUser32) {
        void* pIsVis = (void*)GetProcAddress(hUser32, "IsWindowVisible");
        if (pIsVis) InlineHook5(pIsVis, (void*)hkIsWindowVisible, (void**)&g_RealIsWindowVisible);
    }
#endif
    InterlockedExchange(&g_InitDone, 2);
}

/* Tum 17 proxy C'de - asm jmp crash veriyordu, CALL ile cagiriyoruz */
extern "C" BOOL WINAPI proxy_0(LPCSTR a, DWORD b, DWORD c, LPVOID d) {
    DoInitOnce();
    return ((BOOL(WINAPI*)(LPCSTR,DWORD,DWORD,LPVOID))g_OrigFuncs[0])(a,b,c,d);
}
extern "C" BOOL WINAPI proxy_1(HANDLE h, LPCSTR s, LPVOID v, UINT u) {
    DoInitOnce();
    return ((BOOL(WINAPI*)(HANDLE,LPCSTR,LPVOID,UINT))g_OrigFuncs[1])(h,s,v,u);
}
extern "C" BOOL WINAPI proxy_2(DWORD a, LPCSTR b, DWORD c, DWORD d, LPVOID e) {
    DoInitOnce();
    return ((BOOL(WINAPI*)(DWORD,LPCSTR,DWORD,DWORD,LPVOID))g_OrigFuncs[2])(a,b,c,d,e);
}
extern "C" BOOL WINAPI proxy_3(DWORD a, LPCWSTR b, DWORD c, DWORD d, LPVOID e) {
    DoInitOnce();
    return ((BOOL(WINAPI*)(DWORD,LPCWSTR,DWORD,DWORD,LPVOID))g_OrigFuncs[3])(a,b,c,d,e);
}
extern "C" DWORD WINAPI proxy_4(LPCSTR a, LPDWORD b) {
    DoInitOnce();
    return ((DWORD(WINAPI*)(LPCSTR,LPDWORD))g_OrigFuncs[4])(a,b);
}
extern "C" DWORD WINAPI proxy_5(DWORD a, LPCSTR b, LPDWORD c) {
    DoInitOnce();
    return ((DWORD(WINAPI*)(DWORD,LPCSTR,LPDWORD))g_OrigFuncs[5])(a,b,c);
}
extern "C" DWORD WINAPI proxy_6(DWORD a, LPCWSTR b, LPDWORD c) {
    DoInitOnce();
    return ((DWORD(WINAPI*)(DWORD,LPCWSTR,LPDWORD))g_OrigFuncs[6])(a,b,c);
}
extern "C" DWORD WINAPI proxy_7(LPCWSTR a, LPDWORD b) {
    DoInitOnce();
    return ((DWORD(WINAPI*)(LPCWSTR,LPDWORD))g_OrigFuncs[7])(a,b);
}
extern "C" BOOL WINAPI proxy_8(LPCWSTR a, DWORD b, DWORD c, LPVOID d) {
    DoInitOnce();
    return ((BOOL(WINAPI*)(LPCWSTR,DWORD,DWORD,LPVOID))g_OrigFuncs[8])(a,b,c,d);
}
extern "C" DWORD WINAPI proxy_9(DWORD a, LPCSTR b, LPSTR c, PUINT d, PUINT e, PUINT f, LPSTR g, PUINT h) {
    DoInitOnce();
    return ((DWORD(WINAPI*)(DWORD,LPCSTR,LPSTR,PUINT,PUINT,PUINT,LPSTR,PUINT))g_OrigFuncs[9])(a,b,c,d,e,f,g,h);
}
extern "C" DWORD WINAPI proxy_10(DWORD a, LPCWSTR b, LPWSTR c, PUINT d, PUINT e, PUINT f, LPWSTR g, PUINT h) {
    DoInitOnce();
    return ((DWORD(WINAPI*)(DWORD,LPCWSTR,LPWSTR,PUINT,PUINT,PUINT,LPWSTR,PUINT))g_OrigFuncs[10])(a,b,c,d,e,f,g,h);
}
extern "C" DWORD WINAPI proxy_11(DWORD a, LPCSTR b, LPCSTR c, LPCSTR d, LPCSTR e, LPSTR f, LPSTR g, PUINT h) {
    DoInitOnce();
    return ((DWORD(WINAPI*)(DWORD,LPCSTR,LPCSTR,LPCSTR,LPCSTR,LPSTR,LPSTR,PUINT))g_OrigFuncs[11])(a,b,c,d,e,f,g,h);
}
extern "C" DWORD WINAPI proxy_12(DWORD a, LPCWSTR b, LPCWSTR c, LPCWSTR d, LPCWSTR e, LPWSTR f, LPWSTR g, PUINT h) {
    DoInitOnce();
    return ((DWORD(WINAPI*)(DWORD,LPCWSTR,LPCWSTR,LPCWSTR,LPCWSTR,LPWSTR,LPWSTR,PUINT))g_OrigFuncs[12])(a,b,c,d,e,f,g,h);
}
extern "C" DWORD WINAPI proxy_13(DWORD a, LPSTR b, DWORD c) {
    DoInitOnce();
    return ((DWORD(WINAPI*)(DWORD,LPSTR,DWORD))g_OrigFuncs[13])(a,b,c);
}
extern "C" DWORD WINAPI proxy_14(DWORD a, LPWSTR b, DWORD c) {
    DoInitOnce();
    return ((DWORD(WINAPI*)(DWORD,LPWSTR,DWORD))g_OrigFuncs[14])(a,b,c);
}
extern "C" BOOL WINAPI proxy_15(LPCVOID a, LPCSTR b, LPVOID c, PUINT d) {
    DoInitOnce();
    return ((BOOL(WINAPI*)(LPCVOID,LPCSTR,LPVOID,PUINT))g_OrigFuncs[15])(a,b,c,d);
}
extern "C" BOOL WINAPI proxy_16(LPCVOID a, LPCWSTR b, LPVOID c, PUINT d) {
    DoInitOnce();
    return ((BOOL(WINAPI*)(LPCVOID,LPCWSTR,LPVOID,PUINT))g_OrigFuncs[16])(a,b,c,d);
}

BOOL WINAPI DllMain(HINSTANCE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        LoadRealVersion();  /* Once burada - proxy icinden LoadLibrary crash veriyordu */
        InitializeCriticalSection(&g_LogCs);
        DisableThreadLibraryCalls(hModule);
        DiagLog("DllMain done");
    }
    return TRUE;
}
