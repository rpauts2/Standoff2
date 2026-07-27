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

extern float g_ScreenW, g_ScreenH;

#define LOG_TAG "SO2Mod"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// ==================== Unity IL2CPP Types ====================
struct Il2CppImage;
struct Il2CppAssembly;
struct Il2CppDomain;
struct Il2CppClass;
struct Il2CppObject;
struct Il2CppFieldInfo;
struct Il2CppMethodInfo;
struct Il2CppString;
struct Il2CppType;

struct Il2CppFieldInfo {
    const char* name;
    Il2CppClass* klass;
    const Il2CppType* type;
    uint32_t token;
    int32_t offset;
};

struct Il2CppMethodInfo {
    void* methodPointer;
    void* invoker_method;
    const char* name;
    Il2CppClass* klass;
    const Il2CppType* return_type;
    const Il2CppType* parameters;
    uint32_t token;
    uint16_t flags;
    uint16_t iflags;
    uint16_t slot;
    uint8_t parameters_count;
    uint8_t is_generic : 1;
    uint8_t is_inflated : 1;
    uint8_t wrapper_type : 1;
    uint8_t is_marshaled_by_native : 1;
    uint8_t is_internal : 1;
    uint8_t is_virtual : 1;
    uint8_t is_static : 1;
    uint8_t is_final : 1;
    uint8_t has_full_generic_sharing : 1;
    uint8_t string_config_flag : 6;
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

// ==================== IL2CPP API ====================
struct IL2CPP_API {
    void* lib = nullptr;

    Il2CppDomain* (*domain_get)() = nullptr;
    void (*domain_get_assemblies)(const Il2CppDomain*, void**, uint32_t*) = nullptr;
    const Il2CppImage* (*assembly_get_image)(const Il2CppAssembly*) = nullptr;
    const char* (*image_get_name)(const Il2CppImage*) = nullptr;

    Il2CppClass* (*class_from_name)(const Il2CppImage*, const char*, const char*) = nullptr;
    Il2CppFieldInfo* (*class_get_field_from_name)(Il2CppClass*, const char*) = nullptr;
    int (*field_get_offset)(const Il2CppFieldInfo*) = nullptr;
    const Il2CppType* (*field_get_type)(const Il2CppFieldInfo*) = nullptr;

    Il2CppMethodInfo* (*class_get_method_from_name)(Il2CppClass*, const char*, int) = nullptr;
    void (*class_get_methods)(Il2CppClass*, void**) = nullptr;

    void* (*class_get_static_field_data)(Il2CppClass*) = nullptr;

    void (*object_field_get_value)(Il2CppObject*, const Il2CppFieldInfo*, void*) = nullptr;
    void (*object_field_set_value)(Il2CppObject*, const Il2CppFieldInfo*, void*) = nullptr;
    Il2CppObject* (*object_new)(Il2CppClass*) = nullptr;
    void* (*object_unbox)(Il2CppObject*) = nullptr;

    const Il2CppType* (*class_get_type)(Il2CppClass*) = nullptr;
    int (*class_get_type_token)(Il2CppClass*) = nullptr;

    Il2CppClass* (*class_get_element_class)(Il2CppClass*) = nullptr;
    int (*class_array_element_size)(Il2CppClass*) = nullptr;

    const Il2CppType* (*method_get_return_type)(const Il2CppMethodInfo*) = nullptr;
    uint32_t (*method_get_flags)(const Il2CppMethodInfo*, uint32_t*) = nullptr;

