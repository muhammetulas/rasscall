#include <windows.h>
#include <stdio.h>

int main() {
    HMODULE hWs2 = LoadLibraryA("ws2_32.dll");
    if (!hWs2) return 1;
    
    FARPROC pSend = GetProcAddress(hWs2, "send");
    FARPROC pRecv = GetProcAddress(hWs2, "recv");
    
    FILE* f = fopen("ws2_prologue_dump.txt", "w");
    if (f) {
        if (pSend) {
            fprintf(f, "send: ");
            for (int i=0; i<32; i++) fprintf(f, "%02X ", ((unsigned char*)pSend)[i]);
            fprintf(f, "\n");
        }
        if (pRecv) {
            fprintf(f, "recv: ");
            for (int i=0; i<32; i++) fprintf(f, "%02X ", ((unsigned char*)pRecv)[i]);
            fprintf(f, "\n");
        }
        fclose(f);
    }
    return 0;
}
