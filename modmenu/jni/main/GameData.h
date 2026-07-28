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

// ==================== IL2CPP Forward Declarations ====================
struct Il2CppImage;
struct Il2CppAssembly;
struct Il2CppDomain;
struct Il2CppClass;
struct Il2CppObject;
struct Il2CppFieldInfo;
struct Il2CppMethodInfo;
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
    Il2CppMethodInfo* (*class_get_method_from_name)(Il2CppClass*, const char*, int) = nullptr;
    void* (*class_get_static_field_data)(Il2CppClass*) = nullptr;
    bool loaded = false;
};

static IL2CPP_API il2cpp;

static bool initIL2CPP() {
    if (il2cpp.loaded) return true;
    il2cpp.lib = dlopen("libil2cpp.so", RTLD_LAZY | RTLD_NOLOAD);
    if (!il2cpp.lib) { LOGE("No libil2cpp.so"); return false; }
    #define L(name) il2cpp.name = (decltype(il2cpp.name))dlsym(il2cpp.lib, "il2cpp_" #name)
    L(domain_get); L(domain_get_assemblies); L(assembly_get_image); L(image_get_name);
    L(class_from_name); L(class_get_field_from_name); L(field_get_offset);
    L(class_get_method_from_name); L(class_get_static_field_data);
    #undef L
    if (!il2cpp.domain_get || !il2cpp.class_from_name) { LOGE("IL2CPP API incomplete"); return false; }
    il2cpp.loaded = true;
    LOGI("IL2CPP loaded: %p", il2cpp.lib);
    return true;
}

// ==================== IL2CPP Helpers ====================
static Il2CppClass* findClass(const char* img, const char* ns, const char* name) {
    const Il2CppDomain* d = il2cpp.domain_get();
    if (!d) return nullptr;
    void* asms[128]; uint32_t cnt = 0;
    il2cpp.domain_get_assemblies(d, asms, &cnt);
    for (uint32_t i = 0; i < cnt; i++) {
        const Il2CppImage* im = il2cpp.assembly_get_image((const Il2CppAssembly*)asms[i]);
        if (!im) continue;
        const char* n = il2cpp.image_get_name(im);
        if (n && strstr(n, img)) {
            Il2CppClass* k = il2cpp.class_from_name(im, ns, name);
            if (k) return k;
        }
    }
    return nullptr;
}

static int getFieldOff(Il2CppClass* k, const char* n) {
    if (!k) return -1;
    Il2CppFieldInfo* f = il2cpp.class_get_field_from_name(k, n);
    if (!f) return -1;
    return il2cpp.field_get_offset ? il2cpp.field_get_offset(f) : f->offset;
}

static Il2CppMethodInfo* getMethod(Il2CppClass* k, const char* n, int p = 0) {
    return k ? il2cpp.class_get_method_from_name(k, n, p) : nullptr;
}

static void* getPtr(void* o, int off) { return (o && off >= 0) ? *(void**)((uint8_t*)o + off) : nullptr; }
static bool getBool(void* o, int off) { return (o && off >= 0) ? *(bool*)((uint8_t*)o + off) : false; }
static int getInt(void* o, int off) { return (o && off >= 0) ? *(int*)((uint8_t*)o + off) : 0; }
static float getFloat(void* o, int off) { return (o && off >= 0) ? *(float*)((uint8_t*)o + off) : 0.f; }

// ==================== Unity Types in Memory ====================
struct MemVec3 { void* pad[2]; float x, y, z; };
struct Mat4 { void* pad[2]; float m[16]; };
struct ScreenPt { float x, y, z; };

struct Il2CppArray {
    void* klass;
    void* monitor;
    int32_t max_length;
    uint32_t _padding;
    void* elements[1];
};

// Il2CppString: klass(8) + monitor(8) + length(4) + padding(4) + chars[length*2+1]
struct Il2CppString {
    void* klass;
    void* monitor;
    int32_t length;
    uint32_t _padding;
    char16_t chars[1];
};

