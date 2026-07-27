#pragma once

#include <cstdint>

// ==================== Standoff 2 v0.36.0 IL2CPP Offsets ====================
// Extracted from dump.cs TypeDefIndex values
// These are field offsets from object base (after Unity MonoBehaviour 32-byte header)

namespace GameOffsets {

    // ---- IL2CPP Runtime ----
    // libil2cpp.so exported functions (found via dlsym)
    namespace IL2CPP {
        constexpr const char* LIB = "libil2cpp.so";
    }

    // ---- UnityEngine core ----
    namespace Object {
        // UnityEngine.Object -> m_CachedPtr at 0x0 (native pointer)
    }

    namespace Transform {
        // Internal: positions stored as TransformData
        // Access via TransformAccess (UnityEngine.Transforms)
    }

    namespace Camera {
        // UnityEngine.Camera fields (TypeDefIndex: 44)
        // WorldToScreenPoint is a method, not a field
    }

    namespace MonoBehaviour {
        // UnityEngine.MonoBehaviour extends Behaviour extends Component extends Object
        // m_CachedPtr is in Object base
    }

    // ---- Controller base class (TypeDefIndex: 767) ----
    // Extends Axlebolt.Standoff.Common.PhotonBehavior
    // MonoBehaviour header (0x00-0x1F) + PhotonBehavior fields
    namespace Controller {
        constexpr int SIZE = 0x58;
        // 0x28: GGBGGDAGBGGGACD
        constexpr int TEAM_ENUM = 0x2C;          // EBBFAFBCBFGDGGE (team)
        constexpr int BIPED_MAP = 0x30;           // BipedMap*
        constexpr int BODY_TRANSFORM = 0x38;      // Transform*
        constexpr int PHOTON_PLAYER = 0x40;       // PhotonPlayer*
        constexpr int ACTOR_ID_1 = 0x48;          // int
        constexpr int ACTOR_ID_2 = 0x4C;          // int
        constexpr int IS_ACTIVE = 0x50;           // bool
    }

    // ---- PlayerController (TypeDefIndex: 776) ----
    // Extends Photon.PunBehaviour
    namespace PlayerController {
        constexpr int SIZE = 0x170;

        constexpr int MAX_CT_HEALTH = 0x08;       // int
        constexpr int MAX_CT_ARMOR = 0x0C;        // int
        constexpr int MAX_TR_HEALTH = 0x10;       // int
        constexpr int MAX_TR_ARMOR = 0x14;        // int

        constexpr int MAIN_CAMERA_HOLDER = 0x28;  // Transform*
        constexpr int FPS_CAMERA_HOLDER = 0x30;   // GameObject*

        constexpr int LEVEL_ZONES = 0x40;         // PlayerLevelZonesController*
        constexpr int CHARACTER_VIEW = 0x48;      // PlayerCharacterView*
        constexpr int CHARACTER_VIEW_2 = 0x50;    // PlayerCharacterView*

        constexpr int ALIVE_FLAG_1 = 0x78;        // bool
        constexpr int TEAM_ENUM = 0x79;           // EBBFAFBCBFGDGGE (team)
        constexpr int HEALTH_FLOAT = 0x7C;        // float (backed field)

        constexpr int AIM_CONTROLLER = 0x80;      // AimController*
        constexpr int WEAPONRY_CONTROLLER = 0x88; // WeaponryController*
        constexpr int MECANIM_CONTROLLER = 0x90;  // MecanimController*
        constexpr int MOVEMENT_CONTROLLER = 0x98; // MovementController*
        constexpr int ARMS_ANIMATION = 0xA0;      // ArmsAnimationController*
        constexpr int HIT_CONTROLLER = 0xA8;      // PlayerHitController*
        constexpr int MATERIAL_CONTROLLER = 0xB0; // PlayerMaterialController*
        constexpr int OCCLUSION_CONTROLLER = 0xB8;// PlayerOcclusionController*
        constexpr int NETWORK_CONTROLLER = 0xC0;  // NetworkController*

        constexpr int ALIVE_FLAG_2 = 0xD8;        // bool
        constexpr int ALIVE_FLAG_3 = 0xD9;        // bool
        constexpr int LOCAL_TIME = 0xDC;           // float

