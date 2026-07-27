#include <jni.h>
#include <dlfcn.h>
#include <string>
#include <thread>
#include <unistd.h>
#include <android/log.h>

#define LOG_TAG "Loader"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static JavaVM* g_VM = nullptr;

typedef void (*LoadJNI)(JavaVM*);

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGI("JNI_OnLoad");
    g_VM = vm;

    JNIEnv* env;
    vm->GetEnv((void**)&env, JNI_VERSION_1_6);

    // Load Main in a thread
    std::thread([vm]() {
        usleep(2000000);

        void* handle = dlopen("libMain.so", RTLD_LAZY);
        if (!handle) {
            LOGE("dlopen libMain.so failed: %s", dlerror());
            return;
        }
        LOGI("libMain.so loaded");

        LoadJNI fn = (LoadJNI)dlsym(handle, "loadJNI");
        if (fn) {
            fn(vm);
            LOGI("loadJNI called");
        } else {
            LOGE("loadJNI not found");
        }
    }).detach();

    return JNI_VERSION_1_6;
}
