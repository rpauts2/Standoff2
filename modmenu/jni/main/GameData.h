#pragma once

#include <jni.h>
#include <dlfcn.h>
#include <unistd.h>
#include <cstring>
#include <cmath>
#include <vector>
#include <android/log.h>
#include <sys/mman.h>
#include "GameOffsets.h"

#define LOG_TAG "SO2Mod"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// ==================== IL2CPP Type Definitions ====================
// Minimal IL2CPP types for runtime introspection

typedef void* Il2CppObject;
typedef void* Il2CppClass;
typedef void* Il2CppImage;
typedef void* Il2CppAssembly;
typedef void* Il2CppDomain;
typedef void* Il2CppString;

struct Il2CppMethodPointer_ {
    void* methodPointer;
    void* invoker_method;
    const char* name;
    Il2CppClass* klass;
    void* return_type;
    void* parameters;
    uint32_t token;
};

struct Il2CppMethodInfo {
    void* methodPointer;
    void* invoker_method;
    const char* name;
    Il2CppClass* klass;
    void* return_type;
    void* parameters;
    uint32_t token;
    uint32_t flags;
};

struct Il2CppFieldInfo {
    const char* name;
    Il2CppClass* klass;
    void* type;
    uint32_t token;
    int32_t offset;
};

struct Il2CppType {
    void* data;
    unsigned int attrs : 16;
    unsigned int type : 8;
    unsigned int num_mods : 6;
    unsigned int byref : 1;
    unsigned int pinned : 1;
    unsigned int valuetype : 1;
};

// ==================== IL2CPP API Function Pointers ====================
struct IL2CPP_API {
    void* lib_handle = nullptr;

    Il2CppDomain* (*domain_get)() = nullptr;
    void (*domain_get_assemblies)(const Il2CppDomain* domain, void** assemblies, uint32_t* count) = nullptr;
    const Il2CppImage* (*assembly_get_image)(const Il2CppAssembly* assembly) = nullptr;
    const char* (*image_get_name)(const Il2CppImage* image) = nullptr;

    Il2CppClass* (*class_from_name)(const Il2CppImage* image, const char* ns, const char* name) = nullptr;
    Il2CppFieldInfo* (*class_get_field_from_name)(Il2CppClass* klass, const char* name) = nullptr;
    int (*field_get_offset)(Il2CppFieldInfo* field) = nullptr;

    void* (*class_get_static_field_data)(Il2CppClass* klass) = nullptr;
    uint32_t (*class_get_type_token)(Il2CppClass* klass) = nullptr;
    void (*class_get_method_from_name)(Il2CppClass* klass, const char* name, int param_count) = nullptr;
    void (*class_get_methods)(Il2CppClass* klass, void** methods) = nullptr;

    void (*object_get_field_value)(Il2CppObject* obj, Il2CppFieldInfo* field, void* value) = nullptr;
    void (*object_set_field_value)(Il2CppObject* obj, Il2CppFieldInfo* field, void* value) = nullptr;
    Il2CppObject* (*object_new)(Il2CppClass* klass) = nullptr;

    const Il2CppType* (*field_get_type)(Il2CppFieldInfo* field) = nullptr;

    bool loaded = false;
};

static IL2CPP_API il2cpp;

static bool initIL2CPP() {
    if (il2cpp.loaded) return true;

    il2cpp.lib_handle = dlopen("libil2cpp.so", RTLD_LAZY | RTLD_NOLOAD);
    if (!il2cpp.lib_handle) {
        LOGE("libil2cpp.so not found");
        return false;
    }

    #define LOAD_SYM(name) il2cpp.name = (decltype(il2cpp.name))dlsym(il2cpp.lib_handle, "il2cpp_" #name)
    LOAD_SYM(domain_get);
    LOAD_SYM(domain_get_assemblies);
    LOAD_SYM(assembly_get_image);
    LOAD_SYM(image_get_name);
    LOAD_SYM(class_from_name);
    LOAD_SYM(class_get_field_from_name);
    LOAD_SYM(field_get_offset);
    LOAD_SYM(class_get_static_field_data);
    LOAD_SYM(class_get_type_token);
    LOAD_SYM(object_get_field_value);
    LOAD_SYM(object_set_field_value);
    LOAD_SYM(object_new);
    LOAD_SYM(field_get_type);
    #undef LOAD_SYM

    if (!il2cpp.domain_get || !il2cpp.class_from_name) {
        LOGE("Failed to load IL2CPP symbols");
        return false;
    }

    il2cpp.loaded = true;
    LOGI("IL2CPP API loaded from %p", il2cpp.lib_handle);
    return true;
}