static std::string readIl2CppString(void* obj, int offset) {
    if (!obj) return "";
    Il2CppString* s = (Il2CppString*)((uint8_t*)obj + offset);
    if (s->length <= 0 || s->length > 128) return "";
    char buf[256] = {};
    for (int i = 0; i < s->length && i < 127; i++)
        buf[i] = (char)s->chars[i];
    return std::string(buf);
}

// ==================== Memory Scanner ====================
struct MemRegion {
    uintptr_t start, end;
    bool read, write, exec;
};

static std::vector<MemRegion> parseMaps() {
    std::vector<MemRegion> regions;
    FILE* fp = fopen("/proc/self/maps", "r");
    if (!fp) return regions;
    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        MemRegion r = {};
        char perms[5];
        if (sscanf(line, "%lx-%lx %4s", &r.start, &r.end, perms) != 3) continue;
        r.read = perms[0] == 'r';
        r.write = perms[1] == 'w';
        r.exec = perms[2] == 'x';
        // Only scan writable non-executable regions (heap)
        if (r.read && r.write && !r.exec && (r.end - r.start) > 0x10000)
            regions.push_back(r);
    }
    fclose(fp);
    return regions;
}

// Scan memory for pointers to a specific Il2CppClass
// Returns list of valid object pointers
static std::vector<void*> findObjectsOfClass(Il2CppClass* targetClass) {
    std::vector<void*> result;
    if (!targetClass) return result;

    std::vector<MemRegion> regions = parseMaps();
    LOGI("Scanning %zu regions for class %p", regions.size(), targetClass);

    for (auto& reg : regions) {
        uintptr_t start = reg.start;
        uintptr_t end = reg.end;
        uint8_t* ptr = (uint8_t*)start;

        // Scan in 8-byte steps (pointer-aligned)
        for (uintptr_t addr = start; addr < end - 8; addr += 8) {
            void* val = *(void**)addr;
            if (val == targetClass) {
                // Found a pointer to our class - this is likely an object header
                // Object layout: [class_ptr(8)][monitor(8)][fields...]
                // The object itself is at 'addr'
                void* obj = (void*)addr;

                // Validate: check if the memory around it looks like a valid object
                // Read a few fields and check if they're reasonable
                // For MonoBehaviour: first field after header should be valid pointer or 0
                uintptr_t objAddr = addr;

                // Basic sanity: address should be in valid range
                if (objAddr > 0x10000 && objAddr < 0x7FFFFFFFFFFF) {
                    result.push_back(obj);
                }
            }
        }
    }

    LOGI("Found %zu objects for class %p", result.size(), targetClass);
    return result;
}

// ==================== PlayerData ====================
struct PlayerData {
    void* playerController = nullptr;
    void* characterPlayer = nullptr;
    bool valid = false;
    bool isLocal = false;
    float posX = 0, posY = 0, posZ = 0;
    float headX = 0, headY = 0, headZ = 0;
    float screenX = 0, screenY = 0;
    float headScreenX = 0, headScreenY = 0;
    float boxW = 0, boxH = 0;
    bool onScreen = false;
    int health = 100, maxHealth = 100;
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
    static GameData& getInstance() { static GameData i; return i; }
    bool initialized = false;

    // Classes
    Il2CppClass *cPC = nullptr, *cPM = nullptr, *cGC = nullptr, *cGM = nullptr;
    Il2CppClass *cCharP = nullptr, *cAim = nullptr, *cMove = nullptr;
    Il2CppClass *cWeapCtrl = nullptr, *cWeapon = nullptr, *cNet = nullptr;
    Il2CppClass *cPN = nullptr, *cNetPeer = nullptr, *cPP = nullptr;
    Il2CppClass *cCam = nullptr, *cObj = nullptr, *cTrans = nullptr;
    Il2CppClass *cWeaponParams = nullptr, *cPV = nullptr;
    Il2CppClass *cPCond = nullptr, *cFloatVal = nullptr;

    // Field offsets - PlayerController
    int pc_aim = -1, pc_move = -1, pc_weap = -1, pc_net = -1;
    int pc_cam = -1, pc_transform = -1, pc_team = -1;
    int pc_alive = -1, pc_alive2 = -1, pc_health = -1;
    int pc_photonView = -1, pc_actorId = -1;
    int pc_maxCT = -1, pc_maxTR = -1;

