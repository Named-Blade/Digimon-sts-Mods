#include "logMod.hpp"
#include "DLLMain.hpp"

bool isLog = true;
bool isFuncLog = false;

uint64_t id_count = 0;
int current_id = 0;
int new_current_id = 0;

// Lua API function to swap current_id
int l_swap_current_id(lua_State *L) {
    // Check that the first argument is a number
    luaL_checktype(L, 1, LUA_TNUMBER);

    // Get the new value from Lua
    int new_id = (int)lua_tointeger(L, 1);

    // Save the old value
    int old_id = current_id;

    // Swap in the new value
    new_current_id = new_id;

    // Return the old value to Lua
    lua_pushinteger(L, old_id);
    return 1;  // number of return values
}

// Optional: Lua API function to read current_id without changing it
int l_get_current_id(lua_State *L) {
    lua_pushinteger(L, current_id);
    return 1;
}

std::map<int, lua_State*> L_threads;

void luaEndCallback(int id) {
    L_threads[id] = nullptr;
}

uint64_t luaFuncOvr(uint64_t lua_state, uint64_t _2, uint64_t _3, uint64_t _4) {
    uint64_t output = luaFunc(lua_state,_2,_3,_4);
    if (!is_print_hooked((lua_State*)lua_state)) {

        int id = id_count++;
        current_id = id;
        attach_debug_info((lua_State*)lua_state, id, luaEndCallback);
        lua_State* L = lua_newthread((lua_State*)lua_state);
        L_threads[id] = (L);
        lua_pop((lua_State*)lua_state, 1);

        lua_register (L, "swap_current_id", l_swap_current_id);
        lua_register (L, "get_current_id", l_get_current_id);

        if (isLog) install_print_hook((lua_State*)lua_state);
    }
    if (isFuncLog) scan_new_c_functions((lua_State*)lua_state);
    return output;
}

uint64_t luaFuncOvr2(uint64_t lua_state, uint64_t _2, uint64_t _3, uint64_t _4) {
    uint64_t output = luaFunc2(lua_state,_2,_3,_4);
    if (!is_print_hooked((lua_State*)lua_state)) {

        int id = id_count++;
        current_id = id;
        attach_debug_info((lua_State*)lua_state, id, luaEndCallback);
        lua_State* L = lua_newthread((lua_State*)lua_state);
        L_threads[id] = (L);
        lua_pop((lua_State*)lua_state, 1);

        lua_register (L, "swap_current_id", l_swap_current_id);
        lua_register (L, "get_current_id", l_get_current_id);


        if (isLog) install_print_hook((lua_State*)lua_state);
    }
    if (isFuncLog) scan_new_c_functions((lua_State*)lua_state);

    return output;
}

void lua_repl() {
    std::string line;
    con_allocate(0);

    while (true) {
        if (L_threads[current_id] != nullptr) {
            std::cout << "> ";
            if (!std::getline(std::cin, line)) break; // EOF or Ctrl+D
            if (line.empty()) continue;
            Log(line.c_str(), " ", NumberToHexString((uintptr_t)L_threads[current_id]));

            if (L_threads[current_id] != nullptr) {
                // Load the line as a Lua chunk
                int loadStatus = luaL_loadstring(L_threads[current_id], line.c_str());
                if (loadStatus != LUA_OK) {
                    std::cerr << "Error: " << lua_tostring(L_threads[current_id], -1) << "\n";
                    lua_pop(L_threads[current_id], 1); // remove error message
                    continue;
                }

                // Execute the chunk
                int pcallStatus = lua_pcall(L_threads[current_id], 0, LUA_MULTRET, 0);
                if (pcallStatus != LUA_OK) {
                    std::cerr << "Runtime error: " << lua_tostring(L_threads[current_id], -1) << "\n";
                    lua_pop(L_threads[current_id], 1); // remove error message
                    continue;
                }

                // Print any returned values
                int nResults = lua_gettop(L_threads[current_id]);
                for (int i = 1; i <= nResults; ++i) {
                    if (lua_isnil(L_threads[current_id], i)) std::cout << "nil";
                    else if (lua_isboolean(L_threads[current_id], i)) std::cout << (lua_toboolean(L_threads[current_id], i) ? "true" : "false");
                    else if (lua_isnumber(L_threads[current_id], i)) std::cout << lua_tonumber(L_threads[current_id], i);
                    else if (lua_isstring(L_threads[current_id], i)) std::cout << lua_tostring(L_threads[current_id], i);
                    else std::cout << luaL_typename(L_threads[current_id], i);
                    if (i < nResults) std::cout << "\t";
                }
                if (nResults > 0) std::cout << "\n";

                lua_settop(L_threads[current_id], 0); // clear the stack
            }
        }
        if (new_current_id != 0) {
            current_id = new_current_id;
            new_current_id = 0;
        }
    }
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

    lua_repl();
}
