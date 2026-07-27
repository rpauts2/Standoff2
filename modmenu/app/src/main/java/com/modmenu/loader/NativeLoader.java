package com.modmenu.loader;

public class NativeLoader {
    static {
        System.loadLibrary("Loader");
    }

    public static native void loadNative();
    public static native void toggleMenu();
    public static native boolean isFeatureEnabled(int index);
    public static native float getFeatureValue(int index);
}