    // Field offsets - NetworkingPeer
    int np_mActors = -1, np_mPlayerListCopy = -1;

    // Field offsets - PhotonPlayer
    int pp_actorID = -1, pp_nameField = -1, pp_isLocal = -1;

    // Field offsets - CharacterPlayer
    int cp_pc = -1;

    // Field offsets - WeaponryController
    int wc_weaponCtrl = -1;

    // Field offsets - WeaponController
    int wpc_params = -1;

    // Field offsets - WeaponParameters
    int wp_displayName = -1;

    // Field offsets - PlayerManager
    int pm_localPlayer = -1;

    // Field offsets - PhotonView
    int pv_ownerId = -1;

    // Methods
    Il2CppMethodInfo *mW2S = nullptr, *mMain = nullptr;
    Il2CppMethodInfo *mPos = nullptr, *mEuler = nullptr, *mSetEuler = nullptr;
    Il2CppMethodInfo *mW2CMat = nullptr, *mProjMat = nullptr;

    // State
    void* localController = nullptr;
    void* mainCamera = nullptr;
    Mat4 w2cMat = {}, projMat = {};
    bool hasMat = false;
    std::vector<PlayerData> players;

    // Cached scan results (refresh every N frames)
    std::vector<void*> cachedCharacterPlayers;
    int scanCooldown = 0;

