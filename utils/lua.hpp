extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}
#include <stdio.h>

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

    // Prefix for all output
    fputs("[logMod] lua print():", stdout);

    if (n == 0) {
        fputc('\n', stdout);
        fflush(stdout);
        return 0;
    }

    for (int i = 1; i <= n; ++i) {
        size_t len;
        // luaL_tolstring converts the Lua value to a string safely
        const char *s = luaL_tolstring(L, i, &len);
        if (!s) s = "(null)";
        if (i > 1) fputc('\t', stdout);
        fwrite(s, 1, len, stdout);
        lua_pop(L, 1); // remove the string pushed by luaL_tolstring
    }

    fputc('\n', stdout);
    fflush(stdout);
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