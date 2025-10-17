#pragma once

#include <algorithm>
#include <Windows.h>

#include <ModUtils.hpp>
#include <lua.hpp>
#include <config.hpp>

void* luaFuncPtr = (void*)0x1404580B0;

void* luaFuncPtr2 = (void*)0x140BAE580;

typedef uint64_t (*luaFuncType) (uint64_t, uint64_t, uint64_t, uint64_t);

luaFuncType luaFunc;
luaFuncType luaFunc2;