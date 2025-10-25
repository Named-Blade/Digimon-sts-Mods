#include "logMod.hpp"
#include "DLLMain.hpp"

bool isLog = true;
bool isFuncLog = false;

uint64_t luaFuncOvr(uint64_t lua_state, uint64_t _2, uint64_t _3, uint64_t _4) {
    uint64_t output = luaFunc(lua_state,_2,_3,_4);
    if (!is_print_hooked((lua_State*)lua_state)) {
        if (isLog) install_print_hook((lua_State*)lua_state);
    }
    if (isFuncLog) scan_new_c_functions((lua_State*)lua_state);
    return output;
}

uint64_t luaFuncOvr2(uint64_t lua_state, uint64_t _2, uint64_t _3, uint64_t _4) {
    uint64_t output = luaFunc2(lua_state,_2,_3,_4);
    if (!is_print_hooked((lua_State*)lua_state)) {
        if (isLog) install_print_hook((lua_State*)lua_state);
    }
    if (isFuncLog) scan_new_c_functions((lua_State*)lua_state);

    return output;
}

void base() {
    Log("SLEEP");
    Sleep(5000);
    Log("START");

    MH_STATUS status = MH_Initialize();
	Log("MinHook Status: ",MH_StatusToString(status));

    MH_STATUS hookStatus = MH_CreateHook(luaFuncPtr, luaFuncOvr,(void**)&luaFunc);
    Log("Hook ",luaFuncPtr," Status: ",MH_StatusToString(hookStatus));
    MH_STATUS queueStatus = MH_QueueEnableHook(luaFuncPtr);
    Log("Queue ",luaFuncPtr," Status: ",MH_StatusToString(queueStatus));

    MH_STATUS hookStatus2 = MH_CreateHook(luaFuncPtr2, luaFuncOvr2,(void**)&luaFunc2);
    Log("Hook ",luaFuncPtr2," Status: ",MH_StatusToString(hookStatus2));
    MH_STATUS queueStatus2 = MH_QueueEnableHook(luaFuncPtr2);
    Log("Queue ",luaFuncPtr2," Status: ",MH_StatusToString(queueStatus2));
    
    MH_STATUS applyStatus = MH_ApplyQueued();
	Log("Apply Status: ",MH_StatusToString(applyStatus));
}
