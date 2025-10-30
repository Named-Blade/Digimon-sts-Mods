extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <ModUtils.hpp>

void inspect_state(lua_State* L) {
    if (!L) { printf("NULL!\n"); return; }

    // Non-destructive check 1: stack top
    int top = lua_gettop(L);
    printf("lua_gettop = %d\n", top);

    // Non-destructive check 2: type of index 1 (if present)
    if (top >= 1) {
        int t = lua_type(L, 1);
        printf("type at index 1 = %s (%d)\n", lua_typename(L, t), t);
    }

    // Non-destructive check 3: registry introspection (read-only)
    lua_pushnil(L);
    // WARNING: use lua_next only if you know registry layout and are careful — shown as example (do not mutate).
    // Here: just attempt to read a well-known registry entry if present
    lua_getfield(L, LUA_REGISTRYINDEX, "_LOADED"); // push registry._LOADED (may be nil)
    if (!lua_isnil(L, -1)) {
        printf("registry._LOADED present\n");
    } else {
        printf("registry._LOADED nil or absent\n");
    }
    lua_pop(L, 1); // cleanup
}

#define REG_ORIG_PRINT "myhook_original_print_v1"
#define REG_HOOK_MARKER "myhook_print_marker_v1"

// Replacement print: converts each arg to string and writes to stdout
static int hooked_print(lua_State *L) {
    int n = lua_gettop(L);

    if (n == 0) {
        ModUtils::Log("lua print():");
        return 0;
    }

    std::stringstream stream;
    stream << "lua print():";

    for (int i = 1; i <= n; ++i) {
        size_t len;
        const char *s = luaL_tolstring(L, i, &len);
        if (!s) s = "(null)";
        if (i > 1) stream << '\t';
        stream << std::string(s, len);
        lua_pop(L, 1);
    }

    ModUtils::Log(stream.str());
    return 0;
}

// Optional: call the original print (if stored) with the current arguments.
// Returns whatever the original print returns (not typical — original print returns 0)
static int call_original_print(lua_State *L) {
    // push original function from registry
    lua_getfield(L, LUA_REGISTRYINDEX, REG_ORIG_PRINT);
    if (lua_isfunction(L, -1)) {
        int nargs = lua_gettop(L) - 1; // args currently on stack after pushing orig_print
        // move original function below the arguments: stack is [arg1...argN, orig_print]
        // we want [orig_print, arg1...argN]
        lua_insert(L, 1);
        if (lua_pcall(L, nargs, 0, 0) != LUA_OK) {
            // print error to stderr, but don't crash
            const char *err = lua_tostring(L, -1);
            fprintf(stderr, "call_original_print error: %s\n", err ? err : "(unknown)");
            lua_pop(L, 1);
        }
    } else {
        lua_pop(L, 1); // remove non-function
    }
    return 0;
}

// Install hook and marker
void install_print_hook(lua_State *L) {
    // Save original print
    lua_getglobal(L, "print");
    lua_setfield(L, LUA_REGISTRYINDEX, REG_ORIG_PRINT);

    // Push hooked function
    lua_pushcfunction(L, hooked_print);
    lua_setglobal(L, "print");

    // Push a marker function (unique address)
    lua_pushcfunction(L, hooked_print);
    lua_setfield(L, LUA_REGISTRYINDEX, REG_HOOK_MARKER);
}

// Restore original print
void restore_print(lua_State *L) {
    lua_getfield(L, LUA_REGISTRYINDEX, REG_ORIG_PRINT);
    if (lua_isfunction(L, -1)) {
        lua_setglobal(L, "print");
    } else {
        lua_pop(L, 1);
        lua_pushnil(L);
        lua_setglobal(L, "print");
    }
    lua_pushnil(L);
    lua_setfield(L, LUA_REGISTRYINDEX, REG_ORIG_PRINT);
    lua_pushnil(L);
    lua_setfield(L, LUA_REGISTRYINDEX, REG_HOOK_MARKER);
}

// Check if print is currently hooked
int is_print_hooked(lua_State *L) {
    lua_getglobal(L, "print");                // push current print
    lua_getfield(L, LUA_REGISTRYINDEX, REG_HOOK_MARKER); // push marker

    int hooked = lua_rawequal(L, -1, -2);     // compare addresses
    lua_pop(L, 2);                             // cleanup
    return hooked;                             // 1 if hooked, 0 if not
}


