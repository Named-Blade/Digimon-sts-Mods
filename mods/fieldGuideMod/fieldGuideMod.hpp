#pragma once

#include <algorithm>
#include <Windows.h>
#include <CallHook.h>
#include <iostream>
#include <cmath>
#include <chrono>
#include <thread>

#include <ModUtils.hpp>
#include <config.hpp>

std::string cameraAob = "E8 ?? ?? ?? ?? 48 8B 4F 38 48 8D 97 80 00 00 00 E8 ?? ?? ?? ??";
int cameraOffset = 16;
int cameraFuncOffset = cameraOffset+1;
int cameraSize = 4;

struct Quaternion {
    float x, y, z, w;
};

struct Translation {
    float x, y, z;
};

struct ObjectData {
    char8_t _1[0x70];
	Quaternion q;
    Translation t;
};

typedef ObjectData* (*cameraFuncType) (uint64_t, uint64_t);

cameraFuncType cameraFunc;