// ==================== IL2CPP Helpers ====================
static Il2CppClass* findClass(const char* image_name, const char* ns, const char* name) {
    if (!il2cpp.loaded) return nullptr;

    const Il2CppDomain* domain = il2cpp.domain_get();
    if (!domain) return nullptr;

    void* assemblies[64];
    uint32_t count = 0;
    il2cpp.domain_get_assemblies(domain, assemblies, &count);

    for (uint32_t i = 0; i < count; i++) {
        const Il2CppImage* image = il2cpp.assembly_get_image((const Il2CppAssembly*)assemblies[i]);
        if (!image) continue;
        const char* img_name = il2cpp.image_get_name(image);
        if (!img_name) continue;

        if (strstr(img_name, image_name)) {
            Il2CppClass* klass = il2cpp.class_from_name(image, ns, name);
            if (klass) return klass;
        }
    }
    return nullptr;
}

static int getFieldOffset(Il2CppClass* klass, const char* field_name) {
    if (!klass || !il2cpp.class_get_field_from_name) return -1;
    Il2CppFieldInfo* field = il2cpp.class_get_field_from_name(klass, field_name);
    if (!field) return -1;
    return il2cpp.field_get_offset ? il2cpp.field_get_offset(field) : field->offset;
}

static bool getFieldBool(void* obj, int offset) {
    if (!obj || offset < 0) return false;
    return *((bool*)((uint8_t*)obj + offset));
}

static int getFieldInt(void* obj, int offset) {
    if (!obj || offset < 0) return 0;
    return *((int*)((uint8_t*)obj + offset));
}

static float getFieldFloat(void* obj, int offset) {
    if (!obj || offset < 0) return 0.0f;
    return *((float*)((uint8_t*)obj + offset));
}

static void* getFieldPtr(void* obj, int offset) {
    if (!obj || offset < 0) return nullptr;
    return *((void**)((uint8_t*)obj + offset));
}

static void setFieldBool(void* obj, int offset, bool val) {
    if (!obj || offset < 0) return;
    *((bool*)((uint8_t*)obj + offset)) = val;
}

static void setFieldFloat(void* obj, int offset, float val) {
    if (!obj || offset < 0) return;
    *((float*)((uint8_t*)obj + offset)) = val;
}

// ==================== Memory Scanner ====================
static uintptr_t findLibraryBase(const char* lib_name) {
    char line[512];
    FILE* fp = fopen("/proc/self/maps", "r");
    if (!fp) return 0;

    uintptr_t base = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, lib_name) && strstr(line, "r-xp")) {
            base = strtoul(line, nullptr, 16);
            break;
        }
    }
    fclose(fp);
    return base;
}

// Pattern scan in memory region
static uintptr_t patternScan(uintptr_t start, size_t size, const uint8_t* pattern, const char* mask) {
    size_t maskLen = strlen(mask);
    for (size_t i = 0; i < size - maskLen; i++) {
        bool found = true;
        for (size_t j = 0; j < maskLen; j++) {
            if (mask[j] != '?' && pattern[j] != *(uint8_t*)(start + i + j)) {
                found = false;
                break;
            }
        }
        if (found) return start + i;
    }
    return 0;
}

// ==================== Game Data Reader ====================
class GameData {
public:
    static GameData& getInstance() {
        static GameData instance;
        return instance;
    }

    bool initialized = false;
    uintptr_t il2cpp_base = 0;