    void init() {
        if (initialized) return;
        if (!initIL2CPP()) return;

        auto fc = [](const char* i, const char* n, const char* c) { return findClass(i, n, c); };
        cPC = fc("Assembly-CSharp","Axlebolt.Standoff.Player","PlayerController");
        cPM = fc("Assembly-CSharp","Axlebolt.Standoff.Player","PlayerManager");
        cGC = fc("Assembly-CSharp","Axlebolt.Standoff.Game","GameController");
        cCharP = fc("Assembly-CSharp","Axlebolt.Standoff.Networking","CharacterPlayer");
        cWeapCtrl = fc("Assembly-CSharp","Axlebolt.Standoff.Inventory","WeaponryController");
        cWeapon = fc("Assembly-CSharp","Axlebolt.Standoff.Inventory","WeaponController");
        cPN = fc("PhotonUnityNetworking","Photon.Pun","PhotonNetwork");
        cNetPeer = fc("PhotonUnityNetworking","Photon.Pun","NetworkingPeer");
        cPP = fc("PhotonUnityNetworking","ExitGames.Client.Photon","PhotonPlayer");
        cCam = fc("UnityEngine.CoreModule","UnityEngine","Camera");
        cTrans = fc("UnityEngine.CoreModule","UnityEngine","Transform");
        cWeaponParams = fc("Assembly-CSharp","Axlebolt.Standoff.Inventory","WeaponParameters");
        cPV = fc("PhotonUnityNetworking","Photon.Pun","PhotonView");
        cPCond = fc("Assembly-CSharp","Axlebolt.Standoff.Missions.Configuration","PlayerCondition");
        cFloatVal = fc("Assembly-CSharp","Axlebolt.Standoff.Missions.Configuration","FloatValue");

        if (!cPC) cPC = fc("Assembly-CSharp","","PlayerController");
        if (!cPM) cPM = fc("Assembly-CSharp","","PlayerManager");
        if (!cCharP) cCharP = fc("Assembly-CSharp","","CharacterPlayer");
        if (!cPN) cPN = fc("PhotonUnityNetworking","","PhotonNetwork");
        if (!cNetPeer) cNetPeer = fc("PhotonUnityNetworking","","NetworkingPeer");
        if (!cPP) cPP = fc("PhotonUnityNetworking","","PhotonPlayer");
        if (!cPV) cPV = fc("PhotonUnityNetworking","","PhotonView");

        LOGI("Classes: PC=%p CP=%p PN=%p NP=%p PP=%p PV=%p", cPC, cCharP, cPN, cNetPeer, cPP, cPV);

        auto fo = [&](Il2CppClass* k, const char* f) -> int {
            int o = getFieldOff(k, f); return o;
        };

        if (cPC) {
            pc_aim = fo(cPC,"<DAFBHDCBBFDBDBD>k__BackingField");
            pc_move = fo(cPC,"<DBHAFHFGGFGFECA>k__BackingField");
            pc_weap = fo(cPC,"<AECFFADGAGBBHHB>k__BackingField");
            pc_net = fo(cPC,"<CEGFDEEGFGDBDBF>k__BackingField");
            pc_cam = fo(cPC,"HDDBDFEHGCFFBEA");
            pc_transform = fo(cPC,"AEHBEAEHHDBAECC");
            pc_team = fo(cPC,"EAGCGEABDGFGBHC");
            pc_alive = fo(cPC,"EDBGBDAHEAEDFCC");
            pc_alive2 = fo(cPC,"<AFBHCBEGEDFFEBE>k__BackingField");
            pc_health = fo(cPC,"<AEDDGECCHCFBBCH>k__BackingField");
            pc_photonView = fo(cPC,"<HHCEEHGACEBEDFC>k__BackingField");
            pc_actorId = fo(cPC,"<FACDFFBDHEBHAFG>k__BackingField");
            pc_maxCT = fo(cPC,"MaxCtHealth");
            pc_maxTR = fo(cPC,"MaxTrHealth");
        }
        if (cCharP) cp_pc = fo(cCharP,"DHGHGGFDECGFAEE");
        if (cWeapCtrl) wc_weaponCtrl = fo(cWeapCtrl,"<GBCHHHGABFGDBFD>k__BackingField");
        if (cWeapon) wpc_params = fo(cWeapon,"<BAFFFEGDBGEECDA>k__BackingField");
        if (cWeaponParams) wp_displayName = fo(cWeaponParams,"_displayName");
        if (cPM) pm_localPlayer = fo(cPM,"<CFCHCBFEEFHBAEH>k__BackingField");
        if (cNetPeer) {
            np_mActors = fo(cNetPeer,"mActors");
            np_mPlayerListCopy = fo(cNetPeer,"mPlayerListCopy");
        }
        if (cPP) {
            pp_actorID = fo(cPP,"actorID");
            pp_nameField = fo(cPP,"nameField");
            pp_isLocal = fo(cPP,"IsLocal");
        }
        if (cPV) {
            pv_ownerId = fo(cPV,"ownerId");
            LOGI("PhotonView.ownerId offset: 0x%x", pv_ownerId);
        }

        if (cCam) {
            mW2S = getMethod(cCam,"WorldToScreenPoint",1);
            mMain = getMethod(cCam,"get_main",0);
            mW2CMat = getMethod(cCam,"get_worldToCameraMatrix",0);
            mProjMat = getMethod(cCam,"get_projectionMatrix",0);
        }
        if (cTrans) {
            mPos = getMethod(cTrans,"get_position",0);
            mEuler = getMethod(cTrans,"get_eulerAngles",0);
            mSetEuler = getMethod(cTrans,"set_eulerAngles",1);
        }

        initialized = true;
        LOGI("GameData initialized");
        updateCamera();
    }

    // ---- Camera ----
    void updateCamera() {
        if (!mMain || !mMain->methodPointer) return;
        mainCamera = ((void* (*)())mMain->methodPointer)();
        if (mainCamera) readCameraMatrices();
    }

    void readCameraMatrices() {
        if (!mainCamera) return;
        if (mW2CMat && mW2CMat->methodPointer)
            w2cMat = ((Mat4(*)(void*))mW2CMat->methodPointer)(mainCamera);
        if (mProjMat && mProjMat->methodPointer)
            projMat = ((Mat4(*)(void*))mProjMat->methodPointer)(mainCamera);
        hasMat = (w2cMat.m[0] != 0 || w2cMat.m[5] != 0);
    }

    bool worldToScreen(float wx, float wy, float wz, float& sx, float& sy) {
        if (!mainCamera || !mW2S || !mW2S->methodPointer) return false;
        struct V3 { float x,y,z; };
        ScreenPt r = ((ScreenPt(*)(void*,V3))mW2S->methodPointer)(mainCamera, {wx,wy,wz});
        if (r.z < 0) return false;
        sx = r.x; sy = r.y;
        return true;
    }

