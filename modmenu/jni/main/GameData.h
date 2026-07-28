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

// Il2CppArray layout: klass(8) + monitor(8) + max_length(4) + padding(4) + elements[]
struct Il2CppArray {
    void* klass;
    void* monitor;
    int32_t max_length;
    uint32_t _padding;
    void* elements[1]; // variable length
};

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
    Il2CppClass *cWeaponParams = nullptr;

    // Field offsets - PlayerController
    int pc_aim = -1, pc_move = -1, pc_weap = -1, pc_net = -1;
    int pc_cam = -1, pc_transform = -1, pc_team = -1;
    int pc_alive = -1, pc_alive2 = -1, pc_health = -1;
    int pc_photonView = -1, pc_actorId = -1;
    int pc_maxCT = -1, pc_maxTR = -1;
    int pc_charView = -1;

    // Field offsets - NetworkingPeer
    int np_localPlayer = -1;
    int np_mActors = -1;
    int np_mPlayerListCopy = -1;
    int np_mOtherPlayerListCopy = -1;

    // Field offsets - PhotonPlayer
    int pp_actorID = -1;
    int pp_nameField = -1;
    int pp_isLocal = -1;

    // Field offsets - CharacterPlayer
    int cp_playerController = -1;

    // Field offsets - WeaponryController
    int wc_weaponCtrl = -1;

    // Field offsets - WeaponController
    int wpc_params = -1;

    // Field offsets - WeaponParameters
    int wp_displayName = -1;

    // Field offsets - PlayerManager
    int pm_localPlayer = -1;

    // Methods
    Il2CppMethodInfo *mW2S = nullptr, *mMain = nullptr;
    Il2CppMethodInfo *mPos = nullptr, *mEuler = nullptr, *mSetEuler = nullptr;
    Il2CppMethodInfo *mW2CMat = nullptr, *mProjMat = nullptr;
    Il2CppMethodInfo *mGetPlayerList = nullptr;

    // State
    void* localController = nullptr;
    void* mainCamera = nullptr;
    Mat4 w2cMat = {}, projMat = {};
    bool hasMat = false;
    std::vector<PlayerData> players;

    void init() {
        if (initialized) return;
        if (!initIL2CPP()) return;

        // Find classes
        auto fc = [](const char* i, const char* n, const char* c) { return findClass(i, n, c); };
        cPC = fc("Assembly-CSharp","Axlebolt.Standoff.Player","PlayerController");
        cPM = fc("Assembly-CSharp","Axlebolt.Standoff.Player","PlayerManager");
        cGC = fc("Assembly-CSharp","Axlebolt.Standoff.Game","GameController");
        cGM = fc("Assembly-CSharp","Axlebolt.Standoff.Game","GameManager");
        cCharP = fc("Assembly-CSharp","Axlebolt.Standoff.Networking","CharacterPlayer");
        cAim = fc("Assembly-CSharp","Axlebolt.Standoff.Player","AimController");
        cMove = fc("Assembly-CSharp","Axlebolt.Standoff.Player","MovementController");
        cWeapCtrl = fc("Assembly-CSharp","Axlebolt.Standoff.Inventory","WeaponryController");
        cWeapon = fc("Assembly-CSharp","Axlebolt.Standoff.Inventory","WeaponController");
        cNet = fc("Assembly-CSharp","Axlebolt.Standoff.Player","NetworkController");
        cPN = fc("PhotonUnityNetworking","Photon.Pun","PhotonNetwork");
        cNetPeer = fc("PhotonUnityNetworking","Photon.Pun","NetworkingPeer");
        cPP = fc("PhotonUnityNetworking","ExitGames.Client.Photon","PhotonPlayer");
        cCam = fc("UnityEngine.CoreModule","UnityEngine","Camera");
        cObj = fc("UnityEngine.CoreModule","UnityEngine","Object");
        cTrans = fc("UnityEngine.CoreModule","UnityEngine","Transform");
        cWeaponParams = fc("Assembly-CSharp","Axlebolt.Standoff.Inventory","WeaponParameters");

        // Fallbacks
        if (!cPC) cPC = fc("Assembly-CSharp","","PlayerController");
        if (!cPM) cPM = fc("Assembly-CSharp","","PlayerManager");
        if (!cCharP) cCharP = fc("Assembly-CSharp","","CharacterPlayer");
        if (!cPN) cPN = fc("PhotonUnityNetworking","","PhotonNetwork");
        if (!cNetPeer) cNetPeer = fc("PhotonUnityNetworking","","NetworkingPeer");
        if (!cPP) cPP = fc("PhotonUnityNetworking","","PhotonPlayer");

        LOGI("Classes: PC=%p PM=%p PN=%p NP=%p PP=%p Cam=%p", cPC, cPM, cPN, cNetPeer, cPP, cCam);

        // Resolve offsets
        auto fo = [&](Il2CppClass* k, const char* f) -> int {
            int o = getFieldOff(k, f);
            if (o >= 0) LOGI("  %s = 0x%x", f, o);
            return o;
        };

        // PlayerController
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

        // CharacterPlayer
        if (cCharP) cp_playerController = fo(cCharP,"DHGHGGFDECGFAEE");

        // WeaponryController
        if (cWeapCtrl) wc_weaponCtrl = fo(cWeapCtrl,"<GBCHHHGABFGDBFD>k__BackingField");

        // WeaponController
        if (cWeapon) wpc_params = fo(cWeapon,"<BAFFFEGDBGEECDA>k__BackingField");

        // WeaponParameters
        if (cWeaponParams) wp_displayName = fo(cWeaponParams,"_displayName");

        // PlayerManager
        if (cPM) pm_localPlayer = fo(cPM,"<CFCHCBFEEFHBAEH>k__BackingField");

        // NetworkingPeer
        if (cNetPeer) {
            np_localPlayer = fo(cNetPeer,"<LocalPlayer>k__BackingField");
            np_mActors = fo(cNetPeer,"mActors");
            np_mPlayerListCopy = fo(cNetPeer,"mPlayerListCopy");
            np_mOtherPlayerListCopy = fo(cNetPeer,"mOtherPlayerListCopy");
        }

        // PhotonPlayer
        if (cPP) {
            pp_actorID = fo(cPP,"actorID");
            pp_nameField = fo(cPP,"nameField");
            pp_isLocal = fo(cPP,"IsLocal");
        }

        // PhotonNetwork
        if (cPN) {
            mGetPlayerList = getMethod(cPN,"get_playerList",0);
            LOGI("PhotonNetwork.playerList: %p", mGetPlayerList);
        }

        // Methods
        if (cCam) {
            mW2S = getMethod(cCam,"WorldToScreenPoint",1);
            mMain = getMethod(cCam,"get_main",0);
            mW2CMat = getMethod(cCam,"get_worldToCameraMatrix",0);
            mProjMat = getMethod(cCam,"get_projectionMatrix",0);
            LOGI("Cam: W2S=%p main=%p w2c=%p proj=%p", mW2S, mMain, mW2CMat, mProjMat);
        }
        if (cTrans) {
            mPos = getMethod(cTrans,"get_position",0);
            mEuler = getMethod(cTrans,"get_eulerAngles",0);
            mSetEuler = getMethod(cTrans,"set_eulerAngles",1);
            LOGI("Trans: pos=%p euler=%p setEuler=%p", mPos, mEuler, mSetEuler);
        }

        initialized = true;
        LOGI("GameData initialized");
        updateCamera();
    }

    // ---- Camera ----
    void updateCamera() {
        if (!mMain || !mMain->methodPointer) return;
        mainCamera = ((void* (*)())mMain->methodPointer)();
        if (mainCamera) {
            LOGI("Camera.main: %p", mainCamera);
            readCameraMatrices();
        }
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

    // ---- Get NetworkingPeer instance ----
    void* getNetworkingPeer() {
        if (!cPN || !cNetPeer) return nullptr;

        // PhotonNetwork is a static class; its instance fields live in static field data
        void* sd = il2cpp.class_get_static_field_data ? il2cpp.class_get_static_field_data(cPN) : nullptr;
        if (!sd) {
            // Fallback: try to find via PhotonNetwork.networkingPeer field offset
            int off = getFieldOff(cPN, "networkingPeer");
            if (off < 0) {
                // Scan all fields for NetworkingPeer type
                // The field name might be obfuscated
                // From dump.cs: networkingPeer is at offset 0x18 in the class
                off = 0x18; // Known offset from dump.cs
            }
            return getPtr(nullptr, 0); // Need static data
        }
        return getPtr(sd, 0x18); // networkingPeer offset from dump.cs
    }

    // ---- Read PhotonPlayer array ----
    std::vector<void*> readPhotonPlayerArray(void* arrayObj) {
        std::vector<void*> result;
        if (!arrayObj) return result;

        Il2CppArray* arr = (Il2CppArray*)arrayObj;
        int count = arr->max_length;
        if (count <= 0 || count > 64) return result;

        for (int i = 0; i < count; i++) {
            void* elem = arr->elements[i];
            if (elem) result.push_back(elem);
        }
        return result;
    }

    // ---- Local Player ----
    void updateLocalPlayer() {
        localController = nullptr;

        // Method 1: PlayerManager static field
        if (cPM && pm_localPlayer >= 0) {
            void* sd = il2cpp.class_get_static_field_data ?
                       il2cpp.class_get_static_field_data(cPM) : nullptr;
            if (sd) localController = getPtr(sd, pm_localPlayer);
        }

        // Method 2: NetworkingPeer.LocalPlayer
        if (!localController && cNetPeer && np_localPlayer >= 0) {
            void* peer = getNetworkingPeer();
            if (peer) localController = getPtr(peer, np_localPlayer);
        }

        if (localController) LOGI("Local player: %p", localController);
    }

    // ---- Scene Scan via Photon ----
    void updatePlayers() {
        players.clear();
        if (!localController) return;

        int localTeam = getInt(localController, pc_team);

        // Get local position for distance calc
        float lpx = 0, lpy = 0, lpz = 0;
        void* lt = getPtr(localController, pc_transform);
        if (lt) getTransformPos(lt, lpx, lpy, lpz);

        // Get NetworkingPeer
        void* peer = getNetworkingPeer();
        if (!peer) {
            LOGI("No NetworkingPeer");
            return;
        }

        // Read mPlayerListCopy
        void* playerArray = nullptr;
        if (np_mPlayerListCopy >= 0) {
            playerArray = getPtr(peer, np_mPlayerListCopy);
        }

        if (!playerArray) {
            // Fallback: read mActors dictionary
            LOGI("No mPlayerListCopy, trying mActors");
            return;
        }

        // Iterate PhotonPlayer array
        std::vector<void*> photonPlayers = readPhotonPlayerArray(playerArray);
        LOGI("Found %zu PhotonPlayers", photonPlayers.size());

        for (void* pp : photonPlayers) {
            if (!pp) continue;

            // Read PhotonPlayer fields
            int actorId = getInt(pp, pp_actorID);
            bool isLocal = getBool(pp, pp_isLocal);

            if (actorId <= 0) continue;

            // Read name
            char* nameStr = nullptr;
            if (pp_nameField >= 0) {
                void* nameObj = getPtr(pp, pp_nameField);
                if (nameObj) {
                    // Il2CppString: after header (0x14), chars start
                    // length is at offset 0x10
                    int nameLen = *(int*)((uint8_t*)nameObj + 0x10);
                    if (nameLen > 0 && nameLen < 60) {
                        nameStr = (char*)((uint8_t*)nameObj + 0x14);
                    }
                }
            }

            // Find matching PlayerController by actor ID
            void* foundPC = nullptr;

            // Search through mActors dictionary (Dictionary<int,PhotonPlayer>)
            // But we already have the PhotonPlayer - we need the PlayerController
            // PlayerController.photonView.OwnerActorNr matches the PhotonPlayer.actorID
            // Or PlayerController has actor ID at pc_actorId

            // Scan approach: iterate all PlayerControllers and match by actor ID
            // Actually, we can get the PlayerController from CharacterPlayer
            // But we don't have CharacterPlayer list either

            // Best approach: use the known offset pattern
            // PlayerController._actorId = actorId (from PhotonView)
            // We can search for this match

            // For now: if we have local player, try to find others
            // through the game's internal list

            // Create PlayerData
            PlayerData pd;
            pd.actorId = actorId;
            pd.isLocal = isLocal;
            if (nameStr) strncpy(pd.name, nameStr, sizeof(pd.name) - 1);

            // We need the PlayerController for this actor
            // Try to find it through the scene
            // For now, if we can get the PC, mark it
            // TODO: implement full PC lookup

            // If we have a PlayerController reference somehow
            // pd.playerController = foundPC;
            // readPlayerData(pd, lpx, lpy, lpz);

            if (!isLocal) {
                players.push_back(pd);
            }
        }

        LOGI("Players: %zu", players.size());
    }

    // ---- Read all player data ----
    void readPlayerData(PlayerData& p, float lpx, float lpy, float lpz) {
        if (!p.playerController) return;
        p.valid = true;

        if (pc_team >= 0) p.team = getInt(p.playerController, pc_team);
        if (pc_alive >= 0) p.isAlive = getBool(p.playerController, pc_alive);

        if (pc_maxCT >= 0 && pc_maxTR >= 0) {
            int mh = (p.team == 1) ? getInt(p.playerController, pc_maxCT) :
                     getInt(p.playerController, pc_maxTR);
            p.maxHealth = mh > 0 ? mh : 100;
        }

        // Position
        void* tr = getPtr(p.playerController, pc_transform);
        if (tr) {
            getTransformPos(tr, p.posX, p.posY, p.posZ);
            p.headX = p.posX; p.headY = p.posY + 1.7f; p.headZ = p.posZ;
        }

        // Distance
        float dx = p.posX - lpx, dy = p.posY - lpy, dz = p.posZ - lpz;
        p.distance = sqrtf(dx*dx + dy*dy + dz*dz);

        // WorldToScreen
        if (mainCamera) {
            p.onScreen = worldToScreen(p.posX, p.posY, p.posZ, p.screenX, p.screenY);
            worldToScreen(p.headX, p.headY, p.headZ, p.headScreenX, p.headScreenY);
            if (p.distance > 0) {
                p.boxW = 40.f / p.distance * g_ScreenW * 0.005f;
                p.boxH = p.headScreenY - p.screenY;
                if (p.boxH < 10) p.boxH = 50;
            }
        }

        // Weapon
        if (pc_weap >= 0) {
            void* wc = getPtr(p.playerController, pc_weap);
            if (wc && wc_weaponCtrl >= 0) {
                void* w = getPtr(wc, wc_weaponCtrl);
                if (w && wpc_params >= 0) {
                    void* wp = getPtr(w, wpc_params);
                    if (wp && wp_displayName >= 0) {
                        void* dn = getPtr(wp, wp_displayName);
                        if (dn) {
                            int len = *(int*)((uint8_t*)dn + 0x10);
                            if (len > 0 && len < 60) {
                                char* s = (char*)((uint8_t*)dn + 0x14);
                                strncpy(p.weapon, s, sizeof(p.weapon) - 1);
                            }
                        }
                    }
                }
            }
        }
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
