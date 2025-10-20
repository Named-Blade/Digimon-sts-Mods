#pragma once

#include <algorithm>
#include <Windows.h>
#include <CallHook.h>

#include <ModUtils.hpp>
#include <config.hpp>

std::string isCensorAob = "E8 ?? ?? ?? ?? 0F B6 D8 48 8B 55 C8 48 83 FA 10";

typedef bool (*isCensorType) ();

isCensorType isCensor;