#define MAX_VISITED 4096
#define MAX_FUNCTIONS 1024*32
#define OUTPUT_FILE "lua_functions.txt"

typedef struct {
    char* name;
    lua_CFunction ptr;
} FunctionEntry;

static FunctionEntry known_functions[MAX_FUNCTIONS];
static int known_count = 0;

// Check if function already exists in the known map
static int is_known(const char* name, lua_CFunction fptr) {
    for (int i = 0; i < known_count; ++i) {
        if (known_functions[i].ptr == fptr && strcmp(known_functions[i].name, name) == 0)
            return 1;
    }
    return 0;
}

// Add function to the map and write to file
static void add_function(const char* name, lua_CFunction fptr) {
    if (known_count >= MAX_FUNCTIONS) return;
    known_functions[known_count].name = strdup(name);
    known_functions[known_count].ptr = fptr;
    known_count++;

    // Append to file
    FILE* f = fopen(OUTPUT_FILE, "a");
    if (f) {
        fprintf(f, "%s -> %p\n", name, (void*)fptr);
        fclose(f);
    }
}

// Recursive traversal with cycle protection
static void list_c_functions_safe(lua_State* L, int index, const char* prefix, void** visited, int* visited_count) {
    index = lua_absindex(L, index);

    // cycle check
    for (int i = 0; i < *visited_count; ++i) {
        if (visited[i] == lua_topointer(L, index)) return;
    }
    if (*visited_count < MAX_VISITED)
        visited[(*visited_count)++] = (void*)lua_topointer(L, index);

    lua_pushnil(L);
    while (lua_next(L, index) != 0) {
        // -1 = value, -2 = key
        const char* key_str = NULL;
        char full_name[512] = {0};

        if (lua_type(L, -2) == LUA_TSTRING)
            key_str = lua_tostring(L, -2);

        if (key_str && prefix && prefix[0])
            snprintf(full_name, sizeof(full_name), "%s.%s", prefix, key_str);
        else if (key_str)
            snprintf(full_name, sizeof(full_name), "%s", key_str);
        else
            full_name[0] = '\0'; // no string key, skip name prefix

        // if value is a C function
        if (lua_isfunction(L, -1) && lua_iscfunction(L, -1)) {
            lua_CFunction fptr = lua_tocfunction(L, -1);
            const char* name_to_log = (full_name[0] != '\0') ? full_name : "(anonymous)";
            if (!is_known(name_to_log, fptr)) {
                printf("New C function found: %s -> %p\n", name_to_log, (void*)fptr);
                add_function(name_to_log, fptr);
            }
        }

        // if value is a table, recurse regardless of key type
        if (lua_istable(L, -1)) {
            list_c_functions_safe(L, lua_gettop(L), full_name, visited, visited_count);
        }

        lua_pop(L, 1); // pop value, keep key for next iteration
    }
}

// Scan globals and log only new C functions
void scan_new_c_functions(lua_State* L) {
    void* visited[MAX_VISITED];
    int visited_count = 0;
    lua_pushglobaltable(L);
    list_c_functions_safe(L, lua_gettop(L), "", visited, &visited_count);
    lua_pop(L, 1);
}

typedef void (*LuaCallbackType) (int);

typedef struct {
    int my_debug_id;
    LuaCallbackType callback;
} DebugInfo;

static int my_debug_gc(lua_State *L) {
    DebugInfo *info = (DebugInfo *)lua_touserdata(L, 1);
    ModUtils::Log("Lua state closing, id = %d\n", info->my_debug_id);
    if (info->callback != nullptr) {
        info->callback(info->my_debug_id);
    }
    return 0;
}

void attach_debug_info(lua_State *L, int id, LuaCallbackType callback = nullptr) {
    DebugInfo *info = (DebugInfo *)lua_newuserdata(L, sizeof(DebugInfo));
    info->my_debug_id = id;
    info->callback = callback;
    if (luaL_newmetatable(L, "MyDebugMeta")) {
        lua_pushcfunction(L, my_debug_gc);
        lua_setfield(L, -2, "__gc");
    }
    lua_setmetatable(L, -2);

    lua_setfield(L, LUA_REGISTRYINDEX, "my_debug_userdata");
}