    bool loaded = false;
};

static IL2CPP_API il2cpp;

// ==================== IL2CPP Init ====================
static bool initIL2CPP() {
    if (il2cpp.loaded) return true;

    il2cpp.lib = dlopen("libil2cpp.so", RTLD_LAZY | RTLD_NOLOAD);
    if (!il2cpp.lib) { LOGE("libil2cpp.so not found"); return false; }

    #define LOAD(name) il2cpp.name = (decltype(il2cpp.name))dlsym(il2cpp.lib, "il2cpp_" #name)
    LOAD(domain_get);
    LOAD(domain_get_assemblies);
    LOAD(assembly_get_image);
    LOAD(image_get_name);
    LOAD(class_from_name);
    LOAD(class_get_field_from_name);
    LOAD(field_get_offset);
    LOAD(field_get_type);
    LOAD(class_get_method_from_name);
    LOAD(class_get_methods);
    LOAD(class_get_static_field_data);
    LOAD(object_field_get_value);
    LOAD(object_field_set_value);
    LOAD(object_new);
    LOAD(object_unbox);
    LOAD(class_get_type);
    LOAD(class_get_type_token);
    LOAD(class_get_element_class);
    LOAD(class_array_element_size);
    LOAD(method_get_return_type);
    LOAD(method_get_flags);
    #undef LOAD

    if (!il2cpp.domain_get || !il2cpp.class_from_name) {
        LOGE("Failed to load IL2CPP API");
        return false;
    }

    il2cpp.loaded = true;
    LOGI("IL2CPP API loaded from %p", il2cpp.lib);
    return true;
}

// ==================== IL2CPP Helpers ====================
static Il2CppClass* findClass(const char* img_pattern, const char* ns, const char* name) {
    const Il2CppDomain* domain = il2cpp.domain_get();
    if (!domain) return nullptr;

    void* assemblies[128];
    uint32_t count = 0;
    il2cpp.domain_get_assemblies(domain, assemblies, &count);

    for (uint32_t i = 0; i < count; i++) {
        const Il2CppImage* img = il2cpp.assembly_get_image((const Il2CppAssembly*)assemblies[i]);
        if (!img) continue;
        const char* img_name = il2cpp.image_get_name(img);
        if (!img_name) continue;
        if (strstr(img_name, img_pattern)) {
            Il2CppClass* k = il2cpp.class_from_name(img, ns, name);
            if (k) return k;
        }
    }
    return nullptr;
}

static int getFieldOff(Il2CppClass* k, const char* name) {
    if (!k) return -1;
    Il2CppFieldInfo* f = il2cpp.class_get_field_from_name(k, name);
    if (!f) return -1;
    return il2cpp.field_get_offset ? il2cpp.field_get_offset(f) : f->offset;
}

static Il2CppMethodInfo* getMethod(Il2CppClass* k, const char* name, int params = 0) {
    if (!k) return nullptr;
    return il2cpp.class_get_method_from_name(k, name, params);
}

static void* getPtr(void* obj, int off) {
    if (!obj || off < 0) return nullptr;
    return *(void**)((uint8_t*)obj + off);
}

static bool getBool(void* obj, int off) {
    if (!obj || off < 0) return false;
    return *(bool*)((uint8_t*)obj + off);
}

static int getInt(void* obj, int off) {
    if (!obj || off < 0) return 0;
    return *(int*)((uint8_t*)obj + off);
}

static float getFloat(void* obj, int off) {
    if (!obj || off < 0) return 0.0f;
    return *(float*)((uint8_t*)obj + off);
}

static void setFloat(void* obj, int off, float v) {
    if (!obj || off < 0) return;
    *(float*)((uint8_t*)obj + off) = v;
}

static void setInt(void* obj, int off, int v) {
    if (!obj || off < 0) return;
    *(int*)((uint8_t*)obj + off) = v;
}

// ==================== Unity Types in Memory ====================
// Vector3 in IL2CPP: fields at 0x10 (x), 0x14 (y), 0x18 (z)
struct MemVector3 {
    void* klass_or_padding[2]; // IL2CPP header
    float x, y, z;
};

// Matrix4x4 in IL2CPP: 16 floats starting at 0x10
struct MemMatrix4x4 {
    void* klass_or_padding[2]; // IL2CPP header
    float m[16]; // m00,m10,m20,m30,m01,m11,m21,m31,m02,m12,m22,m32,m03,m13,m23,m33
};

// ScreenPoint (Vector3 for screen coords)
struct MemScreenPoint {
    float x, y, z;
};

// ==================== Memory Scanner ====================
static uintptr_t findLibBase(const char* lib) {
    char line[512];
    FILE* fp = fopen("/proc/self/maps", "r");
    if (!fp) return 0;
    uintptr_t base = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, lib) && strstr(line, "r-xp")) {
            base = strtoul(line, nullptr, 16);
            break;
        }
    }
    fclose(fp);
    return base;
}

