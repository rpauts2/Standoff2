#pragma once
#include <string>
#include <vector>
#include <cstdio>
#include <cstring>
#include <cstdarg>

#ifdef __ANDROID__
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "ModMenu", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "ModMenu", __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "ModMenu", __VA_ARGS__)
#else
#define LOGI(...) printf(__VA_ARGS__)
#define LOGE(...) printf(__VA_ARGS__)
#define LOGD(...) printf(__VA_ARGS__)
#endif

#define OBFUSCATE(str) str

struct Feature {
    const char* name;
    bool enabled;
    float value;
    float minVal;
    float maxVal;
    int type; // 0=toggle, 1=slider, 2=spinner
    const char** items;
    int itemCount;
    int selectedItem;
};

class FeatureManager {
public:
    static FeatureManager& getInstance() {
        static FeatureManager instance;
        return instance;
    }

    void addFeature(const char* name, int type = 0, float minVal = 0, float maxVal = 1, float defaultVal = 0) {
        Feature f;
        f.name = name;
        f.enabled = false;
        f.value = defaultVal;
        f.minVal = minVal;
        f.maxVal = maxVal;
        f.type = type;
        f.items = nullptr;
        f.itemCount = 0;
        f.selectedItem = 0;
        features.push_back(f);
    }

    void addFeatureWithItems(const char* name, const char** items, int count) {
        Feature f;
        f.name = name;
        f.enabled = false;
        f.value = 0;
        f.minVal = 0;
        f.maxVal = 1;
        f.type = 2;
        f.items = items;
        f.itemCount = count;
        f.selectedItem = 0;
        features.push_back(f);
    }

    bool isEnabled(int index) const {
        if (index >= 0 && index < features.size()) return features[index].enabled;
        return false;
    }

    bool isEnabled(const char* name) const {
        for (const auto& f : features) {
            if (strcmp(f.name, name) == 0) return f.enabled;
        }
        return false;
    }

    float getValue(int index) const {
        if (index >= 0 && index < features.size()) return features[index].value;
        return 0;
    }

    float getValue(const char* name) const {
        for (const auto& f : features) {
            if (strcmp(f.name, name) == 0) return f.value;
        }
        return 0;
    }

    int getSelectedItem(int index) const {
        if (index >= 0 && index < features.size()) return features[index].selectedItem;
        return 0;
    }

    size_t count() const { return features.size(); }

    Feature& get(int index) { return features[index]; }

private:
    FeatureManager() {
        // ESP
        addFeature("Box ESP", 0);
        addFeature("Snaplines ESP", 0);
        addFeature("Health Bar", 0);
        addFeature("Name ESP", 0);
        addFeature("Distance ESP", 0);
        addFeature("Weapon ESP", 0);
        addFeature("Head Dot", 0);

        // Aimbot
        addFeature("Aimbot", 0);
        addFeature("Aimbot FOV", 1, 10, 300, 90);
        addFeature("Aimbot Smooth", 1, 1, 20, 5);
        addFeature("Aimbot Bone", 2);
        addFeature("Silent Aim", 0);
        addFeature("Predict Movement", 0);

        // Triggerbot
        addFeature("Triggerbot", 0);
        addFeature("Trigger Delay", 1, 0, 500, 50);

        // Player
        addFeature("No Recoil", 0);
        addFeature("No Spread", 0);
        addFeature("Infinite Ammo", 0);
        addFeature("Rapid Fire", 0);
        addFeature("Magic Bullet", 0);
        addFeature("One Hit Kill", 0);

        // World
        addFeature("Wallhack", 0);
        addFeature("Chams", 0);
        addFeature("Chams Style", 2);
        addFeature("Glow ESP", 0);
        addFeature("Crosshair", 0);
        addFeature("Crosshair Style", 2);
        addFeature("No Fog", 0);
        addFeature("Night Mode", 0);
        addFeature("Colorful Sky", 0);

        // Movement
        addFeature("Speed Hack", 0);
        addFeature("Speed Multiplier", 1, 1, 5, 1);
        addFeature("Bunny Hop", 0);
        addFeature("No Clip", 0);
        addFeature("Fly Hack", 0);

        // Misc
        addFeature("Anti Spectate", 0);
        addFeature("Streamer Mode", 0);
        addFeature("Fake Lag", 0);
        addFeature("Fake Name", 0);

        // Feature counts for spinner
        static const char* boneItems[] = {"Head", "Neck", "Chest", "Stomach"};
        static const char* chamsItems[] = {"Normal", "Flat", "Glow", "Wireframe"};
        static const char* crosshairItems[] = {"Dot", "Cross", "Circle", "Triangle"};

        features[12].items = boneItems; features[12].itemCount = 4;
        features[25].items = chamsItems; features[25].itemCount = 4;
        features[28].items = crosshairItems; features[28].itemCount = 4;
    }

    std::vector<Feature> features;
};