    bool getTransformPos(void* t, float& x, float& y, float& z) {
        if (!t || !mPos || !mPos->methodPointer) return false;
        MemVec3 p = ((MemVec3(*)(void*))mPos->methodPointer)(t);
        x = p.x; y = p.y; z = p.z;
        return true;
    }

    void setTransformEuler(void* t, float x, float y, float z) {
        if (!t || !mSetEuler || !mSetEuler->methodPointer) return;
        ((void(*)(void*,float,float,float))mSetEuler->methodPointer)(t, x, y, z);
    }

    // ---- NetworkingPeer ----
    void* getNetworkingPeer() {
        if (!cPN) return nullptr;
        void* sd = il2cpp.class_get_static_field_data ? il2cpp.class_get_static_field_data(cPN) : nullptr;
        if (!sd) return nullptr;
        // networkingPeer is an instance field of the static class, at offset from static field data
        // From dump.cs: PhotonNetwork.networkingPeer is at class offset 0x18
        // In static field data, instance fields of a static class are stored sequentially
        return getPtr(sd, 0);
    }

    // ---- Read PhotonPlayer[] ----
    std::vector<void*> readPPArray(void* arrObj) {
        std::vector<void*> r;
        if (!arrObj) return r;
        Il2CppArray* a = (Il2CppArray*)arrObj;
        int cnt = a->max_length;
        if (cnt <= 0 || cnt > 64) return r;
        for (int i = 0; i < cnt; i++)
            if (a->elements[i]) r.push_back(a->elements[i]);
        return r;
    }

    // ---- Local Player ----
    void updateLocalPlayer() {
        localController = nullptr;
        if (cPM && pm_localPlayer >= 0) {
            void* sd = il2cpp.class_get_static_field_data ?
                       il2cpp.class_get_static_field_data(cPM) : nullptr;
            if (sd) localController = getPtr(sd, pm_localPlayer);
        }
        if (localController) LOGI("Local: %p", localController);
    }

    // ---- Find PlayerController by actor ID ----
    void* findPCByActorId(int actorId) {
        if (!cCharP || !cp_pc || !cPV || pv_ownerId < 0) return nullptr;

        // Refresh scan cache periodically
        if (scanCooldown <= 0 || cachedCharacterPlayers.empty()) {
            cachedCharacterPlayers = findObjectsOfClass(cCharP);
            scanCooldown = 300; // refresh every ~5 seconds at 60fps
        }
        scanCooldown--;

        for (void* cp : cachedCharacterPlayers) {
            if (!cp) continue;
            void* pc = getPtr(cp, cp_pc);
            if (!pc) continue;

            // Read PhotonView from PlayerController
            void* pv = getPtr(pc, pc_photonView);
            if (!pv) continue;

            // Read ownerId from PhotonView
            int oid = getInt(pv, pv_ownerId);
            if (oid == actorId) return pc;
        }
        return nullptr;
    }

