#include "logMod.hpp"
#include "DLLMain.hpp"

bool f = false;

uint64_t luaFuncOvr(uint64_t lua_state, uint64_t _2, uint64_t _3, uint64_t _4) {
    uint64_t output = luaFunc(lua_state,_2,_3,_4);
    if (!f) {
        f = !f;
        install_print_hook((lua_State*)lua_state);
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

    MH_STATUS hookStatus = MH_CreateHook(luaFuncPtr, luaFuncOvr,(void**)&luaFunc);
    Log("Hook ",luaFuncPtr," Status: ",MH_StatusToString(hookStatus));
    MH_STATUS queueStatus = MH_QueueEnableHook(luaFuncPtr);
    Log("Queue ",luaFuncPtr," Status: ",MH_StatusToString(queueStatus));
    
    MH_STATUS applyStatus = MH_ApplyQueued();
	Log("Apply Status: ",MH_StatusToString(applyStatus));
}