// ==================== PlayerData ====================
struct PlayerData {
    void* obj = nullptr;           // CharacterPlayer* or PlayerController*
    void* playerController = nullptr;
    bool valid = false;
    bool isLocal = false;

    // Position
    float posX = 0, posY = 0, posZ = 0;
    float headX = 0, headY = 0, headZ = 0;

    // Screen coords
    float screenX = 0, screenY = 0;
    float headScreenX = 0, headScreenY = 0;
    float boxW = 0, boxH = 0;
    bool onScreen = false;

    // Game data
    int health = 100;
    int maxHealth = 100;
    int team = 0;
    bool isAlive = true;
    int actorId = -1;
    float distance = 0;
    char name[64] = {};
    char weapon[64] = {};
};

// ==================== GameData ====================
class GameData {
public:
    static GameData& getInstance() {
        static GameData instance;
        return instance;
    }

    bool initialized = false;
    uintptr_t il2cppBase = 0;

    // Class pointers
    Il2CppClass* cPlayerController = nullptr;
    Il2CppClass* cPlayerManager = nullptr;
    Il2CppClass* cGameController = nullptr;
    Il2CppClass* cGameManager = nullptr;
    Il2CppClass* cCharacterPlayer = nullptr;
    Il2CppClass* cAimController = nullptr;
    Il2CppClass* cMovementController = nullptr;
    Il2CppClass* cWeaponryController = nullptr;
    Il2CppClass* cWeaponController = nullptr;
    Il2CppClass* cNetworkController = nullptr;
    Il2CppClass* cPhotonNetwork = nullptr;
    Il2CppClass* cCamera = nullptr;
    Il2CppClass* cObject = nullptr;        // UnityEngine.Object
    Il2CppClass* cGameObject = nullptr;
    Il2CppClass* cTransform = nullptr;
    Il2CppClass* cPlayerNameText = nullptr;
    Il2CppClass* cPlayerCondition = nullptr;

    // Field offsets (resolved at runtime)
    int fo_localPlayer = -1;
    int fo_aimCtrl = -1;
    int fo_moveCtrl = -1;
    int fo_weaponry = -1;
    int fo_networkCtrl = -1;
    int fo_mainCamera = -1;
    int fo_bodyTransform = -1;
    int fo_team = -1;
    int fo_alive = -1;
    int fo_alive2 = -1;
    int fo_health = -1;
    int fo_photonView = -1;
    int fo_actorId = -1;
    int fo_maxCtHealth = -1;
    int fo_maxTrHealth = -1;
    int fo_playerCharView = -1;
    int fo_charPlayer = -1; // CharacterPlayer -> PlayerController
    int fo_weaponCtrl = -1; // WeaponryController -> current weapon
    int fo_weaponParams = -1; // WeaponController -> weapon params
    int fo_displayName = -1; // WeaponParameters._displayName

    // Method pointers
    Il2CppMethodInfo* mWorldToScreen = nullptr;
    Il2CppMethodInfo* mFindObjectsOfType = nullptr;
    Il2CppMethodInfo* mGetMain = nullptr;
    Il2CppMethodInfo* mGetPosition = nullptr;
    Il2CppMethodInfo* mGetForward = nullptr;
    Il2CppMethodInfo* mGetEulerAngles = nullptr;
    Il2CppMethodInfo* mSetEulerAngles = nullptr;
    Il2CppMethodInfo* mGetComponent = nullptr;
    Il2CppMethodInfo* mGetWorldToCameraMatrix = nullptr;
    Il2CppMethodInfo* mGetProjectionMatrix = nullptr;
    Il2CppMethodInfo* mGetName = nullptr;
    Il2CppMethodInfo* mGetPlayerList = nullptr;

    // Game state
    void* localPlayer = nullptr;
    void* localController = nullptr;
    void* mainCamera = nullptr;
    MemMatrix4x4 worldToCameraMatrix = {};
    MemMatrix4x4 projectionMatrix = {};
    bool hasMatrices = false;

    std::vector<PlayerData> players;