        constexpr int SOUND_CONTROLLER = 0xE0;    // PlayerSoundController*
        constexpr int PLAYER_MAIN_CAMERA = 0xE8;  // PlayerMainCamera*
        constexpr int PLAYER_FPS_CAMERA = 0xF0;   // PlayerFPSCamera*
        constexpr int MARKER_TRIGGER = 0xF8;      // PlayerMarkerTrigger*
        constexpr int BODY_TRANSFORM_2 = 0x100;   // Transform*
        constexpr int CONTROLLERS_ARRAY = 0x108;  // Controller[]*
        constexpr int CHARACTER_CONTROLLER = 0x118;// CharacterController*
        constexpr int SKINNED_MESH = 0x120;       // SkinnedMeshLodGroup*
        constexpr int CHARACTER_LOD = 0x128;      // CharacterLodGroup*
        constexpr int ALIVE_FLAG_4 = 0x130;       // bool
        constexpr int ALIVE_FLAG_5 = 0x131;       // bool

        constexpr int PHOTON_VIEW = 0x150;        // PhotonView*
        constexpr int ACTOR_ID = 0x158;           // int
        constexpr int PLAYER_INDEX = 0x15C;       // int
        constexpr int PHOTON_PLAYER_REF = 0x160;  // PhotonPlayer*
    }

    // ---- PlayerMainCamera (TypeDefIndex: 779) ----
    namespace PlayerMainCamera {
        constexpr int SIZE = 0x50;
        constexpr int CAMERA = 0x20;              // UnityEngine.Camera*
        constexpr int SCOPE_ZOOMER = 0x28;        // CameraScopeZoomer*
        constexpr int CAMERA_ANIM = 0x30;         // CameraAnimationController*
        constexpr int CAMERA_TRANSFORM = 0x38;    // Transform*
        constexpr int MAIN_CAMERA_REF = 0x40;     // MainCamera*
        constexpr int PLAYER_CONTROLLER = 0x48;   // PlayerController*
    }

    // ---- AimController (TypeDefIndex: 1011) ----
    namespace AimController {
        constexpr int SIZE = 0x250;
        constexpr int SENSITIVITY_X = 0x58;       // float
        constexpr int SENSITIVITY_Y = 0x5C;       // float
        constexpr int MIN_X = 0x60;               // float
        constexpr int MAX_X = 0x64;               // float
        constexpr int FPS_GO = 0x68;              // Transform*
        constexpr int SPINE_DIRECTOR = 0x70;      // Transform*
        constexpr int FPSCAMERA = 0x78;           // Transform*
        constexpr int CAM_TRANSFORM = 0x80;       // Transform*
        constexpr int AIMING_PARAMETERS = 0x88;   // AimingParameters*
        constexpr int PLAYER_CONTROLLER = 0xB0;   // PlayerController*
        constexpr int MOVEMENT_CONTROLLER = 0xB8; // MovementController*
        constexpr int CAMERA_BONE = 0xC0;         // Transform*
        constexpr int MECANIM_CONTROLLER = 0xC8;  // MecanimController*
        constexpr int PITCH_TRANSFORM = 0xD0;     // Transform*
        constexpr int AIM_PITCH = 0x1E4;          // float (backed field)
        constexpr int AIM_YAW = 0x1E8;            // float (backed field)
        constexpr int WEAPON_CONTROLLER = 0x208;  // WeaponryController*
    }

    // ---- MovementController (TypeDefIndex: 916) ----
    namespace MovementController {
        constexpr int SIZE = 0xF0;
        constexpr int PLAYER_CONTROLLER = 0x58;   // PlayerController*
        constexpr int OCCLUSION_CONTROLLER = 0x60;// PlayerOcclusionController*
        constexpr int NEVER_IDLE = 0x68;          // bool
        constexpr int BODY_TRANSFORM = 0x70;      // Transform*
        constexpr int SPEED_FACTOR = 0x80;        // float
        constexpr int JUMP_SPEED = 0x84;          // float
        constexpr int CHARACTER_CONTROLLER = 0x88;// CharacterController*
        constexpr int TRANSLATION_PARAMS = 0xA8;  // PlayerTranslationParameters*
        constexpr int CHARACTER_TRANSFORM = 0xC0; // Transform*
        constexpr int MECANIM_CONTROLLER = 0xC8;  // MecanimController*
        constexpr int VELOCITY_Y = 0xD0;          // float
    }

    // ---- WeaponryController (TypeDefIndex: 817) ----
    namespace WeaponryController {
        constexpr int SIZE = 0x100;
        constexpr int PLAYER_CONTROLLER = 0x20;   // PlayerController*
        constexpr int MECANIM_CONTROLLER = 0x28;  // MecanimController*
        constexpr int WEAPON_ANIM = 0x30;         // WeaponAnimationController*
        constexpr int CURRENT_WEAPON_ID = 0xA0;   // int
    }

