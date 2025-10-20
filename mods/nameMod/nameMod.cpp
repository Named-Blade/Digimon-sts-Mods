#include "nameMod.hpp"
#include "DLLMain.hpp"

bool isCensorHook(HookContext* context, void* funcPtr){
    return false;
}

void base() {
    Sleep(5000);
    con_allocate(false);
    Log("START");

    CallHook::initialize();

    uintptr_t isCensorAddress = AobScan(isCensorAob);
	if (isCensorAddress != 0){
		auto hook = new CallHookTemplate<OverrideHook>(reinterpret_cast<void *>(isCensorAddress), isCensorHook);
	}

}