    // ---- Scene Scan via Photon ----
    void updatePlayers() {
        players.clear();
        if (!localController) return;

        int localTeam = getInt(localController, pc_team);
        float lpx = 0, lpy = 0, lpz = 0;
        void* lt = getPtr(localController, pc_transform);
        if (lt) getTransformPos(lt, lpx, lpy, lpz);

        // Get PhotonNetwork static fields
        void* pnSD = il2cpp.class_get_static_field_data ?
                     il2cpp.class_get_static_field_data(cPN) : nullptr;
        if (!pnSD) return;

        // PhotonNetwork.networkingPeer is at offset 0x18 in static field data
        void* peer = getPtr(pnSD, 0);
        if (!peer) {
            LOGI("No peer");
            return;
        }

        // Read mPlayerListCopy from NetworkingPeer
        void* ppArray = nullptr;
        if (np_mPlayerListCopy >= 0)
            ppArray = getPtr(peer, np_mPlayerListCopy);

        if (!ppArray) {
            LOGI("No mPlayerListCopy");
            return;
        }

        std::vector<void*> ppList = readPPArray(ppArray);
        LOGI("PhotonPlayers: %zu", ppList.size());

        for (void* pp : ppList) {
            if (!pp) continue;
            int actorId = getInt(pp, pp_actorID);
            bool isLocal = getBool(pp, pp_isLocal);
            if (actorId <= 0) continue;

            PlayerData pd;
            pd.actorId = actorId;
            pd.isLocal = isLocal;

            // Read name
            std::string nm = readIl2CppString(pp, pp_nameField);
            if (!nm.empty()) strncpy(pd.name, nm.c_str(), sizeof(pd.name) - 1);

            // Find matching PlayerController
            void* pc = findPCByActorId(actorId);
            if (pc) {
                pd.playerController = pc;
                pd.valid = true;

                // Read all data
                if (pc_team >= 0) pd.team = getInt(pc, pc_team);
                if (pc_alive >= 0) pd.isAlive = getBool(pc, pc_alive);
                if (pc_actorId >= 0) pd.actorId = getInt(pc, pc_actorId);

                // Health from max values
                if (pc_maxCT >= 0 && pc_maxTR >= 0) {
                    int mh = (pd.team == 1) ? getInt(pc, pc_maxCT) : getInt(pc, pc_maxTR);
                    pd.maxHealth = mh > 0 ? mh : 100;
                }

                // Position
                void* tr = getPtr(pc, pc_transform);
                if (tr) {
                    getTransformPos(tr, pd.posX, pd.posY, pd.posZ);
                    pd.headX = pd.posX; pd.headY = pd.posY + 1.7f; pd.headZ = pd.posZ;
                }

                // Distance
                float dx = pd.posX - lpx, dy = pd.posY - lpy, dz = pd.posZ - lpz;
                pd.distance = sqrtf(dx*dx + dy*dy + dz*dz);

                // WorldToScreen
                if (mainCamera) {
                    pd.onScreen = worldToScreen(pd.posX, pd.posY, pd.posZ, pd.screenX, pd.screenY);
                    worldToScreen(pd.headX, pd.headY, pd.headZ, pd.headScreenX, pd.headScreenY);
                    if (pd.distance > 0) {
                        pd.boxW = 40.f / pd.distance * g_ScreenW * 0.005f;
                        pd.boxH = pd.headScreenY - pd.screenY;
                        if (pd.boxH < 10) pd.boxH = 50;
                    }
                }

                // Weapon
                if (pc_weap >= 0) {
                    void* wc = getPtr(pc, pc_weap);
                    if (wc && wc_weaponCtrl >= 0) {
                        void* w = getPtr(wc, wc_weaponCtrl);
                        if (w && wpc_params >= 0) {
                            void* wp = getPtr(w, wpc_params);
                            std::string wn = readIl2CppString(wp, wp_displayName);
                            if (!wn.empty()) strncpy(pd.weapon, wn.c_str(), sizeof(pd.weapon) - 1);
                        }
                    }
                }
            }

            if (!isLocal) players.push_back(pd);
        }
        LOGI("Players: %zu", players.size());
    }

    // ---- Aimbot ----
    void aimAtTarget(float tx, float ty, float tz, float smooth) {
        if (!localController || !mEuler || !mSetEuler) return;
        void* tr = getPtr(localController, pc_transform);
        if (!tr) return;
        float lx, ly, lz;
        if (!getTransformPos(tr, lx, ly, lz)) return;
        float dx = tx - lx, dy = ty - ly, dz = tz - lz;
        float yaw = atan2f(dx, dz) * 180.f / M_PI;
        float hyp = sqrtf(dx*dx + dz*dz);
        float pitch = -atan2f(dy, hyp) * 180.f / M_PI;
        MemVec3 cur = ((MemVec3(*)(void*))mEuler->methodPointer)(tr);
        float ny = cur.x + (yaw - cur.x) / smooth;
        float np2 = cur.y + (pitch - cur.y) / smooth;
        setTransformEuler(tr, ny, np2, 0);
    }
};