    void init() {
        if (initialized) return;

        il2cppBase = findLibBase("libil2cpp.so");
        if (!il2cppBase) { LOGE("No libil2cpp.so"); return; }
        LOGI("libil2cpp.so: 0x%lx", il2cppBase);

        if (!initIL2CPP()) return;

        // Find all classes
        auto fc = [](const char* img, const char* ns, const char* n) -> Il2CppClass* {
            return findClass(img, ns, n);
        };

        cPlayerController = fc("Assembly-CSharp", "Axlebolt.Standoff.Player", "PlayerController");
        cPlayerManager = fc("Assembly-CSharp", "Axlebolt.Standoff.Player", "PlayerManager");
        cGameController = fc("Assembly-CSharp", "Axlebolt.Standoff.Game", "GameController");
        cGameManager = fc("Assembly-CSharp", "Axlebolt.Standoff.Game", "GameManager");
        cCharacterPlayer = fc("Assembly-CSharp", "Axlebolt.Standoff.Networking", "CharacterPlayer");
        cAimController = fc("Assembly-CSharp", "Axlebolt.Standoff.Player", "AimController");
        cMovementController = fc("Assembly-CSharp", "Axlebolt.Standoff.Player", "MovementController");
        cWeaponryController = fc("Assembly-CSharp", "Axlebolt.Standoff.Inventory", "WeaponryController");
        cWeaponController = fc("Assembly-CSharp", "Axlebolt.Standoff.Inventory", "WeaponController");
        cNetworkController = fc("Assembly-CSharp", "Axlebolt.Standoff.Player", "NetworkController");
        cPhotonNetwork = fc("PhotonUnityNetworking", "Photon.Pun", "PhotonNetwork");
        cCamera = fc("UnityEngine.CoreModule", "UnityEngine", "Camera");
        cObject = fc("UnityEngine.CoreModule", "UnityEngine", "Object");
        cGameObject = fc("UnityEngine.CoreModule", "UnityEngine", "GameObject");
        cTransform = fc("UnityEngine.CoreModule", "UnityEngine", "Transform");
        cPlayerCondition = fc("Assembly-CSharp", "Axlebolt.Standoff.Missions.Configuration", "PlayerCondition");

        // Fallback class search
        if (!cPlayerController) cPlayerController = fc("Assembly-CSharp", "", "PlayerController");
        if (!cPlayerManager) cPlayerManager = fc("Assembly-CSharp", "", "PlayerManager");
        if (!cCharacterPlayer) cCharacterPlayer = fc("Assembly-CSharp", "", "CharacterPlayer");
        if (!cCamera) cCamera = fc("UnityEngine.dll", "UnityEngine", "Camera");

        LOGI("Classes: PC=%p PM=%p GC=%p CharP=%p Cam=%p Obj=%p",
             cPlayerController, cPlayerManager, cGameController, cCharacterPlayer, cCamera, cObject);

        // Resolve field offsets from IL2CPP
        auto fo = [&](Il2CppClass* k, const char* f) -> int {
            if (!k) return -1;
            int o = getFieldOff(k, f);
            if (o >= 0) LOGI("  %s::%s = 0x%x", "?", f, o);
            return o;
        };

        // PlayerController fields
        if (cPlayerController) {
            fo_aimCtrl = fo(cPlayerController, "<DAFBHDCBBFDBDBD>k__BackingField");
            fo_moveCtrl = fo(cPlayerController, "<DBHAFHFGGFGFECA>k__BackingField");
            fo_weaponry = fo(cPlayerController, "<AECFFADGAGBBHHB>k__BackingField");
            fo_networkCtrl = fo(cPlayerController, "<CEGFDEEGFGDBDBF>k__BackingField");
            fo_mainCamera = fo(cPlayerController, "HDDBDFEHGCFFBEA");
            fo_bodyTransform = fo(cPlayerController, "AEHBEAEHHDBAECC");
            fo_team = fo(cPlayerController, "EAGCGEABDGFGBHC");
            fo_alive = fo(cPlayerController, "EDBGBDAHEAEDFCC");
            fo_alive2 = fo(cPlayerController, "<AFBHCBEGEDFFEBE>k__BackingField");
            fo_health = fo(cPlayerController, "<AEDDGECCHCFBBCH>k__BackingField");
            fo_photonView = fo(cPlayerController, "<HHCEEHGACEBEDFC>k__BackingField");
            fo_actorId = fo(cPlayerController, "<FACDFFBDHEBHAFG>k__BackingField");
            fo_maxCtHealth = fo(cPlayerController, "MaxCtHealth");
            fo_maxTrHealth = fo(cPlayerController, "MaxTrHealth");
            fo_playerCharView = fo(cPlayerController, "FHGACBBGHEEBEEA");
        }

        // CharacterPlayer fields
        if (cCharacterPlayer) {
            fo_charPlayer = fo(cCharacterPlayer, "DHGHGGFDECGFAEE");
        }

        // WeaponryController
        if (cWeaponryController) {
            fo_weaponCtrl = fo(cWeaponryController, "<GBCHHHGABFGDBFD>k__BackingField");
        }

        // PlayerManager
        if (cPlayerManager) {
            fo_localPlayer = fo(cPlayerManager, "<CFCHCBFEEFHBAEH>k__BackingField");
        }

        // Resolve method pointers
        if (cCamera) {
            mWorldToScreen = getMethod(cCamera, "WorldToScreenPoint", 1);
            mGetMain = getMethod(cCamera, "get_main", 0);
            mGetWorldToCameraMatrix = getMethod(cCamera, "get_worldToCameraMatrix", 0);
            mGetProjectionMatrix = getMethod(cCamera, "get_projectionMatrix", 0);
            mGetPosition = getMethod(cCamera, "get_position", 0);
            mGetEulerAngles = getMethod(cCamera, "get_eulerAngles", 0);
            LOGI("Camera methods: W2S=%p main=%p w2cMat=%p projMat=%p",
                 mWorldToScreen, mGetMain, mGetWorldToCameraMatrix, mGetProjectionMatrix);
        }

        if (cTransform) {
            mGetPosition = getMethod(cTransform, "get_position", 0);
            mGetForward = getMethod(cTransform, "get_forward", 0);
            mGetEulerAngles = getMethod(cTransform, "get_eulerAngles", 0);
            mSetEulerAngles = getMethod(cTransform, "set_eulerAngles", 1);
            LOGI("Transform methods: pos=%p fwd=%p euler=%p setEuler=%p",
                 mGetPosition, mGetForward, mGetEulerAngles, mSetEulerAngles);
        }

        // Object.FindObjectsOfType
        if (cObject) {
            mFindObjectsOfType = getMethod(cObject, "FindObjectsOfType", 1);
            if (!mFindObjectsOfType)
                mFindObjectsOfType = getMethod(cObject, "FindObjectsByType", 2);
            LOGI("FindObjectsOfType: %p", mFindObjectsOfType);
        }

        // PhotonNetwork
        if (cPhotonNetwork) {
            mGetPlayerList = getMethod(cPhotonNetwork, "get_playerList", 0);
            LOGI("PhotonNetwork.playerList: %p", mGetPlayerList);
        }

        initialized = true;
        LOGI("GameData initialized successfully");

        // Get camera immediately
        updateCamera();
    }

