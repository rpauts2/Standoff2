#pragma once
#include <EGL/egl.h>
#include <dlfcn.h>

#ifdef __ANDROID__
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "ModMenu", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "ModMenu", __VA_ARGS__)
#endif

typedef EGLBoolean (*eglSwapBuffers_t)(EGLDisplay, EGLSurface);
static eglSwapBuffers_t o_eglSwapBuffers = nullptr;

namespace ImGuiHook {
    inline bool initialized = false;
    inline EGLDisplay currentDisplay = nullptr;
    inline EGLSurface currentSurface = nullptr;

    void Initialize();
    void Shutdown();
}

extern "C" {
    void loadJNI(JavaVM* vm);
    void initImGuiHook();
}