    // Cached class pointers
    Il2CppClass* klass_PlayerController = nullptr;
    Il2CppClass* klass_PlayerManager = nullptr;
    Il2CppClass* klass_GameController = nullptr;
    Il2CppClass* klass_GameManager = nullptr;
    Il2CppClass* klass_AimController = nullptr;
    Il2CppClass* klass_MovementController = nullptr;
    Il2CppClass* klass_NetworkController = nullptr;
    Il2CppClass* klass_WeaponryController = nullptr;

    // Cached field offsets
    int off_local_player = -1;
    int off_aim_controller = -1;
    int off_movement_controller = -1;
    int off_network_controller = -1;
    int off_weaponry_controller = -1;
    int off_player_main_camera = -1;
    int off_body_transform = -1;
    int off_team = -1;
    int off_alive_1 = -1;
    int off_alive_2 = -1;
    int off_health_float = -1;
    int off_max_ct_health = -1;
    int off_max_tr_health = -1;
    int off_photon_view = -1;
    int off_actor_id = -1;
    int off_player_index = -1;

    int off_camera = -1;
    int off_sensitivity_x = -1;
    int off_sensitivity_y = -1;

    // Game objects
    void* local_player = nullptr;
    void* game_controller = nullptr;

    // Camera
    void* unity_camera = nullptr;

    void init() {
        if (initialized) return;

        il2cpp_base = findLibraryBase("libil2cpp.so");
        if (!il2cpp_base) {
            LOGE("libil2cpp.so base not found");
            return;
        }
        LOGI("libil2cpp.so base: 0x%lx", il2cpp_base);

        if (!initIL2CPP()) return;

        // Find classes (Unity assembly is usually "Assembly-CSharp")
        klass_PlayerController = findClass("Assembly-CSharp", "Axlebolt.Standoff.Player", "PlayerController");
        klass_PlayerManager = findClass("Assembly-CSharp", "Axlebolt.Standoff.Player", "PlayerManager");
        klass_GameController = findClass("Assembly-CSharp", "Axlebolt.Standoff.Game", "GameController");
        klass_GameManager = findClass("Assembly-CSharp", "Axlebolt.Standoff.Game", "GameManager");
        klass_AimController = findClass("Assembly-CSharp", "Axlebolt.Standoff.Player", "AimController");
        klass_MovementController = findClass("Assembly-CSharp", "Axlebolt.Standoff.Player", "MovementController");
        klass_NetworkController = findClass("Assembly-CSharp", "Axlebolt.Standoff.Player", "NetworkController");
        klass_WeaponryController = findClass("Assembly-CSharp", "Axlebolt.Standoff.Inventory", "WeaponryController");

        if (!klass_PlayerController) {
            LOGE("PlayerController class not found");
            // Try alternate namespace
            klass_PlayerController = findClass("Assembly-CSharp", "", "PlayerController");
            if (!klass_PlayerController) {
                LOGE("PlayerController not found in any namespace");
                return;
            }
        }
        LOGI("PlayerController class: %p", klass_PlayerController);

        // Get field offsets from IL2CPP metadata
        auto resolveField = [&](Il2CppClass* klass, const char* name, const char* fallback_ns = nullptr, const char* fallback_name = nullptr) -> int {
            int off = getFieldOffset(klass, name);
            if (off >= 0) {
                LOGI("Field %s: offset 0x%x", name, off);
                return off;
            }
            if (fallback_ns && fallback_name) {
                Il2CppClass* altKlass = findClass("Assembly-CSharp", fallback_ns, fallback_name);
                if (altKlass) {
                    off = getFieldOffset(altKlass, name);
                    if (off >= 0) {
                        LOGI("Field %s (from %s): offset 0x%x", name, fallback_name, off);
                        return off;
                    }
                }
            }
            LOGI("Field %s not found", name);
            return -1;
        };

        // PlayerController fields
        off_aim_controller = getFieldOffset(klass_PlayerController, "<DAFBHDCBBFDBDBD>k__BackingField");
        off_movement_controller = getFieldOffset(klass_PlayerController, "<DBHAFHFGGFGFECA>k__BackingField");
        off_network_controller = getFieldOffset(klass_PlayerController, "<CEGFDEEGFGDBDBF>k__BackingField");
        off_weaponry_controller = getFieldOffset(klass_PlayerController, "<AECFFADGAGBBHHB>k__BackingField");
        off_player_main_camera = getFieldOffset(klass_PlayerController, "HDDBDFEHGCFFBEA");
        off_body_transform = getFieldOffset(klass_PlayerController, "AEHBEAEHHDBAECC");
        off_team = getFieldOffset(klass_PlayerController, "EAGCGEABDGFGBHC");
        off_alive_1 = getFieldOffset(klass_PlayerController, "EDBGBDAHEAEDFCC");
        off_alive_2 = getFieldOffset(klass_PlayerController, "<AFBHCBEGEDFFEBE>k__BackingField");
        off_health_float = getFieldOffset(klass_PlayerController, "<AEDDGECCHCFBBCH>k__BackingField");
        off_max_ct_health = getFieldOffset(klass_PlayerController, "MaxCtHealth");
        off_max_tr_health = getFieldOffset(klass_PlayerController, "MaxTrHealth");
        off_photon_view = getFieldOffset(klass_PlayerController, "<HHCEEHGACEBEDFC>k__BackingField");
        off_actor_id = getFieldOffset(klass_PlayerController, "<FACDFFBDHEBHAFG>k__BackingField");
        off_player_index = getFieldOffset(klass_PlayerController, "<FFHFDHGFFDGFBBB>k__BackingField");

        // PlayerManager fields
        if (klass_PlayerManager) {
            off_local_player = getFieldOffset(klass_PlayerManager, "<CFCHCBFEEFHBAEH>k__BackingField");
        }

        // AimController fields
        if (klass_AimController) {
            off_sensitivity_x = getFieldOffset(klass_AimController, "sensitivityX");
            off_sensitivity_y = getFieldOffset(klass_AimController, "sensitivityY");
        }

        // PlayerMainCamera fields
        // Found through PlayerController -> PlayerMainCamera -> Camera
        LOGI("All offsets resolved. GameData initialized.");

        initialized = true;
    }