    // ---- Camera ----
    void updateCamera() {
        if (!cCamera || !mGetMain) return;

        // Camera.main
        void* camClass[2] = { (void*)cCamera, nullptr };
        void* args[1] = { nullptr };

        // IL2CPP static method call: invoke get_main
        if (mGetMain->methodPointer) {
            auto fn = (void* (*)())mGetMain->methodPointer;
            mainCamera = fn();
            if (mainCamera) {
                LOGI("Camera.main = %p", mainCamera);
                readCameraMatrices();
            }
        }
    }

    void readCameraMatrices() {
        if (!mainCamera) return;

        // Read worldToCameraMatrix
        if (mGetWorldToCameraMatrix && mGetWorldToCameraMatrix->methodPointer) {
            auto fn = (MemMatrix4x4 (*)(void*))mGetWorldToCameraMatrix->methodPointer;
            worldToCameraMatrix = fn(mainCamera);
        }

        // Read projectionMatrix
        if (mGetProjectionMatrix && mGetProjectionMatrix->methodPointer) {
            auto fn = (MemMatrix4x4 (*)(void*))mGetProjectionMatrix->methodPointer;
            projectionMatrix = fn(mainCamera);
        }

        hasMatrices = (worldToCameraMatrix.m[0] != 0 || worldToCameraMatrix.m[5] != 0);
    }

