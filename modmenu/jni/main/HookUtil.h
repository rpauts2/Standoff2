#pragma once
#include <cstring>
#include <cstdio>
#include <dlfcn.h>
#include <sys/mman.h>
#include <unistd.h>
#include <elf.h>
#include <link.h>
#include <android/log.h>

#undef LOGI
#undef LOGE
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "ModMenu", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "ModMenu", __VA_ARGS__)

namespace HookUtil {

// Get the base address of a loaded library from /proc/self/maps
inline void* GetLibraryBase(const char* libName) {
    char line[512];
    FILE* fp = fopen("/proc/self/maps", "r");
    if (!fp) return nullptr;

    void* base = nullptr;
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, libName) && strstr(line, "r-xp")) {
            unsigned long start;
            if (sscanf(line, "%lx-", &start) == 1) {
                base = (void*)start;
                break;
            }
        }
    }
    fclose(fp);
    return base;
}

// Check if a library is loaded
inline bool IsLibraryLoaded(const char* libName) {
    char line[512];
    FILE* fp = fopen("/proc/self/maps", "r");
    if (!fp) return false;

    bool found = false;
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, libName)) {
            found = true;
            break;
        }
    }
    fclose(fp);
    return found;
}

// Simple inline hook using mprotect + direct patch
// This is ARM64-specific
inline bool InlineHook(void* target, void* hook, void** original) {
    if (!target || !hook) return false;

    // Make the target memory writable and executable
    long pageSize = sysconf(_SC_PAGESIZE);
    void* pageBase = (void*)((unsigned long)target & ~(pageSize - 1));
    if (mprotect(pageBase, pageSize * 2, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
        LOGE("mprotect failed for %p", target);
        return false;
    }

    // Save original bytes for trampoline
    uint32_t* targetInstr = (uint32_t*)target;

    // Create a trampoline (simple version: just save and redirect)
    // For ARM64, we use a simple absolute jump
    // This is a simplified version - real implementations use more sophisticated trampolines

    // Allocate executable memory for trampoline
    void* trampoline = mmap(nullptr, 4096,
                            PROT_READ | PROT_WRITE | PROT_EXEC,
                            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (trampoline == MAP_FAILED) {
        LOGE("mmap trampoline failed");
        return false;
    }

    // Copy original instructions to trampoline (first 16 bytes = 4 instructions)
    memcpy(trampoline, target, 16);

    // Add branch back to original+16
    // ARM64: add x16, pc, #16; br x16
    uint32_t* tramp = (uint32_t*)trampoline;
    tramp[4] = 0xD2800210; // mov x16, #16
    tramp[5] = 0xD1000210; // sub x16, x16, #0 (not needed, but safe)

    // Actually, simpler approach: just use ADR + BR
    // adrp x16, #0; add x16, x16, #(target+16 & 0xfff); br x16
    int64_t offset = (int64_t)((uint8_t*)target + 16) - (int64_t)&tramp[6];
    int64_t pageOffset = offset & 0xFFF;
    int64_t pageDelta = offset >> 12;
    tramp[6] = 0x90000010 | ((pageDelta & 0x7FFFF) << 5); // adrp x16, delta
    tramp[7] = 0x91000210 | ((pageOffset & 0xFFF) << 10);  // add x16, x16, #pageOffset
    tramp[8] = 0xD61F0200;                                   // br x16

    *original = trampoline;

    // Now patch the target to jump to our hook
    int64_t hookOffset = (int64_t)hook - (int64_t)target;
    int64_t hookPageOffset = hookOffset & 0xFFF;
    int64_t hookPageDelta = hookOffset >> 12;

    targetInstr[0] = 0x90000010 | ((hookPageDelta & 0x7FFFF) << 5); // adrp x16, delta
    targetInstr[1] = 0x91000210 | ((hookPageOffset & 0xFFF) << 10); // add x16, x16, #offset
    targetInstr[2] = 0xD61F0200;                                     // br x16
    targetInstr[3] = 0xD503201F;                                     // nop (in case of alignment)

    __builtin___clear_cache((char*)target, (char*)target + 16);

    LOGI("Hook installed at %p -> %p (trampoline: %p)", target, hook, trampoline);
    return true;
}

// Hook a function from a loaded library using dlsym
inline bool HookFunction(const char* libName, const char* funcName,
                         void* hook, void** original) {
    void* handle = dlopen(libName, RTLD_LAZY | RTLD_NOLOAD);
    if (!handle) {
        LOGE("dlopen(%s) failed: %s", libName, dlerror());
        return false;
    }

    void* target = dlsym(handle, funcName);
    dlclose(handle);

    if (!target) {
        LOGE("dlsym(%s, %s) failed: %s", libName, funcName, dlerror());
        return false;
    }

    LOGI("Found %s.%s at %p", libName, funcName, target);
    return InlineHook(target, hook, original);
}

} // namespace HookUtil
