.386
.model flat, C
extern DoInitOnce:proc
extern g_OrigFuncs:dword

.CODE
; Naked proxy: pushad, DoInitOnce, popad, add esp,4 (call'un ret adresi), jmp real
; Stack gercek fonksiyonda [caller_ret][p1][p2].. olmali; add esp,4 ile bizim ret atiliyor.

; proxy_0 = C'de (dllmain.cpp)
proxy_1 proc
    pushad
    call DoInitOnce
    popad
    add esp, 4
    mov eax, g_OrigFuncs
    mov eax, [eax+4]
    jmp eax
proxy_1 endp

proxy_2 proc
    pushad
    call DoInitOnce
    popad
    add esp, 4
    mov eax, g_OrigFuncs
    mov eax, [eax+8]
    jmp eax
proxy_2 endp

proxy_3 proc
    pushad
    call DoInitOnce
    popad
    add esp, 4
    mov eax, g_OrigFuncs
    mov eax, [eax+12]
    jmp eax
proxy_3 endp

proxy_4 proc
    pushad
    call DoInitOnce
    popad
    add esp, 4
    mov eax, g_OrigFuncs
    mov eax, [eax+16]
    jmp eax
proxy_4 endp

proxy_5 proc
    pushad
    call DoInitOnce
    popad
    add esp, 4
    mov eax, g_OrigFuncs
    mov eax, [eax+20]
    jmp eax
proxy_5 endp

proxy_6 proc
    pushad
    call DoInitOnce
    popad
    add esp, 4
    mov eax, g_OrigFuncs
    mov eax, [eax+24]
    jmp eax
proxy_6 endp

proxy_7 proc
    pushad
    call DoInitOnce
    popad
    add esp, 4
    mov eax, g_OrigFuncs
    mov eax, [eax+28]
    jmp eax
proxy_7 endp

proxy_8 proc
    pushad
    call DoInitOnce
    popad
    add esp, 4
    mov eax, g_OrigFuncs
    mov eax, [eax+32]
    jmp eax
proxy_8 endp

proxy_9 proc
    pushad
    call DoInitOnce
    popad
    add esp, 4
    mov eax, g_OrigFuncs
    mov eax, [eax+36]
    jmp eax
proxy_9 endp

proxy_10 proc
    pushad
    call DoInitOnce
    popad
    add esp, 4
    mov eax, g_OrigFuncs
    mov eax, [eax+40]
    jmp eax
proxy_10 endp

proxy_11 proc
    pushad
    call DoInitOnce
    popad
    add esp, 4
    mov eax, g_OrigFuncs
    mov eax, [eax+44]
    jmp eax
proxy_11 endp

proxy_12 proc
    pushad
    call DoInitOnce
    popad
    add esp, 4
    mov eax, g_OrigFuncs
    mov eax, [eax+48]
    jmp eax
proxy_12 endp

proxy_13 proc
    pushad
    call DoInitOnce
    popad
    add esp, 4
    mov eax, g_OrigFuncs
    mov eax, [eax+52]
    jmp eax
proxy_13 endp

proxy_14 proc
    pushad
    call DoInitOnce
    popad
    add esp, 4
    mov eax, g_OrigFuncs
    mov eax, [eax+56]
    jmp eax
proxy_14 endp

proxy_15 proc
    pushad
    call DoInitOnce
    popad
    add esp, 4
    mov eax, g_OrigFuncs
    mov eax, [eax+60]
    jmp eax
proxy_15 endp

proxy_16 proc
    pushad
    call DoInitOnce
    popad
    add esp, 4
    mov eax, g_OrigFuncs
    mov eax, [eax+64]
    jmp eax
proxy_16 endp

END