    // ---- WorldToScreen via Camera method ----
    bool worldToScreen(float wx, float wy, float wz, float& sx, float& sy) {
        if (!mainCamera || !mWorldToScreen || !mWorldToScreen->methodPointer) return false;

        // Camera.WorldToScreenPoint(Vector3 position)
        // In IL2CPP: Vector3 is passed by value on stack
        struct Vec3Arg { float x, y, z; };

        Vec3Arg pos = { wx, wy, wz };
        MemScreenPoint result = {};

        auto fn = (MemScreenPoint (*)(void*, Vec3Arg))mWorldToScreen->methodPointer;
        result = fn(mainCamera, pos);

        if (result.z < 0) return false; // Behind camera

        sx = result.x;
        sy = result.y;
        return true;
    }

    // ---- Transform.GetPosition ----
    bool getTransformPosition(void* transform, float& x, float& y, float& z) {
        if (!transform || !mGetPosition || !mGetPosition->methodPointer) return false;

        auto fn = (MemVector3 (*)(void*))mGetPosition->methodPointer;
        MemVector3 pos = fn(transform);
        x = pos.x; y = pos.y; z = pos.z;
        return true;
    }

    // ---- Transform.SetEulerAngles ----
    void setTransformEulerAngles(void* transform, float x, float y, float z) {
        if (!transform || !mSetEulerAngles || !mSetEulerAngles->methodPointer) return;
        struct Vec3Arg { float x, y, z; };
        auto fn = (void (*)(void*, Vec3Arg))mSetEulerAngles->methodPointer;
        fn(transform, {x, y, z});
    }

    // ---- Local Player ----
    void updateLocalPlayer() {
        localPlayer = nullptr;
        localController = nullptr;

        if (!cPlayerManager || fo_localPlayer < 0) return;

        // Get static PlayerManager instance
        void* staticData = il2cpp.class_get_static_field_data ?
                           il2cpp.class_get_static_field_data(cPlayerManager) : nullptr;
        if (!staticData) return;

        void* lp = getPtr(staticData, fo_localPlayer);
        if (!lp) return;

        localController = lp;
        LOGI("Local player: %p", lp);
    }

    // ---- Scene Scan ----
    void updatePlayers() {
        players.clear();
        if (!localController) return;

        // Get local team
        int localTeam = getInt(localController, fo_team);

        // Get local player transform for distance calc
        void* localTransform = getPtr(localController, fo_bodyTransform);
        float lpx = 0, lpy = 0, lpz = 0;
        if (localTransform) getTransformPosition(localTransform, lpx, lpy, lpz);

        // Try to find all PlayerController via CharacterPlayer
        // CharacterPlayer has PlayerController at 0x50
        // We'll scan the object list or use FindObjectsOfType

        // Alternative: iterate PhotonView objects to find players
        // For now, use a direct memory scan approach

        // Scan for PlayerController objects by looking for CharacterPlayer components
        // In Unity, we can use Object.FindObjectsOfType<CharacterPlayer>()
        if (mFindObjectsOfType && mFindObjectsOfType->methodPointer && cCharacterPlayer) {
            // FindObjectsOfType(Il2CppRuntimeType type, FindObjectsSortMode sortMode)
            // We need the RuntimeType for CharacterPlayer
            // This requires il2cpp_class_get_type and creating a RuntimeType

            // Simplified: just iterate a known range
            // Actually, let's try a different approach - read from known singleton locations
        }

        // Approach: iterate through the GameController's player list
        // GameController at 0x2C0 has local PlayerController
        // Or we can scan the scene hierarchy

        // For now, let's find players through the PhotonView network
        // We know PlayerController objects exist in memory - scan for them
        // by reading the game's player array

        // Simple approach: find CharacterPlayer objects by scanning libil2cpp metadata
        // and then reading their PlayerController reference

        // Most reliable: use the game's own player tracking
        // PlayerManager stores local player, and the game has a list of all players
        // Let's try to find the list through the GameManager

        // We'll collect players from multiple sources
        collectPlayersFromPhoton();
    }

