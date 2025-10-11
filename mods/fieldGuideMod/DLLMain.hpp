#pragma once

#include <cstdio>
#include <iostream>

inline void con_allocate(bool no_flush) noexcept {
    AllocConsole();

    FILE* out;
    FILE* in;
    FILE* err;
    freopen_s(&out, "CONOUT$", "w", stdout);
    freopen_s(&in, "CONIN$", "r", stdin);
    freopen_s(&err, "CONOUT$", "w", stderr);

    // Re-sync C++ streams to new console handles
    std::ios::sync_with_stdio(true);
    std::wcout.clear();
    std::cout.clear();
    std::wcerr.clear();
    std::cerr.clear();

    if (no_flush) {
        std::ios_base::sync_with_stdio(false);
        std::setvbuf(stdout, nullptr, _IOFBF, BUFSIZ);
    }
}

inline void con_noflush() noexcept {}

void base();

BOOL DllMain(HINSTANCE hinstDll, DWORD fdwReason, LPVOID lpvReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH)
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&base, NULL, 0,
            NULL);
    return TRUE;
}