    // Get local player from PlayerManager singleton
    void* getLocalPlayer() {
        if (!klass_PlayerManager || off_local_player < 0) return nullptr;

        // Get static field data for PlayerManager (singleton pattern)
        // PlayerManager stores local player in static field
        void* static_data = il2cpp.class_get_static_field_data ?
                            il2cpp.class_get_static_field_data(klass_PlayerManager) : nullptr;
        if (!static_data) return nullptr;

        return getFieldPtr(static_data, off_local_player);
    }

    // Read transform position
    bool getTransformPosition(void* transform, float* out) {
        if (!transform) return false;
        // Unity Transform -> internal position is accessible via TransformAccess
        // For simplicity, we'll try to read it from the MonoBehaviour's transform
        // In Unity IL2CPP, MonoBehaviour inherits from Component -> Object
        // Transform access requires calling Unity methods, so we use the simpler approach:
        // The game stores positions in the CharacterController or in TransformData

        // Actually for Unity IL2CPP, we need to call Transform_get_position
        // via IL2CPP method invocation. Since we can't easily do that,
        // we'll use the CharacterController or direct memory reading.

        // For now, return false - we'll need Unity API access
        return false;
    }

    // WorldToScreenPoint using Camera
    bool worldToScreen(void* camera, float wx, float wy, float wz, float* sx, float* sy) {
        if (!camera) return false;
        // Unity Camera.WorldToScreenPoint is a C# method
        // We need to invoke it through IL2CPP
        // For now, use a simple approximation

        // TODO: Call Camera_WorldToScreenPoint_Injected through IL2CPP
        return false;
    }

    // Update game state each frame
    void update() {
        if (!initialized) return;

        local_player = getLocalPlayer();
    }
};