    void collectPlayersFromPhoton() {
        // Try to find all active CharacterPlayer objects
        // In Unity, CharacterPlayer extends MonoBehaviour which has a cached GameObject pointer
        // We can find them by scanning memory for pointers to CharacterPlayer vtable

        // Alternative simpler approach: the game keeps a player list somewhere
        // Let's check if we can get other players from PhotonNetwork

        // For now, use hardcoded player count from the scene
        // TODO: Implement proper scene scanning

        // Read player count from the game state
        // The actual implementation would walk the scene tree
        // For now, we mark the system as ready and the ESP will show data when available
    }

    // ---- Read Player Data ----
    void readPlayerData(PlayerData& p) {
        if (!p.playerController) return;

        p.valid = true;

        // Team
        if (fo_team >= 0) p.team = getInt(p.playerController, fo_team);

        // Alive
        if (fo_alive >= 0) p.isAlive = getBool(p.playerController, fo_alive);

        // Health (approximate from max health values)
        if (fo_maxCtHealth >= 0 && fo_maxTrHealth >= 0) {
            int maxHP = (p.team == 1) ? getInt(p.playerController, fo_maxCtHealth) :
                        getInt(p.playerController, fo_maxTrHealth);
            p.maxHealth = maxHP > 0 ? maxHP : 100;
        }

        // Actor ID
        if (fo_actorId >= 0) p.actorId = getInt(p.playerController, fo_actorId);

        // Position from body transform
        void* transform = getPtr(p.playerController, fo_bodyTransform);
        if (transform) {
            getTransformPosition(transform, p.posX, p.posY, p.posZ);

            // Head position (approximate: body Y + 1.7m for standing)
            p.headX = p.posX;
            p.headY = p.posY + 1.7f;
            p.headZ = p.posZ;
        }

        // Distance from local player
        void* localTransform = localController ? getPtr(localController, fo_bodyTransform) : nullptr;
        if (localTransform) {
            float lx, ly, lz;
            if (getTransformPosition(localTransform, lx, ly, lz)) {
                float dx = p.posX - lx;
                float dy = p.posY - ly;
                float dz = p.posZ - lz;
                p.distance = sqrtf(dx*dx + dy*dy + dz*dz);
            }
        }

        // WorldToScreen for body and head
        if (mainCamera) {
            p.onScreen = worldToScreen(p.posX, p.posY, p.posZ, p.screenX, p.screenY);
            worldToScreen(p.headX, p.headY, p.headZ, p.headScreenX, p.headScreenY);

            // Calculate box size from screen distance
            if (p.distance > 0) {
                p.boxW = 40.0f / p.distance * g_ScreenW * 0.005f;
                p.boxH = (p.headScreenY - p.screenY);
                if (p.boxH < 10) p.boxH = 50;
            }
        }

        // Weapon info
        if (fo_weaponry >= 0) {
            void* weaponry = getPtr(p.playerController, fo_weaponry);
            if (weaponry && fo_weaponCtrl >= 0) {
                void* weapon = getPtr(weaponry, fo_weaponCtrl);
                if (weapon) {
                    snprintf(p.weapon, sizeof(p.weapon), "Weapon");
                }
            }
        }
    }

    // ---- Aimbot ----
    void aimAtTarget(float tx, float ty, float tz, float smooth) {
        if (!localController || !mGetEulerAngles || !mSetEulerAngles) return;

        void* transform = getPtr(localController, fo_bodyTransform);
        if (!transform) return;

        // Get local position
        float lx, ly, lz;
        if (!getTransformPosition(transform, lx, ly, lz)) return;

        // Calculate direction
        float dx = tx - lx;
        float dy = ty - ly;
        float dz = tz - lz;

        // Calculate yaw and pitch
        float yaw = atan2f(dx, dz) * 180.0f / M_PI;
        float hyp = sqrtf(dx*dx + dz*dz);
        float pitch = -atan2f(dy, hyp) * 180.0f / M_PI;

        // Get current angles
        auto getEuler = (MemVector3 (*)(void*))mGetEulerAngles->methodPointer;
        MemVector3 cur = getEuler(transform);

        // Smooth interpolation
        float nyaw = cur.x + (yaw - cur.x) / smooth;
        float npitch = cur.y + (pitch - cur.y) / smooth;

        // Set new angles
        setTransformEulerAngles(transform, nyaw, npitch, 0);
    }
};

// ==================== Screen dimensions ====================
// Declared at top of file
