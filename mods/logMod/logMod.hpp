#pragma once

#include <algorithm>
#include <Windows.h>

#include <ModUtils.hpp>
#include <config.hpp>

std::string isUnlockedAob = "E8 ?? ?? ?? ?? 0F B6 08 41 88 8E E8 00 00 00";

void* getBattleDefineValuePtr = (void*)0x14D20E1A0;

typedef uint64_t (*getBattleDefineValueType) (uint64_t);

getBattleDefineValueType getBattleDefineValue;