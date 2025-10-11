#include "logMod.hpp"
#include "DLLMain.hpp"

#include <unordered_set>
#include <mutex>
#include <windows.h>
#include <psapi.h>
#include <intrin.h>

uint64_t getBattleDefineValueOvr(uint64_t input) {
    uint64_t output = getBattleDefineValue(input);

    void* returnAddr = _ReturnAddress();

    static std::mutex mtx;
    static std::unordered_set<uint64_t> loggedCallers;

    {
        std::lock_guard<std::mutex> lock(mtx);
        uint64_t key = (reinterpret_cast<uint64_t>(returnAddr) << 32) ^ input;
        if (loggedCallers.insert(key).second) {
            HMODULE hMod = nullptr;
            GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                              static_cast<LPCTSTR>(returnAddr), &hMod);

            char modName[MAX_PATH];
            GetModuleFileNameA(hMod, modName, sizeof(modName));

            uintptr_t base = reinterpret_cast<uintptr_t>(hMod);
            uintptr_t offset = reinterpret_cast<uintptr_t>(returnAddr) - base;

            Log("caller: ", modName, "+0x", std::hex, offset,
                " | input: ", std::dec, input,
                " - output: ", output);
        }
    }

    return output;
}

void base() {
    Log("SLEEP");
    Sleep(5000);
    con_allocate(false);
    Log("START");

    MH_STATUS status = MH_Initialize();
	Log("MinHook Status: ",MH_StatusToString(status));

    MH_STATUS hookStatus = MH_CreateHook(getBattleDefineValuePtr, getBattleDefineValueOvr,(void**)&getBattleDefineValue);
    Log("Hook ",getBattleDefineValuePtr," Status: ",MH_StatusToString(hookStatus));
    MH_STATUS queueStatus = MH_QueueEnableHook(getBattleDefineValuePtr);
    Log("Queue ",getBattleDefineValuePtr," Status: ",MH_StatusToString(queueStatus));
    
    MH_STATUS applyStatus = MH_ApplyQueued();
	Log("Apply Status: ",MH_StatusToString(applyStatus));

    //uintptr_t isUnlockedAddress = AobScan(isUnlockedAob);
    //if (isUnlockedAddress != 0){
        //ReplaceExpectedBytesAtAddress(isUnlockedAddress+0x5,"0f b6 08","b1 01 90");
    //}
}