    // ---- NetworkController (TypeDefIndex: 773) ----
    namespace NetworkController {
        constexpr int SIZE = 0xC0;
        constexpr int CHARACTER_PLAYER = 0x88;    // CharacterPlayer*
        constexpr int PLAYER_CONTROLLER = 0x90;   // PlayerController*
        constexpr int PHOTON_VIEW = 0x98;         // PhotonView*
        constexpr int LATENCY = 0xA8;             // float
    }

    // ---- CharacterPlayer (TypeDefIndex: 765) ----
    namespace CharacterPlayer {
        constexpr int SIZE = 0x60;
        constexpr int PLAYER_CONTROLLER = 0x50;   // PlayerController*
    }

    // ---- PlayerManager (TypeDefIndex: 788) ----
    namespace PlayerManager {
        constexpr int SIZE = 0x78;
        constexpr int LOCAL_PLAYER = 0x68;        // PlayerController* (local)
        constexpr int SPECTATING_PLAYER = 0x70;   // PlayerController* (spectating)
    }

    // ---- ObjectPlayer (TypeDefIndex: 1064) ----
    namespace ObjectPlayer {
        constexpr int SIZE = 0x48;
        constexpr int FIELD_28 = 0x28;            // float (backed)
        constexpr int FIELD_2C = 0x2C;            // float (backed)
        constexpr int FIELD_30 = 0x30;            // float
        constexpr int FIELD_34 = 0x34;            // float
        constexpr int FIELD_38 = 0x38;            // float
    }

    // ---- WeaponManager (TypeDefIndex: 1822) ----
    namespace WeaponManager {
        constexpr int SIZE = 0x80;
        constexpr int SHOOT_EVENT = 0x28;         // object (event)
        constexpr int SET_WEAPON_EVENT = 0x30;    // object (event)
        constexpr int SWITCH_WEAPON_EVENT = 0x38; // object (event)
    }

    // ---- GameManager (TypeDefIndex: 2166) ----
    namespace GameManager {
        constexpr int SIZE = 0xD0;
        constexpr int GAME_CONTROLLER = 0x60;     // GameController*
        constexpr int LEVEL_DEFINITION = 0xB8;    // LevelDefinition*
    }

    // ---- GameController (TypeDefIndex: 2148) ----
    namespace GameController {
        constexpr int SIZE = 0x2C8;
        constexpr int MAIN_CAMERA = 0x110;        // UnityEngine.Camera*
        constexpr int HUD_CAMERA = 0x118;         // UnityEngine.Camera*
        constexpr int LOCAL_PLAYER = 0x2C0;       // PlayerController*
    }

    // ---- WeaponController (TypeDefIndex: 1812) ----
    namespace WeaponController {
        constexpr int SIZE = 0x100;
        constexpr int PLAYER_CONTROLLER = 0x20;   // PlayerController*
        constexpr int CURRENT_AMMO = 0x80;        // float (backed)
        constexpr int RESERVE_AMMO = 0x84;        // float (backed)
        constexpr int WEAPON_ID = 0xA0;           // int
    }

    // ---- PlayerHitController (TypeDefIndex: 983) ----
    namespace PlayerHitController {
        constexpr int SIZE = 0x108;
        constexpr int HIT_CURVE = 0xA0;           // AnimationCurve*
        constexpr int LOCAL_TIME = 0x100;         // float
    }

    // ---- FloatValue (TypeDefIndex: 1115) ----
    namespace FloatValue {
        constexpr int VALUE = 0x14;               // float
    }

    // ---- TeamValue (TypeDefIndex: 1131) ----
    namespace TeamValue {
        constexpr int VALUE = 0x11;               // byte (enum EBBFAFBCBFGDGGE)
    }

    // ---- PlayerCondition (TypeDefIndex: 1126) ----
    namespace PlayerCondition {
        constexpr int STATE = 0x10;               // PlayerStateValue*
        constexpr int HEALTH = 0x18;              // FloatValue*
        constexpr int ARMOR = 0x20;               // FloatValue*
        constexpr int TEAM = 0x28;                // TeamValue*
    }

    // ---- Team Enum Values ----
    namespace Team {
        constexpr int NONE = 0;
        constexpr int CT = 1;
        constexpr int TR = 2;
    }
}
