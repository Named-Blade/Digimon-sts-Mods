#include "fieldGuideMod.hpp"
#include "DLLMain.hpp"

bool isFieldGuide = false;
ObjectData* objectData;

ObjectData* cameraHook(HookContext* context, void* funcPtr){
    objectData = cameraFunc(context->rcx,context->rdx);
    isFieldGuide = !isFieldGuide;
    return objectData;
}

// --- Quaternion multiplication ---
Quaternion quatMultiply(const Quaternion& q1, const Quaternion& q2) {
    return {
        q1.w*q2.x + q1.x*q2.w + q1.y*q2.z - q1.z*q2.y,
        q1.w*q2.y - q1.x*q2.z + q1.y*q2.w + q1.z*q2.x,
        q1.w*q2.z + q1.x*q2.y - q1.y*q2.x + q1.z*q2.w,
        q1.w*q2.w - q1.x*q2.x - q1.y*q2.y - q1.z*q2.z
    };
}

// --- Create a quaternion from axis-angle ---
Quaternion quatFromAxisAngle(float ax, float ay, float az, float angleRad) {
    float half = angleRad / 2.0f;
    float s = sinf(half);
    return { ax*s, ay*s, az*s, cosf(half) };
}

// --- Normalize quaternion ---
Quaternion quatNormalize(const Quaternion& q) {
    float mag = sqrtf(q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w);
    return { q.x/mag, q.y/mag, q.z/mag, q.w/mag };
}

void base() {
    Sleep(5000);
    con_allocate(false);
    Log("START");

    CallHook::initialize();

    uintptr_t cameraAddress = AobScan(cameraAob);
	if (cameraAddress != 0){
        uintptr_t funcAddr = getAddressFromMemory(cameraAddress, cameraFuncOffset, cameraSize);
        cameraFunc = (cameraFuncType)funcAddr;
		auto hook = new CallHookTemplate<OverrideHook>(reinterpret_cast<void *>(cameraAddress+cameraOffset), cameraHook);
	}

    float speed = 2.0f; // rotations per second
    auto lastTime = std::chrono::high_resolution_clock::now();
    Quaternion currentRot = {0,0,0,1};
    Translation currentTranslation = {0,0,0};

    while (true) {
        auto now = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;

        // --- Determine rotation amounts based on key presses ---
        float rotX = 0.0f, rotY = 0.0f, rotZ = 0.0f;

        if (GetAsyncKeyState(0x57) & 0x8000) rotX += speed * deltaTime; // W key
        if (GetAsyncKeyState(0x53) & 0x8000) rotX -= speed * deltaTime; // S key
        if (GetAsyncKeyState(0x41) & 0x8000) rotY += speed * deltaTime; // A key
        if (GetAsyncKeyState(0x44) & 0x8000) rotY -= speed * deltaTime; // D key
        if (GetAsyncKeyState(0x51) & 0x8000) rotZ += speed * deltaTime; // Q key
        if (GetAsyncKeyState(0x45) & 0x8000) rotZ -= speed * deltaTime; // E key

        // --- Create incremental quaternions for each axis ---
        Quaternion qx = quatFromAxisAngle(1,0,0, rotX);
        Quaternion qy = quatFromAxisAngle(0,1,0, rotY);
        Quaternion qz = quatFromAxisAngle(0,0,1, rotZ);

        // --- Combine rotations (order: Z * Y * X) ---
        Quaternion deltaRot = quatMultiply(quatMultiply(qz, qy), qx);

        // --- Apply incremental rotation ---
        currentRot = quatMultiply(deltaRot, currentRot);
        currentRot = quatNormalize(currentRot);

        if (GetAsyncKeyState(0x27) & 0x8000) currentTranslation.x += speed * deltaTime;
        if (GetAsyncKeyState(0x25) & 0x8000) currentTranslation.x -= speed * deltaTime;
        if (GetAsyncKeyState(0x26) & 0x8000) currentTranslation.y += speed * deltaTime;
        if (GetAsyncKeyState(0x28) & 0x8000) currentTranslation.y -= speed * deltaTime;
        if (GetAsyncKeyState(0xBD) & 0x8000) currentTranslation.z += speed * deltaTime;
        if (GetAsyncKeyState(0xBB) & 0x8000) currentTranslation.z -= speed * deltaTime;

        if (GetAsyncKeyState(0x52) & 0x8000) {
            currentRot = {0,0,0,1};
            currentTranslation = {0,0,0};
        }

        // --- Write to object ---
        if (isFieldGuide) {
            ([](Quaternion currentRot, Translation currentTranslation) {
                __try {
                    objectData->q = currentRot;
                    objectData->t = currentTranslation;
                }
                __except (EXCEPTION_EXECUTE_HANDLER) { 
                    //pass
                }
            })(currentRot, currentTranslation);
        } else {
            currentRot = {0,0,0,1};
            currentTranslation = {0,0,0};
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}
